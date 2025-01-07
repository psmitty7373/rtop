#include "widgets.h"

void widget_free(Widget *w) {
    if (w->identifier) {
        free(w->identifier);
    }
    if (w->type) {
        free(w->type);
    }
    if (w->font) {
        free(w->font);
    }
    if (w->text) {
        free(w->text);
    }

    LogEntry *e = w->head;
    while(e) {
        w->head = w->head->next;
        free(e);
        e = w->head;
    }
}

void widget_log_push(Widget *w, double value) {
    if (w->width <= 2) {
        return;
    }

    LogEntry *e = malloc(sizeof(LogEntry));
    if (e == NULL) {
        perror("Memory allocation error.");
        return;
    }

    e->value = value;
    e->next = NULL;

    if (w->head == NULL) {
        w->head = w->tail = e;
        w->log_count = 1;
    } else {
        w->tail->next = e;
        w->tail = e;

        if (w->log_count == w->width - 2) {
            LogEntry *old_head = w->head;
            w->head = w->head->next;
            free(old_head);
        } else {
            w->log_count++;
        }
    }
}

void widget_draw(Widget *w, FrameBuffer *fb) {
    char buf[20];

    if (w->has_border) {
        fb_draw_line(fb, w->left, w->top, w->left + w->width, w->top, w->border_color); // top
        fb_draw_line(fb, w->left, w->top, w->left, w->top + w->height, w->border_color); // left
        fb_draw_line(fb, w->left, w->top + w->height, w->left + w->width, w->top + w->height, w->border_color); // bottom
        fb_draw_line(fb, w->left + w->width, w->top, w->left + w->width, w->top + w->height,  w->border_color); // right
    }

    if (strcmp(w->type, "graph") == 0) {
        LogEntry *e = w->head;
        size_t i = 1;
        while(e) {
            size_t scaled_value = e->value;
            if ((w->min != 0 || w->max != 0) && w->max > w->min) {
                scaled_value = (e->value - w->min) * (w->height - 2) / (w->max - w->min);
            }
            if (scaled_value > w->height - 2) scaled_value = w->height - 2;

            fb_draw_line(fb, w->left + i, w->top + w->height - 1, w->left + i, w->top + w->height - 1 - scaled_value, w->line_color);
            e = e->next;
            i++;
        }
    } else if (strcmp(w->type, "value") == 0 && w->face) {
        snprintf(buf, sizeof(buf), "%f", w->value);
        ft_draw_string(w->face, fb, buf, w->left, w->top, w->line_color);
    } else if (strcmp(w->type, "text") == 0 && w->face) {
        if (w->text) {
            ft_draw_string(w->face, fb, w->text, w->left, w->top, w->line_color);
        }
    }
}
