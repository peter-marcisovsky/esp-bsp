/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include "unity.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lv_fill_common.h"

// ------------------------------------------------- Defines -----------------------------------------------------------

#define DBG_PRINT_OUTPUT false
#define CANARY_BYTES 4

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char *TAG_LV_FILL_FUNC = "LV Fill Functionality";

static blend_params_t *s_blend_params = NULL;
static test_area_t *s_area = NULL;
static char test_msg_buff[128];

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief Generate all the functionality test combinations
 *
 * - generate functionality test combinations, based on the provided test_params struct
 *
 * @param[in] test_params Pointer to structure with test_parameters
 */
static void functionality_test_matrix(func_test_params_t *test_params);

/**
 * @brief The actual LV Fill ARGB8888 functionality test
 */
static void lv_fill_argb8888_functionality(int w, int h, int stride, int unalign_byte);

/**
 * @brief The actual LV Fill RGB565 functionality test
 */
static void lv_fill_rgb565_functionality(int w, int h, int stride, int unalign_byte);

/**
 * @brief Run the LVGL blend API
 */
static void run_lv_blend_api(void *dest_buff_ansi, void *dest_buff_asm);

// ------------------------------------------------ Test cases ---------------------------------------------------------

/*
Functionality tests

Purpose:
    - Test that an assembly version of LVGL blending API works correctly

Procedure:
    - Initialize structures needed for LVGL blending API
    - Initialize input parameters (test array length, width, allocate array...) of the benchmark test
    - Prepare testing matrix, to cover all the possible combinations of destination array widths, lengths, memory alignment...
    - Run assembly version of the LVGL blending API
    - Run ANSI C version of the LVGL blending API
    - Compare the results
    - Repeat above 3 steps for each test matrix setup
    - Free structures needed for LVGL blending API
*/

// ------------------------------------------------ Test cases stages --------------------------------------------------

TEST_CASE("Test fill functionality ARGB8888", "[fill][functionality][ARGB8888]")
{
    func_test_params_t test_params = {
        .color_format = LV_COLOR_FORMAT_ARGB8888,
        .min_w = 8,             // 8 is the lower limit for the esp32s3 asm implementation, otherwise esp32 is executed
        .min_h = 1,
        .max_w = 16,
        .max_h = 16,
        .min_unalign_byte = 0,
        .max_unalign_byte = 16,
        .unalign_step = 1,
        .stride_step = 1,
        .test_combinations_count = 0,
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_color_format(s_blend_params, LV_COLOR_FORMAT_ARGB8888));
    ESP_LOGI(TAG_LV_FILL_FUNC, "running test for ARGB8888 color format");
    functionality_test_matrix(&test_params);
}

TEST_CASE("Test fill functionality RGB565", "[fill][functionality][RGB565]")
{
    func_test_params_t test_params = {
        .color_format = LV_COLOR_FORMAT_RGB565,
        .min_w = 8,             // 8 is the lower limit for the esp32s3 asm implementation, otherwise esp32 is executed
        .min_h = 1,
        .max_w = 16,
        .max_h = 16,
        .min_unalign_byte = 0,
        .max_unalign_byte = 16,
        .unalign_step = 1,
        .stride_step = 1,
        .test_combinations_count = 0,
    };

    TEST_ASSERT_EQUAL(ESP_OK, get_blend_params(&s_blend_params, &s_area));
    TEST_ASSERT_EQUAL(ESP_OK, set_color_format(s_blend_params, LV_COLOR_FORMAT_RGB565));
    ESP_LOGI(TAG_LV_FILL_FUNC, "running test for RGB565 color format");
    functionality_test_matrix(&test_params);
}

// ------------------------------------------------ Static test functions ----------------------------------------------


static void functionality_test_matrix(func_test_params_t *test_params)
{
    // Step width
    for (int w = test_params->min_w; w <= test_params->max_w; w++) {

        // Step height
        for (int h = test_params->min_h; h <= test_params->max_h; h++) {

            // Step stride
            for (int stride = w; stride <= w * 2; stride += test_params->stride_step) {

                // Step unalignment
                for (int unalign_byte = test_params->min_unalign_byte; unalign_byte <= test_params->max_unalign_byte; unalign_byte += test_params->unalign_step) {

                    switch (test_params->color_format) {
                    case LV_COLOR_FORMAT_ARGB8888: {
                        lv_fill_argb8888_functionality(w, h, stride, unalign_byte);
                        break;
                    }
                    case LV_COLOR_FORMAT_RGB565: {
                        lv_fill_rgb565_functionality(w, h, stride, unalign_byte);
                        break;
                    }
                    default:
                        break;
                    }
                    test_params->test_combinations_count++;
                }
            }
        }
    }
    ESP_LOGI(TAG_LV_FILL_FUNC, "test combinations: %d\n", test_params->test_combinations_count);
}

