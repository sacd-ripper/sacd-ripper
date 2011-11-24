#include "patch-utils.h"

int poke_syscall = 7;

u64 lv2peek(u64 addr) {
	lv2syscall1(6, addr);
	return_to_user_prog(u64);
}

void lv2poke(u64 addr, u64 value) {
	lv2syscall2(poke_syscall, addr, value);
}
void lv2poke32(u64 addr, u32 value) {
	lv2poke(addr, (u64) value << 32 ^ (lv2peek(addr) & 0xFFFFFFFF));
}

void lv1poke(u64 addr, u64 value) {
    lv2syscall2(7, HV_BASE + addr, value);
}

int map_lv1() {
    int result = lv1_undocumented_function_114(0, 0xC, HV_SIZE, &mmap_lpar_addr);
    if (result != 0) {
        return 0;
    }

    result = mm_map_lpar_memory_region(mmap_lpar_addr, HV_BASE, HV_SIZE, 0xC, 0);
    if (result) {
        return 0;
    }

    return 1;
}
void unmap_lv1() {
    if (mmap_lpar_addr != 0) {
		lv1_undocumented_function_115(mmap_lpar_addr);
	}
}

void patch_lv2_protection() {
    // changes protected area of lv2 to first byte only
    lv1poke(0x363a78, 0x0000000000000001ULL);
    lv1poke(0x363a80, 0xe0d251b556c59f05ULL);
    lv1poke(0x363a88, 0xc232fcad552c80d7ULL);
    lv1poke(0x363a90, 0x65140cd200000000ULL);
}

void install_new_poke() {
    // install poke with icbi instruction
    lv2poke(NEW_POKE_SYSCALL_ADDR, 0xF88300007C001FACULL);
    lv2poke(NEW_POKE_SYSCALL_ADDR + 8, 0x4C00012C4E800020ULL);
    poke_syscall = NEW_POKE_SYSCALL;
}
void remove_new_poke() {
    poke_syscall = 7;
    lv2poke(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
    lv2poke(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
}

int remove_protection() {
	//Install new Poke Syscall.
	install_new_poke();

	//Try to map LV1.
	if (!map_lv1()) {
		remove_new_poke();
		return -1;
	}

	//Patch LV2 protection and remove the new Poke Syscall.
	patch_lv2_protection();
	remove_new_poke();

	//Finally unmap LV1.
	unmap_lv1();

	return 0;
}

int get_version() {
	if(lv2peek(FW_341_ADDR) == FW_341_VALUE) {
		return FW_341_VALUE;
	}
	else if(lv2peek(FW_355_ADDR) == FW_355_VALUE) {
		return FW_355_VALUE;
	}
	else {
		return FW_UNK_VALUE;
	}
}
