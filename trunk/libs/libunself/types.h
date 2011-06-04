// Copyright 2010       Sven Peter <svenpeter@gmail.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
#ifndef TYPES_H__
#define TYPES_H__

#include <stdint.h>

static inline uint8_t be8(uint8_t *p)
{
    return *p;
}

static inline uint16_t be16(uint8_t *p)
{
    uint16_t a;

    a  = p[0] << 8;
    a |= p[1];

    return a;
}

static inline uint32_t be32(uint8_t *p)
{
    uint32_t a;

    a  = p[0] << 24;
    a |= p[1] << 16;
    a |= p[2] << 8;
    a |= p[3] << 0;

    return a;
}

static inline uint64_t be64(uint8_t *p)
{
    uint32_t a, b;

    a = be32(p);
    b = be32(p + 4);

    return ((uint64_t) a << 32) | b;
}

static inline void wbe16(uint8_t *p, uint16_t v)
{
    p[0] = v >> 8;
    p[1] = v & 0xff;
}

static inline void wbe32(uint8_t *p, uint32_t v)
{
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

static inline void wbe64(uint8_t *p, uint64_t v)
{
    wbe32(p + 4, v & 0xffffffff);
    wbe32(p, (v >> 32) & 0xffffffff);
}

struct elf_phdr {
	uint32_t p_type;
	uint64_t p_off;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint32_t p_flags;
	uint64_t p_align;

	void *ptr;
};

struct elf_shdr {
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint64_t sh_addr;
	uint64_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
};

#define	ET_NONE		0
#define ET_REL		1
#define	ET_EXEC		2
#define	ET_DYN		3
#define	ET_CORE		4
#define	ET_LOOS		0xfe00
#define	ET_HIOS		0xfeff
#define ET_LOPROC	0xff00
#define	ET_HIPROC	0xffff
struct elf_hdr {
	char e_ident[16];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shtrndx;
}; 

#endif
