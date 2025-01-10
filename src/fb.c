#include "fb.h"

uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t rs = (r * 31) / 255;
    uint16_t gs = (g * 63) / 255;
    uint16_t bs = (b * 31) / 255;

    return (rs << 11) | (gs << 5) | bs;
}

void fb_set_graphics_mode() {
    char *tty_n = "/dev/tty0";
    int console_fd;
        
    console_fd = open(tty_n, O_RDWR);
        
    if (!console_fd) {
        fprintf(stderr,"Could not open console.\n");
        exit(1);
    }

    if (ioctl( console_fd, KDSETMODE, KD_GRAPHICS))
        {
            fprintf(stderr,"Could not set console to KD_GRAPHICS mode.\n");
            exit(1);
        }
            
    close(console_fd);
}

FrameBuffer* fb_init() {
    FrameBuffer *fb = calloc(1, sizeof(FrameBuffer));
    if (fb == NULL) {
        perror("Unable to allocate FrameBuffer.");
        return NULL;
    }

    fb->fd = open("/dev/fb0", O_RDWR);
    if (fb->fd == -1) {
        perror("Error opening framebuffer device");
        goto cleanup;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo)) {
        perror("Error reading variable screen info");
        goto cleanup;
    }

    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed screen info");
        goto cleanup;
    }

    fb->w = vinfo.xres;
    fb->h = vinfo.yres;
    fb->stride = finfo.line_length;
    fb->bpp = vinfo.bits_per_pixel;
    fb->Bpp = fb->bpp / 8;
    fb->sz = vinfo.yres_virtual * finfo.line_length;

    fb->ptr = (uint8_t *)mmap(0, fb->sz, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);

    if (fb->ptr == MAP_FAILED) {
        perror("Error mapping framebuffer memory");
        goto cleanup;
    }

    fb->bb = malloc(fb->sz);
    if (!fb->bb) {
        perror("Error allocating back buffer");
        goto cleanup;
    }

    fb_set_graphics_mode();

    return fb;

    cleanup:
    fb_deinit(fb);

    return NULL;
}

void fb_deinit(FrameBuffer *fb) {
    if (fb->bb) {
        free(fb->bb);
        fb->bb = NULL;
    }
    if (fb->ptr) {
        munmap(fb->ptr, fb->sz);
        fb->ptr = NULL;
    }
    close(fb->fd);
    if (fb) {
        free(fb);
    }

    fb = NULL;
}

void fb_clear(FrameBuffer *fb) {
    memset(fb->bb, 0, fb->sz);
}

void fb_draw_line(FrameBuffer *fb, size_t x1, size_t y1, size_t x2, size_t y2, uint16_t rgb565) {
    int dx = abs((int)x2 - (int)x1);
    int dy = abs((int)y2 - (int)y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        fb_set_pixel(fb, x1, y1, rgb565);

        if (x1 == x2 && y1 == y2) {
            break;
        }

        int e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void fb_draw_line_shaded(FrameBuffer *fb, size_t x1, size_t y1, size_t x2, size_t y2, size_t bottom, uint16_t line_color, uint16_t shade_color) {
    int dx = abs((int)x2 - (int)x1);
    int dy = abs((int)y2 - (int)y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        fb_set_pixel(fb, x1, y1, line_color);

        if (x1 == x2 && y1 == y2) {
            break;
        }

        int e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
            if (y1 < bottom) {
                fb_draw_line(fb, x1, y1 + 1, x1, bottom, shade_color);
            }
        }

        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void fb_set_pixel(FrameBuffer *fb, size_t x, size_t y, uint16_t rgb565) {
    if (x < fb->w && y < fb->h) {
        size_t pixel_offset = y * fb->w + x;
        ((uint16_t *)fb->bb)[pixel_offset] = rgb565;
    }
}

void fb_swap(FrameBuffer *fb) {
    memcpy(fb->ptr, fb->bb, fb->sz);
}