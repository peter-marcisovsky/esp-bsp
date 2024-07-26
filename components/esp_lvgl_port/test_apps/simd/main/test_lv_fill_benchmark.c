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
#define BENCHMARK_CYCLES 1000

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char *TAG_LV_FILL_BENCH = "LV Fill Benchmark";

static blend_params_t *s_blend_params = NULL;
static test_area_t *s_area = NULL;

/**
 * @brief LUT with defined benchmark test results for specific Color code, LVGL API function and IDF target
 */
static const benchmark_res_t benchmark_results_lut[] = {

#ifdef CONFIG_IDF_TARGET_ESP32S3

    {LVGL_API_SIMPLE_FILL, LV_COLOR_FORMAT_ARGB8888, 61, 41},
    {LVGL_API_SIMPLE_FILL, LV_COLOR_FORMAT_RGB565,    5,  5},

#elif CONFIG_IDF_TARGET_ESP32

    {LVGL_API_SIMPLE_FILL, LV_COLOR_FORMAT_ARGB8888,  5,  5},
    {LVGL_API_SIMPLE_FILL, LV_COLOR_FORMAT_RGB565,    5,  5},

#else

    // Empty array

#endif

};

#define BENCHMARK_LUT_LENGTH (sizeof(benchmark_results_lut) / sizeof(benchmark_res_t))

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief ARGB8888 benchmark test case entry
 *
 * - initialize the ARGB8888 benchmark test case
 */
static void argb8888_benchmark(void);

/**
 * @brief RGB565 benchmark test case entry
 *
 * - initialize the RGB565 benchmark test case
 */
static void rgb565_benchmark(void);

/**
 * @brief Initialize the benchmark test
 */
static void lv_fill_benchmark_init(bench_test_params_t *test_params);

/**
 * @brief Run the benchmark test
 */
static float lv_fill_benchmark_run(bench_test_params_t *test_params);

/**
 * @brief Evaluate the benchmark test
 */
static void lv_fill_benchmark_eval(float improvement, float improvement_cc);

// ------------------------------------------------ Test cases ---------------------------------------------------------

/*
Benchmark tests

Requires:
    - To pass functionality tests first

Purpose:
    - Test that an acceleration is achieved by an assembly implementation of LVGL blending API

Procedure:
    - Initialize structures needed for LVGL blending API
    - Initialize input parameters (test array length, width, allocate array...) of the benchmark test
    - Run ANSI C version of the LVGL blending API multiple times (1000-times or so)
    - Firstly use an input test parameters for the most ideal case (16-byte aligned array, array width and height divisible by 4 for ARGB8888 color format)
    - Then use worst-case input test parameters (1-byte aligned array, array width and height NOT divisible by 4 for ARGB8888 color format)
    - Count how many CPU cycles does it take to run a function from the LVGL blending API for each case (ideal and worst case)
    - Repeat 4 previous steps with Assembly accelerated LVGL blending API
    - Compare the counted cycles and evaluate the results
    - Free test array
    - Free structures needed for LVGL blending API
*/

TEST_CASE_MULTIPLE_STAGES("LV Fill benchmark", "[lv_fill][benchmark]",
                          argb8888_benchmark,
                          rgb565_benchmark)

// ------------------------------------------------ Test cases stages --------------------------------------------------

static void argb8888_benchmark(void)
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
        .array_align16 = (void *)dest_array_align16,
        .array_align1 = (void *)dest_array_align1,
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_color_format(s_blend_params, LV_COLOR_FORMAT_ARGB8888));
    TEST_ASSERT_EQUAL(ESP_OK, set_api_function_type(s_blend_params, LVGL_API_SIMPLE_FILL));

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for ARGB8888 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}

static void rgb565_benchmark(void)
{
    uint16_t *dest_array_align16  = (uint16_t *)memalign(16, STRIDE * HEIGHT * sizeof(uint16_t) + (UNALIGN_BYTES * sizeof(uint8_t)));
    TEST_ASSERT_NOT_EQUAL(NULL, dest_array_align16);

    // Apply byte unalignment for the worst-case test scenario
    uint16_t *dest_array_align1 = dest_array_align16 + (UNALIGN_BYTES * sizeof(uint8_t));

    bench_test_params_t test_params = {
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE,
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .array_align16 = (void *)dest_array_align16,
        .array_align1 = (void *)dest_array_align1,
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_color_format(s_blend_params, LV_COLOR_FORMAT_RGB565));
    TEST_ASSERT_EQUAL(ESP_OK, set_api_function_type(s_blend_params, LVGL_API_SIMPLE_FILL));

    ESP_LOGI(TAG_LV_FILL_BENCH, "running test for ARGB8888 color format");
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}
// ------------------------------------------------ Static test functions ----------------------------------------------

