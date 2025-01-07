#ifndef _WIDGETS_H_
#define _WIDGETS_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#include "ft.h"
#include "fb.h"

typedef struct LogEntry {
    struct LogEntry *next;
    double value;
} LogEntry;

typedef struct Widget {
    size_t top;
    size_t left;
    size_t width;
    size_t height;
    int min;
    int max;
    size_t limit;
    size_t size;
    bool has_border;
    uint16_t border_color;
    uint16_t line_color;
    char *identifier;
    char *type;
    char *font;
    char *text;
    FT_Face face;
    LogEntry *head;
    LogEntry *tail;
    double value;
    size_t log_count;
} Widget;

void widget_free(Widget *w);
void widget_log_push(Widget *w, double value);
void widget_draw(Widget *w, FrameBuffer *fb);

#endif