static void lv_fill_argb8888_functionality(int w, int h, int stride, int unalign_byte)
{
    const unsigned int total_len = ((h * stride) + (CANARY_BYTES * 2));

    // Allocate destination arrays for Assembly and ANSI LVGL Blend API
    uint32_t *mem_asm   = (uint32_t *)memalign(16, (total_len * sizeof(uint32_t)) + (unalign_byte * sizeof(uint8_t)));
    uint32_t *mem_ansi  = (uint32_t *)memalign(16, (total_len * sizeof(uint32_t)) + (unalign_byte * sizeof(uint8_t)));
    TEST_ASSERT_NOT_EQUAL(NULL, mem_ansi);
    TEST_ASSERT_NOT_EQUAL(NULL, mem_asm);

    // Apply destination array unalignment
    uint8_t *dest_buff_asm = (uint8_t *)mem_asm + unalign_byte;
    uint8_t *dest_buff_ansi = (uint8_t *)mem_ansi + unalign_byte;

    // Fill both destination arrays with known values
    // Initialize Canary Bytes area in the beginning of the destination buffers by 0
    for (int i = 0; i < CANARY_BYTES; i++) {
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    // Fill the actual part of the destination buffers with any known values
    for (int i = CANARY_BYTES; i < (h * stride) + CANARY_BYTES; i++) {
        ((uint32_t *)dest_buff_asm)[i] = i;
        ((uint32_t *)dest_buff_ansi)[i] = i;
    }

    // Initialize Canary Bytes area at the end of the destination buffers by 0
    for (int i = total_len - CANARY_BYTES; i < total_len; i++) {
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    // Shift array pointers by Canary Bytes amount
    dest_buff_asm += CANARY_BYTES * sizeof(uint32_t);
    dest_buff_ansi += CANARY_BYTES * sizeof(uint32_t);

    // Update all LV areas
    lv_area_set(&s_area->clip, 0, 0, stride - 1, h - 1);
    lv_area_set(&s_area->buf,  0, 0, stride - 1, h - 1);
    lv_area_set(&s_area->blend, 0, 0, w - 1,      h - 1);

    // Run  the LVGL API
    run_lv_blend_api((void *)dest_buff_ansi, (void *)dest_buff_asm);

    // Shift array pointers by Canary Bytes amount back
    dest_buff_asm -= CANARY_BYTES * sizeof(uint32_t);
    dest_buff_ansi -= CANARY_BYTES * sizeof(uint32_t);

    // Print results
#if DBG_PRINT_OUTPUT
    for (uint32_t i = 0; i < total_len; i++) {
        printf("dest_buff[%"PRIi32"] %s ansi = %8"PRIx32" \t asm = %8"PRIx32" \n", i, ((i < 10) ? (" ") : ("")), ((uint32_t *)dest_buff_ansi)[i], ((uint32_t *)dest_buff_asm)[i]);
    }
    printf("\n");
#endif

    // Evaluate the results
    sprintf(test_msg_buff, "LV Fill ARGB8888: w = %d, h = %d, stride = %d, unalign_byte = %d\n", w, h, stride, unalign_byte);

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_ansi, CANARY_BYTES, test_msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_asm, CANARY_BYTES, test_msg_buff);

    // dest_buff_asm and dest_buff_ansi must be equal
    TEST_ASSERT_EQUAL_UINT32_ARRAY_MESSAGE((uint32_t *)dest_buff_asm + CANARY_BYTES, (uint32_t *)dest_buff_ansi + CANARY_BYTES, h * stride, test_msg_buff);

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_ansi + (total_len - CANARY_BYTES), CANARY_BYTES, test_msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_asm + (total_len - CANARY_BYTES), CANARY_BYTES, test_msg_buff);

    free(mem_ansi);
    free(mem_asm);
}

