#ifndef __SAC_ACCESSOR_H__
#define __SAC_ACCESSOR_H__

#include <sys/cdefs.h>
#include <sys/syscall.h>

#include <sys/integertypes.h>
#include <sys/return_code.h>

#include <sys/interrupt.h>
#include <sys/fixed_addr.h>

#include <sys/raw_spu.h>

CDECL_BEGIN

//#define USE_ISOSELF 1

#define INTR_PPU_MB_SHIFT   0
#define INTR_STOP_SHIFT     1
#define INTR_HALT_SHIFT     2
#define INTR_DMA_SHIFT      3
#define INTR_SPU_MB_SHIFT   4 
#define INTR_PPU_MB_MASK    (0x1 << INTR_PPU_MB_SHIFT)
#define INTR_STOP_MASK      (0x1 << INTR_STOP_SHIFT)
#define INTR_HALT_MASK      (0x1 << INTR_HALT_SHIFT)
#define INTR_DMA_MASK       (0x1 << INTR_DMA_SHIFT)
#define INTR_SPU_MB_MASK    (0x1 << INTR_SPU_MB_SHIFT)

#define SPU_INTR_CLASS_2      ( 2 )

#define MAX_PHYSICAL_SPU       6 
#define MAX_RAW_SPU            1

#define EIEIO __asm__ volatile("eieio")

/** configuration of sac accessor thread */
#define PRIMARY_PPU_THREAD_PRIO ( 1001 )
#define PRIMARY_PPU_STACK_SIZE  ( 0x2000 )

#define DMA_BUFFER_SIZE  ( 0x2000 )
#define EXIT_SAC_CMD ( 0xFEFEFEFF )

#ifdef USE_ISOSELF
	
	#define SAC_MODULE_LOCATION "/dev_flash/vsh/module/SacModule.spu.isoself"
	
	static inline int sys_isoself_spu_create(sys_raw_spu_t *id, uint8_t *source_spe) {
	    system_call_6(230, (uint32_t) id, (uint32_t) source_spe, 0, 0, 0, 0);
	    return_to_user_prog(int);
	}
	
	static inline int sys_isoself_spu_destroy(sys_raw_spu_t id) {
	    system_call_1(231, id);
	    return_to_user_prog(int);
	}
	
	static inline int sys_isoself_spu_start(sys_raw_spu_t id) {
	    system_call_1(232, id);
	    return_to_user_prog(int);
	}
	
	static inline int sys_isoself_spu_create_interrupt_tag(sys_raw_spu_t id,
	        sys_class_id_t class_id,
	        sys_hw_thread_t hwthread,
	        sys_interrupt_tag_t *intrtag) {
	    system_call_4(233, id, class_id, hwthread, (uint32_t) intrtag);
	    return_to_user_prog(int);
	}
	
	static inline int sys_isoself_spu_set_int_mask(sys_raw_spu_t id,
	        sys_class_id_t class_id,
	        __CSTD uint64_t mask) {
	    system_call_3(234, id, class_id, mask);
	    return_to_user_prog(int);
	}
	
	static inline int sys_isoself_spu_set_int_stat(sys_raw_spu_t id,
	        sys_class_id_t class_id,
	        __CSTD uint64_t stat) {
	    system_call_3(236, id, class_id, stat);
	    return_to_user_prog(int);
	}
	
	static inline int sys_isoself_spu_get_int_stat(sys_raw_spu_t id,
	        sys_class_id_t class_id,
	        __CSTD uint64_t * stat) {
	    system_call_3(237, id, class_id, (uint32_t) stat);
	    return_to_user_prog(int);
	}
	
	static inline int sys_isoself_spu_read_puint_mb(sys_raw_spu_t id,
	        __CSTD uint32_t * value) {
	    system_call_2(240, id, (uint32_t) value);
	    return_to_user_prog(int);
	}	
	
#else
	#define SAC_MODULE_LOCATION "/dev_usb000/SacModule.spu.elf"
#endif

enum {
      SAC_CMD_INITIALIZE     = 0
    , SAC_CMD_EXIT           = 1
    , SAC_CMD_GENERATE_KEY_1 = 2
    , SAC_CMD_VALIDATE_KEY_1 = 3
    , SAC_CMD_GENERATE_KEY_2 = 4
    , SAC_CMD_VALIDATE_KEY_2 = 5
    , SAC_CMD_VALIDATE_KEY_3 = 6
    , SAC_CMD_ENCRYPT = 7
    , SAC_CMD_DECRYPT = 8
} sac_command_t;

typedef struct {
    uint8_t *buffer;

    uint8_t *module_buffer;
    unsigned int module_size;

    sys_ppu_thread_t handler;

    sys_cond_t mmio_cond;
    sys_mutex_t mmio_mutex;

    sys_raw_spu_t id;
    sys_interrupt_thread_handle_t ih;
    sys_interrupt_tag_t intrtag;

    uint32_t error_code;
} sac_accessor_t;

extern sac_accessor_t *sa;
extern int create_sac_accessor(void);
extern int destroy_sac_accessor(void);
extern int sac_exec_initialize(void);
extern int sac_exec_key_exchange(int);
extern int sac_exec_decrypt_data(uint8_t *, uint32_t, uint8_t *);
extern int sac_exec_exit(void);

CDECL_END

#endif /* __SAC_ACCESSOR_H__ */
