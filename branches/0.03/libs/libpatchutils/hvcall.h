#ifndef HVCALL_H
#define HVCALL_H

#define HVSC_SYSCALL                    811                     // which syscall to overwrite with hvsc redirect
#define HVSC_SYSCALL_ADDR               0x8000000000195540ULL   // where above syscall is in lv2
#define NEW_POKE_SYSCALL                813                     // which syscall to overwrite with new poke
#define NEW_POKE_SYSCALL_ADDR           0x8000000000195A68ULL   // where above syscall is in lv2

#define HV_BASE                         0x8000000014000000ULL   // where in lv2 to map lv1
#define HV_SIZE                         0x370000                // size of lv1 memory to map/dump

#define HPTE_V_BOLTED			0x0000000000000010ULL
#define HPTE_V_LARGE			0x0000000000000004ULL
#define HPTE_V_VALID			0x0000000000000001ULL
#define HPTE_R_PROT_MASK		0x0000000000000003ULL
#define MM_EA2VA(ea)			((ea) & ~0x8000000000000000ULL)

#define SC_QUOTE_(x) #x
#define SYSCALL(num) "li %%r11, " SC_QUOTE_(num) "; sc;"

#define INSTALL_HVSC_REDIRECT(hvcall) uint64_t original_syscall_code_1 = peekq(HVSC_SYSCALL_ADDR); \
	uint64_t original_syscall_code_2 = peekq(HVSC_SYSCALL_ADDR + 8); \
	uint64_t original_syscall_code_3 = peekq(HVSC_SYSCALL_ADDR + 16); \
	uint64_t original_syscall_code_4 = peekq(HVSC_SYSCALL_ADDR + 24); \
	pokeq(HVSC_SYSCALL_ADDR, 0x7C0802A6F8010010ULL);	\
	pokeq(HVSC_SYSCALL_ADDR + 8, 0x3960000044000022ULL | (uint64_t)hvcall << 32);	\
	pokeq(HVSC_SYSCALL_ADDR + 16, 0xE80100107C0803A6ULL); \
	pokeq(HVSC_SYSCALL_ADDR + 24, 0x4e80002060000000ULL);
	
#define REMOVE_HVSC_REDIRECT() pokeq(HVSC_SYSCALL_ADDR, original_syscall_code_1); \
	pokeq(HVSC_SYSCALL_ADDR + 8, original_syscall_code_2); \
	pokeq(HVSC_SYSCALL_ADDR + 16, original_syscall_code_3); \
	pokeq(HVSC_SYSCALL_ADDR + 24, original_syscall_code_4);

int lv1_insert_htab_entry(uint64_t htab_id, uint64_t hpte_group, uint64_t hpte_v, uint64_t hpte_r, uint64_t bolted_flag, uint64_t flags, uint64_t *hpte_index, uint64_t *hpte_evicted_v, uint64_t *hpte_evicted_r);
int lv1_allocate_memory(uint64_t size, uint64_t page_size_exp, uint64_t flags, uint64_t *addr, uint64_t *muid);
int lv1_undocumented_function_114(uint64_t start, uint64_t page_size, uint64_t size, uint64_t *lpar_addr);
void lv1_undocumented_function_115(uint64_t lpar_addr);

uint64_t lv2_alloc(uint64_t size, uint64_t pool);

#endif
