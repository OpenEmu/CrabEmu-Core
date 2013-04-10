/*
    This file is part of CrabEmu.

    Copyright (C) 2009 Lawrence Sebald

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

#ifndef CHEATS_H
#define CHEATS_H

#include "CrabEmu.h"

CLINKAGE

typedef struct smscheat_s {
    uint32 ar_code;
    char desc[64];
    int enabled;
} sms_cheat_t;

extern int sms_cheat_init(void);
extern void sms_cheat_shutdown(void);
extern void sms_cheat_reset(void);

extern void sms_cheat_frame(void);

extern int sms_cheat_add(sms_cheat_t *c);
extern void sms_cheat_read(const char *fn);
extern void sms_cheat_write(const char *fn);

extern void sms_cheat_enable(void);
extern void sms_cheat_disable(void);

ENDCLINK

#endif /* !CHEATS_H */
