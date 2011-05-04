/*
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 2, as published by the Free Software Foundation.
 */

#include "tools.h"
#include "self.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    FILE *in = NULL;
    FILE *out = NULL;
    self_ehdr_t self;
    app_info_t app_info;
    elf_ehdr_t elf;
    elf_phdr_t *phdr = NULL;
    elf_shdr_t *shdr = NULL;
    section_info_t *section_info = NULL;
    sceversion_info_t sceversion_info;
    control_info_t *control_info = NULL;
    metadata_info_t metadata_info;
    metadata_header_t metadata_header;
    metadata_section_header_t *section_headers = NULL;
    uint8_t *keys = NULL;
    signature_info_t signature_info;
    signature_t signature;
    self_section_t *sections = NULL;
    int num_sections;
    int i;
    uint8_t *isoself_data;
    size_t isoself_size;

    if (argc != 3) {
        fprintf(stderr, "usage: %s in.self out.elf\n", argv[0]);
        return -1;
    }

    in = fopen(argv[1], "rb");
    if (in == NULL) {
        ERROR(-2, "Can't open input file");
    }

    self_read_headers(in, &self, &app_info, &elf, &phdr, &shdr,
            &section_info, &sceversion_info, &control_info);

    self_read_metadata(in, &self, &app_info, &metadata_info,
            &metadata_header, &section_headers, &keys,
            &signature_info, &signature);

    num_sections = self_load_sections(in, &self, &elf, &phdr,
            &metadata_header, &section_headers, &keys, &sections);

    fclose(in);

    out = fopen(argv[2], "wb");
    if (out == NULL) {
        ERROR(-2, "Can't open output file");
    }

    for (i = 0; i < num_sections; i++) {
        fseek(out, (long) sections[i].offset, SEEK_SET);
        if (fwrite(sections[i].data, 1, (size_t) sections[i].size, out) != sections[i].size) {
            ERROR(-7, "Error writing section");
        }
    }
    fclose(out);

    isoself_size = find_isoself(sections, num_sections, "decoder.spu", &isoself_data);
    if (isoself_size) {
        out = fopen("decoder.spu.elf", "wb");
        fwrite(isoself_data, isoself_size, 1, out);
        fclose(out);
    }

    self_free_sections(&sections, num_sections);

    return 0;
}
