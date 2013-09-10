// Copyright 2010 fail0verflow <master@fail0verflow.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <string.h>
#include "config.h"
#include "types.h"
#include "elf.h"
#include "helper.h"

static const char elf_magic[] = {0x7f, 'E', 'L', 'F'};

static void elf_load_phdr(spe_ctx_t *ctx, FILE *fp, uint32_t phdr_offset, uint32_t i)
{
	uint8_t phdr[0x20];
	uint32_t offset;
	uint32_t paddr;
	uint32_t size;

	fseek(fp, phdr_offset + 0x20 * i, SEEK_SET);
	fread(phdr, sizeof phdr, 1, fp);

	if (be32(phdr) != 1) {
		dbgprintf("phdr #%u: no LOAD\n", i);
		return;
	}

	offset = be32(phdr + 0x04);
	paddr = be32(phdr + 0x0c);
	size = be32(phdr + 0x10);
	dbgprintf("elf: phdr #%u: %08x bytes; %08x -> %08x\n", i, size, offset, paddr);

	// XXX: integer overflow
	if (offset > LS_SIZE || (offset + size) > LS_SIZE)
		fail(ctx, "phdr exceeds local storage");

	fseek(fp, offset, SEEK_SET);
	fread(ctx->ls + paddr, size, 1, fp);
}

void elf_load(spe_ctx_t *ctx, const char *path)
{
    FILE *fp;
	uint8_t ehdr[0x34];
	uint32_t i;
    uint32_t phdr_offset;
    uint32_t n_phdrs;

	fp = fopen(path, "rb");
	if (fp == NULL)
		fail(ctx, "Unable to load elf");

	fread(ehdr, sizeof ehdr, 1, fp);
	if (memcmp(ehdr, elf_magic, 4))
		fail(ctx, "not an ELF file");

	phdr_offset = be32(ehdr + 0x1c);
	n_phdrs = be16(ehdr + 0x2c);

	dbgprintf("elf: %u phdrs at offset 0x%08x\n", n_phdrs, phdr_offset);

	for (i = 0; i < n_phdrs; i++)
		elf_load_phdr(ctx, fp, phdr_offset, i);

	ctx->pc = be32(ehdr + 0x18);
	dbgprintf("elf: entry is at %08x\n", ctx->pc);

	fclose(fp);
}
