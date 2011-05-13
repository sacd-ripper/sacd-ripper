/*
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 2, as published by the Free Software Foundation.
 */

#include "tools.h"
#include "self.h"
#include "common.h"
#include "aes.h"

#include <string.h>
#include <stdlib.h>

static struct keylist_t *load_keys(app_info_t *app_info);
static int decrypt_metadata(uint8_t *metadata, uint32_t metadata_size,
        struct keylist_t *klist);

int
self_read_headers(FILE *in, self_ehdr_t *self, app_info_t *app_info, elf_ehdr_t *elf,
        elf_phdr_t **phdr, elf_shdr_t **shdr, section_info_t **section_info,
        sceversion_info_t *sceversion_info, control_info_t **control_info) {
    int arch64 = 0;

    // SELF
    if (fread(self, sizeof (self_ehdr_t), 1, in) != 1) {
        return -1;
    }

    self->magic = swap32(self->magic);
    self->version = swap32(self->version);
    self->flags = swap16(self->flags);
    self->type = swap16(self->type);
    self->metadata_offset = swap32(self->metadata_offset);
    self->header_len = swap64(self->header_len);
    self->elf_filesize = swap64(self->elf_filesize);
    self->appinfo_offset = swap64(self->appinfo_offset);
    self->elf_offset = swap64(self->elf_offset);
    self->phdr_offset = swap64(self->phdr_offset);
    self->shdr_offset = swap64(self->shdr_offset);
    self->section_info_offset = swap64(self->section_info_offset);
    self->sceversion_offset = swap64(self->sceversion_offset);
    self->controlinfo_offset = swap64(self->controlinfo_offset);
    self->controlinfo_size = swap64(self->controlinfo_size);

    if (self->magic != SCE_MAGIC) {
        return -1;
    }

    // APP INFO
    if (app_info) {
        fseek(in, (long) self->appinfo_offset, SEEK_SET);
        if (fread(app_info, sizeof (app_info_t), 1, in) != 1) {
            return -1;
        }
        app_info->authid = swap64(app_info->authid);
        app_info->vendor_id = swap32(app_info->vendor_id);
        app_info->self_ehdr_type = swap32(app_info->self_ehdr_type);
        app_info->version = swap32(app_info->version);
    }

    // ELF
    if (elf) {
        fseek(in, (long) self->elf_offset, SEEK_SET);
        if (fread(elf, sizeof (elf_ehdr_t), 1, in) != 1) {
            return -1;
        }
        arch64 = elf->e_ident[4] == 2;
        if (arch64) {
            elf->e_type = swap16(elf->e_type);
            elf->e_machine = swap16(elf->e_machine);
            elf->e_version = swap32(elf->e_version);
            elf->e_entry = swap64(elf->e_entry);
            elf->e_phoff = swap64(elf->e_phoff);
            elf->e_shoff = swap64(elf->e_shoff);
            elf->e_flags = swap16(elf->e_flags);
            elf->e_ehsize = swap32(elf->e_ehsize);
            elf->e_phentsize = swap16(elf->e_phentsize);
            elf->e_phnum = swap16(elf->e_phnum);
            elf->e_shentsize = swap16(elf->e_shentsize);
            elf->e_shnum = swap16(elf->e_shnum);
            elf->e_shstrndx = swap16(elf->e_shstrndx);
        } else {
            elf32_ehdr_t elf32;
            fseek(in, (long) self->elf_offset, SEEK_SET);
            if (fread(&elf32, sizeof (elf32_ehdr_t), 1, in) != 1) {
                return -1;
            }
            elf->e_type = swap16(elf32.e_type);
            elf->e_machine = swap16(elf32.e_machine);
            elf->e_version = swap32(elf32.e_version);
            elf->e_entry = swap32(elf32.e_entry);
            elf->e_phoff = swap32(elf32.e_phoff);
            elf->e_shoff = swap32(elf32.e_shoff);
            elf->e_flags = swap32(elf32.e_flags);
            elf->e_ehsize = swap16(elf32.e_ehsize);
            elf->e_phentsize = swap16(elf32.e_phentsize);
            elf->e_phnum = swap16(elf32.e_phnum);
            elf->e_shentsize = swap16(elf32.e_shentsize);
            elf->e_shnum = swap16(elf32.e_shnum);
            elf->e_shstrndx = swap16(elf32.e_shstrndx);
        }
    }

    // PHDR and SECTION INFO
    if (phdr || section_info) {
        uint16_t phnum = 0;
        uint16_t i;

        if (elf) {
            phnum = elf->e_phnum;
        } else {
            fseek(in, (long) self->elf_offset + 52, SEEK_SET);
            fread(&phnum, sizeof (uint16_t), 1, in);
        }

        if (phdr) {
            elf_phdr_t *elf_phdr = NULL;

            elf_phdr = malloc(sizeof (elf_phdr_t) * phnum);

            if (arch64) {
                fseek(in, (long) self->phdr_offset, SEEK_SET);
                if (fread(elf_phdr, sizeof (elf_phdr_t), phnum, in) != phnum) {
                    return -1;
                }

                for (i = 0; i < phnum; i++) {
                    elf_phdr[i].p_type = swap32(elf_phdr[i].p_type);
                    elf_phdr[i].p_flags = swap32(elf_phdr[i].p_flags);
                    elf_phdr[i].p_offset = swap64(elf_phdr[i].p_offset);
                    elf_phdr[i].p_vaddr = swap64(elf_phdr[i].p_vaddr);
                    elf_phdr[i].p_paddr = swap64(elf_phdr[i].p_paddr);
                    elf_phdr[i].p_filesz = swap64(elf_phdr[i].p_filesz);
                    elf_phdr[i].p_memsz = swap64(elf_phdr[i].p_memsz);
                    elf_phdr[i].p_align = swap64(elf_phdr[i].p_align);
                }
            } else {
                elf32_phdr_t *elf32_phdr = malloc(sizeof (elf32_phdr_t) * phnum);
                fseek(in, (long) self->phdr_offset, SEEK_SET);
                if (fread(elf32_phdr, sizeof (elf32_phdr_t), phnum, in) != phnum) {
                    return -1;
                }

                for (i = 0; i < phnum; i++) {
                    elf_phdr[i].p_type = swap32(elf32_phdr[i].p_type);
                    elf_phdr[i].p_offset = swap32(elf32_phdr[i].p_offset);
                    elf_phdr[i].p_vaddr = swap32(elf32_phdr[i].p_vaddr);
                    elf_phdr[i].p_paddr = swap32(elf32_phdr[i].p_paddr);
                    elf_phdr[i].p_filesz = swap32(elf32_phdr[i].p_filesz);
                    elf_phdr[i].p_memsz = swap32(elf32_phdr[i].p_memsz);
                    elf_phdr[i].p_flags = swap32(elf32_phdr[i].p_flags);
                    elf_phdr[i].p_align = swap32(elf32_phdr[i].p_align);
                }
                free(elf32_phdr);
            }

            *phdr = elf_phdr;
        }

        // SECTION INFO
        if (section_info) {
            section_info_t *sections = NULL;

            sections = malloc(sizeof (section_info_t) * phnum);

            fseek(in, (long) self->section_info_offset, SEEK_SET);
            if (fread(sections, sizeof (section_info_t), phnum, in) != phnum) {
                return -1;
            }

            for (i = 0; i < phnum; i++) {
                sections[i].offset = swap64(sections[i].offset);
                sections[i].size = swap64(sections[i].size);
                sections[i].compressed = swap32(sections[i].compressed);
                sections[i].encrypted = swap32(sections[i].encrypted);
            }

            *section_info = sections;
        }
    }

    // SCE VERSION INFO
    if (sceversion_info) {
        fseek(in, (long) self->sceversion_offset, SEEK_SET);
        if (fread(sceversion_info, sizeof (sceversion_info_t), 1, in) != 1) {
            return -1;
        }
    }

    // CONTROL INFO
    if (control_info) {
        uint32_t offset = 0;
        uint32_t index = 0;
        control_info_t *info = NULL;

        while (offset < self->controlinfo_size) {

            info = realloc(info, sizeof (control_info_t) * (index + 1));

            fseek(in, (long) self->controlinfo_offset + offset, SEEK_SET);

            if (fread(info + index, sizeof (control_info_t), 1, in) != 1) {
                return -1;
            }

            info[index].type = swap32(info[index].type);
            info[index].size = swap32(info[index].size);
            if (info[index].type == 1)
                info[index].info.control_flags.control_flags =
                    swap64(info[index].info.control_flags.control_flags);

            offset += info[index].size;
            index++;
        }
        *control_info = info;
    }


    // SHDR
    if (shdr) {
        uint16_t shnum = 0;
        uint16_t i;
        elf_shdr_t *elf_shdr = NULL;

        if (elf) {
            shnum = elf->e_shnum;
        } else {
            fseek(in, (long) self->elf_offset + 56, SEEK_SET);
            fread(&shnum, sizeof (uint16_t), 1, in);
        }

        if (shnum > 0 && self->shdr_offset != 0) {
            elf_shdr = malloc(sizeof (elf_shdr_t) * shnum);

            if (arch64) {
                fseek(in, (long) self->shdr_offset, SEEK_SET);
                if (fread(elf_shdr, sizeof (elf_shdr_t), shnum, in) != shnum) {
                    return -1;
                }

                for (i = 0; i < shnum; i++) {
                    elf_shdr[i].sh_name = swap32(elf_shdr[i].sh_name);
                    elf_shdr[i].sh_type = swap32(elf_shdr[i].sh_type);
                    elf_shdr[i].sh_flags = swap64(elf_shdr[i].sh_flags);
                    elf_shdr[i].sh_addr = swap64(elf_shdr[i].sh_addr);
                    elf_shdr[i].sh_offset = swap64(elf_shdr[i].sh_offset);
                    elf_shdr[i].sh_size = swap64(elf_shdr[i].sh_size);
                    elf_shdr[i].sh_link = swap32(elf_shdr[i].sh_link);
                    elf_shdr[i].sh_info = swap32(elf_shdr[i].sh_info);
                    elf_shdr[i].sh_addralign = swap64(elf_shdr[i].sh_addralign);
                    elf_shdr[i].sh_entsize = swap64(elf_shdr[i].sh_entsize);
                }
            } else {
                elf32_shdr_t *elf32_shdr = malloc(sizeof (elf32_shdr_t) * shnum);

                fseek(in, (long) self->shdr_offset, SEEK_SET);
                if (fread(elf32_shdr, sizeof (elf32_shdr_t), shnum, in) != shnum) {
                    return -1;
                }

                for (i = 0; i < shnum; i++) {
                    elf_shdr[i].sh_name = swap32(elf32_shdr[i].sh_name);
                    elf_shdr[i].sh_type = swap32(elf32_shdr[i].sh_type);
                    elf_shdr[i].sh_flags = swap32(elf32_shdr[i].sh_flags);
                    elf_shdr[i].sh_addr = swap32(elf32_shdr[i].sh_addr);
                    elf_shdr[i].sh_offset = swap32(elf32_shdr[i].sh_offset);
                    elf_shdr[i].sh_size = swap32(elf32_shdr[i].sh_size);
                    elf_shdr[i].sh_link = swap32(elf32_shdr[i].sh_link);
                    elf_shdr[i].sh_info = swap32(elf32_shdr[i].sh_info);
                    elf_shdr[i].sh_addralign = swap32(elf32_shdr[i].sh_addralign);
                    elf_shdr[i].sh_entsize = swap32(elf32_shdr[i].sh_entsize);
                }
                free(elf32_shdr);
            }

            *shdr = elf_shdr;
        }
    }

	return 0;
}

