// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef EMULATE_H__
#define EMULATE_H__

#include "types.h"

typedef struct spe_mfc_command_area {
    uint32_t mfc_lsa;
    uint32_t mfc_eah;
    uint32_t mfc_eal;
    uint32_t mfc_size_tag;
    union {     
        uint32_t mfc_class_id_cmd;
        uint32_t mfc_cmd_status;      
    };      
    uint32_t prxy_query_type;
    uint32_t prxy_query_mask;     
    uint32_t prxy_tag_status;
} spe_mfc_command_area_t;

typedef struct spe_ctx {
    uint8_t *ls;
    uint32_t reg[128][4];
    uint32_t pc;
    uint32_t trap;

    spe_mfc_command_area_t mfc;

    //mailboxes
    uint32_t spu_out_mbox;
    uint32_t spu_out_cnt;
    uint32_t spu_out_intr_mbox;
    uint32_t spu_out_intr_cnt;
    uint32_t spu_in_mbox[4];
    uint32_t spu_in_cnt;
    int32_t spu_in_rdidx;

    int32_t count;

} spe_ctx_t;

uint32_t emulate(spe_ctx_t *ctx);

typedef int (*spu_instr_rr_t)(spe_ctx_t *ctx, uint32_t ra, uint32_t rb, uint32_t rt);
typedef int (*spu_instr_rrr_t)(spe_ctx_t *ctx, uint32_t ra, uint32_t rb, uint32_t rc, uint32_t rt);
typedef int (*spu_instr_ri7_t)(spe_ctx_t *ctx, uint32_t i7, uint32_t ra, uint32_t rt);
typedef int (*spu_instr_ri8_t)(spe_ctx_t *ctx, uint32_t i7, uint32_t ra, uint32_t rt); 
typedef int (*spu_instr_ri10_t)(spe_ctx_t *ctx, uint32_t i10, uint32_t ra, uint32_t rt);
typedef int (*spu_instr_ri16_t)(spe_ctx_t *ctx, uint32_t i16, uint32_t rt);
typedef int (*spu_instr_ri18_t)(spe_ctx_t *ctx, uint32_t i18, uint32_t rt);
typedef int (*spu_instr_special_t)(spe_ctx_t *ctx, uint32_t instr);

spe_ctx_t* create_spe_context(void);
void destroy_spe_context(spe_ctx_t *ctx);
void spe_in_mbox_write(spe_ctx_t *ctx, uint32_t msg);
void spe_out_intr_mbox_status(spe_ctx_t *ctx);
uint32_t spe_out_intr_mbox_read(spe_ctx_t *ctx);

#endif
