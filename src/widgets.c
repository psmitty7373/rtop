#include "widgets.h"

void widget_free(Widget *w) {
    if (w->identifier) {
        free(w->identifier);
    }
    if (w->type) {
        free(w->type);
    }
    if (w->filename) {
        free(w->filename);
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

    LogEntry *e = calloc(1, sizeof(LogEntry));
    if (e == NULL) {
        perror("Memory allocation error.");
        return;
    }

    e->value = value;

    if (w->head == NULL) {
        w->head = w->tail = e;
        w->log_count = 1;
    } else {
        w->head->prev = e;
        e->next = w->head;
        w->head = e;

        if (w->log_count >= w->width / w->scale + 1) {
            LogEntry *old_tail = w->tail;
            w->tail = w->tail->prev;
            w->tail->next = NULL;
            free(old_tail);
        } else {
            w->log_count++;
        }
    }
}

void widget_draw(Widget *w, FrameBuffer *fb) {
    char buf[20];

    if (w->has_border) {
        fb_draw_line(fb, w->left, w->top, w->left + w->width - 1, w->top, w->border_color); // top
        fb_draw_line(fb, w->left, w->top, w->left, w->top + w->height, w->border_color); // left
        fb_draw_line(fb, w->left, w->top + w->height, w->left + w->width - 1, w->top + w->height, w->border_color); // bottom
        fb_draw_line(fb, w->left + w->width - 1, w->top, w->left + w->width - 1, w->top + w->height,  w->border_color); // right
    }

    if (strcmp(w->type, "graph") == 0) {
        LogEntry *e = w->head;
        if (!e) {
            return;
        }
        size_t i = 0;
        size_t bottom = w->top + w->height - 1;
        size_t prev_x = w->left + 1, prev_y = bottom;
        size_t x = 0, y = 0;
        bool first_point = true;

        while(e) {
            size_t scaled_value = e->value;

            if ((w->min != 0 || w->max != 0) && w->max > w->min) {
                scaled_value = (e->value - w->min) * (w->height - 2) / (w->max - w->min);
            }

            if (scaled_value > w->height - 2) scaled_value = w->height - 2;

            if (i * w->scale < w->left + w->width - 1 && w->left + w->width - 1 - i * w->scale > w->left + 1) {
                x = w->left + w->width - 1 - i * w->scale;
            } else {
                x = w->left + 1;
            }
            y = bottom - scaled_value;

            if (first_point) {
                prev_x = x;
                prev_y = y;
                first_point = false;
                continue;
            }

            fb_draw_line_shaded(fb, prev_x, prev_y, x, y, w->top + w->height - 1, w->line_color, rgb_to_rgb565(0x33, 0x33, 0x33));

            prev_x = x;
            prev_y = y;

            e = e->next;
            i++;
        }
    } else if (strcmp(w->type, "png") == 0 && w->png) {
        if (!w->png->data || !w->png->width || !w->png->height) {
            return;
        }

        size_t i = 0;
        for (size_t y = 0; y < w->png->height; y++) {
            for (size_t x = 0; x < w->png->width; x++) {
                if (x < fb->w && y < fb->h) {
                    fb_set_pixel(fb, x + w->left, y + w->top, w->png->data[i]);
                    i++;
                }
            }
        }
    } else if (strcmp(w->type, "value") == 0 && w->face) {
        char format[16];
        snprintf(format, sizeof(format), "%%.%ldf", w->precision);
        snprintf(buf, sizeof(buf), format, w->value);
        ft_draw_string(w->face, fb, buf, w->left, w->top, w->line_color);
    } else if (strcmp(w->type, "text") == 0 && w->face) {
        if (w->text) {
            ft_draw_string(w->face, fb, w->text, w->left, w->top, w->line_color);
        }
    }
}
