// Copyright 2010            Sven Peter <svenpeter@gmail.com>
// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef TOOLS_H__
#define TOOLS_H__    1
#include <stdint.h>

#include "types.h"

struct id2name_tbl_t
{
    uint32_t   id;
    const char *name;
};

struct key_t
{
    uint8_t  key[32];
    uint8_t  iv[16];

    int      pub_avail;
    int      priv_avail;
    uint8_t  pub[40];
    uint8_t  priv[21];
    uint32_t ctype;
};

struct keylist_t
{
    uint32_t     n;
    struct key_t *keys;
};

enum sce_key
{
    KEY_LV0 = 0,
    KEY_LV1,
    KEY_LV2,
    KEY_APP,
    KEY_ISO,
    KEY_LDR,
    KEY_PKG,
    KEY_SPP
};

const char *id2name(uint32_t id, struct id2name_tbl_t *t, const char *unk);
void fail(const char *fmt, ...); // __attribute__((noreturn));
void decompress(uint8_t *in, uint64_t in_len, uint8_t *out, uint64_t out_len);

struct keylist_t *keys_get(enum sce_key type);

#endif
