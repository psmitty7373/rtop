#ifndef _FT_H_
#define _FT_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#include "fb.h"

static const int32_t utf32_space[2] = {' ', 0};

void ft_draw_string(FT_Face face, FrameBuffer *fb, const char *s, size_t x, size_t y, uint16_t color);
bool ft_init(const char* ttf_file, FT_Face* face, FT_Library* ft, int req_size);

#endif