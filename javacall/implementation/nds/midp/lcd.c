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

#include "javacall_lcd.h" 
#include <nds.h>
#include <stdlib.h>
#include <string.h>

#define NDS_MAIN_SCREEN_WIDTH  256
#define NDS_MAIN_SCREEN_HEIGHT 192

/** Separate colors are 8 bits as in Java RGB */
#define GET_RED(P)   (((P) >> 11) & 0x1F)
#define GET_GREEN(P) (((P) >> 6) & 0x1F)
#define GET_BLUE(P)  ((P) & 0x1F)

#define RGB16(r,g,b) ((r)+(g<<5)+(b<<10)) | 0x8000

#ifdef __cplusplus
extern "C" {
#endif

static int bufID = 0;
static u16* screenBuffers[2];
static u16* subscreenBuffer;
//Double the height for dual screen support
static u16 __attribute__((aligned(16))) _offscreen[NDS_MAIN_SCREEN_WIDTH*(NDS_MAIN_SCREEN_HEIGHT * 2)];

static u8 need_flipping = 0;
static u8 _bg[2];
volatile u8 screen_mode = 1;
static u8 scr_count = 1;

void vb_handler() {

    if (need_flipping) {
        // Do buffer flip. Base is 16KB and screen size is 256x256x2 (128KB)
        if (bgGetMapBase(_bg[0]) == 8) 
            bgSetMapBase(_bg[0], 0);
        else
            bgSetMapBase(_bg[0], 8);
    }

    need_flipping = 0;
}

/**
 * The function javacall_lcd_init is called by during Java VM startup, allowing the
 * platform to perform device specific initializations. The function is required to
 * provide the supported screen capabilities:
 * - Display Width
 * - Display Height
 * - Color encoding: Either 32bit ARGB format, 15 bit 565 encoding or 24 bit RGB encoding
 * 
 * \par
 * 
 * Once this API call is invoked, the VM will receive display focus.\n
 * <b>Note:</b> Consider the case where the platform tries to assume control over the
 * display while the VM is running by pausing the Java platform. In this case, the
 * platform is required to save the VRAM screen buffer: Whenever the Java
 * platform is resumed, the stored screen buffers must be restored to original
 * state.
 * 
 * @param screenWidth width of screen
 * @param screenHeight width of screen
 * @param colorEncoding color encoding, one of the following:
 *              -# JAVACALL_LCD_COLOR_RGB565
 *              -# JAVACALL_LCD_COLOR_ARGB
 *              -# JAVACALL_LCD_COLOR_RGB888   
 *              -# JAVACALL_LCD_COLOR_OTHER    
 *                
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_init(void) {

    //irqSet(IRQ_VBLANK, vb_handler);

    //Defaults with mode1
    mode0();
    javacall_switch_lcd_mode();

    return JAVACALL_OK;
}

/*
    O - used by MIDlet, X - used by VM
    mode 0 - main screen display / sub screen console O X
    mode 1 - main screen console / sub screen display X O
    mode 2 - main screen and sub screen as display - continious X X
    mode 3 - main screen display 0 / sub screen display 1 - 2 independent screens X X
*/
//Display on main
void mode0() {
    mode1();
    //scr_count = 1; //set in mode 1
    screen_mode = 0;
    lcdMainOnTop();
}

//Display on sub
void mode1() {
    screen_mode = 1;
    scr_count = 1;

    //Clear any previous vblank handlers - use default
    //irqClear(IRQ_VBLANK);

    //set the mode for 2 text layers and two extended background layers
    videoSetMode(MODE_5_2D);

    vramSetMainBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_BG_0x06020000
            , VRAM_C_SUB_BG, VRAM_D_LCD);
    
    //set the first two banks as background memory and the third as sub background memory
    //D is not used..if you need a bigger background then you will need to map
    //more vram banks consecutivly (VRAM A-D are all 0x20000 bytes in size)
    //vramSetMainBanks(VRAM_A_MAIN_BG, VRAM_B_MAIN_BG, VRAM_C_SUB_BG, VRAM_D_LCD);

    consoleDemoInit();

    //layer, type, size, map_base, tile_base
    _bg[0] = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    
    //For main screen
    screenBuffers[0] = (u16*)bgGetGfxPtr(_bg[0]);
    screenBuffers[1] = (u16*)bgGetGfxPtr(_bg[0]) + (256 * 256);

    lcdMainOnBottom();

}


