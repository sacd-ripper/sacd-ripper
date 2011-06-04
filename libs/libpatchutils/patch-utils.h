#ifndef _PATCH_UTILS_H__
#define _PATCH_UTILS_H__

extern uint64_t peekq(uint64_t addr);
extern void pokeq(uint64_t addr, uint64_t val);
extern void pokeq32(uint64_t addr, uint32_t val);

extern int map_lv1(void);
extern void unmap_lv1(void);
extern void patch_lv2_protection(void);
extern void install_new_poke(void);
extern void remove_new_poke(void);

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
