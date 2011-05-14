/* Taken from lv1dumper by flukes1 */

#include <stdint.h>
#include "mm.h"
#include "hvcall.h"

// This is mainly adapted from graf's code

int mm_insert_htab_entry(uint64_t va_addr, uint64_t lpar_addr, uint64_t prot, uint64_t * index)
{
	uint64_t hpte_group, hpte_index = 0, hpte_v, hpte_r, hpte_evicted_v, hpte_evicted_r;

	hpte_group = (((va_addr >> 28) ^ ((va_addr & 0xFFFFFFFULL) >> 12)) & 0x7FF) << 3;
	hpte_v = ((va_addr >> 23) << 7) | HPTE_V_VALID;
	hpte_r = lpar_addr | 0x38 | (prot & HPTE_R_PROT_MASK);

	int result = lv1_insert_htab_entry(0, hpte_group, hpte_v, hpte_r, HPTE_V_BOLTED, 0,
									   &hpte_index, &hpte_evicted_v, &hpte_evicted_r);

	if (result != 0) {
	}

	if ((result == 0) && (index != 0))
		*index = hpte_index;

	return (int) result;
}

int mm_map_lpar_memory_region(uint64_t lpar_start_addr, uint64_t ea_start_addr, uint64_t size, uint64_t page_shift,
							  uint64_t prot)
{
	uint32_t i, result;

	for (i = 0; i < size >> page_shift; i++) {
		result = mm_insert_htab_entry(MM_EA2VA(ea_start_addr), lpar_start_addr, prot, 0);
		if (result != 0)
			return result;

		lpar_start_addr += (1 << page_shift);
		ea_start_addr += (1 << page_shift);
	}

	return 0;
}
