// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
#ifndef TYPES_H__
#define TYPES_H__

#include <stdint.h>

typedef unsigned char uint1_t;

static __inline uint8_t be8(uint8_t *p)
{
	return *p;
}

static __inline uint16_t be16(uint8_t *p)
{
	uint16_t a;

	a  = p[0] << 8;
	a |= p[1];

	return a;
}

static __inline uint32_t be32(uint8_t *p)
{
	uint32_t a;

	a  = p[0] << 24;
	a |= p[1] << 16;
	a |= p[2] <<  8;
	a |= p[3] <<  0;

	return a;
}

static __inline uint64_t be64(uint8_t *p)
{
	uint32_t a, b;

	a = be32(p);
	b = be32(p + 4);

	return ((uint64_t)a<<32) | b;
}

static __inline void wbe16(uint8_t *p, uint16_t v)
{
	p[0] = v >> 8;
	p[1] = v & 0xff;
}

static __inline void wbe32(uint8_t *p, uint32_t v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >>  8;
	p[3] = v & 0xff;
}

static __inline void wbe64(uint8_t *p, uint64_t v)
{
	wbe32(p + 4, (uint32_t) v);
	v >>= 32;
	wbe32(p, (uint32_t) v);
}


// sign extension for immediate values inside opcodes
static __inline uint32_t se(uint32_t v, int b)
{
	v <<= 32-b;
	v = ((int32_t)v) >> (32-b);
	return v;
}

static __inline uint32_t se7(uint32_t v) { return se(v, 7); }
static __inline uint32_t se10(uint32_t v) { return se(v, 10); }
static __inline uint32_t se16(uint32_t v) { return se(v, 16); }
static __inline uint32_t se18(uint32_t v) { return se(v, 18); }

#endif
