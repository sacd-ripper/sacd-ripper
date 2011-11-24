#ifndef __MM_H__
#define __MM_H__

#include <stdlib.h>

int mm_insert_htab_entry(uint64_t va_addr, uint64_t lpar_addr, uint64_t prot, uint64_t *index);
int mm_map_lpar_memory_region(uint64_t lpar_start_addr, uint64_t ea_start_addr, uint64_t size, uint64_t page_shift, uint64_t prot);

#endif
