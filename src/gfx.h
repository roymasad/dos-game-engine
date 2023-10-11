#ifndef _gfx_retro_functions_
#define _gfx_retro_functions_

#include <bios.h>
#include <dos.h>
#include <dpmi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/nearptr.h>
#include <sys/timeb.h>
#include <time.h>
#include "gdbstub.h"
#include "const.h"
#include "types.h"


typedef struct /* the structure for a bitmap. */
{
	int x;
	int y;

} VIEWPORT;


typedef struct tagBITMAP /* the structure for a bitmap. */
{
	word width;
	word height;
	byte *data;
} BITMAP;

word *my_clock = (word *) 0x046C;
bool *dirtybits;
VIEWPORT vp = {0, 0};

void set_mode(byte mode) {
	union REGS regs;

	regs.h.ah = SET_MODE;
	regs.h.al = mode;
	int86(VIDEO_INT, &regs, &regs);
}

/**************************************************************************
 *  fskip                                                                 *
 *     Skips bytes in a file.                                             *
 **************************************************************************/

void fskip(FILE *fp, int num_bytes) {
	int i;
	for (i = 0; i < num_bytes; i++)
		fgetc(fp);
}

void load_bmp(char *file, BITMAP *b) {
	FILE *fp;
	long index;
	word num_colors;
	int x;

	/* open the file */
	if ((fp = fopen(file, "rb")) == NULL) {
		printf("Error opening file %s.\n", file);
		exit(1);
	}

	/* check to see if it is a valid bitmap file */
	if (fgetc(fp) != 'B' || fgetc(fp) != 'M') {
		fclose(fp);
		printf("%s is not a bitmap file.\n", file);
		exit(1);
	}

	/* read in the width and height of the image, and the
number of colors used; ignore the rest */
	fskip(fp, 16);
	fread(&b->width, sizeof(word), 1, fp);
	fskip(fp, 2);
	fread(&b->height, sizeof(word), 1, fp);
	fskip(fp, 22);
	fread(&num_colors, sizeof(word), 1, fp);
	fskip(fp, 6);

	/* assume we are working with an 8-bit file */
	if (num_colors == 0)
		num_colors = 256;

	/* try to allocate memory */
	if ((b->data = (byte *) malloc((word) (b->width * b->height))) == NULL) {
		fclose(fp);
		printf("Error allocating memory for file %s.\n", file);
		exit(1);
	}

	/* Ignore the palette information for now.
See palette.c for code to read the palette info. */
	fskip(fp, num_colors * 4);

	/* read the bitmap */
	for (index = (b->height - 1) * b->width; index >= 0; index -= b->width)
		for (x = 0; x < b->width; x++)
			b->data[(word) index + x] = (byte) fgetc(fp);

	fclose(fp);
}

/**************************************************************************
 *  draw_bitmap                                                           *
 *    Draws a bitmap.                                                     *
 **************************************************************************/

void draw_bitmap(BITMAP *bmp, int x, int y, unsigned char *buffer) {
	int j;
	word screen_offset = (y << 8) + (y << 6) + x;
	word bitmap_offset = 0;

	for (j = 0; j < bmp->height; j++) {
		memcpy(&buffer[screen_offset], &bmp->data[bitmap_offset], bmp->width);

		bitmap_offset += bmp->width;
		screen_offset += SCREEN_WIDTH;
	}
}

/**************************************************************************
 *  draw_transparent_bitmap                                               *
 *    Draws a transparent bitmap.                                         *
 **************************************************************************/

void draw_transparent_bitmap(BITMAP *bmp, int x, int y, unsigned char *buffer) {
	int i, j;
	word screen_offset = (y << 8) + (y << 6);
	word bitmap_offset = 0;
	byte data;

	for (j = 0; j < bmp->height; j++) {
		for (i = 0; i < bmp->width; i++, bitmap_offset++) {
			data = bmp->data[bitmap_offset];
			if (data)
				buffer[screen_offset + x + i] = data;
		}
		screen_offset += SCREEN_WIDTH;
	}
}

void plot_pixel(int x, int y, byte color, unsigned char *buffer) {
	/*  y*320 = y*256 + y*64 = y*2^8 + y*2^6   */
	//buffer[(y << 8) + (y << 6) + x] = color;
	buffer[y * SCREEN_WIDTH + x] = color;
}

void plot_pixel_background(int x, int y, byte color, unsigned char *buffer) {
	/*  y*320 = y*256 + y*64 = y*2^8 + y*2^6   */
	//buffer[(y << 8) + (y << 6) + x] = color;
	buffer[y * SCREEN_WIDTH * 2 + x] = color;
}

void plot_pixel_dirtyViewport(int x, int y, byte color, unsigned char *buffer) {
	/*  y*320 = y*256 + y*64 = y*2^8 + y*2^6   */
	if (dirtybits[(y << 8) + (y << 6) + x] == true)
		buffer[(y << 8) + (y << 6) + x] = color;
}

/**************************************************************************
 *  wait                                                                  *
 *    Wait for a specified number of clock ticks (18hz).                  *
 **************************************************************************/

void wait(int ticks) {
	word start;

	start = *my_clock;

	while (*my_clock - start < ticks) {
		*my_clock = *my_clock; /* this line is for some compilers
                         that would otherwise ignore this
                         loop */
	}
}

/**************************************************************************
 *  set_palette                                                           *
 *    Sets all 256 colors of the palette.                                 *
 **************************************************************************/

void set_palette(byte *palette) {
	int i;

	outp(PALETTE_INDEX, 0); /* tell the VGA that palette data
                                   is coming. */
	for (i = 0; i < 256 * 3; i++)
		outp(PALETTE_DATA, palette[i]); /* write the data */
}

void cyclePalette(int px) {

	outp(PALETTE_INDEX, 0); /* tell the VGA that palette data
	// is coming. */

	int r, g, b;
	for (int i = 0; i < 256 * 3; i++) {

		r = 64 + 128 * sin(3.1415 * (px + i) / 32);
		g = 128 + 128 * sin(3.1415 * (px + i) / 64);
		b = 180 + 128 * sin(3.1415 * (px + i) / 128);

		outp(PALETTE_DATA, r);
		outp(PALETTE_DATA, g);
		outp(PALETTE_DATA, b);
	}
}

unsigned char *cropViewport(unsigned char *buffer, int x, int y) {

	unsigned char *position;


	position = buffer;

	position += (y * SCREEN_WIDTH * 2);

	position += x;

	return position;

	//return buffer + ((y << 8) + (y << 6) + x);
}

#endif
