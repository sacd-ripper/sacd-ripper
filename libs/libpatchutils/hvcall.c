/* Taken from lv1dumper by flukes1 */

#include <stdint.h>
#include <stdbool.h>
#include "hvcall.h"
#include "patch-utils.h"

int lv1_insert_htab_entry(uint64_t htab_id, uint64_t hpte_group, uint64_t hpte_v, uint64_t hpte_r, uint64_t bolted_flag,
						  uint64_t flags, uint64_t * hpte_index, uint64_t * hpte_evicted_v, uint64_t * hpte_evicted_r)
{
	INSTALL_HVSC_REDIRECT(0x9E);	// redirect to hvcall 158

	// call lv1_insert_htab_entry
	uint64_t ret = 0, ret_hpte_index = 0, ret_hpte_evicted_v = 0, ret_hpte_evicted_r = 0;
	__asm__ __volatile__("mr %%r3, %4;" "mr %%r4, %5;" "mr %%r5, %6;" "mr %%r6, %7;" "mr %%r7, %8;" "mr %%r8, %9;"
						 SYSCALL(HVSC_SYSCALL) "mr %0, %%r3;" "mr %1, %%r4;" "mr %2, %%r5;" "mr %3, %%r6;":"=r"(ret),
						 "=r"(ret_hpte_index), "=r"(ret_hpte_evicted_v), "=r"(ret_hpte_evicted_r)
						 :"r"(htab_id), "r"(hpte_group), "r"(hpte_v), "r"(hpte_r), "r"(bolted_flag), "r"(flags)
						 :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
						 "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

	REMOVE_HVSC_REDIRECT();

	*hpte_index = ret_hpte_index;
	*hpte_evicted_v = ret_hpte_evicted_v;
	*hpte_evicted_r = ret_hpte_evicted_r;
	return (int) ret;
}

int lv1_allocate_memory(uint64_t size, uint64_t page_size_exp, uint64_t flags, uint64_t * addr, uint64_t * muid)
{
	INSTALL_HVSC_REDIRECT(0x0);	// redirect to hvcall 0

	// call lv1_allocate_memory
	uint64_t ret = 0, ret_addr = 0, ret_muid = 0;
	__asm__ __volatile__("mr %%r3, %3;" "mr %%r4, %4;" "li %%r5, 0;" "mr %%r6, %5;" SYSCALL(HVSC_SYSCALL) "mr %0, %%r3;"
						 "mr %1, %%r4;" "mr %2, %%r5;":"=r"(ret), "=r"(ret_addr), "=r"(ret_muid)
						 :"r"(size), "r"(page_size_exp), "r"(flags)
						 :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
						 "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

	REMOVE_HVSC_REDIRECT();

	*addr = ret_addr;
	*muid = ret_muid;
	return (int) ret;
}

int lv1_undocumented_function_114(uint64_t start, uint64_t page_size, uint64_t size, uint64_t * lpar_addr)
{
	INSTALL_HVSC_REDIRECT(0x72);	// redirect to hvcall 114

	// call lv1_undocumented_function_114
	uint64_t ret = 0, ret_lpar_addr = 0;
	__asm__ __volatile__("mr %%r3, %2;" "mr %%r4, %3;" "mr %%r5, %4;" SYSCALL(HVSC_SYSCALL) "mr %0, %%r3;"
						 "mr %1, %%r4;":"=r"(ret), "=r"(ret_lpar_addr)
						 :"r"(start), "r"(page_size), "r"(size)
						 :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
						 "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

	REMOVE_HVSC_REDIRECT();

	*lpar_addr = ret_lpar_addr;
	return (int) ret;
}

void lv1_undocumented_function_115(uint64_t lpar_addr)
{
	INSTALL_HVSC_REDIRECT(0x73);	// redirect to hvcall 115

	// call lv1_undocumented_function_115
	__asm__ __volatile__("mr %%r3, %0;" SYSCALL(HVSC_SYSCALL)
						 :		// no return registers
						 :"r"(lpar_addr)
						 :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
						 "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

	REMOVE_HVSC_REDIRECT();
}

uint64_t lv2_alloc(uint64_t size, uint64_t pool)
{
	// setup syscall to redirect to alloc func
	uint64_t original_syscall_code_1 = peekq(HVSC_SYSCALL_ADDR);
	uint64_t original_syscall_code_2 = peekq(HVSC_SYSCALL_ADDR + 8);
	uint64_t original_syscall_code_3 = peekq(HVSC_SYSCALL_ADDR + 16);
	pokeq(HVSC_SYSCALL_ADDR, 0x7C0802A67C140378ULL);
	pokeq(HVSC_SYSCALL_ADDR + 8, 0x4BECB6317E80A378ULL);
	pokeq(HVSC_SYSCALL_ADDR + 16, 0x7C0803A64E800020ULL);

	uint64_t ret = 0;
	__asm__ __volatile__("mr %%r3, %1;" "mr %%r4, %2;" SYSCALL(HVSC_SYSCALL) "mr %0, %%r3;":"=r"(ret)
						 :"r"(size), "r"(pool)
						 :"r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "lr", "ctr", "xer",
						 "cr0", "cr1", "cr5", "cr6", "cr7", "memory");

	// restore original syscall code
	pokeq(HVSC_SYSCALL_ADDR, original_syscall_code_1);
	pokeq(HVSC_SYSCALL_ADDR + 8, original_syscall_code_2);
	pokeq(HVSC_SYSCALL_ADDR + 16, original_syscall_code_3);

	return ret;
}
