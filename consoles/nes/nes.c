/*
    This file is part of CrabEmu.

    Copyright (C) 2012, 2013 Lawrence Sebald

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _arch_dreamcast
#include <inttypes.h>
#include <kos/fs.h>
#include <dc/vmufs.h>
#include <dc/vmu_pkg.h>
#include <dc/maple.h>
#include <zlib/zlib.h>
#include "icon.h"
#endif

#include "Crab6502.h"
#include "nes.h"
#include "nesmem.h"
#include "nesppu.h"
#include "nesapu.h"

#include "sound.h"

static const float NTSC_6502_CLOCK = 1789772.5f;
static const int NTSC_LINES_PER_FRAME = 262;
static const float NTSC_FPS = 60.098f;

static const float PAL_6502_CLOCK = 1773447.4f;
static const int PAL_LINES_PER_FRAME = 312;
static const float PAL_FPS = 53.355f;

/* From nesmem.c */
extern uint16 nes_pad;

/* XXXX */
extern int sms_console;

int nes_initialized = 0;
Crab6502_t nescpu;

static int cycles_run, cycles_to_run;

int nes_init(int video_system) {
    (void)video_system;

    if(nes_initialized)
        return -1;

    sms_console = CONSOLE_NES;
    Crab6502_init(&nescpu);

    nes_mem_init();
    nes_ppu_init();
    nes_apu_init();
    nes_initialized = 1;

    sound_init(1, video_system);

    return 0;
}

int nes_reset(void) {
    if(!nes_initialized)
        return -1;

    sound_reset_buffer();

    nes_mem_shutdown();
    nes_mem_init();

    Crab6502_reset(&nescpu);
    nes_ppu_reset();
    nes_apu_reset();

    return 0;
}

void nes_soft_reset(void) {
    if(!nes_initialized)
        return;

    sound_reset_buffer();

    nes_mem_reset();
    Crab6502_reset(&nescpu);
    nes_ppu_reset();
    nes_apu_reset();
}

int nes_shutdown(void) {
    if(!nes_initialized)
        return -1;

    nes_ppu_shutdown();
    nes_mem_shutdown();
    nes_apu_shutdown();
    sound_shutdown();

    nes_initialized = 0;
    return 0;
}

int nes_frame(int run, int skip) {
    int i;

    cycles_run = run;
    cycles_to_run = 113;

    /* Lines 0-239 */
    for(i = 0; i < 240; ++i) {
        cycles_to_run += 113;
        cycles_run += Crab6502_execute(&nescpu, cycles_to_run - cycles_run);
        nes_ppu_execute(i, skip);
    }

    /* Line 240 */
    cycles_to_run += 113;
    cycles_run += Crab6502_execute(&nescpu, cycles_to_run - cycles_run);

    /* Line 241 */
    nes_ppu_vblank_in();
    if(cycles_to_run == cycles_run)
        cycles_run += Crab6502_execute(&nescpu, 1);
    if(nes_ppu_vblnmi_enabled())
        Crab6502_pulse_nmi(&nescpu);
    cycles_to_run += 113;
    cycles_run += Crab6502_execute(&nescpu, cycles_to_run - cycles_run);

    /* Lines 242-260 */
    for(i = 242; i < 261; ++i) {
        cycles_to_run += 113;
        cycles_run += Crab6502_execute(&nescpu, cycles_to_run - cycles_run);
    }

    /* Line 261 (aka, line -1) */
    nes_ppu_vblank_out();
    cycles_to_run += 113;
    cycles_run += Crab6502_execute(&nescpu, cycles_to_run - cycles_run);

    nes_apu_execute(cycles_run);

    return cycles_run - cycles_to_run;
}

int nes_cycles_elapsed(void) {
    return cycles_run + nescpu.cycles_run;
}

void nes_assert_irq(void) {
    Crab6502_assert_irq(&nescpu);
}

void nes_clear_irq(void) {
    Crab6502_clear_irq(&nescpu);
}

void nes_burn_cycles(int cycles) {
    nescpu.cycles_burned += cycles;
}

void nes_button_pressed(int player, int button) {
    uint16 mask = 0;

    if(player < 1 || player > 2)
        return;

    if(button < NES_A || button > NES_RIGHT)
        return;

    switch(button) {
        case NES_A:
        case NES_B:
        case NES_SELECT:
        case NES_START:
        case NES_UP:
        case NES_DOWN:
        case NES_LEFT:
        case NES_RIGHT:
            mask = (player == 1) ? (1 << button) : (1 << (button + 8));
            break;
    }

    /* Update the pad */
    nes_pad |= mask;
}

