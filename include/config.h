#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#include "json.h"
#include "ft.h"
#include "widgets.h"

typedef struct Font {
    FT_Face face;
    char *filename;
} Font;

typedef struct Config {
    Widget *widgets;
    size_t widget_count;
    FT_Library ft;
    Font *fonts;
    size_t font_count;
} Config;

int load_config(const char *filename, Config *config);
void unload_config(Config *config);

#endif