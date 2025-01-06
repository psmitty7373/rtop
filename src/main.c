#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <stdbool.h>
#include <pthread.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#include "config.h"
#include "ft.h"
#include "graph.h"
#include "sock.h"
#include "shared.h"
#include "json.h"

#define TARGET_FPS 60
#define FRAME_TIME (1.0 / TARGET_FPS)


int main() {
    int ret = EXIT_FAILURE;
    FrameBuffer *fb = NULL;   
    struct timespec start, end, sleep_time;
    char fps_text[32] = {0};

    // load config
    Graph *graphs = NULL;
    size_t graph_count = 0;

    if (load_config("config.json", &graphs, &graph_count) == 0) {
        printf("Loaded %zu graphs\n", graph_count);

        for (size_t i = 0; i < graph_count; i++) {
            printf("Graph %zu: top=%zu, left=%zu, width=%zu, height=%zu, limit=%zu, min=%d, max=%d, identifier=%s\n",
                   i, graphs[i].top, graphs[i].left, graphs[i].width, graphs[i].height,
                   graphs[i].limit, graphs[i].min, graphs[i].max, graphs[i].identifier);
        }
    } else {
        goto cleanup;
    }
 
    // setup framebuffer
    fb = fb_init();
    if (fb == NULL) {
        goto cleanup;
    }

    // setup font face
    FT_Face face;
	FT_Library ft = NULL;
    char *ttf_file = "font.ttf";
    if (!ft_init(ttf_file, &face, &ft, 20)) {
        goto cleanup;
    }

    // shared buffer
    SharedBuffer shared_data = {
        .data_available = false,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER
    };

    // start socket thread
    pthread_t listener_thread;

    if (pthread_create(&listener_thread, NULL, udp_listener, &shared_data) != 0) {
        perror("Failed to create listener thread");
        goto cleanup;
    }

    struct json_value_s *root;

    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        if (shared_data.data_available) {
            pthread_mutex_lock(&shared_data.mutex);
            shared_data.data_available = false;
            root = json_parse(shared_data.buffer, shared_data.size);
            pthread_mutex_unlock(&shared_data.mutex);
            
            struct json_object_s *obj = json_value_as_object(root);
            // do we need to check if elems are objs?
            struct json_object_element_s *elem = obj->start;
            while (elem != NULL) {
                if (elem->value->type == json_type_object) {
                    // check to see if we care about this identifier
                    if (strcmp(elem->name->string, "/intelcpu/0/load/0") == 0) {
                        struct json_object_s *obj2 = json_value_as_object(elem->value);
                        struct json_object_element_s *elem2 = obj2->start;
                        while (elem2 != NULL) {
                            if (strcmp(elem2->name->string, "Value") == 0) {
                                struct json_number_s *value = json_value_as_number(elem2->value);
                                int num_value = strtol(value->number, NULL, 10);
                                //graph_push(&g1, num_value);
                            }
                            elem2 = elem2->next;
                        }
                    }
                }
                elem = elem->next;
            }
            free(root);
        }

        fb_clear(fb);

        ft_draw_string(face, fb, (uint8_t *)fps_text, 10, 10);

        for (size_t i = 0; i < graph_count; i++) {
            graph_draw(&graphs[i], fb);
        }

        fb_swap(fb);

        // Measure the time spent on the frame
        clock_gettime(CLOCK_MONOTONIC, &end);
        double frame_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        // Sleep for the remaining time if the frame was rendered too quickly
        if (frame_elapsed < FRAME_TIME) {
            double sleep_duration = FRAME_TIME - frame_elapsed;
            sleep_time.tv_sec = (time_t)sleep_duration;
            sleep_time.tv_nsec = (long)((sleep_duration - sleep_time.tv_sec) * 1e9);
            nanosleep(&sleep_time, NULL);
        }

        clock_gettime(CLOCK_MONOTONIC, &end); // Update `end` to include sleep time
        frame_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        double fps = 1.0 / frame_elapsed;

        snprintf(fps_text, sizeof(fps_text), "FPS: %.2f", fps);
    }

    ret = EXIT_SUCCESS;

cleanup:
    pthread_cancel(listener_thread);
    pthread_join(listener_thread, NULL);
    pthread_mutex_destroy(&shared_data.mutex);
    pthread_cond_destroy(&shared_data.cond);

    if (ft) {
        FT_Done_FreeType(ft);
    }

    if (fb) {
        fb_deinit(fb);
    }

    return ret;
}

