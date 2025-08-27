// graphics.h
#pragma once
#include <sys/types.h>
#include <sys/select.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Unsigned short is 16 bits same as color
typedef unsigned short color_t;

// Some bit masking and shifitng for the RGB
#define RGB(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))

// Graphics functions
void init_graphics();
void exit_graphics();
char getkey();
void sleep_ms(long ms);
void clear_screen(void *img);
void draw_pixel(void *img, int x, int y, color_t color);
void draw_line(void *img, int x1, int y1, int x2, int y2, color_t c);
void fill_triangle(void *img, int x1, int y1, int x2, int y2, int x3, int y3, color_t c);
void *new_offscreen_buffer();
void blit(void *src);