static void lv_fill_rgb565_functionality(int w, int h, int stride, int unalign_byte)
{
    const unsigned int total_len = ((h * stride) + (CANARY_BYTES * 2));

    // Allocate destination arrays for Assembly and ANSI LVGL Blend API
    uint16_t *mem_asm   = (uint16_t *)memalign(16, (total_len * sizeof(uint16_t)) + (unalign_byte * sizeof(uint8_t)));
    uint16_t *mem_ansi  = (uint16_t *)memalign(16, (total_len * sizeof(uint16_t)) + (unalign_byte * sizeof(uint8_t)));
    TEST_ASSERT_NOT_EQUAL(NULL, mem_ansi);
    TEST_ASSERT_NOT_EQUAL(NULL, mem_asm);

    // Apply destination array unalignment
    uint8_t *dest_buff_asm = (uint8_t *)mem_asm + unalign_byte;
    uint8_t *dest_buff_ansi = (uint8_t *)mem_ansi + unalign_byte;

    // Fill both destination arrays with known values
    // Initialize Canary Bytes area in the beginning of the destination buffers by 0
    for (int i = 0; i < CANARY_BYTES; i++) {
        ((uint16_t *)dest_buff_asm)[i] = 0;
        ((uint16_t *)dest_buff_ansi)[i] = 0;
    }

    // Fill the actual part of the destination buffers with any known values
    for (int i = CANARY_BYTES; i < (h * stride) + CANARY_BYTES; i++) {
        ((uint16_t *)dest_buff_asm)[i] = i;
        ((uint16_t *)dest_buff_ansi)[i] = i;
    }

    // Initialize Canary Bytes area at the end of the destination buffers by 0
    for (int i = total_len - CANARY_BYTES; i < total_len; i++) {
        ((uint16_t *)dest_buff_asm)[i] = 0;
        ((uint16_t *)dest_buff_ansi)[i] = 0;
    }

    // Shift array pointers by Canary Bytes amount
    dest_buff_asm += CANARY_BYTES * sizeof(uint16_t);
    dest_buff_ansi += CANARY_BYTES * sizeof(uint16_t);

    // Update all LV areas
    lv_area_set(&s_area->clip, 0, 0, stride - 1, h - 1);
    lv_area_set(&s_area->buf,  0, 0, stride - 1, h - 1);
    lv_area_set(&s_area->blend, 0, 0, w - 1,      h - 1);

    // Run  the LVGL API
    run_lv_blend_api((void *)dest_buff_ansi, (void *)dest_buff_asm);

    // Shift array pointers by Canary Bytes amount back
    dest_buff_asm -= CANARY_BYTES * sizeof(uint16_t);
    dest_buff_ansi -= CANARY_BYTES * sizeof(uint16_t);

    // Print results
#if DBG_PRINT_OUTPUT
    for (uint32_t i = 0; i < total_len; i++) {
        printf("dest_buff[%"PRIi32"] %s ansi = %4"PRIx16" \t asm = %4"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)dest_buff_ansi)[i], ((uint16_t *)dest_buff_asm)[i]);
    }
    printf("\n");
#endif

    // Evaluate test
    sprintf(test_msg_buff, "LV Fill RGB565: w = %d, h = %d, stride = %d, unalign_byte = %d\n", w, h, stride, unalign_byte);

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_ansi, CANARY_BYTES, test_msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_asm, CANARY_BYTES, test_msg_buff);

    // dest_buff_asm and dest_buff_ansi must be equal
    TEST_ASSERT_EQUAL_UINT16_ARRAY_MESSAGE((uint16_t *)dest_buff_asm + CANARY_BYTES, (uint16_t *)dest_buff_ansi + CANARY_BYTES, h * stride, test_msg_buff);

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_ansi + (total_len - CANARY_BYTES), CANARY_BYTES, test_msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_asm + (total_len - CANARY_BYTES), CANARY_BYTES, test_msg_buff);

    free(mem_asm);
    free(mem_ansi);
}

static void run_lv_blend_api(void *dest_buff_ansi, void *dest_buff_asm)
{
    // Clip and Blend areas are accessed in blend_params by reference
    // Only Buf area is accessed by value, thus have to be updated here
    s_blend_params->draw_unit_ansi.target_layer->buf_area = s_area->buf;
    s_blend_params->draw_unit_asm.target_layer->buf_area = s_area->buf;

    // Update the actual data buffers
    s_blend_params->draw_unit_ansi.target_layer->draw_buf->data = dest_buff_ansi;
    s_blend_params->draw_unit_asm.target_layer->draw_buf->data = dest_buff_asm;

    *s_blend_params->use_asm = true;
    ESP_LOGD(TAG_LV_FILL_FUNC, "Calling ASM LVGL blend API");
    lv_draw_sw_blend(&s_blend_params->draw_unit_asm, &s_blend_params->blend_dsc);

    *s_blend_params->use_asm = false;
    ESP_LOGD(TAG_LV_FILL_FUNC, "Calling ANSI LVGL blend API");
    lv_draw_sw_blend(&s_blend_params->draw_unit_ansi, &s_blend_params->blend_dsc);
}
