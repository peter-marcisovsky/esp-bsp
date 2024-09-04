/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include "lv_color.h"
#include "lv_draw_sw_blend.h"

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------- Macros and Types --------------------------------------------------

/**
 * @brief Type of blend DUT function
 */
typedef enum {
    OPERATION_FILL,
    OPERATION_FILL_WITH_OPA,
} blend_operation_t;

/**
 * @brief Opacity parameters
 */
typedef struct {
    unsigned int min;                       // Minimum opacity
    unsigned int max;                       // Maximum opacity
    unsigned int step;                      // Opacity step
} opa_matrix_params_t;

/**
 * @brief Functionality test combinations
 */
typedef struct {
    unsigned int min_w;                     // Minimum width of the test array
    unsigned int min_h;                     // Minimum height of the test array
    unsigned int max_w;                     // Maximum width of the test array
    unsigned int max_h;                     // Maximum height of the test array
    unsigned int src_min_unalign_byte;      // Minimum amount of unaligned bytes of the source test array
    unsigned int dest_min_unalign_byte;     // Minimum amount of unaligned bytes of the destination test array
    unsigned int src_max_unalign_byte;      // Maximum amount of unaligned bytes of the source test array
    unsigned int dest_max_unalign_byte;     // Maximum amount of unaligned bytes of the destination test array
    unsigned int src_unalign_step;          // Increment step in bytes unalignment of the source test array
    unsigned int dest_unalign_step;         // Increment step in bytes unalignment of the destination test array
    unsigned int src_stride_step;           // Increment step in destination stride of the source test array
    unsigned int dest_stride_step;          // Increment step in destination stride of the destination test array
    opa_matrix_params_t bg_opa;             // Background opacity parameters
    opa_matrix_params_t fg_opa;             // Foreground opacity parameters
    unsigned int test_combinations_count;   // Count of fest combinations
} test_matrix_params_t;

/**
 * @brief Functionality test case parameters
 */
typedef struct {
    struct {
        void *p_src;                                          // pointer to the source test buff (common src buffer for both the ANSI and ASM)
        void *p_src_alloc;                                    // pointer to the beginning of the memory allocated for the source ASM test buf, used in free()
        void *p_dest_asm;                                     // pointer to the destination ASM test buf
        void *p_dest_ansi;                                    // pointer to the destination ANSI test buf
        void *p_dest_asm_alloc;                               // pointer to the beginning of the memory allocated for the destination ASM test buf, used in free()
        void *p_dest_ansi_alloc;                              // pointer to the beginning of the memory allocated for the destination ANSI test buf, used in free()
    } buf;
    void (*blend_api_func)(_lv_draw_sw_blend_image_dsc_t *);  // pointer to LVGL API function
    lv_color_format_t color_format;                           // LV color format
    size_t src_data_type_size;                                // Used data type size in the source buffer, eg sizeof(src_buff[0])
    size_t dest_data_type_size;                               // Used data type size in the destination buffer, eg sizeof(dest_buff[0])
    size_t src_buf_len;                                       // Length of the source buffer, including matrix padding (no Canary bytes are used for source buffer)
    size_t active_dest_buf_len;                               // Length of the destination buffer, where the actual data are stored, including matrix padding, not including Canary bytes
    size_t total_dest_buf_len;                                // Total length of the destination buffer (including Canary bytes and matrix padding)
    unsigned int dest_w;                                      // Destination buffer width
    unsigned int dest_h;                                      // Destination buffer height
    unsigned int src_stride;                                  // Source buffer stride
    unsigned int dest_stride;                                 // Destination buffer stride
    unsigned int src_unalign_byte;                            // Source buffer memory unalignment
    unsigned int dest_unalign_byte;                           // Destination buffer memory unalignment
    lv_opa_t bg_opa;                                          // Background opacity
    lv_opa_t fg_opa;                                          // Foreground opacity
    bool static_bg_opa;                                       // Static or dynamic background opacity
    blend_operation_t operation_type;                         // Type of fundamental blend operation
} test_case_params_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif
