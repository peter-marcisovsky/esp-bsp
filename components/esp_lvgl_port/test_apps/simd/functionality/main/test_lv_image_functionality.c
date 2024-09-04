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
#include "lv_image_common.h"
#include "lv_draw_sw_blend.h"
#include "lv_draw_sw_blend_to_argb8888.h"
#include "lv_draw_sw_blend_to_rgb565.h"

// ------------------------------------------------- Defines -----------------------------------------------------------

#define DBG_PRINT_OUTPUT false
#define CANARY_BYTES 4

// ------------------------------------------------- Macros and Types --------------------------------------------------

#define UPDATE_TEST_CASE(test_case_ptr, dest_w, dest_h, src_stride, dest_stride, src_unalign_byte, dest_unalign_byte, bg_opa, fg_opa) ({  \
    (test_case_ptr)->src_buf_len = (size_t)(dest_h * src_stride);                                 \
    (test_case_ptr)->active_dest_buf_len = (size_t)(dest_h * dest_stride);                        \
    (test_case_ptr)->total_dest_buf_len = (size_t)((dest_h * dest_stride) + (CANARY_BYTES * 2));  \
    (test_case_ptr)->dest_w = (dest_w);                         \
    (test_case_ptr)->dest_h = (dest_h);                         \
    (test_case_ptr)->src_stride = (src_stride);                 \
    (test_case_ptr)->dest_stride = (dest_stride);               \
    (test_case_ptr)->src_unalign_byte = (src_unalign_byte);     \
    (test_case_ptr)->dest_unalign_byte = (dest_unalign_byte);   \
    (test_case_ptr)->bg_opa = (bg_opa);                         \
    (test_case_ptr)->fg_opa = (fg_opa);                         \
})

// Update opacity step during the test
// Step finely through the corner cases (around minimal and maximal OPA)
// Step coarsely through the middle OPA values
#define UPDATE_OPA_STEP(opa_step_updated, opa_step, opa, opa_min, opa_max) ({ \
    if ((opa > opa_min + 5) && (opa <= opa_max - 10)) {     \
        opa_step_updated = 20;                              \
    } else {                                                \
        opa_step_updated = opa_step;                        \
    }                                                       \
})

static const char *TAG_LV_FILL_FUNC = "LV Fill Functionality";
static char test_msg_buf[200];

//static lv_color_t fg_color = { .blue = 0x56, .green = 0x34, .red = 0x12, };
//static lv_color_t bg_color = { .blue = 0xEF, .green = 0xCD, .red = 0xAB, };

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief Generate all the functionality test combinations
 *
 * - generate functionality test combinations, based on the provided test_matrix struct
 *
 * @param[in] test_matrix Pointer to structure defining test matrix - all the test combinations
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void functionality_test_matrix(test_matrix_params_t *test_matrix, test_case_params_t *test_case);

/**
 * @brief Fill test buffers for functionality test
 *
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void fill_test_bufs(test_case_params_t *test_case);

/**
 * @brief The actual functionality test
 *
 * - function prepares structures for functionality testing and runs the LVGL API
 *
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void lv_fill_functionality(test_case_params_t *test_case);

/**
 * @brief Evaluate results for 32bit data length
 *
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void test_eval_32bit_data(test_case_params_t *test_case);

/**
 * @brief Evaluate results for 16bit data length
 *
 * @param[in] test_case Pointer ot structure defining functionality test case
 */
static void test_eval_16bit_data(test_case_params_t *test_case);

// ------------------------------------------------ Test cases ---------------------------------------------------------

/*
Functionality tests

Purpose:
    - Test that an assembly version of LVGL blending API achieves the same results as the ANSI version

Procedure:
    - Prepare testing matrix, to cover all the possible combinations of destination array widths, lengths, memory alignment...
    - Run assembly version of the LVGL blending API
    - Run ANSI C version of the LVGL blending API
    - Compare the results
    - Repeat above 3 steps for each test matrix setup
*/

// ------------------------------------------------ Test cases stages --------------------------------------------------

