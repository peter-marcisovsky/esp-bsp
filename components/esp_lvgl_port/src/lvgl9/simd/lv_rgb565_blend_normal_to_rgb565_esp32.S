/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL RGB565 simple fill for ESP32 processor

    .section .text
    .align  4
    .global lv_rgb565_blend_normal_to_rgb565_esp
    .type   lv_rgb565_blend_normal_to_rgb565_esp,@function
// The function implements the following C code:
// void lv_color_blend_to_rgb565(_lv_draw_sw_blend_fill_dsc_t * dsc);

// Input params
//
// dsc - a2

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

lv_rgb565_blend_normal_to_rgb565_esp:

    entry   a1,    64

    l32i.n   a3,    a2,    4                    // a3 - dest_buff
    l32i.n   a4,    a2,    8                    // a4 - dest_w                in uint16_t
    l32i.n   a5,    a2,    12                   // a5 - dest_h                in uint16_t
    l32i.n   a6,    a2,    16                   // a6 - dest_stride           in bytes
    l32i.n   a7,    a2,    20                   // a7 - src_buff
    l32i.n   a8,    a2,    24                   // a8 - src_stride
    slli     a11,   a4,    1                    // a11 - dest_w_bytes = sizeof(uint16_t) * dest_w

    movi.n  a12,    0x3                          // a12 = 0x3, dest_buff align mask
    sub     a6,    a6,   a11                    // dest_stride = dest_stride - dest_w_bytes
    sub     a8,    a8,   a11                    // src_strid = src_strid - dest_w_bytes

