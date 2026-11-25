/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum { WIDTH = 500, HEIGHT = 480, ROW_HEIGHT = 24, SAMPLES = 25 };

static int compare_ticks(const void *a, const void *b) {
    clock_t left = *(const clock_t *)a;
    clock_t right = *(const clock_t *)b;
    return (left > right) - (left < right);
}

static void print_samples(const char *label, clock_t samples[SAMPLES]) {
    qsort(samples, SAMPLES, sizeof(*samples), compare_ticks);
    printf("  %-16s %8.3f / %8.3f ms\n", label,
           1000.0 * (double)samples[SAMPLES / 2] / CLOCKS_PER_SEC,
           1000.0 * (double)samples[(95 * SAMPLES + 99) / 100 - 1] / CLOCKS_PER_SEC);
}

static bool run(size_t rows) {
    cui_context *ui = cui_create((cui_config){.width = WIDTH,
                                             .height = HEIGHT,
                                             .max_elements = rows + 32,
                                             .max_commands = rows + 32});
    if (!ui)
        return false;
    clock_t hover[SAMPLES], submit[SAMPLES], finish[SAMPLES], frame_time[SAMPLES];
    bool success = true;
    for (size_t frame = 0; frame <= SAMPLES; ++frame) {
        size_t hovered_row = frame % (HEIGHT / ROW_HEIGHT);
        cui_event motion = {.type = CUI_EVENT_POINTER_MOVE,
                            .x = WIDTH / 2,
                            .y = (float)(hovered_row * ROW_HEIGHT + ROW_HEIGHT / 2)};
        clock_t start = clock();
        if (!cui_queue_event(ui, motion) || !cui_begin_frame(ui, WIDTH, HEIGHT, 0)) {
            success = false;
            break;
        }
        clock_t begin = clock();
        size_t hit_count = 0;
        bool expected_hit = false;
        for (size_t row = 0; row < rows; ++row) {
            bool hit = cui_hovered(ui, row + 2);
            hit_count += hit;
            expected_hit |= hit && row == hovered_row;
        }
        clock_t queried = clock();
        cui_begin_box(ui, 1, (cui_box){.width = WIDTH, .height = HEIGHT, .clip = true});
        for (size_t row = 0; row < rows; ++row) {
            cui_begin_box(ui, row + 2,
                          (cui_box){.width = WIDTH,
                                    .height = ROW_HEIGHT,
                                    .background = {30, 40, 50, 255}});
            (void)cui_interact(ui, row + 2, CUI_ROLE_BUTTON, cui_string("Message"), false, false,
                               0);
            cui_end_box(ui);
        }
        cui_end_box(ui);
        clock_t submitted = clock();
        (void)cui_end_frame(ui);
        clock_t finished = clock();
        size_t semantic_count = 0;
        (void)cui_semantics(ui, &semantic_count);
        if (semantic_count != rows || cui_last_error(ui) != CUI_OK ||
            (frame && (hit_count != 1 || !expected_hit))) {
            fprintf(stderr, "Invalid frame %zu at %zu rows, %zu semantics, %zu hits, error %d\n",
                    frame, rows, semantic_count, hit_count, (int)cui_last_error(ui));
            success = false;
            break;
        }
        if (frame) {
            hover[frame - 1] = queried - begin;
            submit[frame - 1] = submitted - queried;
            finish[frame - 1] = finished - submitted;
            frame_time[frame - 1] = finished - start;
        }
    }
    if (success) {
        printf("%zu submitted rows, %d warm frames (CPU p50 / p95)\n", rows, SAMPLES);
        print_samples("Hover queries", hover);
        print_samples("Row submission", submit);
        print_samples("End frame", finish);
        print_samples("Whole frame", frame_time);
    }
    cui_destroy(ui);
    return success;
}

int main(void) {
    return run(100) && run(1000) && run(4000) ? EXIT_SUCCESS : EXIT_FAILURE;
}