TEST_CASE("Test image functionality RGB565", "[image][functionality][RGB565]")
{
    test_matrix_params_t test_matrix = {
        .min_w = 8,             // 8 is the lower limit for the esp32s3 asm implementation, otherwise esp32 is executed
        .min_h = 2,
        .max_w = 16,
        .max_h = 16,
        .src_min_unalign_byte = 0,
        .src_max_unalign_byte = 16,
        .src_unalign_step = 1,
        .src_stride_step = 1,
        .dest_min_unalign_byte = 0,
        .dest_max_unalign_byte = 16,
        .dest_unalign_step = 1,
        .dest_stride_step = 1,
        .bg_opa = {.min = LV_OPA_100, .max = LV_OPA_100},  // Do not step background opacity, set to max opacity
        .fg_opa = {.min = LV_OPA_100, .max = LV_OPA_100},  // Do not step foreground opacity, set to max opacity
        .test_combinations_count = 0,
    };

    test_case_params_t test_case = {
        .blend_api_func = &lv_draw_sw_blend_image_to_rgb565,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .src_data_type_size = sizeof(uint16_t),
        .dest_data_type_size = sizeof(uint16_t),
        .operation_type = OPERATION_FILL,
    };

    ESP_LOGI(TAG_LV_FILL_FUNC, "running test for RGB565 color format");
    functionality_test_matrix(&test_matrix, &test_case);
}

// ------------------------------------------------ Static test functions ----------------------------------------------

