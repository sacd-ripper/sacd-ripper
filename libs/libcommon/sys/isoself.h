/**
 * SACD Ripper - http://code.google.com/p/sacd-ripper/
 *
 * Copyright (c) 2010-2011 by respective authors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __ISOSELF_H__
#define __ISOSELF_H__

#ifndef __lv2ppu__
#error you need the psl1ght/lv2 ppu compatible compiler!
#endif

#include <stdint.h>
#include <ppu-lv2.h>
#include <sys/spu.h>
#include <sys/interrupt.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int sys_isoself_spu_create(sys_raw_spu_t *id, uint8_t *source_spe)
{
    lv2syscall6(230, (uint64_t) id, (uint64_t) source_spe, 0, 0, 0, 0);
    return_to_user_prog(int);
}

static inline int sys_isoself_spu_destroy(sys_raw_spu_t id)
{
    lv2syscall1(231, id);
    return_to_user_prog(int);
}

static inline int sys_isoself_spu_start(sys_raw_spu_t id)
{
    lv2syscall1(232, id);
    return_to_user_prog(int);
}

static inline int sys_isoself_spu_create_interrupt_tag(sys_raw_spu_t id,
                                                       uint32_t class_id,
                                                       uint32_t hwthread,
                                                       sys_interrupt_tag_t *intrtag)
{
    lv2syscall4(233, id, class_id, hwthread, (uint64_t) intrtag);
    return_to_user_prog(int);
}

static inline int sys_isoself_spu_set_int_mask(sys_raw_spu_t id,
                                               uint32_t class_id,
                                               uint64_t mask)
{
    lv2syscall3(234, id, class_id, mask);
    return_to_user_prog(int);
}

static inline int sys_isoself_spu_set_int_stat(sys_raw_spu_t id,
                                               uint32_t class_id,
                                               uint64_t stat)
{
    lv2syscall3(236, id, class_id, stat);
    return_to_user_prog(int);
}

static inline int sys_isoself_spu_get_int_stat(sys_raw_spu_t id,
                                               uint32_t class_id,
                                               uint64_t * stat)
{
    lv2syscall3(237, id, class_id, (uint64_t) stat);
    return_to_user_prog(int);
}

static inline int sys_isoself_spu_read_puint_mb(sys_raw_spu_t id,
                                                uint32_t * value)
{
    lv2syscall2(240, id, (uint64_t) value);
    return_to_user_prog(int);
}

#ifdef __cplusplus
};
#endif

#endif /* __ISOSELF_H__ */
