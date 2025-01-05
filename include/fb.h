#ifndef _FB_H_
#define _FB_H_

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/kd.h>
#include <sys/mman.h>

typedef struct _FrameBuffer {
    int fd;
    uint8_t *ptr;
    size_t w;
    size_t h;
    size_t sz;
    size_t bpp;
    size_t Bpp;
    size_t ll;
    size_t stride;
    size_t slop;
    uint8_t *bb;
} FrameBuffer;

uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b);

FrameBuffer* fb_init();
void fb_set_graphics_mode();
void fb_clear(FrameBuffer *fb);
void fb_draw_line(FrameBuffer *fb, size_t x1, size_t y1, size_t x2, size_t y2, uint16_t rgb565);
void fb_set_pixel(FrameBuffer *fb, size_t x, size_t y, uint16_t rgb565);
void fb_swap(FrameBuffer *fb);
void fb_deinit(FrameBuffer *fb);

#endif