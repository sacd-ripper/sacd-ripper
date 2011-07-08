#ifndef __PATCH_UTILS_H__
#define __PATCH_UTILS_H__

#include <ppu-lv2.h>
#include <hvcall.h>

#include "mm.h"

#define FW_341_ADDR  0x80000000002d7580
#define FW_341_VALUE 0x0000000000008534
#define FW_355_ADDR  0x80000000003329b8
#define FW_355_VALUE 0x0000000000008aac
#define FW_UNK_VALUE 0

u64 mmap_lpar_addr;

u64 lv2peek(u64 addr);
void lv2poke(u64 addr, u64 value);
void lv2poke32(u64 addr, u32 value);

void lv1poke(u64 addr, u64 value);

int map_lv1();
void unmap_lv1(void);

void patch_lv2_protection();

void install_new_poke();
void remove_new_poke();

int remove_protection();

int get_version();

#endif