//srli     a9,     a4,    4
//slli     a11,    a4,    1

    .outer_loop:
        // Check if the source buffer is 4 byte aligned
        and     a15,   a7,   a12                    // a15 = src_buff (a7) AND 3
        slli    a11,   a4,    1
        //and     a14,   a3,   a12

        beqz    a15,   _src_buff_4_byte_aligned
            loopnez a15,  ._src_buff_aligning_loop
                l8ui   a10,   a7,  0
                //addi   a11,   a11, -1
                s8i    a10,   a3,  0
                addi.n  a7,   a7,  1
                addi.n  a3,   a3,  1
            ._src_buff_aligning_loop:
        _src_buff_4_byte_aligned:

        sub  a11,   a11,  a15
        and  a14,   a3,   a12
        srli a9,    a11,    5  // TODO CHECK THIS

        bnez   a14,   _dest_buff_NOT_4_byte_aligned

            //srli     a9,     a4,    4
            //slli     a11,    a4,    1

            // Run main loop which sets 32 bytes in one loop run
            loopnez a9, ._main_loop
                l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
                l32i.n      a13,  a7,  4                    // load 32 bits from src_buff a7 to a13 offset 4
                s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0
                s32i.n      a13,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 4

                l32i.n      a10,  a7,  8                    // load 32 bits from src_buff a7 to a10 offset 8
                l32i.n      a13,  a7,  12                   // load 32 bits from src_buff a7 to a13 offset 12
                s32i.n      a10,  a3,  8                    // save 32 bits from a10 to dest_buff a3, offset 8
                s32i.n      a13,  a3,  12                   // save 32 bits from a10 to dest_buff a3, offset 12

                l32i.n      a10,  a7,  16                   // load 32 bits from src_buff a7 to a10 offset 16
                l32i.n      a13,  a7,  20                   // load 32 bits from src_buff a7 to a13 offset 20
                s32i.n      a10,  a3,  16                   // save 32 bits from a10 to dest_buff a3, offset 16
                s32i.n      a13,  a3,  20                   // save 32 bits from a10 to dest_buff a3, offset 20

                l32i.n      a10,  a7,  24                   // load 32 bits from src_buff a7 to a10 offset 24
                l32i.n      a13,  a7,  28                   // load 32 bits from src_buff a7 to a13 offset 28
                s32i.n      a10,  a3,  24                   // save 32 bits from a10 to dest_buff a3, offset 24
                s32i.n      a13,  a3,  28                   // save 32 bits from a10 to dest_buff a3, offset 28

                addi.n      a7,   a7,  32                   // increment src_buff a7 pointer by 32
                addi.n      a3,   a3,  32                   // increment dest_buff a3 pointer by 32
            ._main_loop:

            // Finish the remaining bytes out of the loop
            // Check modulo 8 of the dest_w_bytes, if - then set 16 bytes
            bbci a11, 4, _mod_16_check                      // branch if 2-nd bit of dest_w_bytes is clear
                l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
                l32i.n      a13,  a7,  4                    // load 32 bits from src_buff a7 to a13 offset 4
                s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0
                s32i.n      a13,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 4

                l32i.n      a10,  a7,  8                    // load 32 bits from src_buff a7 to a10 offset 8
                l32i.n      a13,  a7,  12                   // load 32 bits from src_buff a7 to a13 offset 12
                s32i.n      a10,  a3,  8                    // save 32 bits from a10 to dest_buff a3, offset 8
                s32i.n      a13,  a3,  12                   // save 32 bits from a10 to dest_buff a3, offset 12

                addi.n      a7,   a7,  16                   // increment src_buff a7 pointer by 16
                addi.n      a3,   a3,  16                   // increment dest_buff a3 pointer by 16
            _mod_16_check:

            // Finish the remaining bytes out of the loop
            // Check modulo 8 of the dest_w_bytes, if - then set 8 bytes
            bbci a11, 3, _mod_8_check                       // branch if 2-nd bit of dest_w_bytes is clear
                l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
                l32i.n      a13,  a7,  4                    // load 32 bits from src_buff a7 to a13 offset 4
                s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0
                s32i.n      a13,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 4

                addi.n      a7,   a7,  8                    // increment src_buff a7 pointer by 8
                addi.n      a3,   a3,  8                    // increment dest_buff a3 pointer by 8
            _mod_8_check:

            // Check modulo 4 of the dest_w_bytes, if - then set 4 bytes
            bbci a11, 2, _mod_4_check                       // branch if 2-nd bit of dest_w_bytes is clear
                l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
                s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0

                addi.n      a7,   a7,  4                    // increment src_buff a7 pointer by 4
                addi.n      a3,   a3,  4                    // increment dest_buff a3 pointer by 4
            _mod_4_check:

            // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
            bbci a11, 1, _mod_2_check                       // branch if 1-st bit of dest_w_bytes is clear
                l16ui      a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
                s16i       a10,  a3,  0                     // save 16 bits from a10 to dest_buff a3, offset 0

                addi.n      a7,   a7,  2                    // increment src_buff a7 pointer by 2
                addi.n      a3,   a3,  2                    // increment dest_buff a3 pointer by 2
            _mod_2_check:

            // TOD ADD mod 1 check
            // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
            bbci a11, 0, _mod_1_check                       // branch if 1-st bit of dest_w_bytes is clear
                l8ui      a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
                s8i       a10,  a3,  0                     // save 16 bits from a10 to dest_buff a3, offset 0

                addi.n      a7,   a7,  1                    // increment src_buff a7 pointer by 2
                addi.n      a3,   a3,  1                    // increment dest_buff a3 pointer by 2
            _mod_1_check:

            j _end_of_loop

        _dest_buff_NOT_4_byte_aligned:

        srli       a9,     a11,   3
        bbsi  a3,  0, _dest_buff_1_byte_aligned
            // Destination buffer is 2 byte aligned

            //srli     a9,     a4,    1
            //slli     a11,    a4,    1

            loopnez a9, ._main_loop_2_byte_aligned
                l32i.n     a10,  a7,  0
                l32i.n     a13,  a7,  4

                s16i       a10,  a3,  0
                s16i       a13,  a3,  4
                srli       a10,  a10, 16
                srli       a13,  a13, 16
                s16i       a10,  a3,  2
                s16i       a13,  a3,  6

                addi.n     a7,   a7,  8
                addi.n     a3,   a3,  8
            ._main_loop_2_byte_aligned:

            // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
            bbci a11, 2, _mod_4_check_2_byte_aligned        // branch if 1-st bit of dest_w_bytes is clear
                l32i.n     a10,  a7,  0

                s16i       a10,  a3,  0
                srli       a10,  a10, 16
                s16i       a10,  a3,  2
  
                addi.n     a7,   a7,  4
                addi.n     a3,   a3,  4
            _mod_4_check_2_byte_aligned:

            // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
            bbci a11, 1, _mod_2_check_2_byte_aligned        // branch if 1-st bit of dest_w_bytes is clear
                l16ui      a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
                s16i       a10,  a3,  0

                addi.n      a7,   a7,  2                    // increment src_buff a7 pointer by 2
                addi.n      a3,   a3,  2                    // increment dest_buff a3 pointer by 2
            _mod_2_check_2_byte_aligned:

            bbci a11, 0, _mod_1_check_2_byte_aligned        // branch if 1-st bit of dest_w_bytes is clear
                l8ui       a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
                s8i        a10,  a3,  0                     // save 16 bits from a10 to dest_buff a3, offset 0

                addi.n      a7,   a7,  1                    // increment src_buff a7 pointer by 2
                addi.n      a3,   a3,  1                    // increment dest_buff a3 pointer by 2
            _mod_1_check_2_byte_aligned:

            j _end_of_loop

        _dest_buff_1_byte_aligned:
            // Destination buffer is 1 byte aligned

            //srli     a9,     a4,    1
            //slli     a11,    a4,    1
            //srli       a9,     a11,   2

            loopnez a9, ._main_loop_1_byte_aligned
