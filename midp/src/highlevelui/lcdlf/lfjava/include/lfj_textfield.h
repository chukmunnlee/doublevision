/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

/**
 * @file
 * native TextField functions
 */

 #ifndef LFJ_TEXTFIELD_H_INCLUDED
#define LFJ_TEXTFIELD_H_INCLUDED

#include <kni.h>

typedef enum {
    LFJ_TEXTFIELD_COMMIT     =3000,
    LFJ_TEXTFIELD_CANCEL     =3001,
    LFJ_TEXTFIELD_ERROR      =3002,
    LFJ_TEXTFIELD_WOULDBLOCK =3003
} lfj_textbox_status;

/** Masks for determining the constraint mode and modifiers. */
#define LFJ_CONSTRAINT_MASK 0xFFFF
#define LFJ_MODIFIER_MASK 0xFF0000

#endif

