/*
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 2, as published by the Free Software Foundation.
 */

#ifndef SELF_H__
#define SELF_H__

#include <stdint.h>
#include <stdio.h>

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define ATTRIBUTE_PACKED __attribute__ ((packed))
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#if PRAGMA_PACK
#pragma pack(1)
#endif

#define SCE_MAGIC 0x53434500
#define	ELF_MAGIC 0x7f454c46
#define	EI_NIDENT	16
#define SHT_NOTE	 7		/* Notes */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint16_t flags;
    uint16_t type;
    uint32_t metadata_offset;
    uint64_t header_len;
    uint64_t elf_filesize;
    uint64_t unknown;
    uint64_t appinfo_offset;
    uint64_t elf_offset;
    uint64_t phdr_offset;
    uint64_t shdr_offset;
    uint64_t section_info_offset;
    uint64_t sceversion_offset;
    uint64_t controlinfo_offset;
    uint64_t controlinfo_size;
    uint64_t padding;
} ATTRIBUTE_PACKED self_ehdr_t;

typedef struct {
    uint64_t authid;
    uint32_t vendor_id;
    uint32_t self_ehdr_type;
    uint32_t version;
    uint8_t padding[12];
} ATTRIBUTE_PACKED app_info_t;

typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} ATTRIBUTE_PACKED elf32_ehdr_t;

typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint16_t e_flags;
    uint32_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} ATTRIBUTE_PACKED elf_ehdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} ATTRIBUTE_PACKED elf32_phdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} ATTRIBUTE_PACKED elf_phdr_t;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} ATTRIBUTE_PACKED elf32_shdr_t;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} ATTRIBUTE_PACKED elf_shdr_t;

typedef struct {
    uint32_t n_namesz;
    uint32_t n_descsz;
    uint32_t n_type;
} ATTRIBUTE_PACKED elf32_nhdr_t;

typedef struct {
    uint64_t offset;
    uint64_t size;
    uint32_t compressed; // 2=compressed
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t encrypted; // 1=encrypted
} ATTRIBUTE_PACKED section_info_t;

typedef struct {
    uint32_t unknown1;
    uint32_t unknown2;
    uint32_t unknown3;
    uint32_t unknown4;
} ATTRIBUTE_PACKED sceversion_info_t;

typedef struct {
    uint32_t type; // 1==control flags; 2==file digest
    uint32_t size;

    union {
        // type 1

        struct {
            uint64_t control_flags;
            uint8_t padding[32];
        } control_flags;

        // type 2

        struct {
            uint64_t unknown;
            uint8_t digest1[20];
            uint8_t digest2[20];
            uint8_t padding[8];
        } file_digest;
    } info;
} ATTRIBUTE_PACKED control_info_t;

typedef struct {
    //uint8_t ignore[32];
    uint8_t key[16];
    uint8_t key_pad[16];
    uint8_t iv[16];
    uint8_t iv_pad[16];
} ATTRIBUTE_PACKED metadata_info_t;

typedef struct {
    uint64_t signature_input_length;
    uint32_t unknown1;
    uint32_t section_count;
    uint32_t key_count;
    uint32_t signature_info_size;
    uint64_t unknown2;
} ATTRIBUTE_PACKED metadata_header_t;

typedef struct {
    uint64_t data_offset;
    uint64_t data_size;
    uint32_t type; // 1 = shdr, 2 == phdr
    uint32_t program_idx;
    uint32_t unknown;
    uint32_t sha1_idx;
    uint32_t encrypted; // 3=yes; 1=no
    uint32_t key_idx;
    uint32_t iv_idx;
    uint32_t compressed; // 2=yes; 1=no
} ATTRIBUTE_PACKED metadata_section_header_t;

typedef struct {
    uint8_t sha1[20];
    uint8_t padding[12];
    uint8_t hmac_key[64];
} ATTRIBUTE_PACKED SECTION_HASH;

typedef struct {
    uint32_t unknown1;
    uint32_t signature_size;
    uint64_t unknown2;
    uint64_t unknown3;
    uint64_t unknown4;
    uint64_t unknown5;
    uint32_t unknown6;
    uint32_t unknown7;
} ATTRIBUTE_PACKED signature_info_t;

typedef struct {
    uint8_t r[21];
    uint8_t s[21];
    uint8_t padding[6];
} ATTRIBUTE_PACKED signature_t;

typedef struct {
    uint8_t *data;
    uint64_t size;
    uint64_t offset;
} self_section_t;

#if PRAGMA_PACK
#pragma pack()
#endif

void self_read_headers(FILE *in, self_ehdr_t *self, app_info_t *app_info, elf_ehdr_t *elf,
        elf_phdr_t **phdr, elf_shdr_t **shdr, section_info_t **section_info,
        sceversion_info_t *sceversion_info, control_info_t **control_info);

void self_read_metadata(FILE *in, self_ehdr_t *self, app_info_t *app_info,
        metadata_info_t *metadata_info, metadata_header_t *metadata_header,
        metadata_section_header_t **section_headers, uint8_t **keys,
        signature_info_t *signature_info, signature_t *signature);

int self_load_sections(FILE *in, self_ehdr_t *self, elf_ehdr_t *elf, elf_phdr_t **phdr,
        metadata_header_t *metadata_header, metadata_section_header_t **section_headers,
        uint8_t **keys, self_section_t **sections);

void self_free_sections(self_section_t **sections, uint32_t num_sections);

size_t find_isoself(self_section_t *sections, int num_sections,
        const char *spuname, uint8_t **isoself_data);

#endif /* SELF_H__ */