//                l32i.n     a10,  a7,  0
//
//                s8i        a10,  a3,  0
//                addi.n      a3,  a3,  1
//                srli       a10,  a10, 8
//                s16i       a10,  a3,  0
//                srli       a10,  a10, 16
//                s8i        a10,  a3,  2
//
//                addi.n     a3,   a3,  3
//                addi.n     a7,   a7,  4

                l32i.n     a10,  a7,  0
                l32i.n     a13,  a7,  4

                s8i        a10,  a3,  0
                s8i        a13,  a3,  4
                addi.n      a3,  a3,  1
                srli       a10,  a10, 8
                srli       a13,  a13, 8
                s16i       a10,  a3,  0
                s16i       a13,  a3,  4
                srli       a10,  a10, 16
                srli       a13,  a13, 16
                s8i        a10,  a3,  2
                s8i        a13,  a3,  6

                addi.n     a7,   a7,  8
                addi.n     a3,   a3,  7
            ._main_loop_1_byte_aligned:
            
            // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
            bbci a11, 2, _mod_4_check_1_byte_aligned        // branch if 1-st bit of dest_w_bytes is clear
                l32i.n     a10,  a7,  0

                s8i        a10,  a3,  0
                addi.n      a3,  a3,  1
                srli       a10,  a10, 8
                s16i       a10,  a3,  0
                srli       a10,  a10, 16
                s8i        a10,  a3,  2

                addi.n     a7,   a7,  4
                addi.n     a3,   a3,  3
            _mod_4_check_1_byte_aligned:

            // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
            bbci a11, 1, _mod_2_check_1_byte_aligned        // branch if 1-st bit of dest_w_bytes is clear
                l16ui      a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
                s8i        a10,  a3,  0                     // save 16 bits from a10 to dest_buff a3, offset 0
                srli       a10,  a10, 8
                s8i        a10,  a3,  1                     // save 16 bits from a10 to dest_buff a3, offset 0

                addi.n      a7,   a7,  2                    // increment src_buff a7 pointer by 2
                addi.n      a3,   a3,  2                    // increment dest_buff a3 pointer by 2
            _mod_2_check_1_byte_aligned:

            bbci a11, 0, _mod_1_check_1_byte_aligned        // branch if 1-st bit of dest_w_bytes is clear
                l8ui       a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
                s8i        a10,  a3,  0                     // save 16 bits from a10 to dest_buff a3, offset 0

                addi.n      a7,   a7,  1                    // increment src_buff a7 pointer by 2
                addi.n      a3,   a3,  1                    // increment dest_buff a3 pointer by 2
            _mod_1_check_1_byte_aligned:


