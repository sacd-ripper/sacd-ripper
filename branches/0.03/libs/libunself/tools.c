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
#include "aes.h"

static const char           *search_dirs[12] = {
    "/dev_usb000",
    "/dev_usb001",
    "/dev_usb002",
    "/dev_usb003",
    "/dev_usb004",
    "/dev_usb005",
    "/dev_usb006",
    "/dev_usb007",

    "/dev_cf",
    "/dev_sd",
    "/dev_ms",

    NULL
};

static struct id2name_tbl_t t_key2file[] = {
    { KEY_LV0, "lv0" },
    { KEY_LV1, "lv1" },
    { KEY_LV2, "lv2" },
    { KEY_APP, "app" },
    { KEY_ISO, "iso" },
    { KEY_LDR, "ldr" },
    { KEY_PKG, "pkg" },
    { KEY_SPP, "spp" },
    {       0, NULL  }
};

void fail(const char *a, ...)
{
    char    msg[1024];
    va_list va;

    va_start(va, a);
    vsnprintf(msg, sizeof msg, a, va);
    fprintf(stderr, "%s\n", msg);
    perror("perror");

    exit(1);
}

void decompress(uint8_t *in, uint64_t in_len, uint8_t *out, uint64_t out_len)
{
    z_stream s;
    int      ret;

    memset(&s, 0, sizeof(s));

    s.zalloc = Z_NULL;
    s.zfree  = Z_NULL;
    s.opaque = Z_NULL;

    ret = inflateInit(&s);
    if (ret != Z_OK)
        fail("inflateInit returned %d", ret);

    s.avail_in = (uInt) in_len;
    s.next_in  = in;

    s.avail_out = (uInt) out_len;
    s.next_out  = out;

    ret = inflate(&s, Z_FINISH);
    if (ret != Z_OK && ret != Z_STREAM_END)
        fail("inflate returned %d", ret);

    inflateEnd(&s);
}

const char *id2name(uint32_t id, struct id2name_tbl_t *t, const char *unk)
{
    while (t->name != NULL)
    {
        if (id == t->id)
            return t->name;
        t++;
    }
    return unk;
}