//Xinerama - display on main and sub
void mode2() {
    screen_mode = 2;
    scr_count = 2;

    //Reset the display
    lcdMainOnTop();

    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);

    //No console - look at message.log
    vramSetMainBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_BG_0x06020000
            , VRAM_C_SUB_BG, VRAM_D_LCD);

    //Main screen
    _bg[0] = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    screenBuffers[0] = (u16*)bgGetGfxPtr(_bg[0]);
    screenBuffers[1] = (u16*)bgGetGfxPtr(_bg[0]) + (256 * 256);

    //Subscreen
    _bg[1] = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    subscreenBuffer= (u16*)bgGetGfxPtr(_bg[1]);

    //Use 128K from VRAM_D as back buffer for subscreen
    //subscreenBuffers[1] = (u16*)BG_MAP_RAM(24);

}

//2 independent displays
void mode3() {
    screen_mode = 3;
    scr_count = 2;
}


/**
 * The function javacall_lcd_finalize is called by during Java VM shutdown, 
 * allowing the  * platform to perform device specific lcd-related shutdown
 * operations.  
 * The VM guarantees not to call other lcd functions before calling 
 * javacall_lcd_init( ) again.
 *                
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_finalize(void){
    return JAVACALL_OK;
} 
    
/**
 * Get screen raster pointer
 *
 * @return pointer to video ram mapped memory region of size  
 *         ( LCDSGetScreenWidth() * LCDSGetScreenHeight() )  
 */
javacall_pixel* javacall_lcd_get_screen(javacall_lcd_screen_type screenType,
                                        int* screenWidth,
                                        int* screenHeight,
                                        javacall_lcd_color_encoding_type* colorEncoding){

    *screenWidth   = NDS_MAIN_SCREEN_WIDTH;
    *screenHeight  = (NDS_MAIN_SCREEN_HEIGHT * scr_count);
    *colorEncoding = JAVACALL_LCD_COLOR_RGB565;
    return (javacall_pixel* )_offscreen;
}
    
/**
 * The following function is used to flush the image from the Video RAM raster to
 * the LCD display. \n
 * The function call should not be CPU time expensive, and should return
 * immediately. It should avoid memory bulk memory copying of the entire raster.
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_flush(void) {
    need_flipping = 0;
    return javacall_lcd_flush_partial(0, (NDS_MAIN_SCREEN_HEIGHT * scr_count));
}
    
/**
 * Set or unset full screen mode.
 * 
 * This function should return <code>JAVACALL_FAIL</code> if full screen mode
 * is not supported.
 * Subsequent calls to <code>javacall_lcd_get_screen()</code> will return
 * a pointer to the relevant offscreen pixel buffer of the corresponding screen
 * mode as well s the corresponding screen dimensions, after the screen mode has
 * changed.
 * 
 * @param useFullScreen if <code>JAVACALL_TRUE</code>, turn on full screen mode.
 *                      if <code>JAVACALL_FALSE</code>, use normal screen mode.

 * @retval JAVACALL_OK   success
 * @retval JAVACALL_FAIL failure
 */
javacall_result javacall_lcd_set_full_screen_mode(javacall_bool useFullScreen) {
    return JAVACALL_OK;
}

//Helper functions for flushing the top and bottom screens
javacall_result flush_main(int ystart, int yend) {
    u16 R,G,B;
    int i, size;

    if (ystart < 0 || yend < 0 || ystart > yend || yend > NDS_MAIN_SCREEN_HEIGHT) 
        return JAVACALL_FAIL;

    // We are going to use the other buffer
    bufID = bufID ^ 0x01;

    size = NDS_MAIN_SCREEN_HEIGHT*NDS_MAIN_SCREEN_WIDTH;
    for (i = 0; i < size; i++) {
        // Convert RGB565 to DS color format BGR1555
        R=GET_RED(_offscreen[i]);
        G=GET_GREEN(_offscreen[i]);
        B=GET_BLUE(_offscreen[i]);
        *(screenBuffers[bufID] + i) = RGB16(R,G,B);
    }

    need_flipping = 1;

    return JAVACALL_OK;
}
javacall_result flush_sub(int ystart, int yend) {

    int i, size, offsite;
    u16 R, G, B;
    u16* subBase;

    if (ystart < NDS_MAIN_SCREEN_HEIGHT || yend < NDS_MAIN_SCREEN_HEIGHT || ystart > yend 
            || yend > (NDS_MAIN_SCREEN_HEIGHT * 2)) 
        return JAVACALL_FAIL;

    offsite = NDS_MAIN_SCREEN_WIDTH*ystart;
    size = (yend - ystart) * NDS_MAIN_SCREEN_WIDTH;

    subBase = subscreenBuffer + 
                    (ystart - NDS_MAIN_SCREEN_HEIGHT) * NDS_MAIN_SCREEN_WIDTH;

    for (i = 0; i < size; i++) {
        R=GET_RED(_offscreen[offsite + i]);
        G=GET_GREEN(_offscreen[offsite + i]);
        B=GET_BLUE(_offscreen[offsite + i]);
        *(subBase+i) = RGB16(R,G,B);
    }

    return JAVACALL_OK;
}
   
