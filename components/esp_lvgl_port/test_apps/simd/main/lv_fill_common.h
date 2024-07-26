/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "lvgl.h"
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------- Macros and Types --------------------------------------------------

/**
 * @brief Types of LVGL API functions
 */
typedef enum {
    LVGL_API_NOT_SET = 0,
    LVGL_API_SIMPLE_FILL,
    LVGL_API_SIMPLE_FILL_OPA,
    // Add more LVGL API function types
} blend_api_func_t;

/**
 * @brief LV areas used for functionality and benchmark testing
 */
typedef struct {
    lv_area_t clip;         // Protective area around the blending area
    lv_area_t buf;          // Canvas (The whole display area)
    lv_area_t blend;        // Area where the blending happens
} test_area_t;

/**
 * @brief Input parameters for lv_draw_sw_blend function
 */
typedef struct {
    lv_draw_unit_t draw_unit_ansi;          // Draw Unit for C implementation of LV Blend API
    lv_draw_unit_t draw_unit_asm;           // Draw Unit for Assembly implementation of LV Blend API
    lv_draw_sw_blend_dsc_t blend_dsc;       // Common blend descriptor
    bool *use_asm;                          // Extern variable from esp_lvgl_port_lv_blend.h to control switching
    // between assembly or C implementation of the LV Blend API
    blend_api_func_t api_function;          // Type of LVGL blend API function currently under the test
} blend_params_t;

/**
 * @brief Functionality test parameters
 */
typedef struct {
    lv_color_format_t color_format;         // LV color format
    unsigned int min_w;                     // Minimum width of the test array
    unsigned int min_h;                     // Minimum height of the test array
    unsigned int max_w;                     // Maximum width of the test array
    unsigned int max_h;                     // Maximum height of the test array
    unsigned int min_unalign_byte;          // Minimum amount of unaligned bytes of the test array
    unsigned int max_unalign_byte;          // Maximum amount of unaligned bytes of the test array
    unsigned int unalign_step;              // Increment step in bytes unalignment of the test array
    unsigned int stride_step;               // Increment step in stride of the test array
    unsigned int test_combinations_count;   // Count of fest combinations
} func_test_params_t;

/**
 * @brief Benchmark test parameters
 */
typedef struct {
    unsigned int height;                    // Test array height
    unsigned int width;                     // Test array width
    unsigned int stride;                    // Test array stride
    unsigned int cc_height;                 // Corner case test array height
    unsigned int cc_width;                  // Corner case test array width
    unsigned int benchmark_cycles;          // Count of benchmark cycles
    void *array_align16;                    // test array with 16 byte alignment - testing most ideal case
    void *array_align1;                     // test array with 1 byte alignment - testing wort case
} bench_test_params_t;

/**
 * @brief Structure used for Benchmark results LUT
 */
typedef struct {
    blend_api_func_t api_function;          // Type of LVGL blend API function currently under the test
    lv_color_format_t dest_color_format;    // LV color format
    uint8_t res_improve;                    // Benchmark result improvement (values saved as decimal value multiplied by 10).. (uint8_t)(4.5 * 10)
    uint8_t res_improve_cc;                 // Benchmark result improvement for corner case
} benchmark_res_t;

/**
 * @brief Get initialized blend parameters
 *
 * @note init_blend_params() must be called before to init the structures
 *
 * @param[out] blend_params_ret: pointer to pointer to strucure needed to run sw blend API from LVGL
 * @param[out] area_ret: pointer to pointer to set of areas needed for the sw blend
 *
 * @retval ESP_OK: parameters obtained successfully
 * @retval ESP_INVALID_STATE: blend stuctures have not been initialized
 */
esp_err_t get_blend_params(blend_params_t **blend_params_ret, test_area_t **area_ret);

/**
 * @brief Set color format
 *
 * @note init_blend_params() must be called before to init the structures
 *
 * @param[out] blend_params: pointer to strucure needed to run sw blend API from LVGL
 * @param[out] color_format: LVGL color format
 *
 * @retval ESP_OK: Color format set successfully
 * @retval ESP_INVALID_STATE: blend stuctures have not been initialized
 */
esp_err_t set_color_format(blend_params_t *blend_params, lv_color_format_t color_format);

/**
 * @brief Get currently used color format
 *
 * @note init_blend_params() must be called before to init the structures
 *
 * @param[out] blend_params: pointer to strucure needed to run sw blend API from LVGL
 * @param[out] color_format: pointer to LVGL color format
 *
 * @retval ESP_OK: Color format obtained successfully
 * @retval ESP_INVALID_STATE: blend stuctures have not been initialized
 */
esp_err_t get_color_format(blend_params_t *blend_params, lv_color_format_t *color_format);

/**
 * @brief Set LVGL blend API function type
 *
 * @note init_blend_params() must be called before to init the structures
 *
 * @param[out] blend_params: pointer to strucure needed to run sw blend API from LVGL
 * @param[out] api_function: LVGL blend API function type
 *
 * @retval ESP_OK: LVGL blend API function type set successfully
 * @retval ESP_INVALID_STATE: blend stuctures have not been initialized
 */
esp_err_t set_api_function_type(blend_params_t *blend_params, blend_api_func_t api_function);

/**
 * @brief Get LVGL blend API function type
 *
 * @note init_blend_params() must be called before to init the structures
 *
 * @param[out] blend_params: pointer to strucure needed to run sw blend API from LVGL
 * @param[out] color_format: pointer to LVGL blend API function type
 *
 * @retval ESP_OK: LVGL blend API function type obtained successfully
 * @retval ESP_INVALID_STATE: blend stuctures have not been initialized
 */
esp_err_t get_api_function_type(blend_params_t *blend_params, blend_api_func_t *api_function);

/**
 * @brief Init parameters for blending LVGL functions
 *
 * @note Function is called automatically from setUp()
 *
 * @retval ESP_OK: blend params initialized successfully
 * @retval ESP_INVALID_STATE: blend stuctures have already been initialized
 * @retval ESP_ERR_NO_MEM: Insufficient memory
 */
esp_err_t init_blend_params(void);

/**
 * @brief Free parameters for blending LVGL functions
 *
 * @note Function is called automatically from tearDown()
 *
 * @retval ESP_OK: freed successfully
 */
esp_err_t free_blend_params(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif
