#include <bios.h>
#include <dos.h>
#include <dpmi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/nearptr.h>
#include <sys/timeb.h>
#include <time.h>
#include "const.h"
#include "types.h"
#include "gfx.h"

#define GDB_IMPLEMENTATION
#include "gdbstub.h"


byte *VGA = (byte *) 0xA0000; /* this points to video memory. */

unsigned char *double_buffer;
unsigned char *background;

uclock_t lastTic = 0;
uclock_t currentTic = 0;
uclock_t lastLoop = 0;
int fps = 0;
uclock_t second = 0;
long fps30ms = (UCLOCKS_PER_SEC / 1000.0) * 16.1666666;
BITMAP bmp;
int button = 0;
unsigned long cx, dx, offset;
union REGS regs;
int x = 160, y = 100;
float px = 0;


void doSomething() {

	memcpy(double_buffer, cropViewport(background, vp.x, vp.y), 64000);

	//byte color = rand() % 255;

	// regs.x.ax = 3;
	// int86(0x33, &regs, &regs);
	// x = regs.x.cx;
	// y = regs.x.dx;
	// if (regs.x.bx) button = 1;

	regs.x.ax = 0x0b;
	int86(0x33, &regs, &regs);
	x += (signed short) regs.x.cx;
	y += (signed short) regs.x.dx;
	if (x < 20) {
		x = 19;
		vp.x--;
	}

	if (y < 20) {
		y = 19;
		vp.y--;
	}


	if (x > 278) {
		x = 279;
		vp.x++;
	}

	if (y > 158) {
		y = 159;
		vp.y++;
	}

	if (vp.x < 0) vp.x = 0;
	if (vp.x > SCREEN_WIDTH) vp.x = SCREEN_WIDTH;
	if (vp.y < 0) vp.y = 0;
	if (vp.y > SCREEN_HEIGHT) vp.y = SCREEN_HEIGHT;


	// cyclePalette(px);
	// px = px + 1;
	// if (px > 255)
	// 	px = 0;

	draw_transparent_bitmap(&bmp, x - 20, y - 20, double_buffer);

	//wait(19); //1sec
	while ((inp(INPUT_STATUS_1) &
			VRETRACE))// make sure we're not currently in a vblank already
		;
	while (!(inp(INPUT_STATUS_1) & VRETRACE))// wait for next vblank to begin
		;

	memcpy(VGA, double_buffer, 64000);
	memset(double_buffer, 0, 64000);
}

void drawBackground2() {

	double color2;
	for (int x = 0; x < SCREEN_WIDTH; x++)
		for (int y = 0; y < SCREEN_HEIGHT; y++) {

			color2 = (128 + (40.0 * sin(x / 64.0))

					  + 64.0 + (40.0 * sin(y / 16.0))

					  + 128.0 + (40.0 * sin((x + y) / 16.0)) +

					  0.0 + (40.0 * sin(sqrt((x * x + y * y)) / 4.0)))

					 / 4;

			plot_pixel(x, y, (int) color2, background);
		}
}

void drawBackground() {

	double color2 = 0;
	for (int x = 0; x < SCREEN_WIDTH * 1; x = x + 20)
		for (int y = 0; y < SCREEN_HEIGHT * 1; y = y + 20) {

			color2 += 5;

			for (int i = 0; i < 20; i++)
				for (int ii = 0; ii < 20; ii++) {

					plot_pixel_background(x + i, y + ii, (int) color2, background);
				}
		}
}


int main(void) {
	// gdb_start();

	if (__djgpp_nearptr_enable() == 0) {
		printf("Could get access to first 640K of memory.\n");
		exit(-1);
	}

	VGA += __djgpp_conventional_base;

	double_buffer = (unsigned char *) malloc(SCREEN_WIDTH * SCREEN_HEIGHT);
	if (double_buffer == NULL) {
		printf("Not enough memory for double buffer.\n");
		exit(1);
	}

	background = (unsigned char *) malloc(SCREEN_WIDTH * 2 * SCREEN_HEIGHT * 2);
	if (background == NULL) {
		printf("Not enough memory for backgroud.\n");
		exit(1);
	}

	dirtybits = (bool *) malloc(SCREEN_WIDTH * SCREEN_HEIGHT);
	if (dirtybits == NULL) {
		printf("Not enough memory for dirty bits.\n");
		exit(1);
	}
	for (int x = 0; x < SCREEN_WIDTH; x++)
		for (int y = 0; y < SCREEN_HEIGHT; y++)
			dirtybits[(y << 8) + (y << 6) + x] = true;


	set_mode(VGA_256_COLOR_MODE); /* set the video mode. */
	my_clock = (void *) my_clock + __djgpp_conventional_base;

	srand(*my_clock); /* seed the number generator. */

	load_bmp("assets/rocket.bmp", &bmp); /* open the file */

	regs.x.ax = 0;
	int86(0x33, &regs, &regs);
	unsigned long mouse_on = regs.x.ax;
	if (!mouse_on) {
		printf("Mouse not found.\n");
		exit(1);
	}

	drawBackground();
	//drawBackground2();

	while (!kbhit()) {

		currentTic = uclock();
		second = UCLOCKS_PER_SEC;

		if (currentTic > (lastTic + second)) {

			lastTic = currentTic;
			// printf("FPS %d \n", fps);
			fps = 0;
		}
		if (currentTic > lastLoop + fps30ms) {

			fps++;
			doSomething();
			lastLoop = currentTic;
		}

		// Set a GDB checkpoint, needed to receive interrupt commands
		// from the debugger. You should do this in all your game loops.
		// gdb_checkpoint();
	}

	// set_mode(TEXT_MODE);

	// Read an asset file and print its content
	// FILE *file = fopen("assets/test.txt", "r");
	// fseek(file, 0, SEEK_END);
	// long int num_bytes = ftell(file);
	// fseek(file, 0, SEEK_SET);
	// unsigned char *file_content = (unsigned char *) malloc(num_bytes + 1);
	// fread(file_content, 1, num_bytes, file);
	// file_content[num_bytes] = 0;
	// printf("%s\n", file_content);


	__djgpp_nearptr_disable();

	int x;
	x = getchar();
	return 0;
}
