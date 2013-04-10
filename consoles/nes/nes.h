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

#ifndef NES_H
#define NES_H

#include "CrabEmu.h"

CLINKAGE

#define CONSOLE_NES     6

extern int nes_init(int video_system);
extern int nes_reset(void);
extern void nes_soft_reset(void);
extern int nes_shutdown(void);
extern int nes_frame(int run, int skip);

extern int nes_cycles_elapsed(void);
extern void nes_assert_irq(void);
extern void nes_clear_irq(void);
extern void nes_burn_cycles(int cycles);

#define  NES_MASTER_CLOCK     21477272.7272
#define  NES_SCANLINE_CYCLES  113

/* Button Defines */
#define NES_A       0
#define NES_B       1
#define NES_SELECT  2
#define NES_START   3
#define NES_UP      4
#define NES_DOWN    5
#define NES_LEFT    6
#define NES_RIGHT   7

extern void nes_button_pressed(int player, int button);
extern void nes_button_released(int player, int button);

extern int nes_save_state(const char *filename);
extern int nes_load_state(const char *filename);

ENDCLINK

#endif /* !NES_H */
