#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "fb.h"

typedef struct LogEntry {
    struct LogEntry *next;
    int value;
} LogEntry;

typedef struct _Graph {
    size_t top;
    size_t left;
    size_t width;
    size_t height;
    int min;
    int max;
    size_t sz;
    size_t limit;
    char *identifier;
    LogEntry *head;
    LogEntry *tail;
} Graph;

void graph_push(Graph *g, int value);
void graph_draw(Graph *g, FrameBuffer *fb);

#endif
