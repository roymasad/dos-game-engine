#ifndef _gfx_retro_consts_
#define _gfx_retro_consts_

#define VIDEO_INT 0x10          /* the BIOS video interrupt. */
#define SET_MODE 0x00           /* BIOS func to set the video mode. */
#define VGA_256_COLOR_MODE 0x13 /* use to set 256-color mode. */
#define TEXT_MODE 0x03          /* use to set 80x25 text mode. */

#define SCREEN_WIDTH 320  /* width in pixels of mode 0x13 */
#define SCREEN_HEIGHT 200 /* height in pixels of mode 0x13 */
#define NUM_COLORS 256    /* number of colors in mode 0x13 */

#define INPUT_STATUS_1 0x03da
#define VRETRACE 0x08

#define PALETTE_INDEX 0x03c8
#define PALETTE_DATA 0x03c9

#endif