int
self_read_metadata(FILE *in, self_ehdr_t *self, app_info_t *app_info,
        metadata_info_t *metadata_info, metadata_header_t *metadata_header,
        metadata_section_header_t **section_headers, uint8_t **keys,
        signature_info_t *signature_info, signature_t *signature) {
    uint8_t *metadata = NULL;
    uint32_t metadata_size = (uint32_t) self->header_len - self->metadata_offset - 0x20;
    uint8_t *ptr = NULL;
    uint32_t i;

    metadata = malloc(metadata_size);
    fseek(in, self->metadata_offset + 0x20, SEEK_SET);

    if (fread(metadata, 1, metadata_size, in) != metadata_size) {
        return -1;
    }

    if (self->flags != 0x800) {
        struct keylist_t *klist;

        klist = load_keys(app_info);
        if (klist == NULL)
            return -1;

        if (decrypt_metadata(metadata, metadata_size, klist) < 0)
            return -1;
    }

    ptr = metadata;
    *metadata_info = *((metadata_info_t *) ptr);
    ptr += sizeof (metadata_info_t);
    if (metadata_header) {
        *metadata_header = *((metadata_header_t *) ptr);
        metadata_header->signature_input_length =
                swap64(metadata_header->signature_input_length);
        metadata_header->section_count = swap32(metadata_header->section_count);
        metadata_header->key_count = swap32(metadata_header->key_count);
        metadata_header->signature_info_size =
                swap32(metadata_header->signature_info_size);
    }
    ptr += sizeof (metadata_header_t);
    if (section_headers) {
        *section_headers = malloc(sizeof (metadata_section_header_t) *
                metadata_header->section_count);
        for (i = 0; i < metadata_header->section_count; i++) {
            (*section_headers)[i] = *((metadata_section_header_t *) ptr);
            ptr += sizeof (metadata_section_header_t);
            (*section_headers)[i].data_offset = swap64((*section_headers)[i].data_offset);
            (*section_headers)[i].data_size = swap64((*section_headers)[i].data_size);
            (*section_headers)[i].type = swap32((*section_headers)[i].type);
            (*section_headers)[i].program_idx = swap32((*section_headers)[i].program_idx);
            (*section_headers)[i].sha1_idx = swap32((*section_headers)[i].sha1_idx);
            (*section_headers)[i].encrypted = swap32((*section_headers)[i].encrypted);
            (*section_headers)[i].key_idx = swap32((*section_headers)[i].key_idx);
            (*section_headers)[i].iv_idx = swap32((*section_headers)[i].iv_idx);
            (*section_headers)[i].compressed = swap32((*section_headers)[i].compressed);
        };
    } else {
        ptr += sizeof (metadata_section_header_t) * metadata_header->section_count;
    }
    *keys = malloc(metadata_header->key_count * 0x10);
    memcpy(*keys, ptr, metadata_header->key_count * 0x10);
    ptr += metadata_header->key_count * 0x10;
    if (signature_info) {
        *signature_info = *((signature_info_t *) ptr);
        signature_info->signature_size = swap32(signature_info->signature_size);
    }
    ptr += sizeof (signature_info_t);
    if (signature) {
        *signature = *((signature_t *) ptr);
    }
    ptr += sizeof (signature_t);
    free(metadata);
    
    return 0;
}