static void functionality_test_matrix(test_matrix_params_t *test_matrix, test_case_params_t *test_case)
{
    // Avoid infinite loop
    if (!test_matrix->bg_opa.step) {
        test_matrix->bg_opa.step = 1;
    }
    if (!test_matrix->fg_opa.step) {
        test_matrix->fg_opa.step = 1;
    }

    int bg_opa_step = test_matrix->bg_opa.step;
    int fg_opa_step = test_matrix->fg_opa.step;

    // Step destination array width
    for (int dest_w = test_matrix->min_w; dest_w <= test_matrix->max_w; dest_w++) {

        // Step destination array height
        for (int dest_h = test_matrix->min_h; dest_h <= test_matrix->max_h; dest_h++) {

            // Step source array stride
            for (int src_stride = dest_w; src_stride <= dest_w * 2; src_stride += test_matrix->src_stride_step) {

                // Step destination array stride
                for (int dest_stride = dest_w; dest_stride <= dest_w * 2; dest_stride += test_matrix->dest_stride_step) {

                    // Step source array unalignment
                    for (int src_unalign_byte = test_matrix->src_min_unalign_byte; src_unalign_byte <= test_matrix->src_max_unalign_byte; src_unalign_byte += test_matrix->src_unalign_step) {

                        // Step destination array unalignment
                        for (int dest_unalign_byte = test_matrix->dest_min_unalign_byte; dest_unalign_byte <= test_matrix->dest_max_unalign_byte; dest_unalign_byte += test_matrix->dest_unalign_step) {

                            // Step background opacity
                            for (int bg_opa = test_matrix->bg_opa.min; bg_opa <= test_matrix->bg_opa.max; bg_opa += bg_opa_step) {
                                UPDATE_OPA_STEP(bg_opa_step, test_matrix->bg_opa.step, bg_opa, test_matrix->bg_opa.min, test_matrix->bg_opa.max);

                                // Step foreground opacity
                                for (int fg_opa = test_matrix->fg_opa.min; fg_opa <= test_matrix->fg_opa.max; fg_opa += fg_opa_step) {
                                    UPDATE_OPA_STEP(fg_opa_step, test_matrix->fg_opa.step, fg_opa, test_matrix->fg_opa.min, test_matrix->fg_opa.max);

                                    //printf("HI\n");
                                    // Call functionality test
                                    UPDATE_TEST_CASE(test_case, dest_w, dest_h, src_stride, dest_stride, src_unalign_byte, dest_unalign_byte, bg_opa, fg_opa);
                                    lv_fill_functionality(test_case);
                                    test_matrix->test_combinations_count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    ESP_LOGI(TAG_LV_FILL_FUNC, "test combinations: %d\n", test_matrix->test_combinations_count);
}

static void lv_fill_functionality(test_case_params_t *test_case)
{
    fill_test_bufs(test_case);

    _lv_draw_sw_blend_image_dsc_t dsc_asm = {
        .dest_buf = test_case->buf.p_dest_asm,
        .dest_w = test_case->dest_w,
        .dest_h = test_case->dest_h,
        .dest_stride = test_case->dest_stride * test_case->dest_data_type_size,  // dest_stride * sizeof(data_type)
        .mask_buf = NULL,
        .mask_stride = 0,
        .src_buf = test_case->buf.p_src,
        .src_stride = test_case->src_stride * test_case->src_data_type_size,     // src_stride * sizeof(data_type)
        .src_color_format = test_case->color_format,
        .opa = test_case->fg_opa,
        .blend_mode = LV_BLEND_MODE_NORMAL,
        .use_asm = true,
    };

    // Init structure for LVGL blend API, to call the ANSI API
    _lv_draw_sw_blend_image_dsc_t dsc_ansi = dsc_asm;
    dsc_ansi.dest_buf = test_case->buf.p_dest_ansi;
    dsc_ansi.use_asm = false;

    test_case->blend_api_func(&dsc_asm);    // Call the LVGL API with Assembly code
    test_case->blend_api_func(&dsc_ansi);   // Call the LVGL API with ANSI code

    // Shift array pointers by Canary Bytes amount back
    test_case->buf.p_dest_asm -= CANARY_BYTES * test_case->dest_data_type_size;
    test_case->buf.p_dest_ansi -= CANARY_BYTES * test_case->dest_data_type_size;

    // Evaluate the results
    sprintf(test_msg_buf, "Test case: dest_w = %d, dest_h = %d, dest_stride = %d, src_stride = %d, dest_unalign_byte = %d, src_unalign_byte = %d, bg_opa = %d, fg_opa = %d\n",
            test_case->dest_w, test_case->dest_h, test_case->dest_stride, test_case->src_stride, test_case->dest_unalign_byte, test_case->src_unalign_byte, test_case->bg_opa, test_case->fg_opa);
#if DBG_PRINT_OUTPUT
    printf("%s\n", test_msg_buf);
#endif
    switch (test_case->color_format) {
    case LV_COLOR_FORMAT_ARGB8888:
        test_eval_32bit_data(test_case);
        break;
    case LV_COLOR_FORMAT_RGB565:
        test_eval_16bit_data(test_case);
        break;
    default:
        TEST_ASSERT_MESSAGE(false, "LV Color format not found");
        break;
    }

    free(test_case->buf.p_dest_asm_alloc);
    free(test_case->buf.p_dest_ansi_alloc);
    free(test_case->buf.p_src_alloc);
}

static void fill_test_bufs(test_case_params_t *test_case)
{
    const size_t src_data_type_size = test_case->src_data_type_size;        // sizeof() of used data type in the source buffer
    const size_t dest_data_type_size = test_case->dest_data_type_size;      // sizeof() of used data type in the destination buffer
    const size_t src_buf_len = test_case->src_buf_len;                      // Total source buffer length, data part of the source buffer including matrix padding (no Canary bytes are used for source buffer)
    const size_t total_dest_buf_len = test_case->total_dest_buf_len;        // Total destination buffer length, data part of the destination buffer including the Canary bytes and matrix padding
    const size_t active_dest_buf_len = test_case->active_dest_buf_len;      // Length of the data part of the destination buffer including matrix padding
    const unsigned int src_unalign_byte = test_case->src_unalign_byte;      // Unalignment bytes for source buffer
    const unsigned int dest_unalign_byte = test_case->dest_unalign_byte;    // Unalignment bytes for destination buffer

    // Allocate destination arrays and source array for Assembly and ANSI LVGL Blend API
    void *src_mem_common = memalign(16, (src_buf_len * src_data_type_size) + src_unalign_byte);
    void *dest_mem_asm   = memalign(16, (total_dest_buf_len * dest_data_type_size) + dest_unalign_byte);
    void *dest_mem_ansi  = memalign(16, (total_dest_buf_len * dest_data_type_size) + dest_unalign_byte);
    TEST_ASSERT_NOT_NULL_MESSAGE(src_mem_common, "Lack of memory");
    TEST_ASSERT_NOT_NULL_MESSAGE(dest_mem_asm, "Lack of memory");
    TEST_ASSERT_NOT_NULL_MESSAGE(dest_mem_ansi, "Lack of memory");

    // Save a pointer to the beginning of the allocated memory which will be used to free()
    test_case->buf.p_src_alloc = src_mem_common;
    test_case->buf.p_dest_asm_alloc = dest_mem_asm;
    test_case->buf.p_dest_ansi_alloc = dest_mem_ansi;

    // Apply destination and source array unalignment
    uint8_t *src_buf_common = (uint8_t *)src_mem_common + src_unalign_byte;
    uint8_t *dest_buf_asm = (uint8_t *)dest_mem_asm + dest_unalign_byte;
    uint8_t *dest_buf_ansi = (uint8_t *)dest_mem_ansi + dest_unalign_byte;

    // Set the whole buffer to 0, including the Canary bytes part
    memset(src_buf_common, 0, src_buf_len * src_data_type_size);
    memset(dest_buf_asm, 0, total_dest_buf_len * src_data_type_size);
    memset(dest_buf_ansi, 0, total_dest_buf_len * src_data_type_size);

    switch (test_case->operation_type) {
    case OPERATION_FILL:
        // Fill the actual part of the destination buffers with known values,
        // Values must be same, because of the stride
        for (int i = CANARY_BYTES * 2; i < (active_dest_buf_len + CANARY_BYTES) * 2; i++) {
            dest_buf_asm[i] = (uint8_t)(((i - CANARY_BYTES * 2) * 2) % 256);          // Fill dest buffs with even values
            dest_buf_ansi[i] = (uint8_t)(((i - CANARY_BYTES * 2) * 2) % 256);
        }

        //printf("Destination buffers fill:\n");
        //for (uint32_t i = 0; i < test_case->total_dest_buf_len; i++) {
        //    printf("dest_buf[%"PRIi32"] %s ansi = %8"PRIx16" \t asm = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)dest_buf_asm)[i], ((uint16_t *)dest_buf_ansi)[i]);
        //}
        //printf("\n");

        for (int i = 0; i < src_buf_len * 2; i++) {
            src_buf_common[i] = (uint8_t)((i * 2 + 1) % 256);                      // Fill src buffs with odd values
        }
        break;
    default:
        break;
    }

    // Shift array pointers by Canary Bytes amount
    dest_buf_asm += CANARY_BYTES * dest_data_type_size;
    dest_buf_ansi += CANARY_BYTES * dest_data_type_size;

    // Save a pointer to the working part of the memory, where the test data are stored
    test_case->buf.p_src = (void *)src_buf_common;
    test_case->buf.p_dest_asm = (void *)dest_buf_asm;
    test_case->buf.p_dest_ansi = (void *)dest_buf_ansi;

#if DBG_PRINT_OUTPUT
    printf("Destination buffers fill:\n");
    for (uint32_t i = 0; i < test_case->active_dest_buf_len; i++) {
        printf("dest_buf[%"PRIi32"] %s ansi = %8"PRIx16" \t asm = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_dest_ansi)[i], ((uint16_t *)test_case->buf.p_dest_asm)[i]);
    }
    printf("\n");

    printf("Source buffer fill:\n");
    for (uint32_t i = 0; i < test_case->src_buf_len; i++) {
        printf("src_buf[%"PRIi32"] %s = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_src)[i]);
    }
    printf("\n");
#endif

}

static void test_eval_32bit_data(test_case_params_t *test_case)
{
    // Print results 32bit data
#if DBG_PRINT_OUTPUT
    printf("Destination buffers fill:\n");
    for (uint32_t i = 0; i < test_case->total_dest_buf_len; i++) {
        printf("dest_buf[%"PRIi32"] %s ansi = %8"PRIx32" \t asm = %8"PRIx32" \n", i, ((i < 10) ? (" ") : ("")), ((uint32_t *)test_case->buf.p_dest_ansi)[i], ((uint32_t *)test_case->buf.p_dest_asm)[i]);
    }
    printf("\n");

    printf("Source buffer fill:\n");
    for (uint32_t i = 0; i < test_case->src_buf_len; i++) {
        printf("src_buf[%"PRIi32"] %s = %8"PRIx32" \n", i, ((i < 10) ? (" ") : ("")), ((uint32_t *)test_case->buf.p_src)[i]);
    }
    printf("\n");
#endif

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)test_case->buf.p_dest_ansi, CANARY_BYTES, test_msg_buf);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)test_case->buf.p_dest_asm, CANARY_BYTES, test_msg_buf);

    // dest_buf_asm and dest_buf_ansi must be equal
    TEST_ASSERT_EQUAL_UINT32_ARRAY_MESSAGE((uint32_t *)test_case->buf.p_dest_ansi + CANARY_BYTES, (uint32_t *)test_case->buf.p_dest_asm + CANARY_BYTES, test_case->active_dest_buf_len, test_msg_buf);

    // Data part of the destination buffer and source buffer (not considering matrix padding) must be equal
    uint32_t *dest_row_begin = (uint32_t *)test_case->buf.p_dest_asm + CANARY_BYTES;
    uint32_t *src_row_begin = (uint32_t *)test_case->buf.p_src;
    for (int row = 0; row < test_case->dest_h; row++) {
        TEST_ASSERT_EQUAL_UINT32_ARRAY(dest_row_begin, src_row_begin, test_case->dest_w);
        dest_row_begin += test_case->dest_stride;   // Move pointer of the destination buffer to next row
        src_row_begin += test_case->src_stride;     // Move pointer of the source buffer to next row
    }

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)test_case->buf.p_dest_ansi + (test_case->total_dest_buf_len - CANARY_BYTES), CANARY_BYTES, test_msg_buf);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)test_case->buf.p_dest_asm + (test_case->total_dest_buf_len - CANARY_BYTES), CANARY_BYTES, test_msg_buf);
}

static void test_eval_16bit_data(test_case_params_t *test_case)
{
    // Print results, 16bit data
#if DBG_PRINT_OUTPUT
    printf("Destination buffers fill:\n");
    for (uint32_t i = 0; i < test_case->total_dest_buf_len; i++) {
        printf("dest_buf[%"PRIi32"] %s ansi = %8"PRIx16" \t asm = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_dest_ansi)[i], ((uint16_t *)test_case->buf.p_dest_asm)[i]);
    }
    printf("\n");

    printf("Source buffer fill:\n");
    for (uint32_t i = 0; i < test_case->src_buf_len; i++) {
        printf("src_buf[%"PRIi32"] %s = %8"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)test_case->buf.p_src)[i]);
    }
    printf("\n");
#endif

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_ansi, CANARY_BYTES, test_msg_buf);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_asm, CANARY_BYTES, test_msg_buf);

    // dest_buf_asm and dest_buf_ansi must be equal
    TEST_ASSERT_EQUAL_UINT16_ARRAY_MESSAGE((uint16_t *)test_case->buf.p_dest_ansi + CANARY_BYTES, (uint16_t *)test_case->buf.p_dest_asm + CANARY_BYTES, test_case->active_dest_buf_len, test_msg_buf);

    // Data part of the destination buffer and source buffer (not considering matrix padding) must be equal
    uint16_t *dest_row_begin = (uint16_t *)test_case->buf.p_dest_asm + CANARY_BYTES;
    uint16_t *src_row_begin = (uint16_t *)test_case->buf.p_src;
    for (int row = 0; row < test_case->dest_h; row++) {
        TEST_ASSERT_EQUAL_UINT16_ARRAY(dest_row_begin, src_row_begin, test_case->dest_w);
        dest_row_begin += test_case->dest_stride;   // Move pointer of the destination buffer to next row
        src_row_begin += test_case->src_stride;     // Move pointer of the source buffer to next row
    }

    // Canary bytes area must stay 0
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_ansi + (test_case->total_dest_buf_len - CANARY_BYTES), CANARY_BYTES, test_msg_buf);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)test_case->buf.p_dest_asm + (test_case->total_dest_buf_len - CANARY_BYTES), CANARY_BYTES, test_msg_buf);
}
