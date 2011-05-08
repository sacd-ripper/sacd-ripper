// Copyright 2010            Sven Peter <svenpeter@gmail.com>
// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <zlib.h>
#include <dirent.h>

#include "tools.h"

static struct id2name_tbl_t t_key2file[] = {
    {KEY_LV0, "lv0"},
    {KEY_LV1, "lv1"},
    {KEY_LV2, "lv2"},
    {KEY_APP, "app"},
    {KEY_ISO, "iso"},
    {KEY_LDR, "ldr"},
    {KEY_PKG, "pkg"},
    {KEY_SPP, "spp"},
    {0, NULL}
};

void fail(const char *a, ...) {
    char msg[1024];
    va_list va;

    va_start(va, a);
    vsnprintf(msg, sizeof msg, a, va);
    fprintf(stderr, "%s\n", msg);
    perror("perror");

    exit(1);
}

void decompress(uint8_t *in, uint64_t in_len, uint8_t *out, uint64_t out_len) {
    z_stream s;
    int ret;

    memset(&s, 0, sizeof (s));

    s.zalloc = Z_NULL;
    s.zfree = Z_NULL;
    s.opaque = Z_NULL;

    ret = inflateInit(&s);
    if (ret != Z_OK)
        fail("inflateInit returned %d", ret);

    s.avail_in = (uInt) in_len;
    s.next_in = in;

    s.avail_out = (uInt) out_len;
    s.next_out = out;

    ret = inflate(&s, Z_FINISH);
    if (ret != Z_OK && ret != Z_STREAM_END)
        fail("inflate returned %d", ret);

    inflateEnd(&s);
}

const char *id2name(uint32_t id, struct id2name_tbl_t *t, const char *unk) {
    while (t->name != NULL) {
        if (id == t->id)
            return t->name;
        t++;
    }
    return unk;
}

static int key_build_path(char *ptr) {
    char *home = NULL;
    char *dir = NULL;

    memset(ptr, 0, 256);

    dir = getenv("PS3_KEYS");
    if (dir != NULL) {
        strncpy(ptr, dir, 256);
        return 0;
    }

#ifdef WIN32
    home = getenv("USERPROFILE");
#else
    home = getenv("HOME");
#endif
    if (home == NULL) {
        snprintf(ptr, 256, "ps3keys");
    } else {
#ifdef WIN32
        snprintf(ptr, 256, "%s\\ps3keys\\", home);
#else
        snprintf(ptr, 256, "%s/.ps3/", home);
#endif
    }

    return 0;
}

static int key_read(const char *path, uint32_t len, uint8_t *dst) {
    FILE *fp = NULL;
    uint32_t read;
    int ret = -1;

    fp = fopen(path, "rb");
    if (fp == NULL)
        goto fail;

    read = fread(dst, len, 1, fp);

    if (read != 1)
        goto fail;

    ret = 0;

fail:
    if (fp != NULL)
        fclose(fp);

    return ret;
}

struct keylist_t *keys_get(enum sce_key type) {
    const char *name = NULL;
    char base[256];
    char path[256];
    void *tmp = NULL;
    char *id;
    DIR *dp;
    struct dirent *dent;
    struct keylist_t *klist;
    uint8_t bfr[4];

    klist = malloc(sizeof *klist);
    if (klist == NULL)
        goto fail;

    memset(klist, 0, sizeof *klist);

    name = id2name(type, t_key2file, NULL);
    if (name == NULL)
        goto fail;

    if (key_build_path(base) < 0)
        goto fail;

    dp = opendir(base);
    if (dp == NULL)
        goto fail;

    while ((dent = readdir(dp)) != NULL) {
        if (strncmp(dent->d_name, name, strlen(name)) == 0 &&
                strstr(dent->d_name, "key") != NULL) {
            tmp = realloc(klist->keys, (klist->n + 1) * sizeof (struct key_t));
            if (tmp == NULL)
                goto fail;

            id = strrchr(dent->d_name, '-');
            if (id != NULL)
                id++;

            klist->keys = tmp;
            memset(&klist->keys[klist->n], 0, sizeof (struct key_t));

            snprintf(path, sizeof path, "%s/%s-key-%s", base, name, id);
            if (key_read(path, 32, klist->keys[klist->n].key) != 0) {
                printf("  key file:   %s (ERROR)\n", path);
            }

            snprintf(path, sizeof path, "%s/%s-iv-%s", base, name, id);
            if (key_read(path, 16, klist->keys[klist->n].iv) != 0) {
                printf("  iv file:    %s (ERROR)\n", path);
            }

            klist->keys[klist->n].pub_avail = -1;
            klist->keys[klist->n].priv_avail = -1;

            snprintf(path, sizeof path, "%s/%s-pub-%s", base, name, id);
            if (key_read(path, 40, klist->keys[klist->n].pub) == 0) {
                snprintf(path, sizeof path, "%s/%s-ctype-%s", base, name, id);
                key_read(path, 4, bfr);

                klist->keys[klist->n].pub_avail = 1;
                klist->keys[klist->n].ctype = be32(bfr);
            } else {
                printf("  pub file:   %s (ERROR)\n", path);
            }

            snprintf(path, sizeof path, "%s/%s-priv-%s", base, name, id);
            if (key_read(path, 21, klist->keys[klist->n].priv) == 0) {
                klist->keys[klist->n].priv_avail = 1;
            } else {
                printf("  priv file:  %s (ERROR)\n", path);
            }


            klist->n++;
        }
    }

    return klist;

fail:
    if (klist != NULL) {
        if (klist->keys != NULL)
            free(klist->keys);
        free(klist);
    }
    klist = NULL;

    return NULL;
}