/**
 * Flush the screen raster to the display. 
 * This function should not be CPU intensive and should not perform bulk memory
 * copy operations.
 * The following API uses partial flushing of the VRAM, thus may reduce the
 * runtime of the expensive flush operation: It should be implemented on
 * platforms that support it
 * 
 * @param ystart start vertical scan line to start from
 * @param yend last vertical scan line to refresh
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail 
 */

javacall_result javacall_lcd_flush_partial(int ystart, int yend) {

    int main_yend, sub_ystart;

    stop_key_poll();
    if (ystart < NDS_MAIN_SCREEN_HEIGHT) {
        main_yend = yend < NDS_MAIN_SCREEN_HEIGHT? yend : NDS_MAIN_SCREEN_HEIGHT;
        if (flush_main(ystart, main_yend) != JAVACALL_OK) {
            return JAVACALL_FAIL;
        }
    } 
    if (yend > NDS_MAIN_SCREEN_HEIGHT) {
        sub_ystart = ystart > NDS_MAIN_SCREEN_HEIGHT? ystart : NDS_MAIN_SCREEN_HEIGHT;
        if (flush_sub(sub_ystart, yend) != JAVACALL_OK) {
            return JAVACALL_FAIL;
        }
    }
    start_key_poll();
    return JAVACALL_OK;
}

javacall_bool javacall_lcd_reverse_orientation() {
    return JAVACALL_OK;
}
 
javacall_bool javacall_lcd_get_reverse_orientation() {
    return JAVACALL_OK;
}

/**
 * checks the implementation supports native softbutton layer.
 * 
 * @retval JAVACALL_TRUE   implementation supports native softbutton layer
 * @retval JAVACALL_FALSE  implementation does not support native softbutton layer
 */
javacall_bool javacall_lcd_is_native_softbutton_layer_supported () {
    return JAVACALL_FALSE;
}

/**
 * The following function is used to set the softbutton label in the native
 * soft button layer.
 * 
 * @param label the label for the softbutton
 * @param len the length of the label
 * @param index the corresponding index of the command
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_lcd_set_native_softbutton_label(const javacall_utf16* label,
                                                         int len,
                                                         int index){
     return JAVACALL_FAIL;
}

int javacall_lcd_get_screen_width() {
    return NDS_MAIN_SCREEN_WIDTH;
}
 
int javacall_lcd_get_screen_height() {
    return (NDS_MAIN_SCREEN_HEIGHT * scr_count);
}

void clear_subscreen() {
    clear_screen(subscreenBuffer);
}

void javacall_switch_lcd_mode() {
    irqDisable(IRQ_VBLANK);
    reset_lcd_state();

    //Only cycle between 0 - 2; 
    screen_mode = ++screen_mode % 3;
    //Is this a good idea?
    //swiWaitForVBlank();
    clear_screen(screenBuffers[0]);
    clear_screen(screenBuffers[1]);
    clear_screen(subscreenBuffer);
    switch (screen_mode) {
        case 0:
            mode0();
            break;
        case 1:
            mode1();
            break;
        case 2:
            mode2();
            break;
    }
    javanotify_rotation();

    irqClear(IRQ_VBLANK);
    irqSet(IRQ_VBLANK, vb_handler);
    irqEnable(IRQ_VBLANK);
}

void reset_lcd_state()  {
    bgSetMapBase(_bg[0], 0);
    bufID = 0;
    need_flipping = 0;
}

void clear_screen(u16* screen) {
    int i;
    for(i = 0; i < NDS_MAIN_SCREEN_WIDTH * NDS_MAIN_SCREEN_HEIGHT; ++i) 
        *screen++ = RGB15(0,0,0);
}
    
#ifdef __cplusplus
} //extern "C"
#endif