static int key_read(const char *path, uint32_t len, uint8_t *dst)
{
    FILE     *fp = NULL;
    uint32_t read;
    int      ret = -1;

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

struct keylist_t *keys_get(enum sce_key type)
{
    const char       **search_dir = search_dirs;
    const char       *name        = NULL;
    char             base[256];
    char             path[256];
    void             *tmp = NULL;
    char             *id;
    DIR              *dp;
    struct dirent    *dent;
    struct keylist_t *klist;
    uint8_t          bfr[4];

    klist = malloc(sizeof *klist);
    if (klist == NULL)
        goto fail;

    memset(klist, 0, sizeof *klist);

    name = id2name(type, t_key2file, NULL);
    if (name == NULL)
        goto fail;

    while (*search_dir)
    {
        strcpy(base, *search_dir);

        dp = opendir(base);
        if (dp != NULL)
        {
            while ((dent = readdir(dp)) != NULL)
            {
                if (strncmp(dent->d_name, name, strlen(name)) == 0 &&
                    strstr(dent->d_name, "key") != NULL)
                {
                    tmp = realloc(klist->keys, (klist->n + 1) * sizeof(struct key_t));
                    if (tmp == NULL)
                        goto fail;

                    id = strrchr(dent->d_name, '-');
                    if (id != NULL)
                        id++;

                    klist->keys = tmp;
                    memset(&klist->keys[klist->n], 0, sizeof(struct key_t));

                    snprintf(path, sizeof path, "%s/%s-key-%s", base, name, id);
                    if (key_read(path, 32, klist->keys[klist->n].key) != 0)
                    {
                        printf("  key file:   %s (ERROR)\n", path);
                    }

                    snprintf(path, sizeof path, "%s/%s-iv-%s", base, name, id);
                    if (key_read(path, 16, klist->keys[klist->n].iv) != 0)
                    {
                        printf("  iv file:    %s (ERROR)\n", path);
                    }

                    klist->keys[klist->n].pub_avail  = -1;
                    klist->keys[klist->n].priv_avail = -1;

                    snprintf(path, sizeof path, "%s/%s-pub-%s", base, name, id);
                    if (key_read(path, 40, klist->keys[klist->n].pub) == 0)
                    {
                        snprintf(path, sizeof path, "%s/%s-ctype-%s", base, name, id);
                        key_read(path, 4, bfr);

                        klist->keys[klist->n].pub_avail = 1;
                        klist->keys[klist->n].ctype     = be32(bfr);
                    }
                    else
                    {
                        printf("  pub file:   %s (ERROR)\n", path);
                    }

                    snprintf(path, sizeof path, "%s/%s-priv-%s", base, name, id);
                    if (key_read(path, 21, klist->keys[klist->n].priv) == 0)
                    {
                        klist->keys[klist->n].priv_avail = 1;
                    }
                    else
                    {
                        printf("  priv file:  %s (ERROR)\n", path);
                    }

                    klist->n++;
                }
            }
        }

        search_dir++;
    }

    return klist;

 fail:
    if (klist != NULL)
    {
        if (klist->keys != NULL)
            free(klist->keys);
        free(klist);
    }
    klist = NULL;

    return NULL;
}

void *read_file(const char *path)
{
	FILE *fd;
	size_t sz;
	void *ptr;

	fd = fopen(path, "rb");
	if(!fd)
		return 0;

    fseek (fd, 0, SEEK_END);
    sz = ftell (fd);
    rewind(fd);
  
    ptr = malloc(sz);
	if (fread(ptr, sz, 1, fd) != 1)
    {
        free(ptr);
        fclose(fd);
        return 0;
    }
	fclose(fd);

	return ptr;
}

//
// ELF helpers
//
int elf_read_hdr(uint8_t *hdr, struct elf_hdr *h)
{
	int arch64;
	memcpy(h->e_ident, hdr, 16);
	hdr += 16;

	arch64 = h->e_ident[4] == 2;

	h->e_type = be16(hdr);
	hdr += 2;
	h->e_machine = be16(hdr);
	hdr += 2;
	h->e_version = be32(hdr);
	hdr += 4;

	if (arch64) {
		h->e_entry = be64(hdr);
		h->e_phoff = be64(hdr + 8);
		h->e_shoff = be64(hdr + 16);
		hdr += 24;
	} else {
		h->e_entry = be32(hdr);
		h->e_phoff = be32(hdr + 4);
		h->e_shoff = be32(hdr + 8);
		hdr += 12;
	}

	h->e_flags = be32(hdr);
	hdr += 4;

	h->e_ehsize = be16(hdr);
	hdr += 2;
	h->e_phentsize = be16(hdr);
	hdr += 2;
	h->e_phnum = be16(hdr);
	hdr += 2;
	h->e_shentsize = be16(hdr);
	hdr += 2;
	h->e_shnum = be16(hdr);
	hdr += 2;
	h->e_shtrndx = be16(hdr);

	return arch64;
}

void elf_read_phdr(int arch64, uint8_t *phdr, struct elf_phdr *p)
{
	if (arch64) {
		p->p_type =   be32(phdr + 0);
		p->p_flags =  be32(phdr + 4);
		p->p_off =    be64(phdr + 1*8);
		p->p_vaddr =  be64(phdr + 2*8);
		p->p_paddr =  be64(phdr + 3*8);
		p->p_filesz = be64(phdr + 4*8);
		p->p_memsz =  be64(phdr + 5*8);
		p->p_align =  be64(phdr + 6*8);
	} else {
		p->p_type =   be32(phdr + 0*4);
		p->p_off =    be32(phdr + 1*4);
		p->p_vaddr =  be32(phdr + 2*4);
		p->p_paddr =  be32(phdr + 3*4);
		p->p_filesz = be32(phdr + 4*4);
		p->p_memsz =  be32(phdr + 5*4);
		p->p_flags =  be32(phdr + 6*4);
		p->p_align =  be32(phdr + 7*4);
	}
}

void elf_read_shdr(int arch64, uint8_t *shdr, struct elf_shdr *s)
{
	if (arch64) {
		s->sh_name =	  be32(shdr + 0*4);
		s->sh_type =	  be32(shdr + 1*4);
		s->sh_flags =	  be64(shdr + 2*4);
		s->sh_addr =	  be64(shdr + 2*4 + 1*8);
		s->sh_offset =	  be64(shdr + 2*4 + 2*8);
		s->sh_size =	  be64(shdr + 2*4 + 3*8);
		s->sh_link =	  be32(shdr + 2*4 + 4*8);
		s->sh_info =	  be32(shdr + 3*4 + 4*8);
		s->sh_addralign = be64(shdr + 4*4 + 4*8);
		s->sh_entsize =   be64(shdr + 4*4 + 5*8);
	} else {
		s->sh_name =	  be32(shdr + 0*4);
		s->sh_type =	  be32(shdr + 1*4);
		s->sh_flags =	  be32(shdr + 2*4);
		s->sh_addr =	  be32(shdr + 3*4);
		s->sh_offset =	  be32(shdr + 4*4);
		s->sh_size =	  be32(shdr + 5*4);
		s->sh_link =	  be32(shdr + 6*4);
		s->sh_info =	  be32(shdr + 7*4);
		s->sh_addralign = be32(shdr + 8*4);
		s->sh_entsize =   be32(shdr + 9*4);
	}
}

void elf_write_shdr(int arch64, uint8_t *shdr, struct elf_shdr *s)
{
	if (arch64) {
		wbe32(shdr + 0*4, s->sh_name);
		wbe32(shdr + 1*4, s->sh_type);
		wbe64(shdr + 2*4, s->sh_flags);
		wbe64(shdr + 2*4 + 1*8, s->sh_addr);
		wbe64(shdr + 2*4 + 2*8, s->sh_offset);
		wbe64(shdr + 2*4 + 3*8, s->sh_size);
		wbe32(shdr + 2*4 + 4*8, s->sh_link);
		wbe32(shdr + 3*4 + 4*8, s->sh_info);
		wbe64(shdr + 4*4 + 4*8, s->sh_addralign);
		wbe64(shdr + 4*4 + 5*8, s->sh_entsize);
	} else {
		wbe32(shdr + 0*4, s->sh_name);
		wbe32(shdr + 1*4, s->sh_type);
		wbe32(shdr + 2*4, s->sh_flags);
		wbe32(shdr + 3*4, s->sh_addr);
		wbe32(shdr + 4*4, s->sh_offset);
		wbe32(shdr + 5*4, s->sh_size);
		wbe32(shdr + 6*4, s->sh_link);
		wbe32(shdr + 7*4, s->sh_info);
		wbe32(shdr + 8*4, s->sh_addralign);
		wbe32(shdr + 9*4, s->sh_entsize);
	}
}

int sce_decrypt_header(uint8_t *ptr, struct keylist_t *klist)
{
	uint32_t meta_offset;
	uint32_t meta_len;
	uint64_t header_len;
	uint32_t i, j;
	uint8_t tmp[0x40];
	int success = 0;


	meta_offset = be32(ptr + 0x0c);
	header_len  = be64(ptr + 0x10);

	for (i = 0; i < klist->n; i++) {
		AES_cbc256_decrypt(klist->keys[i].key,
			  klist->keys[i].iv,
			  ptr + meta_offset + 0x20,
			  0x40,
			  tmp);

		success = 1;
		for (j = 0x10; j < (0x10 + 0x10); j++)
			if (tmp[j] != 0)
				success = 0;

		for (j = 0x30; j < (0x30 + 0x10); j++)
			if (tmp[j] != 0)
			       success = 0;

		if (success == 1) {
			memcpy(ptr + meta_offset + 0x20, tmp, 0x40);
			break;
		}
	}

	if (success != 1)
		return -1;

	memcpy(tmp, ptr + meta_offset + 0x40, 0x10);
	AES_ctr128_encrypt(ptr + meta_offset + 0x20,
		  tmp,
		  ptr + meta_offset + 0x60,
		  0x20,
		  ptr + meta_offset + 0x60);

	meta_len = header_len - meta_offset;

	AES_ctr128_encrypt(ptr + meta_offset + 0x20,
		  tmp,
		  ptr + meta_offset + 0x80,
		  meta_len - 0x80,
		  ptr + meta_offset + 0x80);

	return i;
}

int sce_decrypt_data(uint8_t *ptr)
{
	uint64_t meta_offset;
	uint32_t meta_len;
	uint32_t meta_n_hdr;
	uint64_t header_len;
	uint32_t i;

	uint64_t offset;
	uint64_t size;
	uint32_t keyid;
	uint32_t ivid;
	uint8_t *tmp;

	uint8_t iv[16];

	meta_offset = be32(ptr + 0x0c);
	header_len  = be64(ptr + 0x10);
	meta_len = header_len - meta_offset;
	meta_n_hdr = be32(ptr + meta_offset + 0x60 + 0xc);

	for (i = 0; i < meta_n_hdr; i++) {
		tmp = ptr + meta_offset + 0x80 + 0x30*i;
		offset = be64(tmp);
		size = be64(tmp + 8);
		keyid = be32(tmp + 0x24);
		ivid = be32(tmp + 0x28);

		if (keyid == 0xffffffff || ivid == 0xffffffff)
			continue;

		memcpy(iv, ptr + meta_offset + 0x80 + 0x30 * meta_n_hdr + ivid * 0x10, 0x10);
		AES_ctr128_encrypt(ptr + meta_offset + 0x80 + 0x30 * meta_n_hdr + keyid * 0x10,
		          iv,
 		          ptr + offset,
			  size,
			  ptr + offset);
	}

	return 0;
}
