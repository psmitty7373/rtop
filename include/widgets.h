#ifndef _WIDGETS_H_
#define _WIDGETS_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#include "ft.h"
#include "fb.h"

typedef struct Font {
    FT_Face face;
    char *filename;
} Font;

typedef struct Png {
    char *filename;
    size_t size;
    size_t width;
    size_t height;
    uint16_t *data;
} Png;

typedef struct LogEntry {
    struct LogEntry *next;
    struct LogEntry *prev;
    double value;
} LogEntry;

typedef struct Widget {
    char *type;
    char *identifier;
    // location
    size_t top;
    size_t left;
    size_t width;
    size_t height;
    // scaling
    int min;
    int max;
    size_t scale;
    size_t precision;
    // colors
    bool has_border;
    uint16_t border_color;
    uint16_t line_color;
    // linked to file
    char *filename;
    // text
    char *text;
    size_t size;
    // internal
    FT_Face face;
    Png *png;
    LogEntry *head;
    LogEntry *tail;
    double value;
    size_t log_count;
} Widget;

void widget_free(Widget *w);
void widget_log_push(Widget *w, double value);
void widget_draw(Widget *w, FrameBuffer *fb);

#endif
