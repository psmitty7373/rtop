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

#include "ft.h"
#include "graph.h"
#include "sock.h"
#include "shared.h"

#define TARGET_FPS 60
#define FRAME_TIME (1.0 / TARGET_FPS)


int main() {
    int ret = EXIT_FAILURE;
    
    FrameBuffer *fb = fb_init();
    if (fb == NULL) {
        goto cleanup;
    }
    
    FT_Face face;
	FT_Library ft = NULL;
    char *ttf_file = "font.ttf";
    
    struct timespec start, end, sleep_time;
    char fps_text[32] = {0};

    // setup font face
    if (!ft_init(ttf_file, &face, &ft, 20)) {
        goto cleanup;
    }

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

    Graph g1 = {0};
    g1.top = 100;
    g1.left = 0;
    g1.width = 720;
    g1.height = 100;
    g1.limit = 720;

    Graph g2 = {0};
    g2.top = 200;
    g2.left = 0;
    g2.width = 720;
    g2.height = 100;
    g2.limit = 720;

    Graph g3 = {0};
    g3.top = 300;
    g3.left = 0;
    g3.width = 720;
    g3.height = 100;
    g3.limit = 720;

    Graph g4 = {0};
    g4.top = 400;
    g4.left = 0;
    g4.width = 720;
    g4.height = 100;
    g4.limit = 720;

    while (true) {
        // Start the timer at the beginning of the frame
        clock_gettime(CLOCK_MONOTONIC, &start);

        // Clear the back buffer
        fb_clear(fb);

        // Draw FPS text
        ft_draw_string(face, fb, (uint8_t *)fps_text, 10, 10);

        graph_draw(&g1, fb);
        graph_push(&g1, rand() % 100);

        graph_draw(&g2, fb);
        graph_push(&g2, rand() % 100);

        graph_draw(&g3, fb);
        graph_push(&g3, rand() % 100);

        graph_draw(&g4, fb);
        graph_push(&g4, rand() % 100);

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

        // Calculate FPS based on actual frame elapsed time
        clock_gettime(CLOCK_MONOTONIC, &end); // Update `end` to include sleep time
        frame_elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        double fps = 1.0 / frame_elapsed;

        // Update FPS text with the calculated value
        snprintf(fps_text, sizeof(fps_text), "FPS: %.2f", fps);
    }

    ret = EXIT_SUCCESS;

cleanup:
    if (ft) {
        FT_Done_FreeType(ft);
    }

    if (fb) {
        fb_deinit(fb);
    }

    return ret;
}

