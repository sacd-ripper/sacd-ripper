/*
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 * flukes1
 * kmeaw
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <ppu-lv2.h>
#include "patch-utils.h"
#include "hvcall.h"
#include "mm.h"

uint64_t   mmap_lpar_addr;
static int poke_syscall = 7;

uint64_t peekq(uint64_t addr)
{
    lv2syscall1(6, addr);
    return_to_user_prog(uint64_t);
}

void pokeq(uint64_t addr, uint64_t val)
{
    lv2syscall2(poke_syscall, addr, val);
}

void pokeq32(uint64_t addr, uint32_t val)
{
    uint32_t next = peekq(addr) & 0xffffffffUL;
    pokeq(addr, (uint64_t) val << 32 | next);
}

static void lv1_poke(uint64_t addr, uint64_t val)
{
    lv2syscall2(7, HV_BASE + addr, val);
}

static inline void _poke(uint64_t addr, uint64_t val)
{
    pokeq(0x8000000000000000ULL + addr, val);
}

static inline void _poke32(uint64_t addr, uint32_t val)
{
    pokeq32(0x8000000000000000ULL + addr, val);
}

int map_lv1(void)
{
    int result = lv1_undocumented_function_114(0, 0xC, HV_SIZE, &mmap_lpar_addr);
    if (result != 0)
    {
        return 0;
    }

    result = mm_map_lpar_memory_region(mmap_lpar_addr, HV_BASE, HV_SIZE, 0xC, 0);
    if (result)
    {
        return 0;
    }

    return 1;
}

void unmap_lv1(void)
{
    if (mmap_lpar_addr != 0)
        lv1_undocumented_function_115(mmap_lpar_addr);
}

void patch_lv2_protection(void)
{
    // changes protected area of lv2 to first byte only
    lv1_poke(0x363a78, 0x0000000000000001ULL);
    lv1_poke(0x363a80, 0xe0d251b556c59f05ULL);
    lv1_poke(0x363a88, 0xc232fcad552c80d7ULL);
    lv1_poke(0x363a90, 0x65140cd200000000ULL);
}

void install_new_poke(void)
{
    // install poke with icbi instruction
    pokeq(NEW_POKE_SYSCALL_ADDR, 0xF88300007C001FACULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4C00012C4E800020ULL);
    poke_syscall = NEW_POKE_SYSCALL;
}

void remove_new_poke(void)
{
    poke_syscall = 7;
    pokeq(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
}

/* vim: set ts=4 sw=4 sts=4 tw=120 */
