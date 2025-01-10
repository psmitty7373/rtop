#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <stdbool.h>
#include <pthread.h>

volatile bool running = true;

#include "widgets.h"
#include "config.h"
#include "sock.h"
#include "shared.h"
#include "json.h"

#define TARGET_FPS 60
#define FRAME_TIME (1.0 / TARGET_FPS)

void handle_sigint(int sig) {
    printf("\nCaught signal %d (Ctrl+C). Exiting!\n", sig);
    running = false;
}

int main() {
    int ret = EXIT_FAILURE;
    FrameBuffer *fb = NULL;   
    struct timespec start, end, sleep_time;
    char fps_text[32] = {0};
    pthread_t listener_thread = 0;

    Config config = {0};

    // shared buffer
    SharedBuffer shared_data = {
        .data_available = false,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER
    };

    signal(SIGINT, handle_sigint);

    // load config
    if (!load_config("config.json", &config) == 0) {
        goto cleanup;
    }
    printf("Loaded: %ld\n", config.widget_count);
 
    // setup framebuffer
    fb = fb_init();
    if (fb == NULL) {
        goto cleanup;
    }

    // start socket thread
    if (pthread_create(&listener_thread, NULL, udp_listener, &shared_data) != 0) {
        perror("Failed to create listener thread");
        goto cleanup;
    }
    // TODO make sure thread is running still

    struct json_value_s *root;

    while (running) {
        clock_gettime(CLOCK_MONOTONIC, &start);

        // new data available
        if (shared_data.data_available) {
            pthread_mutex_lock(&shared_data.mutex);
            shared_data.data_available = false;
            root = json_parse(shared_data.buffer, shared_data.size);
            pthread_mutex_unlock(&shared_data.mutex);

            if (!root) {
                fprintf(stderr, "JSON root parsing failure\n");
                continue;
            }

            struct json_object_s *obj = json_value_as_object(root);
            if (!obj) {
                fprintf(stderr, "JSON root object parsing failure\n");
                continue;
            }

            struct json_object_element_s *elem = obj->start;
            while (elem != NULL) {
                if (elem->value->type == json_type_object) {
                    // check to see if we care about this identifier
                    for (size_t i = 0; i < config.widget_count; i++) {
                        if (config.widgets[i].identifier != NULL && strcmp(elem->name->string, config.widgets[i].identifier) == 0) {
                            struct json_object_s *obj2 = json_value_as_object(elem->value);
                            if (!obj2) {
                                fprintf(stderr, "JSON object value parsing failure\n");
                                continue;
                            }
                            struct json_object_element_s *elem2 = obj2->start;
                            while (elem2 != NULL) {
                                if (strcmp(elem2->name->string, "Value") == 0) {
                                    struct json_number_s *value = json_value_as_number(elem2->value);
                                    if (!value) {
                                        fprintf(stderr, "JSON number value parsing failure\n");
                                        elem2 = elem2->next;
                                        continue;
                                    }
                                    double num_value = strtod(value->number, NULL);

                                    // if its a graph, push the new value
                                    if (strcmp(config.widgets[i].type, "graph") == 0) {
                                        widget_log_push(&config.widgets[i], num_value);
                                    }

                                    else if (strcmp(config.widgets[i].type, "value") == 0) {
                                        config.widgets[i].value = num_value;
                                    }
                                }
                                elem2 = elem2->next;
                            }
                        }
                    }
                }
                elem = elem->next;
            }
            free(root);
        }

        // drawing
        fb_clear(fb);

        // draw fps string
        ft_draw_string(config.fonts[0].face, fb, fps_text, 360, 680, rgb_to_rgb565(0xff,0xff,0xff));

        // draw graphs
        for (size_t i = 0; i < config.widget_count; i++) {
            widget_draw(&config.widgets[i], fb);
        }

        // swap buffers
        fb_swap(fb);

        // maintain framerate
        clock_gettime(CLOCK_MONOTONIC, &end);
        double frame_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        if (frame_elapsed < FRAME_TIME) {
            double sleep_duration = FRAME_TIME - frame_elapsed;
            sleep_time.tv_sec = (time_t)sleep_duration;
            sleep_time.tv_nsec = (long)((sleep_duration - sleep_time.tv_sec) * 1e9);
            nanosleep(&sleep_time, NULL);
        }

        // draw fps
        clock_gettime(CLOCK_MONOTONIC, &end); // Update `end` to include sleep time
        frame_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        double fps = 1.0 / frame_elapsed;

        snprintf(fps_text, sizeof(fps_text), "FPS: %.2f", fps);
    }

    ret = EXIT_SUCCESS;

cleanup:
    running = false;
    pthread_join(listener_thread, NULL);
    pthread_mutex_destroy(&shared_data.mutex);
    pthread_cond_destroy(&shared_data.cond);

    unload_config(&config);

    if (fb) {
        fb_deinit(fb);
    }

    return ret;
}

