/*
 * Copyright (c) 2020 Graham Sanderson
 *
 * WJB  9/ 4/21 Additional commenting
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
 
#include "hardware/regs/sio.h"
#include "hardware/regs/addressmap.h"

.cpu cortex-m0plus
.thumb

.section .time_critical.pal16

#define INTERP_OFFSET0(x) (x - SIO_INTERP0_ACCUM0_OFFSET)
#define INTERP_OFFSET1(x) (INTERP_OFFSET0(x) + SIO_INTERP1_ACCUM0_OFFSET - SIO_INTERP0_ACCUM0_OFFSET)

// setup: note palette is 256 entries of 32bits; i.e. all 2 pixel combinations
//
// interp_config c = interp_default_config();
// interp_config_set_shift(&c, 22);
// interp_config_set_mask(&c, 2, 9);
// interp_set_config(interp0, 0, &c);
// interp_config_set_shift(&c, 14);
// interp_config_set_cross_input(&c, true);
// interp_set_config(interp0, 1, &c);
// interp_set_base(interp0, 0, (uintptr_t)palette256);
// interp_set_base(interp0, 1, (uintptr_t)palette256);

// dest - pixel buffer 16bpp
// src - source pixel buffer 4bpp (must be 32 bit aligned)
// count - is number of pixels to convert / 2
// extern void convert_from_pal16(uint32_t *dest, uint8_t *src, uint count);
    // r0 = &dest
    // r1 = &src
    // r2 = count

.global convert_from_pal16
.type convert_from_pal16,%function
.thumb_func
convert_from_pal16:
    push {r4-r7, lr}                                                // Save registers
    add r2, r1                                                      // r2 = &src + count
    ldr r7, =(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET)                 // Address of SIO_INTERP0 registers

1:
    ldmia r1!, {r3}                                                 // r3 = *(src++)
    str r3, [r7, #INTERP_OFFSET0(SIO_INTERP0_ACCUM0_OFFSET)]        // SIO_INTERP0_ACCUM0 = r3
    ldr r6, [r7, #INTERP_OFFSET0(SIO_INTERP0_PEEK_LANE0_OFFSET)]    // r6 = SIO_INTERP0_PEEK_LANE0
    ldr r5, [r7, #INTERP_OFFSET0(SIO_INTERP0_PEEK_LANE1_OFFSET)]    // r5 = SIO_INTERP0_PEEK_LANE1
    ldr r6, [r6]                                                    // r6 = *(r6) = palette256[r3>>24]
    ldr r5, [r5]                                                    // r5 = *(r5) = palette256[(r3>>16)&0xFF]
    lsl r3, #16                                                     // r3 <<= 16
    str r3, [r7, #INTERP_OFFSET0(SIO_INTERP0_ACCUM0_OFFSET)]        // SIO_INTERP0_ACCUM0 = r3
    ldr r4, [r7, #INTERP_OFFSET0(SIO_INTERP0_PEEK_LANE0_OFFSET)]    // r4 = SIO_INTERP0_PEEK_LANE0
    ldr r3, [r7, #INTERP_OFFSET0(SIO_INTERP0_PEEK_LANE1_OFFSET)]    // r3 = SIO_INTERP0_PEEK_LANE1
    ldr r4, [r4]                                                    // r4 = *(r4) = palette256[r3>>24]
    ldr r3, [r3]                                                    // r3 = *(r3) = palette256[(r3>>16)&0xFF]
    stmia r0!, {r3, r4, r5, r6}     // *(dest++) = r3, *(dest++) = r4, *(dest++) = r5, *(dest++) = r6
    cmp r1, r2                                                      // Compare (r1 - r2)
    blo 1b                                                          // Repeat if r1 < r2 (= &src[count])

    pop {r4-r7, pc}                                                 // Restore registers and exit
