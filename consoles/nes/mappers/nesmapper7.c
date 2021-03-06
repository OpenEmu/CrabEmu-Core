/*
    This file is part of CrabEmu.

    Copyright (C) 2013 Lawrence Sebald

    CrabEmu is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 
    as published by the Free Software Foundation.

    CrabEmu is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CrabEmu; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nesmem.h"
#include "nesppu.h"

/* iNES Mapper 7: AxROM */
static uint8 bank_reg = 0;
static int prg_banks;

static void fill_map(void) {
    int i, page = (bank_reg & 0x07) << 7;

    for(i = 0; i < 0x80; ++i) {
        nes_read_map[i + 0x80] = nes_prg_rom + ((i + page) << 8);
    }

    nes_mem_update_map();

    if(bank_reg & 0x10)
        nes_ppu_set_tblmirrors(1, 1, 1, 1);
    else
        nes_ppu_set_tblmirrors(0, 0, 0, 0);
}

static void init(void) {
    int is_rom = 1, i;

    prg_banks = nes_prg_rom_size >> 8;
    bank_reg = 0;

    /* Fill in the read map */
    for(i = 0; i < 0x80; ++i) {
        nes_read_map[i + 0x80] = nes_prg_rom + ((i & (prg_banks - 1)) << 8);
    }

    for(i = 0; i < 0x20; ++i) {
        nes_read_map[i + 0x60] = nes_sram + (i << 8);
        nes_write_map[i + 0x60] = nes_sram + (i << 8);
    }

    /* Set up the PPU */
    if(!nes_chr_rom_size)
        is_rom = 0;

    nes_ppu_set_patterntbl(0, nes_chr_rom, is_rom);
    nes_ppu_set_patterntbl(1, nes_chr_rom + 0x1000, is_rom);
    nes_ppu_set_tblmirrors(0, 0, 0, 0);

    nes_mem_update_map();
}

static void reset(void) {
    bank_reg = 0;
    fill_map();
}

static uint8 read_byte(void *cpu __UNUSED__, uint16 addr) {
    switch(addr >> 12) {
        case 2:
        case 3:
            return nes_ppu_readreg(addr & 0x0007);

        case 4:
            if(addr < 0x4020)
                return nes_mem_readreg(addr);
            break;
    }

    return nes_read_map[addr >> 8][(uint8)addr];
}

static void write_byte(void *cpu __UNUSED__, uint16 addr, uint8 val) {
    /* Is the write bound anywhere interesting? */
    switch(addr >> 12) {
        case 2:
        case 3:
            nes_ppu_writereg(addr & 0x0007, val);
            break;

        case 4:
            if(addr < 0x4020)
                nes_mem_writereg(addr, val);
            break;

        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            bank_reg = val;
            fill_map();
            break;

    }

    nes_write_map[addr >> 8][(uint8)addr] = val;
}

static int write_cxt(FILE *fp) {
    uint8 data[4];

    data[0] = 'M';
    data[1] = 'P';
    data[2] = 'P';
    data[3] = 'R';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(20, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    UINT16_TO_BUF(0, data);
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 0) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    data[0] = bank_reg;
    data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);

    return 0;
}

static int read_cxt(const uint8 *buf) {
    uint16 ver;
    uint32 len;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 20)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Read in the registers */
    bank_reg = buf[16];

    fill_map();

    return 0;
}

static uint32 cxt_len(void) {
    return 20;
}

/* Mapper definition. */
nes_mapper_t nes_mapper_7 = {
    7,
    &init,
    &reset,
    &read_byte,
    &write_byte,
    &write_cxt,
    &read_cxt,
    &cxt_len
};
