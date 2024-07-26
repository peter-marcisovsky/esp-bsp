/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "lv_fill_common.h"
#include "lvgl.h"
#include "esp_lvgl_port_lv_blend.h"

#define LV_FILL_CHECK(cond, ret_val) ({                                     \
            if (!(cond)) {                                                  \
                return (ret_val);                                           \
            }                                                               \
})

// Extern variable from esp_lvgl_port_lv_blend.h to control switching between Assembly or C implementation
bool LV_BLEND_USE_ASM;

static blend_params_t *s_blend_params = NULL;
static test_area_t *s_area = NULL;

static lv_color_t test_color_common = {
    .blue = 0x56,
    .green = 0x34,
    .red = 0x12,
};

esp_err_t get_blend_params(blend_params_t **blend_params_ret, test_area_t **area_ret)
{
    LV_FILL_CHECK((s_area != NULL || s_blend_params != NULL), ESP_ERR_INVALID_STATE);

    *blend_params_ret = s_blend_params;
    *area_ret = s_area;

    return ESP_OK;
}

esp_err_t set_color_format(blend_params_t *blend_params, lv_color_format_t color_format)
{
    LV_FILL_CHECK(blend_params != NULL, ESP_ERR_INVALID_STATE);

    blend_params->draw_unit_ansi.target_layer->color_format = color_format;
    blend_params->draw_unit_asm.target_layer->color_format = color_format;

    return ESP_OK;
}

esp_err_t get_color_format(blend_params_t *blend_params, lv_color_format_t *color_format)
{
    LV_FILL_CHECK(blend_params != NULL, ESP_ERR_INVALID_STATE);

    *color_format = blend_params->draw_unit_ansi.target_layer->color_format;

    return ESP_OK;
}

esp_err_t set_api_function_type(blend_params_t *blend_params, blend_api_func_t api_function)
{
    LV_FILL_CHECK(blend_params != NULL, ESP_ERR_INVALID_STATE);

    blend_params->api_function = api_function;

    return ESP_OK;
}

esp_err_t get_api_function_type(blend_params_t *blend_params, blend_api_func_t *api_function)
{
    LV_FILL_CHECK(blend_params != NULL, ESP_ERR_INVALID_STATE);

    *api_function =  blend_params->api_function;

    return ESP_OK;
}

esp_err_t init_blend_params(void)
{
    LV_FILL_CHECK((s_area == NULL || s_blend_params == NULL), ESP_ERR_INVALID_STATE);

    // Allocate space for test_area_t
    test_area_t *test_area = (test_area_t *)malloc(sizeof(test_area_t));

    // Allocate space for lv_draw_buf_t
    lv_draw_buf_t *draw_buf_ansi = (lv_draw_buf_t *)malloc(sizeof(lv_draw_buf_t));
    lv_draw_buf_t *draw_buf_asm = (lv_draw_buf_t *)malloc(sizeof(lv_draw_buf_t));

    // Allocate space for lv_layer_t
    lv_layer_t *target_layer_ansi = (lv_layer_t *)malloc(sizeof(lv_layer_t));
    lv_layer_t *target_layer_asm = (lv_layer_t *)malloc(sizeof(lv_layer_t));

    // Allocate space for blend_params_t
    blend_params_t *blend_params = (blend_params_t *)malloc(sizeof(blend_params_t));

    if (test_area == NULL ||
            draw_buf_ansi == NULL ||
            draw_buf_asm == NULL ||
            target_layer_ansi == NULL ||
            target_layer_asm == NULL ||
            blend_params == NULL) {
        return ESP_ERR_NO_MEM;
    }

    lv_draw_sw_blend_dsc_t blend_dsc = {
        .blend_area = &test_area->blend,
        .src_buf = NULL,
        .opa = LV_OPA_MAX,
        .color = test_color_common,
        .mask_buf = NULL,
        .mask_res = LV_DRAW_SW_MASK_RES_FULL_COVER,
        .mask_area = NULL,
    };

    // Initialize draw buf data with NULL
    lv_draw_buf_t draw_buf = {
        .data = NULL,
    };

    // Fill allocated space with initialized struct
    memcpy(draw_buf_ansi, &draw_buf, sizeof(lv_draw_buf_t));
    memcpy(draw_buf_asm, &draw_buf, sizeof(lv_draw_buf_t));

    // Initialize targe layer
    lv_layer_t target_layer = {
        //.color_format     will be set with setter function
        //.buf_area         not being pointer, can't be updated now and will have to be updated for each test case
        //.draw_buf         draw_buf_ansi, vs draw_buf_asm is set below
    };

    // Fill allocated space with initialized struct
    memcpy(target_layer_ansi, &target_layer, sizeof(lv_layer_t));
    memcpy(target_layer_asm, &target_layer, sizeof(lv_layer_t));

    // Initialize draw buf with member with draws buf for ANSI and ASM
    target_layer_ansi->draw_buf = draw_buf_ansi;
    target_layer_asm->draw_buf = draw_buf_asm;

    // Initialize draw unit
    lv_draw_unit_t draw_unit_ansi = {
        .target_layer = target_layer_ansi,
        .clip_area = &test_area->clip,
    };

    lv_draw_unit_t draw_unit_asm = {
        .target_layer = target_layer_asm,
        .clip_area = &test_area->clip,
    };

    // Initialize blend params struct
    blend_params_t blend_params_local = {
        .blend_dsc = blend_dsc,
        .draw_unit_ansi = draw_unit_ansi,
        .draw_unit_asm = draw_unit_asm,
        .use_asm = &LV_BLEND_USE_ASM,
        .api_function = LVGL_API_NOT_SET,
    };

    memcpy(blend_params, &blend_params_local, sizeof(blend_params_t));
    s_blend_params = blend_params;
    s_area = test_area;

    return ESP_OK;
}

esp_err_t free_blend_params(void)
{
    // Free area
    if (s_area != NULL) {
        free(s_area);
    }

    // Free blend params
    if (s_blend_params != NULL) {
        // Check target layer ansi
        if (s_blend_params->draw_unit_ansi.target_layer != NULL) {
            // Check draw buf from target layer ansi
            if (s_blend_params->draw_unit_ansi.target_layer->draw_buf != NULL) {
                free(s_blend_params->draw_unit_ansi.target_layer->draw_buf);
            }
            free(s_blend_params->draw_unit_ansi.target_layer);
        }

        // Check target layer asm
        if (s_blend_params->draw_unit_asm.target_layer != NULL) {
            // Check draw buf from target layer asm
            if (s_blend_params->draw_unit_asm.target_layer->draw_buf != NULL) {
                free(s_blend_params->draw_unit_asm.target_layer->draw_buf);
            }
            free(s_blend_params->draw_unit_asm.target_layer);
        }
        free(s_blend_params);
    }

    s_area = NULL;
    s_blend_params = NULL;

    return ESP_OK;
}
