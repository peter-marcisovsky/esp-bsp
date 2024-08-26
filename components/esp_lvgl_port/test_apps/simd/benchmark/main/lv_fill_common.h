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
    lv_draw_unit_t draw_unit;               // Draw Unit for LV Blend API
    lv_draw_sw_blend_dsc_t blend_dsc;       // Common blend descriptor
} blend_params_t;

/**
 * @brief Type of blend DUT function
 */
typedef enum {
    OPERATION_FILL,
    OPERATION_FILL_WITH_OPA,
} blend_operation_t;

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
    void *dest_array;                       // destination array for testing most ideal case
    void *dest_array_cc;                    // destination array for testing wort case
    bool dynamic_bg_opa;                    // Use either static or dynamic background OPA
    blend_operation_t operation_type;       // LVGL operation type
} bench_test_params_t;

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
 * @brief Set foreground opacity
 *
 * @note init_blend_params() must be called before to init the structures
 *
 * @param[out] blend_params: pointer to strucure needed to run sw blend API from LVGL
 * @param[out] opa: opacity
 *
 * @retval ESP_OK: opacity set successfully
 * @retval ESP_INVALID_STATE: blend stuctures have not been initialized
 */
esp_err_t set_opacity(blend_params_t *blend_params, lv_opa_t opa);


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
