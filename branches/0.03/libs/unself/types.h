// Copyright 2010       Sven Peter <svenpeter@gmail.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
#ifndef TYPES_H__
#define TYPES_H__

#include "config.h"

#include <stdint.h>

static inline uint8_t be8(uint8_t *p) {
    return *p;
}

static inline uint16_t be16(uint8_t *p) {
    uint16_t a;

    a = p[0] << 8;
    a |= p[1];

    return a;
}

static inline uint32_t be32(uint8_t *p) {
    uint32_t a;

    a = p[0] << 24;
    a |= p[1] << 16;
    a |= p[2] << 8;
    a |= p[3] << 0;

    return a;
}

static inline uint64_t be64(uint8_t *p) {
    uint32_t a, b;

    a = be32(p);
    b = be32(p + 4);

    return ((uint64_t) a << 32) | b;
}

static inline void wbe16(uint8_t *p, uint16_t v) {
    p[0] = v >> 8;
    p[1] = v & 0xff;
}

static inline void wbe32(uint8_t *p, uint32_t v) {
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

static inline void wbe64(uint8_t *p, uint64_t v) {
    wbe32(p + 4, v & 0xffffffff);
    wbe32(p, (v >> 32) & 0xffffffff);
}

#endif