int
self_load_sections(FILE *in, self_ehdr_t *self, elf_ehdr_t *elf, elf_phdr_t **phdr,
        metadata_header_t *metadata_header, metadata_section_header_t **section_headers,
        uint8_t **keys, self_section_t **sections) {
    int num_sections = 0;
    uint32_t i;

    num_sections = metadata_header->section_count + 1;

    *sections = malloc(sizeof (self_section_t) * num_sections);
    // ELF header
    for (i = 0; i < num_sections; i++) {
        size_t size = 0;
        metadata_section_header_t *hdr;

        if (i == 0) {
            uint32_t elf_size;

            hdr = (*section_headers);
            elf_size = elf->e_ehsize + (elf->e_phentsize * elf->e_phnum);
            size = (size_t) (hdr->data_offset - self->header_len);
            if (size < elf_size)
                size = elf_size;
            (*sections)[i].offset = 0;
            (*sections)[i].size = size;
            (*sections)[i].data = malloc(size);

            fseek(in, (long) self->header_len, SEEK_SET);
            if (fread((*sections)[0].data, 1, size, in) != size) {
                return -1;
            }

            fseek(in, (long) self->elf_offset, SEEK_SET);
            if (fread((*sections)[0].data, 1, size, in) != size) {
                return -1;
            }
        } else {
            uint8_t *temp_data = NULL;

            hdr = (*section_headers) + i - 1;
            if (hdr->type == 2) {
                // phdr
                size = (size_t) (*phdr)[hdr->program_idx].p_filesz;
                (*sections)[i].offset = (*phdr)[hdr->program_idx].p_offset;
            } else if (hdr->type == 1) {
                // shdr
                size = (size_t) (*section_headers)[i - 1].data_size;
                (*sections)[i].offset = elf->e_shoff;
            } else {
                fail("Unknown section type");
            }

            (*sections)[i].size = size;
            (*sections)[i].data = malloc(size);

            temp_data = malloc((size_t) hdr->data_size);
            fseek(in, (long) hdr->data_offset, SEEK_SET);
            if (fread(temp_data, 1, (size_t) hdr->data_size, in) != hdr->data_size) {
                return -1;
            }

            if (hdr->encrypted == 3)
                AES_ctr128_encrypt(*keys + 0x10 * hdr->key_idx, *keys + 0x10 * hdr->iv_idx,
                    temp_data, hdr->data_size, temp_data);

            if (hdr->compressed == 2)
                decompress(temp_data, hdr->data_size, (*sections)[i].data, size);
            else
                memcpy((*sections)[i].data, temp_data, size);

            free(temp_data);
        }
    }

    return num_sections;
}