void nes_button_released(int player, int button) {
    uint16 mask = 0;

    if(player < 1 || player > 2)
        return;

    if(button < NES_A || button > NES_RIGHT)
        return;

    switch(button) {
        case NES_A:
        case NES_B:
        case NES_SELECT:
        case NES_START:
        case NES_UP:
        case NES_DOWN:
        case NES_LEFT:
        case NES_RIGHT:
            mask = (player == 1) ? (1 << button) : (1 << (button + 8));
            break;
    }

    /* Update the pad */
    nes_pad &= ~mask;
}

static int nes_6502_write_context(FILE *fp) {
    uint8 data[4];

    data[0] = '6';
    data[1] = '5';
    data[2] = '0';
    data[3] = '2';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(24, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    /* Write out all the registers. */
    UINT16_TO_BUF(nescpu.pc.w, data);
    data[2] = nescpu.a;
    data[3] = nescpu.x;
    fwrite(data, 1, 4, fp);

    data[0] = nescpu.y;
    data[1] = nescpu.p;
    data[2] = nescpu.s;
    data[3] = nescpu.cli;
    fwrite(data, 1, 4, fp);

    return 0;
}

static int nes_6502_read_context(const uint8 *buf) {
    uint32 len;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 24)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Read in the data */
    BUF_TO_UINT16(buf + 16, nescpu.pc.w);
    nescpu.a = buf[18];
    nescpu.x = buf[19];
    nescpu.y = buf[20];
    nescpu.p = buf[21];
    nescpu.s = buf[22];
    nescpu.cli = buf[23];

    return 0;
}