//        // Check if the des_buff is 2-byte aligned
//        beqz  a15, _dest_buff_2_byte_aligned      // branch if a15 is equal to zero
//            l16ui      a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
//            s16i       a10,  a3,  0                     // save 16 bits from a10 to dest_buff a3, offset 0
//
//            addi.n      a3,   a3,  2                    // increment dest_buff a3 pointer by 2
//            addi.n      a7,   a7,  2                    // increment src_buff a7 pointer by 2
//
//            s16i        a12,  a3,   0            // save 16 bits from 16-bit color a12 to dest_buff a3, offset 0
//            l32i.n      a9,   a1,   8            // a9  = load cache.decr.loop_len
//            l32i.n      a11,  a1,   12           // a11 = load cache.decr.dest_w_bytes
//            addi.n      a3,   a3,   2            // increment dest_buff pointer by 2
//            j           _dest_buff_unaligned
//        _dest_buff_2_byte_aligned:
//
//        l32i.n      a9,   a1,   0                // a11 = load cache.orig.loop_len
//        l32i.n      a11,  a1,   4                // a11 = load cache.orig.dest_w_bytes
//
//        _dest_buff_unaligned:
//
//        // Run main loop which sets 16 bytes in one loop run
//        loopnez a9, ._main_loop
//            l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
//            l32i.n      a13,  a7,  4                    // load 32 bits from src_buff a7 to a13 offset 4
//            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0
//            s32i.n      a13,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 4
//
//            l32i.n      a10,  a7,  8                    // load 32 bits from src_buff a7 to a10 offset 8
//            l32i.n      a13,  a7,  12                   // load 32 bits from src_buff a7 to a13 offset 12
//            s32i.n      a10,  a3,  8                    // save 32 bits from a10 to dest_buff a3, offset 8
//            s32i.n      a13,  a3,  12                   // save 32 bits from a10 to dest_buff a3, offset 12
//
//            l32i.n      a10,  a7,  16                   // load 32 bits from src_buff a7 to a10 offset 16
//            l32i.n      a13,  a7,  20                   // load 32 bits from src_buff a7 to a13 offset 20
//            s32i.n      a10,  a3,  16                   // save 32 bits from a10 to dest_buff a3, offset 16
//            s32i.n      a13,  a3,  20                   // save 32 bits from a10 to dest_buff a3, offset 20
//
//            l32i.n      a10,  a7,  24                   // load 32 bits from src_buff a7 to a10 offset 24
//            l32i.n      a13,  a7,  28                   // load 32 bits from src_buff a7 to a13 offset 28
//            s32i.n      a10,  a3,  24                   // save 32 bits from a10 to dest_buff a3, offset 24
//            s32i.n      a13,  a3,  28                   // save 32 bits from a10 to dest_buff a3, offset 28
//
//            addi.n      a3,   a3,  32                   // increment dest_buff a3 pointer by 32
//            addi.n      a7,   a7,  32                   // increment src_buff a7 pointer by 32
//        ._main_loop:
//
//        // Finish the remaining bytes out of the loop
//        // Check modulo 8 of the dest_w_bytes, if - then set 16 bytes
//        bbci a11, 4, _mod_16_check                      // branch if 2-nd bit of dest_w_bytes is clear
//            l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
//            l32i.n      a13,  a7,  4                    // load 32 bits from src_buff a7 to a13 offset 4
//            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0
//            s32i.n      a13,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 4
//
//            l32i.n      a10,  a7,  8                    // load 32 bits from src_buff a7 to a10 offset 8
//            l32i.n      a13,  a7,  12                   // load 32 bits from src_buff a7 to a13 offset 12
//            s32i.n      a10,  a3,  8                    // save 32 bits from a10 to dest_buff a3, offset 8
//            s32i.n      a13,  a3,  12                   // save 32 bits from a10 to dest_buff a3, offset 12
//
//            addi.n      a3,   a3,  16                   // increment dest_buff a3 pointer by 16
//            addi.n      a7,   a7,  16                   // increment src_buff a7 pointer by 16
//        _mod_16_check:
//
//        // Finish the remaining bytes out of the loop
//        // Check modulo 8 of the dest_w_bytes, if - then set 8 bytes
//        bbci a11, 3, _mod_8_check                       // branch if 2-nd bit of dest_w_bytes is clear
//            l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
//            l32i.n      a13,  a7,  4                    // load 32 bits from src_buff a7 to a13 offset 4
//            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0
//            s32i.n      a13,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 4
//
//            addi.n      a3,   a3,  8                    // increment dest_buff a3 pointer by 8
//            addi.n      a7,   a7,  8                    // increment src_buff a7 pointer by 8
//        _mod_8_check:
//
//        // Check modulo 4 of the dest_w_bytes, if - then set 4 bytes
//        bbci a11, 2, _mod_4_check                       // branch if 2-nd bit of dest_w_bytes is clear
//            l32i.n      a10,  a7,  0                    // load 32 bits from src_buff a7 to a10 offset 0
//            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0
//
//            addi.n      a3,   a3,  4                    // increment dest_buff a3 pointer by 4
//            addi.n      a7,   a7,  4                    // increment src_buff a7 pointer by 4
//        _mod_4_check:
//
//        // Check modulo 2 of the dest_w_bytes, if - then set 2 bytes
//        bbci a11, 1, _mod_2_check                       // branch if 1-st bit of dest_w_bytes is clear
//            l16ui      a10,  a7,  0                     // load 16 bits from src_buff a7 to a10 offset 0
//            s16i       a10,  a3,  0                     // save 16 bits from a10 to dest_buff a3, offset 0
//
//            addi.n      a3,   a3,  2                    // increment dest_buff a3 pointer by 2
//            addi.n      a7,   a7,  2                    // increment src_buff a7 pointer by 2
//        _mod_2_check:

        _end_of_loop:

        addi.n  a5,  a5,  -1                            // decrease the outer loop
        add     a3,  a3,  a6                            // dest_buff + dest_stride
        add     a7,  a7,  a8                            // src_buff + src_stride
        //and     a15,  a12,  a3                           // a15 = dest_buff AND 0x3 (chck if the address is 4-byte aligned)
    bnez a5, .outer_loop

    movi.n   a2, 1                                      // return LV_RESULT_OK = 1
    retw.n                                              // return