static void lv_fill_benchmark_init(bench_test_params_t *test_params)
{
    // Update all LV areas with most ideal test cases
    lv_area_set(&s_area->clip,  0, 0, test_params->stride - 1, test_params->height - 1);
    lv_area_set(&s_area->buf,   0, 0, test_params->stride - 1, test_params->height - 1);
    lv_area_set(&s_area->blend, 0, 0, test_params->width - 1,  test_params->height - 1);

    s_blend_params->draw_unit_ansi.target_layer->buf_area = s_area->buf;
    s_blend_params->draw_unit_ansi.target_layer->draw_buf->data = (void *)test_params->array_align16;

    // Run benchmark with the most ideal input parameters
    // Dest array is 16 byte aligned, dest_w and dest_h are dividable by 4
    *s_blend_params->use_asm = true;
    float cycles_asm  = lv_fill_benchmark_run(test_params);     // Call assembly implementation

    *s_blend_params->use_asm = false;
    float cycles_ansi = lv_fill_benchmark_run(test_params);     // Call Ansi implementation
    const float improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark aes3 ideal case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark ansi ideal case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Improvement: %.2f times\n", improvement);

    // Update all LV areas with worst cases (Corner cases)
    lv_area_set(&s_area->clip,  0, 0, test_params->stride - 1, test_params->cc_height - 1);
    lv_area_set(&s_area->buf,   0, 0, test_params->stride - 1, test_params->cc_height - 1);
    lv_area_set(&s_area->blend, 0, 0, test_params->cc_width - 1,  test_params->cc_height - 1);

    s_blend_params->draw_unit_ansi.target_layer->buf_area = s_area->buf;
    s_blend_params->draw_unit_ansi.target_layer->draw_buf->data = (void *)test_params->array_align1;

    // Run benchmark with the corner case parameters
    // Dest array is 1 byte aligned, dest_w and dest_h are not dividable by 4
    *s_blend_params->use_asm = true;
    cycles_asm  = lv_fill_benchmark_run(test_params);           // Call assembly implementation

    *s_blend_params->use_asm = false;
    cycles_ansi = lv_fill_benchmark_run(test_params);           // Call Ansi implementation
    const float improvement_cc = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark aes3 common case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark ansi common case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Improvement: %.2f times", improvement_cc);
    lv_fill_benchmark_eval(improvement, improvement_cc);
}

static float lv_fill_benchmark_run(bench_test_params_t *test_params)
{
    lv_draw_sw_blend(&s_blend_params->draw_unit_ansi, &s_blend_params->blend_dsc);          // Call the DUT function for the first time to init the benchmark test

    const unsigned int start_b = xthal_get_ccount();
    for (int i = 0; i < test_params->benchmark_cycles; i++) {
        lv_draw_sw_blend(&s_blend_params->draw_unit_ansi, &s_blend_params->blend_dsc);
    }
    const unsigned int end_b = xthal_get_ccount();

    const float total_b = end_b - start_b;
    const float cycles = total_b / (test_params->benchmark_cycles);
    return cycles;
}

static void lv_fill_benchmark_eval(float improvement, float improvement_cc)
{
    lv_color_format_t color_format;
    blend_api_func_t api_function;
    float lut_improvement = 0, lut_improvement_cc = 0;
    bool lut_member_found = false;

    TEST_ASSERT_EQUAL(ESP_OK, get_color_format(s_blend_params, &color_format));
    TEST_ASSERT_EQUAL(ESP_OK, get_api_function_type(s_blend_params, &api_function));

    for (int i = 0; i < BENCHMARK_LUT_LENGTH; i++) {
        if (benchmark_results_lut[i].api_function == api_function &&
                benchmark_results_lut[i].dest_color_format == color_format) {

            lut_improvement = (float)(benchmark_results_lut[i].res_improve) / 10.0;
            lut_improvement_cc = (float)(benchmark_results_lut[i].res_improve_cc) / 10.0;
            lut_member_found = true;
            break;
        }
    }

    if (!lut_member_found) {
        TEST_ASSERT_MESSAGE(false, "LUT member in benchmark results LUT not found");
    }

    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(lut_improvement, improvement);
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(lut_improvement_cc, improvement_cc);
}
