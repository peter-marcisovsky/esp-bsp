/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <malloc.h>
#include <sdkconfig.h>

#include "unity.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // for xthal_get_ccount()
#include "lvgl.h"
#include "lv_fill_common.h"

#define WIDTH 128
#define HEIGHT 128
#define STRIDE WIDTH
#define UNALIGN_BYTES 1
#define BENCHMARK_CYCLES 750

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char *TAG_LV_FILL_BENCH = "LV Fill Benchmark";
static lv_color_t bg_color = { .blue = 0xEF, .green = 0xCD, .red = 0xAB, };

static blend_params_t *s_blend_params = NULL;
static test_area_t *s_area = NULL;

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief Initialize the benchmark test
 */
static void lv_fill_benchmark_init(bench_test_params_t *test_params);

/**
 * @brief Run the benchmark test
 */
static float lv_fill_benchmark_run(bench_test_params_t *test_params);

// ------------------------------------------------ Test cases ---------------------------------------------------------

/*
Benchmark tests

Requires:
    - To pass functionality tests first

Purpose:
    - Test that an acceleration is achieved by an assembly implementation of LVGL blending API

Procedure:
    - Depending on menuconfig settings, chose either Assembly version of LVGL blend API, or the ANSI version
    - Build the test app with selected configuration
    - Initialize structures needed for LVGL blending API
    - Initialize input parameters (test array length, width, allocate array...) of the benchmark test
    - Run LVGL blending API multiple times (1000-times or so)
    - Firstly use an input test parameters for the most ideal case (16-byte aligned array, array width and height divisible by 4 for ARGB8888 color format)
    - Then use worst-case input test parameters (1-byte aligned array, array width and height NOT divisible by 4 for ARGB8888 color format)
    - Count how many CPU cycles does it take to run a function from the LVGL blending API for each case (ideal and worst case)
    - Free test arrays and structures needed for LVGL blending API
    - If needed, in the menuconfig, select the other version of the LVGL blend API and compare the results
*/
// ------------------------------------------------ Test cases stages --------------------------------------------------

TEST_CASE("LV Fill benchmark ARGB8888", "[lv_fill][ARGB8888]")
{
    uint32_t *dest_array_align16  = (uint32_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint32_t) + (UNALIGN_BYTES * sizeof(uint8_t)));
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);

    // Apply byte unalignment for the worst-case test scenario
    uint32_t *dest_array_align1 = dest_array_align16 + (UNALIGN_BYTES * sizeof(uint8_t));

    bench_test_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE,
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .src_array = NULL,
        .dest_array = (void *)dest_array_align16,
        .src_array_cc = NULL,
        .dest_array_cc = (void *)dest_array_align1,
        .dynamic_bg_opa = false,
        .operation_type = OPERATION_FILL,
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_dest_color_format(s_blend_params, LV_COLOR_FORMAT_ARGB8888));
    TEST_ASSERT_EQUAL(ESP_OK, set_opacity(s_blend_params, LV_OPA_MAX));

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for ARGB8888 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}

TEST_CASE("LV Fill with OPA benchmark ARGB8888", "[lv_fill][opa][ARGB8888]")
{
    uint32_t *dest_array  = (uint32_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint32_t) + (UNALIGN_BYTES * sizeof(uint8_t)));
    memset((void *)dest_array, 0, STRIDE * HEIGHT);  // set array to zero, also BG OPA will be zero
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array);

    bench_test_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE,
        .cc_height = HEIGHT,
        .cc_width = WIDTH,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .src_array = NULL,
        .dest_array = (void *)dest_array,
        .src_array_cc = NULL,
        .dest_array_cc = (void *)dest_array,    // Array testing corner case must be modified for each test run separately
        .dynamic_bg_opa = false,
        .operation_type = OPERATION_FILL_WITH_OPA
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_dest_color_format(s_blend_params, LV_COLOR_FORMAT_ARGB8888));
    TEST_ASSERT_EQUAL(ESP_OK, set_opacity(s_blend_params, LV_OPA_10));

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for ARGB8888 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array);
}

TEST_CASE("LV Fill benchmark RGB565", "[lv_fill][RGB565]")
{
    uint16_t *dest_array_align16  = (uint16_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint16_t) + (UNALIGN_BYTES * sizeof(uint8_t)));
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);
    printf("%p\n", dest_array_align16);

    // Apply 1-byte unalignment for the worst-case test scenario
    uint16_t *dest_array_align1 = ((uint16_t *)((uint8_t *)dest_array_align16 + UNALIGN_BYTES));

    printf("%p\n", dest_array_align16);
    printf("%p\n", dest_array_align1);


    bench_test_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE,
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .src_array = NULL,
        .dest_array = (void *)dest_array_align16,
        .src_array_cc = NULL,
        .dest_array_cc = (void *)dest_array_align1,
        .dynamic_bg_opa = false,
        .operation_type = OPERATION_FILL
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_dest_color_format(s_blend_params, LV_COLOR_FORMAT_RGB565));
    TEST_ASSERT_EQUAL(ESP_OK, set_opacity(s_blend_params, LV_OPA_MAX));

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for RGB565 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}