#ifdef _arch_dreamcast
static int nes_save_state_int(const char *filename)
#else
int nes_save_state(const char *filename)
#endif
{
    FILE *fp;
    uint8 data[4];

    if(nes_initialized == 0)
    /* This shouldn't happen.... */
        return -1;

    fp = fopen(filename, "wb");
    if(!fp)
        return -1;

    fprintf(fp, "CrabEmu Save State");

    /* Write save state version */
    data[0] = 0x00;
    data[1] = 0x02;
    fwrite(data, 1, 2, fp);

    /* Write out the Console Metadata block */
    data[0] = 'C';
    data[1] = 'O';
    data[2] = 'N';
    data[3] = 'S';
    fwrite(data, 1, 4, fp);             /* Block ID */

    UINT32_TO_BUF(24, data);
    fwrite(data, 1, 4, fp);             /* Length */

    UINT16_TO_BUF(1, data);
    fwrite(data, 1, 2, fp);             /* Version */
    fwrite(data, 1, 2, fp);             /* Flags (Importance = 1) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);             /* Child pointer */

    UINT32_TO_BUF(2, data);
    fwrite(data, 1, 4, fp);             /* Console (2 = NES) */

    data[0] = data[1] = data[2] = data[3] = 0;
    fwrite(data, 1, 4, fp);

    /* Write each block's state */
    if(nes_game_write_context(fp)) {
        fclose(fp);
        return -1;
    }
    else if(nes_6502_write_context(fp)) {
        fclose(fp);
        return -1;
    }
    else if(nes_apu_write_context(fp)) {
        fclose(fp);
        return -1;
    }
    else if(nes_ppu_write_context(fp)) {
        fclose(fp);
        return -1;
    }
    else if(nes_mem_write_context(fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

static int nes_cons_read_context(const uint8 *buf) {
    uint32 len, cons;
    uint16 ver;

    /* Check the size */
    BUF_TO_UINT32(buf + 4, len);
    if(len != 24)
        return -1;

    /* Check the version number */
    BUF_TO_UINT16(buf + 8, ver);
    if(ver != 1)
        return -1;

    /* Check the child pointer */
    if(buf[12] != 0 || buf[13] != 0 || buf[14] != 0 || buf[15] != 0)
        return -1;

    /* Read the console type to make sure its sane */
    BUF_TO_UINT32(buf + 16, cons);
    if(cons != 2)
        return -1;

    /* XXXX: Ignore the rest for now... */
    return 0;
}

#ifdef _arch_dreamcast
static int nes_load_state_int(const char *filename)
#else
int nes_load_state(const char *filename)
#endif
{
    FILE *fp;
    char str[19];
    uint8 byte;
    uint8 buf[4];
    int rv;
    uint32 fourcc, len;
    uint16 flags;
    uint8 *ptr;
    size_t read;

    if(!nes_initialized) {
        /* This shouldn't happen.... */
        return -1;
    }

    fp = fopen(filename, "rb");
    if(!fp)
        return -1;

    fread(str, 18, 1, fp);
    str[18] = 0;
    if(strcmp("CrabEmu Save State", str)) {
        fclose(fp);
        return -2;
    }

    /* Read save state version */
    fread(&byte, 1, 1, fp);
    if(byte != 0x00) {
        fclose(fp);
        return -2;
    }

    fread(&byte, 1, 1, fp);
    if(byte != 0x02) {
        fclose(fp);
        return -2;
    }

    /* Process each chunk */
    for(;;) {
        rv = 0;

        /* Read in the fourcc */
        read = fread(buf, 1, 4, fp);
        if(!read)
            break;
        else if(read != 4)
            return -1;

        BUF_TO_UINT32(buf, fourcc);

        /* Read in the length */
        if(fread(buf, 1, 4, fp) != 4)
            return -1;

        BUF_TO_UINT32(buf, len);

        /* Check for a malformed block */
        if(len < 16)
            return -1;

        /* Allocate space for the block and read it in. */
        if(!(ptr = (uint8 *)malloc(len)))
            return -1;
        UINT32_TO_BUF(fourcc, ptr);
        UINT32_TO_BUF(len, ptr + 4);

        if(fread(ptr + 8, 1, len - 8, fp) != len - 8) {
            free(ptr);
            return -1;
        }

        switch(fourcc) {
            case FOURCC_TO_UINT32('C', 'O', 'N', 'S'):
                rv = nes_cons_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('G', 'A', 'M', 'E'):
                rv = nes_game_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('6', '5', '0', '2'):
                rv = nes_6502_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('N', 'A', 'P', 'U'):
                rv = nes_apu_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('N', 'P', 'P', 'U'):
                rv = nes_ppu_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('D', 'R', 'A', 'M'):
                rv = nes_mem_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('M', 'A', 'P', 'R'):
                rv = nes_mapper_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('N', 'C', 'R', 'M'):
                rv = nes_chr_ram_read_context(ptr);
                break;

            case FOURCC_TO_UINT32('N', 'S', 'R', 'M'):
                rv = nes_sram_read_context(ptr);
                break;

            default:
                /* See if its marked as essential... */
                BUF_TO_UINT16(ptr + 10, flags);
                if(flags & 1) {
                    rv = -1;
#ifdef DEBUG
                    printf("Unknown block %c%c%c%c, bailing out!\n", ptr[0],
                           ptr[1], ptr[2], ptr[3]);
#endif
                }
                else {
#ifdef DEBUG
                    printf("Ignoring unknown block %c%c%c%c\n", ptr[0], ptr[1],
                           ptr[2], ptr[3]);
#endif
                }
        }

        /* Clean up this pass */
        free(ptr);

        if(rv) {
#ifdef DEBUG
            printf("Error parsing block %c%c%c%c\n", ptr[0], ptr[1], ptr[2],
                   ptr[3]);
#endif
            return rv;
        }
    }

    return 0;
}

#ifdef _arch_dreamcast
int nes_save_state(const char *filename) {
    char tmpfn[4096];
    int rv;
    uint32 crc, adler;
    uint8 *buf, *buf2;
    FILE *fp;
    long len;
    uLong clen;
    vmu_pkg_t pkg;
    uint8 *pkg_out;
    int pkg_size;
    maple_device_t *vmu;
    file_t f;
    int blocks_freed = 0;

    /* Make sure there's a VMU in port A1. */
    if(!(vmu = maple_enum_dev(0, 1))) {
        return -100;
    }

    if(!vmu->valid || !(vmu->info.functions & MAPLE_FUNC_MEMCARD)) {
        return -100;
    }

    sprintf(tmpfn, "/ram/%s", filename);
    rv = nes_save_state_int(tmpfn);

    if(rv)
        return rv;

    /* Read in the uncompressed save state */
    fp = fopen(tmpfn, "rb");
    if(!fp) {
        fs_unlink(tmpfn);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(!(buf = (uint8 *)malloc(len))) {
        fclose(fp);
        fs_unlink(tmpfn);
        return -1;
    }

    fread(buf, 1, len, fp);
    fclose(fp);

    fs_unlink(tmpfn);

    /* Create a compressed version */
    clen = len * 2;

    if(!(buf2 = (uint8 *)malloc(clen + 8))) {
        free(buf);
        return -1;
    }

    if(compress2(buf2 + 8, &clen, buf, len, 9) != Z_OK) {
        free(buf2);
        free(buf);
        return -1;
    }

    /* Clean up the old buffer and save the length of the uncompressed data into
       the new buffer */
    free(buf);
    *((uint32 *)buf2) = (uint32)len;
    *(((uint32 *)buf2) + 1) = (uint32)clen;

    /* Make the VMU save */
    nes_get_checksums(&crc, &adler);
    sprintf(tmpfn, "/vmu/a1/ss-%08" PRIX32, (uint32_t)crc);

    if(filename != NULL) {
        strncpy(pkg.desc_long, filename, 32);
        pkg.desc_long[31] = 0;
    }
    else {
        sprintf(pkg.desc_long, "CRC: %08" PRIX32 " Adler: %08" PRIX32,
                (uint32_t)crc, (uint32_t)adler);
    }

    strcpy(pkg.desc_short, "CrabEmu State");
    strcpy(pkg.app_id, "CrabEmu");
    pkg.icon_cnt = 1;
    pkg.icon_anim_speed = 0;
    memcpy(pkg.icon_pal, icon_pal, 32);
    pkg.icon_data = icon_img;
    pkg.eyecatch_type = VMUPKG_EC_NONE;
    pkg.data_len = clen + 4;
    pkg.data = buf2;

    vmu_pkg_build(&pkg, &pkg_out, &pkg_size);

    /* See if a file exists with that name, since we'll overwrite it. */
    f = fs_open(tmpfn, O_RDONLY);
    if(f != FILEHND_INVALID) {
        blocks_freed = fs_total(f) >> 9;
        fs_close(f);
    }

    /* Make sure there's enough free space on the VMU. */
    if(vmufs_free_blocks(vmu) + blocks_freed < (pkg_size >> 9)) {
        free(pkg_out);
        free(buf2);
        return pkg_size >> 9;
    }

    if(!(fp = fopen(tmpfn, "wb"))) {
        free(pkg_out);
        free(buf2);
        return -1;
    }

    if(fwrite(pkg_out, 1, pkg_size, fp) != (size_t)pkg_size)
        rv = -1;

    fclose(fp);

    free(pkg_out);
    free(buf2);

    return rv;
}

int nes_load_state(const char *filename __UNUSED__) {
    vmu_pkg_t pkg;
    uint8 *pkg_out, *raw;
    int pkg_size;
    FILE *fp;
    char savename[32];
    uint32 real_size;
    uint32 crc, adler;
    int rv;

    /* Grab the checksums and build the filename */
    nes_get_checksums(&crc, &adler);
    sprintf(savename, "/vmu/a1/ss-%08" PRIX32, (uint32_t)crc);

    /* Read the file in and grab the uncompressed length */
    if(!(fp = fopen(savename, "rb")))
        return -1;

    fseek(fp, 0, SEEK_END);
    pkg_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(!(pkg_out = (uint8 *)malloc(pkg_size)))
        return -1;

    fread(pkg_out, pkg_size, 1, fp);
    fclose(fp);

    vmu_pkg_parse(pkg_out, &pkg);
    real_size = *((uint32 *)pkg.data);
    pkg_size = *(((uint32 *)pkg.data) + 1);

    /* Uncompress the data and write it out to a /ram file */
    if(!(raw = (uint8 *)malloc(real_size))) {
        free(pkg_out);
        return -1;
    }

    uncompress(raw, &real_size, ((uint8 *)pkg.data) + 8, pkg.data_len - 8);
    free(pkg_out);

    sprintf(savename, "/ram/cs-%08" PRIX32, (uint32_t)crc);
    if(!(fp = fopen(savename, "wb"))) {
        free(raw);
        return -1;
    }

    fwrite(raw, 1, real_size, fp);
    fclose(fp);
    free(raw);

    rv = nes_load_state_int(savename);
    fs_unlink(savename);

    return rv;
}
#endif