size_t
find_isoself(self_section_t *sections, int num_sections, const char *spuname, uint8_t **isoself_data) {
    elf32_ehdr_t elf32_ehdr;
    size_t elf_size;
    int i, j;
    uint8_t *data_ptr;

    for (i = 0; i < num_sections; i++) {

        // skip 4 bytes at a time as we have at least a 4 byte alignment
        for (data_ptr = sections[i].data; data_ptr < sections[i].data + sections[i].size; data_ptr += 4) {

            // is this an ELF header?
            if (*(uint32_t*) data_ptr == swap32(ELF_MAGIC)) {

                // grab the ELF header and convert endianness
                memcpy(&elf32_ehdr, data_ptr, sizeof (elf32_ehdr_t));
                elf32_ehdr.e_type = swap16(elf32_ehdr.e_type);
                elf32_ehdr.e_machine = swap16(elf32_ehdr.e_machine);
                elf32_ehdr.e_version = swap32(elf32_ehdr.e_version);
                elf32_ehdr.e_entry = swap32(elf32_ehdr.e_entry);
                elf32_ehdr.e_phoff = swap32(elf32_ehdr.e_phoff);
                elf32_ehdr.e_shoff = swap32(elf32_ehdr.e_shoff);
                elf32_ehdr.e_flags = swap32(elf32_ehdr.e_flags);
                elf32_ehdr.e_ehsize = swap16(elf32_ehdr.e_ehsize);
                elf32_ehdr.e_phentsize = swap16(elf32_ehdr.e_phentsize);
                elf32_ehdr.e_phnum = swap16(elf32_ehdr.e_phnum);
                elf32_ehdr.e_shentsize = swap16(elf32_ehdr.e_shentsize);
                elf32_ehdr.e_shnum = swap16(elf32_ehdr.e_shnum);
                elf32_ehdr.e_shstrndx = swap16(elf32_ehdr.e_shstrndx);

                // calculate (HACK) the elf size and align at 128 bytes
                elf_size = (elf32_ehdr.e_shoff + (elf32_ehdr.e_shentsize * elf32_ehdr.e_shnum) + 128) & ~0x7F;

                // is this a 32-bit ELF and is the ELF within the current section?
                if (elf32_ehdr.e_ident[4] == 1 && data_ptr + elf_size < sections[i].data + sections[i].size) {

                    // get a pointer to the string table
                    elf32_shdr_t *elf32_shdr_ptr = (elf32_shdr_t *) (data_ptr + elf32_ehdr.e_shoff +
                            elf32_ehdr.e_shentsize * elf32_ehdr.e_shstrndx);
                    const char *section_str_tab = (char*) data_ptr + swap32(elf32_shdr_ptr->sh_offset);

                    // go through all sections
                    for (j = 0; j < elf32_ehdr.e_shnum; j++) {
                    	
                    		// point to the current section header
                        elf32_shdr_ptr = (elf32_shdr_t *) (data_ptr + elf32_ehdr.e_shoff + elf32_ehdr.e_shentsize * j);
                        
	                    	// find the SHT_NOTE section with .note.spu_name
                        if (swap32(elf32_shdr_ptr->sh_type) == SHT_NOTE &&
                                strcmp(section_str_tab + swap32(elf32_shdr_ptr->sh_name), ".note.spu_name") == 0) {

                            // get a pointer to the SPUNAME note section
                            elf32_nhdr_t *elf32_nhdr = (elf32_nhdr_t *) (data_ptr + swap32(elf32_shdr_ptr->sh_offset));
                            const char *spuname_tmp = (char *) data_ptr + swap32(elf32_shdr_ptr->sh_offset) + sizeof (elf32_nhdr_t);

                            // compare the SPUNAME with the spuname arg
                            if (strcmp(spuname_tmp, "SPUNAME") == 0 &&
                                    strstr(spuname_tmp + swap32(elf32_nhdr->n_namesz), spuname)
                                    ) {
                                *isoself_data = data_ptr;
                                return elf_size;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

void
self_free_sections(self_section_t **sections, uint32_t num_sections) {
    uint32_t i;

    for (i = 0; i < num_sections; i++) {
        free((*sections)[i].data);
    }
    free(*sections);
    *sections = NULL;
}

static struct keylist_t *
load_keys(app_info_t *app_info) {
    enum sce_key id;

    switch (app_info->self_ehdr_type) {
        case 1:
            id = KEY_LV0;
            break;
        case 2:
            id = KEY_LV1;
            break;
        case 3:
            id = KEY_LV2;
            break;
        case 4:
            id = KEY_APP;
            break;
        case 5:
            id = KEY_ISO;
            break;
        case 6:
            id = KEY_LDR;
            break;
        default:
            fprintf(stderr, "SELF type is invalid : 0x%08X\n", app_info->self_ehdr_type);
            exit(-4);
    }

    return keys_get(id);
}

static int
decrypt_metadata(uint8_t *metadata, uint32_t metadata_size,
        struct keylist_t *klist) {
    uint32_t i;
    metadata_info_t metadata_info;
    uint8_t zero[16] = {0};

    memset(zero, 0, 16);
    for (i = 0; i < klist->n; i++) {
        AES_cbc256_decrypt(klist->keys[i].key, klist->keys[i].iv,
                metadata, sizeof (metadata_info_t), (uint8_t *) & metadata_info);

        if (memcmp(metadata_info.key_pad, zero, 16) == 0 &&
                memcmp(metadata_info.iv_pad, zero, 16) == 0) {
            memcpy(metadata, &metadata_info, sizeof (metadata_info_t));
            break;
        }
    }

    if (i >= klist->n)
        return -1;

    AES_ctr128_encrypt(metadata_info.key, metadata_info.iv,
            metadata + sizeof (metadata_info_t),
            metadata_size - sizeof (metadata_info_t),
            metadata + sizeof (metadata_info_t));

    return i;
}
