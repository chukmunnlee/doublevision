/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include "javacall_keypress.h"
#include "javacall_defs.h"
#include "javacall_penevent.h"
#include <nds.h>

#define NDS_MAIN_SCREEN_HEIGHT 192

#ifdef __cplusplus
extern "C" {
#endif

extern volatile u8 screen_mode;
extern volatile bool in_keyboard;

static u16 pen_x = 0;
static u16 pen_y = 0;
static u32 pressed, held, released;

void poll_key(javacall_handle handle) {

    touchPosition touch;

    if (in_keyboard)
        return;

    scanKeys(); 
    pressed = keysDown();
    //held = keysHeld();
    held = keysDownRepeat();
    released = keysUp();

    do {
        //KEY PRESSED event
        if (pressed & KEY_UP) {
            javanotify_key_event(JAVACALL_KEY_UP, JAVACALL_KEYPRESSED);
            break;
        }
        if (pressed & KEY_DOWN) {
            javanotify_key_event(JAVACALL_KEY_DOWN, JAVACALL_KEYPRESSED);
            break;
        }
        if (pressed & KEY_LEFT) {
            javanotify_key_event(JAVACALL_KEY_LEFT, JAVACALL_KEYPRESSED);
            break;
        }
        if (pressed & KEY_RIGHT) {
            javanotify_key_event(JAVACALL_KEY_RIGHT, JAVACALL_KEYPRESSED);
            break;
        }
        if ((pressed & KEY_L) || (pressed & KEY_Y)){
            javanotify_key_event(JAVACALL_KEY_SOFT1, JAVACALL_KEYPRESSED);
            break;
        }
        if ((pressed & KEY_R) || (pressed & KEY_X)){
            javanotify_key_event(JAVACALL_KEY_SOFT2, JAVACALL_KEYPRESSED);
            break;
        }
        if (pressed & KEY_A) {
            javanotify_key_event(JAVACALL_KEY_SELECT, JAVACALL_KEYPRESSED);
            break;
        }
        if (pressed & KEY_B) {
            javanotify_key_event(JAVACALL_KEY_CLEAR, JAVACALL_KEYPRESSED);
            break;
        }
        if ((pressed & KEY_TOUCH) && screen_mode) {
            touchRead(&touch);
            pen_x = touch.px;
            pen_y = touch.py + ((screen_mode == 1)? 0: NDS_MAIN_SCREEN_HEIGHT);
            javanotify_pen_event(pen_x, pen_y , JAVACALL_PENPRESSED);
            break;
        }
        if (pressed & KEY_SELECT) {
            javacall_switch_lcd_mode();
            break;
        }
        if (pressed & KEY_LID) {
            javanotify_pause();
            break;
        }
        //KEY REPEAT event
        if (held & KEY_UP) {
            javanotify_key_event(JAVACALL_KEY_UP, JAVACALL_KEYREPEATED);
            break;
        }
        if (held & KEY_DOWN) {
            javanotify_key_event(JAVACALL_KEY_DOWN, JAVACALL_KEYREPEATED);
            break;
        }
        if (held & KEY_LEFT) {
            javanotify_key_event(JAVACALL_KEY_LEFT, JAVACALL_KEYREPEATED);
            break;
        }
        if (held & KEY_RIGHT) {
            javanotify_key_event(JAVACALL_KEY_RIGHT, JAVACALL_KEYREPEATED);
            break;
        }
        if ((held & KEY_L) || (held & KEY_Y)){
            javanotify_key_event(JAVACALL_KEY_SOFT1, JAVACALL_KEYREPEATED);
            break;
        }
        if ((held & KEY_R) || (held & KEY_X)){
            javanotify_key_event(JAVACALL_KEY_SOFT2, JAVACALL_KEYREPEATED);
            break;
        }
        if (held & KEY_A) {
            javanotify_key_event(JAVACALL_KEY_SELECT, JAVACALL_KEYREPEATED);
            break;
        }
        if (held & KEY_B) {
            javanotify_key_event(JAVACALL_KEY_CLEAR, JAVACALL_KEYREPEATED);
            break;
        }
        if ((held & KEY_TOUCH) && screen_mode) {
            touchRead(&touch);
            pen_x = touch.px;
            pen_y = touch.py + ((screen_mode == 1)? 0: NDS_MAIN_SCREEN_HEIGHT);
            javanotify_pen_event(pen_x, pen_y, JAVACALL_PENDRAGGED);
        }
        //KEY RELEASED event
        if (released & KEY_UP) {
            javanotify_key_event(JAVACALL_KEY_UP, JAVACALL_KEYRELEASED);
            break;
        }
        if (released & KEY_DOWN) {
            javanotify_key_event(JAVACALL_KEY_DOWN, JAVACALL_KEYRELEASED);
            break;
        }
        if (released & KEY_LEFT) {
            javanotify_key_event(JAVACALL_KEY_LEFT, JAVACALL_KEYRELEASED);
            break;
        }
        if (released & KEY_RIGHT) {
            javanotify_key_event(JAVACALL_KEY_RIGHT, JAVACALL_KEYRELEASED);
            break;
        }
        if ((released & KEY_L) || (released & KEY_Y)){
            javanotify_key_event(JAVACALL_KEY_SOFT1, JAVACALL_KEYRELEASED);
            break;
        }
        if ((released & KEY_R) || (released & KEY_X)){
            javanotify_key_event(JAVACALL_KEY_SOFT2, JAVACALL_KEYRELEASED);
            break;
        }
        if (released & KEY_A) {
            javanotify_key_event(JAVACALL_KEY_SELECT, JAVACALL_KEYRELEASED);
            break;
        }
        if (released & KEY_B) {
            javanotify_key_event(JAVACALL_KEY_CLEAR, JAVACALL_KEYRELEASED);
            break;
        }
        if ((released & KEY_TOUCH) && screen_mode) {
            javanotify_pen_event(pen_x, pen_y, JAVACALL_PENRELEASED);
            break;
        }
        if (released & KEY_LID) {
            javanotify_resume();
            break;
        }

    } while (0);
}



static javacall_handle key_timer;
void javacall_init_keys() {
    //Flush the keys XXXX
    scanKeys();
    keysDown();
    keysHeld();
    //Flush the keys XXXX
    pressed = 0;
    held = 0;
    released = 0;

    //Configure keysDownRepeat() 
    //First param is number of scanKeys() to call after keysDown() to start repeating keys
    //Second param is number of scanKeys() to call between when button is held to be consider as a repeat
    keysSetRepeat(200, 500);

    start_key_poll();
}

void start_key_poll() {
    javacall_time_initialize_timer(1, JAVACALL_TRUE, poll_key, &key_timer);
}

void stop_key_poll() {
    javacall_time_finalize_timer(key_timer);
}

#ifdef __cplusplus
} //extern "C"
#endif
