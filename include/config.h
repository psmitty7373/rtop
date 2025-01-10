#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <png.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#include "json.h"
#include "ft.h"
#include "widgets.h"

#define MAX_WIDTH 10000
#define MAX_HEIGHT 10000
#define HEADER_SIZE 8

typedef struct Config {
    // widgets
    Widget *widgets;
    size_t widget_count;
    // fonts
    FT_Library ft;
    Font *fonts;
    size_t font_count;
    // pngs
    Png *pngs;
    size_t png_count;
} Config;

int load_config(const char *filename, Config *config);
void unload_config(Config *config);

#endif