TEST_CASE("LV Image benchmark RGB565", "[image][RGB565]")
{
    uint16_t *src_array_align16 = (uint16_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint16_t) + (UNALIGN_BYTES * sizeof(uint8_t)));
    uint16_t *dest_array_align16  = (uint16_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint16_t) + (UNALIGN_BYTES * sizeof(uint8_t)));
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);

    // Apply byte unalignment for the worst-case test scenario
    uint16_t *dest_array_align1 = ((uint16_t *)((uint8_t *)dest_array_align16 + UNALIGN_BYTES));
    uint16_t *src_array_align1 = ((uint16_t *)((uint8_t *)src_array_align16 + UNALIGN_BYTES));

    bench_test_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE,
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .src_array = (void *)src_array_align16,
        .dest_array = (void *)dest_array_align16,
        .src_array_cc = (void *)src_array_align1,
        .dest_array_cc = (void *)dest_array_align16,
        .dynamic_bg_opa = false,
        .operation_type = OPERATION_FILL        // TODO change
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_src_color_format(s_blend_params, LV_COLOR_FORMAT_RGB565));
    TEST_ASSERT_EQUAL(ESP_OK, set_dest_color_format(s_blend_params, LV_COLOR_FORMAT_RGB565));
    TEST_ASSERT_EQUAL(ESP_OK, set_opacity(s_blend_params, LV_OPA_MAX));

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for RGB565 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
    free(src_array_align16);
}
// ------------------------------------------------ Static test functions ----------------------------------------------

static void lv_fill_benchmark_init(bench_test_params_t *test_params)
{
    float cycles, per_sample;
    // Update all LV areas with most ideal test cases
    lv_area_set(&s_area->clip,  0, 0, test_params->stride - 1, test_params->height - 1);
    lv_area_set(&s_area->buf,   0, 0, test_params->stride - 1, test_params->height - 1);
    lv_area_set(&s_area->blend, 0, 0, test_params->width - 1,  test_params->height - 1);
    lv_area_set(&s_area->src,   0, 0, test_params->width - 1,  test_params->height - 1);

    s_blend_params->draw_unit.target_layer->buf_area = s_area->buf;
    s_blend_params->draw_unit.target_layer->draw_buf->data = (void *)test_params->dest_array;
    s_blend_params->blend_dsc.src_buf = (void *)test_params->src_array;
    s_blend_params->blend_dsc.src_stride = test_params->stride * sizeof(uint16_t);

    // Run benchmark with the most ideal input parameters
    cycles = lv_fill_benchmark_run(test_params);     // Call LVGL API
    per_sample = cycles / ((float)(test_params->width * test_params->height));

    ESP_LOGI(TAG_LV_FILL_BENCH, "ideal case: %.3f cycles for %dx%d matrix, %.3f cycles per sample", cycles, test_params->width, test_params->height, per_sample);

    // Update all LV areas with worst cases (Corner cases)
    lv_area_set(&s_area->clip,  0, 0, test_params->stride - 1, test_params->cc_height - 1);
    lv_area_set(&s_area->buf,   0, 0, test_params->stride - 1, test_params->cc_height - 1);
    lv_area_set(&s_area->blend, 0, 0, test_params->cc_width - 1,  test_params->cc_height - 1);
    lv_area_set(&s_area->src,   0, 0, test_params->cc_width - 1,  test_params->cc_height - 1);

    s_blend_params->draw_unit.target_layer->buf_area = s_area->buf;
    s_blend_params->draw_unit.target_layer->draw_buf->data = (void *)test_params->dest_array_cc;
    s_blend_params->blend_dsc.src_buf = (void *)test_params->src_array_cc;

    if (test_params->operation_type != OPERATION_FILL) {    // Don't use dynamic OPA background for simple fill
        test_params->dynamic_bg_opa = true;
    }

    // Run benchmark with the corner case parameters
    cycles = lv_fill_benchmark_run(test_params);           // Call LVGL API
    per_sample = cycles / ((float)(test_params->cc_width * test_params->cc_height));

    ESP_LOGI(TAG_LV_FILL_BENCH, "common case: %.3f cycles for %dx%d matrix, %.3f cycles per sample", cycles, test_params->cc_width, test_params->cc_height, per_sample);
}

static void reinit_dest_array(bench_test_params_t *test_params)
{
    switch (test_params->operation_type) {
    case OPERATION_FILL:
        // No need to refill dest_array for Simple fill test
        break;
    case OPERATION_FILL_WITH_OPA:
        // We need to re initialize dest_array, since the previous run of the benchmark test has modified the dest_array
        // For dynamic background opacity, we fill each sample with different opacity to induce the worst case - being
        // most demanding for processing power
        if (test_params->dynamic_bg_opa) {
            lv_color32_t bg_color_argb8888 = lv_color_to_32(bg_color, LV_OPA_50);
            for (int i = 0; i < STRIDE * HEIGHT; i++) {
                // Dynamic BG OPA
                bg_color_argb8888.alpha = (uint8_t)(i % 255);
                ((lv_color32_t *)test_params->dest_array)[i] = bg_color_argb8888;
            }
        } else {
            // For static background, we set each sample with constant opacity level
            memset(test_params->dest_array, 0, STRIDE * HEIGHT);
        }
        break;
    default:
        break;
    }
}

static float lv_fill_benchmark_run(bench_test_params_t *test_params)
{
    // Call the DUT function for the first time to init the benchmark test
    lv_draw_sw_blend(&s_blend_params->draw_unit, &s_blend_params->blend_dsc);

    uint64_t total_cpu_count = 0;
    for (int i = 0; i < test_params->benchmark_cycles; i++) {

        // Re-initialize destination array before each test run
        reinit_dest_array(test_params);

        // We must count cycles, only during the DUT function execution
        const unsigned int start_cpu_count = xthal_get_ccount();                    // Start counting CPU cycles
        lv_draw_sw_blend(&s_blend_params->draw_unit, &s_blend_params->blend_dsc);   // run DUT function
        const unsigned int end_cpu_count = xthal_get_ccount();                      // Stop counting CPU cycles
        total_cpu_count += (uint64_t)(end_cpu_count - start_cpu_count);             // Count total CPU cycles
    }

    printf("total_cpu_count %lld\n", total_cpu_count);

    return ((float)total_cpu_count) / ((float)test_params->benchmark_cycles);
}
