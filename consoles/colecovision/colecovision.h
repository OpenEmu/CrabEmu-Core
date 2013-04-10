/*
    This file is part of CrabEmu.

    Copyright (C) 2012 Lawrence Sebald

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

#ifndef COLECOVISION_H
#define COLECOVISION_H

#include "CrabEmu.h"

CLINKAGE

#define CONSOLE_COLECOVISION    5

/* Button defines */
#define COLECOVISION_UP         0
#define COLECOVISION_DOWN       1
#define COLECOVISION_LEFT       2
#define COLECOVISION_RIGHT      3
#define COLECOVISION_L_ACTION   4
#define COLECOVISION_R_ACTION   5
#define COLECOVISION_1          6
#define COLECOVISION_2          7
#define COLECOVISION_3          8
#define COLECOVISION_4          9
#define COLECOVISION_5          10
#define COLECOVISION_6          11
#define COLECOVISION_7          12
#define COLECOVISION_8          13
#define COLECOVISION_9          14
#define COLECOVISION_STAR       15
#define COLECOVISION_0          16
#define COLECOVISION_POUND      17

extern int coleco_init(int video_system);
extern int coleco_reset(void);
extern void coleco_soft_reset(void);
extern int coleco_shutdown(void);

extern int coleco_frame(int run, int skip);

extern void coleco_button_pressed(int player, int button);
extern void coleco_button_released(int player, int button);

extern int coleco_save_state(const char *fn);
extern int coleco_load_state(const char *fn);

ENDCLINK

#endif /* !COLECOVISION_H */
