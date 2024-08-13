/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is derived from the LVGL project.
 * See https://github.com/lvgl/lvgl for details.
 */

/**
 * @file lv_color_op.h
 *
 */

#ifndef LV_COLOR_OP_H
#define LV_COLOR_OP_H

#ifdef __cplusplus
extern "C" {
#endif

#define DBG_PRINT_OUTPUT false

/*********************
 *      INCLUDES
 *********************/
#include "lv_math.h"
#include "lv_color.h"
#include "lv_types.h"
#include "stdio.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Mix two colors with a given ratio.
 * @param c1 the first color to mix (usually the foreground)
 * @param c2 the second color to mix (usually the background)
 * @param mix The ratio of the colors. 0: full `c2`, 255: full `c1`, 127: half `c1` and half`c2`
 * @return the mixed color
 */
static inline lv_color_t LV_ATTRIBUTE_FAST_MEM lv_color_mix(lv_color_t c1, lv_color_t c2, uint8_t mix)
{
    lv_color_t ret;

    ret.red = LV_UDIV255((uint16_t)c1.red * mix + c2.red * (255 - mix) + LV_COLOR_MIX_ROUND_OFS);
    ret.green = LV_UDIV255((uint16_t)c1.green * mix + c2.green * (255 - mix) + LV_COLOR_MIX_ROUND_OFS);
    ret.blue = LV_UDIV255((uint16_t)c1.blue * mix + c2.blue * (255 - mix) + LV_COLOR_MIX_ROUND_OFS);
    return ret;
}

/**
 *
 * @param fg
 * @param bg
 * @return
 * @note Use bg.alpha in the return value
 * @note Use fg.alpha as mix ratio
 */
static inline lv_color32_t lv_color_mix32(lv_color32_t fg, lv_color32_t bg)
{
    if (fg.alpha >= LV_OPA_MAX) {
#if DBG_PRINT_OUTPUT
        printf("fg.opa = %d\n", fg.alpha);
        printf("bg.opa = %d\n", bg.alpha);
        fg.alpha = bg.alpha;
        printf("lv_color_mix32_1\n");
#endif
        return fg;
    }
    if (fg.alpha <= LV_OPA_MIN) {
#if DBG_PRINT_OUTPUT
        printf("lv_color_mix32_2\n");
#endif
        return bg;
    }

#if DBG_PRINT_OUTPUT
    printf("lv_color_mix32_3\n");
    printf("FG A = %d\n", fg.alpha);
    printf("FG R = %d   G = %d   B = %d\n", fg.red, fg.green, fg.blue);
    printf("BG R = %d   G = %d   B = %d\n", bg.red, bg.green, bg.blue);
#endif

    bg.red = (uint32_t)((uint32_t)fg.red * fg.alpha + (uint32_t)bg.red * (255 - fg.alpha)) >> 8;
    bg.green = (uint32_t)((uint32_t)fg.green * fg.alpha + (uint32_t)bg.green * (255 - fg.alpha)) >> 8;
    bg.blue = (uint32_t)((uint32_t)fg.blue * fg.alpha + (uint32_t)bg.blue * (255 - fg.alpha)) >> 8;

#if DBG_PRINT_OUTPUT
    printf("BG R = %d   G = %d   B = %d\n", bg.red, bg.green, bg.blue);
#endif

    return bg;
}

/**********************
 *  PREDEFINED COLORS
 **********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_COLOR_H*/
