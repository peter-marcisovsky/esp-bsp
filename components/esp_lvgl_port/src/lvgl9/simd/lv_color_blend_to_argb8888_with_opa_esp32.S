/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL ARGB8888 simple fill for ESP32 processor

    .section .text
    .align  4
    .global lv_color_blend_to_argb8888_with_opa_esp
    .type   lv_color_blend_to_argb8888_with_opa_esp,@function

// The function implements the following C code:
// void lv_color_blend_to_argb8888_with_opa(_lv_draw_sw_blend_fill_dsc_t * dsc);

// Input params                     Variables
//
// dsc - a2                         BG OPA - a13
//                                  FG OPA - a9

// typedef struct {
//     uint32_t opa;                l32i    0
//     void * dst_buf;              l32i    4
//     uint32_t dst_w;              l32i    8
//     uint32_t dst_h;              l32i    12
//     uint32_t dst_stride;         l32i    16
//     const void * src_buf;        l32i    20
//     uint32_t src_stride;         l32i    24
//     const lv_opa_t * mask_buf;   l32i    28
//     uint32_t mask_stride;        l32i    32
// } asm_dsc_t;

lv_color_blend_to_argb8888_with_opa_esp:

    entry   a1,    64

    l8ui     a9,    a2,    0                    // a9 - FG opacity
    l32i.n   a3,    a2,    4                    // a3 - dest_buff
    l32i.n   a4,    a2,    8                    // a4 - dest_w                in uint32_t
    l32i.n   a5,    a2,    12                   // a5 - dest_h                in uint32_t
    l32i.n   a6,    a2,    16                   // a6 - dest_stride           in bytes
    l32i.n   a7,    a2,    20                   // a7 - src_buff (color)
    l32i.n   a8,    a7,    0                    // a8 - color as value
    slli     a11,   a4,    2                    // a11 - dest_w_bytes = sizeof(uint32_t) * dest_w

    slli     a15,   a9,    24                   // Shift opacity left in 24
    or       a10,   a15,   a8                   // apply oppacity 0xOORRGGBB
    sub      a6,    a6,    a11                  // dest_stride = dest_stride - dest_w_bytes

    // cache init
    movi.n   a12,    0
    s32i.n   a12,    a1,   0                    // cache.FG
    s32i.n   a12,    a1,   4                    // cache.BG
    s32i.n   a12,    a1,   8                    // cache.res
    movi     a12,    255
    s32i.n   a12,    a1,   12                   // cache.res_alpha
    s32i.n   a12,    a1,   16                   // cache.ratio

    .outer_loop:

        loopnez a4, ._main_loop

            l32i.n      a15,   a3,   0              // load BG value from dest buffer
            addi        a12,   a9,   3              // Add 3 to the FG oppacity be able to compare the the FG OPA with the immediate value 256
            extui       a13,   a15,  24,   8        // a13 = BG OPA, extract BG OPA from background

            // Branch if FG OPA + 3 (a12) lower than 256
            bltui       a12,  256,   _save_fg_if_fg_aplha                                   // if (fg.alpha >= LV_OPA_MAX) {
                s32i.n      a10,  a3,  0            // Save FG (a10) to the dest buff       //   return fg;
                j           _end_of_loop            // Jump to the end of the loop          // }
            _save_fg_if_fg_aplha:

            // Branch if BG OPA (a13) is greater than or equal to 3
            bgeui        a13,  3,  _save_fg_if_bg_alpha                                     // if (bg.alpha <= LV_OPA_MIN) {
                s32i.n      a10,  a3,  0            // Save FG (a10) to the dest buff       //   return fg;
                j          _end_of_loop             // Jump to the end of the loop          // }
            _save_fg_if_bg_alpha:

            // Branch if FG OPA (a9) is greater than or equal to 3
            bgeui        a9,  3, _save_bg_if_fg_aplha                                       // if (fg.alpha <= LV_OPA_MIN) {
                // We dont need to save bg, because we did not modify it                    //   return bg;
                j          _end_of_loop             // Jump to the end of the loop          // }
            _save_bg_if_fg_aplha:

            addi         a14,  a13,  1              // Add 1 to the BG OPA to be able to compare the value to 256
            // Branch if BG OPA + 1 (a14) not equal to 256
            bnei         a14,  256, _mix_bg_fg                                              // if (bg.alpha == 255) {
                                                                                            //   return lv_color_mix32(fg, bg);
                                                                                            // }

                // Branch if FG OPA + 3 (a12) lower than 256                                // if (fg.alpha >= LV_OPA_MAX) {
                bltui       a12,  256,   _save_fg_if_fg_aplha_in_mix                        //   fg.alpha = bg.alpha;
                    // FG OPA = BG OPA, return FG                                           //   return fg;
                    slli       a14,  a13,  24       // a14 = BG OPA << 24                   // }
                    and        a12,  a10,  a14      // FG.OPA = BG.OPA
                    s32i.n     a12,  a3,   0        // Save FG with BG OPA (a12) to the dest buff
                    j           _end_of_loop        // Jump to the end of the loop
                _save_fg_if_fg_aplha_in_mix:

                // Branch if FG OPA (a9) is greater than or equal to 3                      // if (fg.alpha <= LV_OPA_MIN) {
                bgeui        a9,  3, _save_bg_if_fg_aplha_in_mix                            //   return bg;
                    // We dont need to save bg, because we did not modify it                // }
                    j          _end_of_loop         // Jump to the end of the loop
                _save_bg_if_fg_aplha_in_mix:

                // Simple mix

                // Extract alpha and prepare constants
                movi     a14,  255
                sub      a8,   a14,   a9            // a8 = 255 - FG OPA

                // Mix Red channel
                extui    a14,   a10,   16,   8      // a14 = fg.red
                extui    a12,   a15,   16,   8      // a12 = bg.red
                mul16u   a14,   a14,   a9           // fg.red * fg.alpha
                mul16u   a12,   a12,   a8           // bg.red * (255 - fg.alpha)
                add      a14,   a14,   a12          // a14 = fg.red * fg.alpha + bg.red * (255 - fg.alpha)
                srli     a14,   a14,   8            // Normalize: a14 = a14 >> 8
                slli     a7,    a14,   16           // Shift red to correct place

                // Mix Green channel
                extui    a14,   a10,   8,   8       // a14 = fg.green
                extui    a12,   a15,   8,   8       // a12 = bg.green
                mul16u   a14,   a14,   a9           // fg.green * fg.alpha
                mul16u   a12,   a12,   a8           // bg.green * (255 - fg.alpha)
                add      a14,   a14,   a12          // a14 = fg.green * fg.alpha + bg.green * (255 - fg.alpha)
                srli     a14,   a14,   8            // Normalize: a14 = a14 >> 8
                slli     a14,   a14,   8            // Shift red to correct place
                or       a7,    a14,   a7           // Combine red and green to a7

                // Mix Blue channel
                extui    a14,   a10,   0,   8       // a14 = fg.blue
                extui    a12,   a15,   0,   8       // a12 = bg.blue
                mul16u   a14,   a14,   a9           // fg.blue * fg.alpha
                mul16u   a12,   a12,   a8           // bg.blue * (255 - fg.alpha)
                add      a14,   a14,   a12          // a14 = fg.blue * fg.alpha + bg.blue * (255 - fg.alpha)
                srli     a14,   a14,   8            // Normalize: a14 = a14 >> 8
                or       a7,    a14,   a7           // Combine all colors to a7

                // Compose the final color
                slli     a14,   a13,   24           // a14 = BG OPA << 24
                or       a7,    a14,   a7           // a7  = BG OPA << 24 OR RGB chanels
                s32i.n   a7,    a3,    0            // Save background to the dest buff
                j        _end_of_loop               // Jump to the end of the loop

        _mix_bg_fg:

        l8ui       a12,   a1,   7                   // a12 = load cache.BG
        // Branch if BG OPA (a13) and cache.BG OPA (a12) are not equal
        bne        a13,   a12,  _OPA_cache_not_equal
        l8ui       a7,    a1,   3                   // a7 = load cache.FG
        // Branch if FG OPA (a9) and cahge.FG OPA (a7) are not equal
        bne        a9,    a7,   _OPA_cache_not_equal
        j _OPA_cache_equal

            _OPA_cache_not_equal:
            movi     a14,   255                     // load 255 to a14
            sub	     a12,   a14,   a9               // a12 = 255 - FG OPA
            sub	     a11,   a14,   a13              // a11 = 255 - BG OPA
            mul16u   a11,   a11,   a12              // a11 = (255 - FG OPA) * (255 - BG OPA)
            srli     a11,   a11,   8                // a11 = a11 >> 8
            sub      a11,   a14,   a11              // a11 = 255 - a11
            s32i.n   a11,   a1,    12               // a11 = save to cache.res_alpha

            mul16u   a12,   a14,   a9               // a12 = 255 * FG OPA
            quou     a14,   a12,   a11              // a14 = a12 / a11
            s32i.n   a14,   a1,    16               // a14 = save to cache.ratio_saved

        _OPA_cache_equal:

        l32i.n    a11,   a1,   0                    // a11 = load cache.FG

        // branch if FG (a10) and cache.FG (a11) are not equal
        bne       a10,   a11,   _cache_not_equal
        l32i.n    a12,   a1,    4                   // a12 = load cache.BG
        // branch if BG (a15) and cache.BG (a12) are not equal
        bne       a15,   a12,   _cache_not_equal
        j _cahe_equal

            _cache_not_equal:
            l32i.n    a9,    a1,   16               // a9 = load from cache.ratio_saved (load FG OPA)
            s32i.n    a10,   a1,   0                // a10 = save to cache.FG
            addi      a12,   a9,   3                // Add 3 to the FG oppacity be able to compare the the FG OPA with the immediate value 256
            s32i.n    a15,   a1,   4                // a15 = save to cache.BG

            // Branch if FG OPA + 3 (a12) lower than 256                                    // if (fg.alpha >= LV_OPA_MAX) {
            bltui       a12,  256,   _save_fg_if_fg_aplha_in_mix_1                          //   fg.alpha = bg.alpha;
                // FG OPA = BG OPA, return FG                                               //   return fg
                l32i.n     a14,   a1,    12         // a14 = load cache.res_alpha           // }
                slli       a14,   a14,   24         // a14 = res alpha saved << 24
                movi       a12,   0x00ffffff        // a12 = Opa mask
                and        a12,   a10,   a12        // a12 = FG AND Opa mask
                or         a7,    a14,   a12        // a7  = FG.OPA = res_alpha saved

                s32i.n     a7,   a1,    8           // a7 = save to cache.res
                s32i.n     a7,   a3,    0           // Save FG (a7) to the dest buff
                l8ui       a9,    a2,    0          // a9 - load back the original FG opacity
                j          _end_of_loop             // Jump to the end of the loop
            _save_fg_if_fg_aplha_in_mix_1:

            // Branch if FG OPA (a9) is greater than or equal to 3                          // if (fg.alpha <= LV_OPA_MIN) {
            bgeui        a9,  3, _save_bg_if_fg_aplha_in_mix_1                              // return bg;
                l32i.n     a14,   a1,    12         // a14 = load cache.res_alpha           // }
                slli       a14,   a14,   24         // a14 = res alpha saved << 24
                movi       a12,   0x00ffffff        // a12 = Opa mask
                and        a12,   a15,   a12        // a12 = BG AND Opa mask
                or         a15,   a14,   a12        // BG.OPA = res_alpha saved

                s32i.n     a15,   a1,    8          // a15 = save to cache.res
                s32i.n     a15,   a3,    0          // Save background to the dest buff
                l8ui       a9,    a2,    0          // a9 - load back the original FG opacity
                j          _end_of_loop             // Jump to the end of the loop
            _save_bg_if_fg_aplha_in_mix_1:

            // Simple mix

            // Extract alpha and prepare constants
            movi     a14,   255
            l32i.n   a13,   a1,    12               // a13 = load cahe.res_alpha (load BG OPA)
            sub      a8,    a14,   a9               // a8 = 255 - FG OPA

            // Mix Red channel
            extui    a14,   a10,   16,   8          // a14 = fg.red
            extui    a12,   a15,   16,   8          // a12 = bg.red
            mul16u   a14,   a14,   a9               // fg.red * fg.alpha
            mul16u   a12,   a12,   a8               // bg.red * (255 - fg.alpha)
            add      a14,   a14,   a12              // a14 = fg.red * fg.alpha + bg.red * (255 - fg.alpha)
            srli     a14,   a14,   8                // Normalize: a14 = a14 >> 8
            slli     a7,    a14,   16               // Shift red to correct place

            // Mix Green channel
            extui    a14,   a10,   8,   8           // a14 = fg.green
            extui    a12,   a15,   8,   8           // a12 = bg.green
            mul16u   a14,   a14,   a9               // fg.green * fg.alpha
            mul16u   a12,   a12,   a8               // bg.green * (255 - fg.alpha)
            add      a14,   a14,   a12              // a14 = fg.green * fg.alpha + bg.green * (255 - fg.alpha)
            srli     a14,   a14,   8                // Normalize: a14 = a14 >> 8
            slli     a14,   a14,   8                // Shift red to correct place
            or       a7,    a14,   a7               // Combine red and green to a7

            // Mix Blue channel
            extui    a14,   a10,   0,   8           // a14 = fg.blue
            extui    a12,   a15,   0,   8           // a12 = bg.blue
            mul16u   a14,   a14,   a9               // fg.blue * fg.alpha
            mul16u   a12,   a12,   a8               // bg.blue * (255 - fg.alpha)
            add      a14,   a14,   a12              // a14 = fg.blue * fg.alpha + bg.blue * (255 - fg.alpha)
            srli     a14,   a14,   8                // Normalize: a14 = a14 >> 8
            or       a7,    a14,   a7               // Combine all colors to a7

            l32i.n   a14,   a1,    12               // a14 = load cache.res_alpha
            slli     a14,   a14,   24               // a14 = res alpha saved << 24

            // Compose the final color
            or       a7,    a14,   a7               // a7  = BG OPA << 24 OR RGB chanels
            s32i.n   a7,    a3,    0                // Save background to the dest buff
            l8ui     a9,    a2,    0                // a9 - load back the original FG opacity
            s32i.n   a7,    a1,    8                // a7 = save to cache.res
            j        _end_of_loop                   // Jump to the end of the loop

        _cahe_equal:

        l32i.n    a14,   a1,   8                    // a14 = load cache.res
        s32i.n    a14,   a3,   0                    // Save res Saved to the dest buff

        _end_of_loop:

        addi.n      a3,   a3,  4                    // Increment the pointer

        ._main_loop:

        add     a3,  a3,  a6                        // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                        // decrease the outer loop

    bnez a5, .outer_loop

    movi.n   a2, 1                                  // return LV_RESULT_OK = 1
    retw.n                                          // return

