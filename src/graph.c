#include "graph.h"

void graph_push(Graph *g, int value) {
    if (g->limit == 0) {
        return;
    }

    LogEntry *e = calloc(1, sizeof(LogEntry));
    if (e == NULL) {
        perror("Memory allocation error.");
        return;
    }

    e->value = value;

    if (g->head == NULL) {
        g->head = g->tail = e;
        g->sz = 1;
        return;
    }

    g->tail->next = e;
    g->tail = e;

    if (g->sz == g->limit) {
        LogEntry *t = g->head;
        g->head = g->head->next;
        free(t);
    } else {
        g->sz++;
    }
}

void graph_draw(Graph *g, FrameBuffer *fb) {
    // draw border
    uint16_t green = rgb_to_rgb565(0x00, 0xff, 0x00);
    fb_draw_line(fb, g->left, g->top, g->left + g->width, g->top, rgb_to_rgb565(0xff, 0x00, 0x00)); // top
    fb_draw_line(fb, g->left, g->top, g->left, g->top + g->height, rgb_to_rgb565(0xff, 0x00, 0x00)); // left
    fb_draw_line(fb, g->left, g->top + g->height, g->left + g->width, g->top + g->height, rgb_to_rgb565(0xff, 0x00, 0x00)); // bottom
    fb_draw_line(fb, g->left + g->width, g->top, g->left + g->width, g->top + g->height,  rgb_to_rgb565(0xff, 0x00, 0x00)); // right

    if (g->head) {
        LogEntry *e = g->head;
        for (size_t i = 0; i < g->sz; i++) {
            fb_draw_line(fb, g->left + i, g->top + g->height, g->left + i, g->top + g->height - e->value, green);
            e = e->next;
            if (e == NULL) {
                break;
            }
        }
    }
}
