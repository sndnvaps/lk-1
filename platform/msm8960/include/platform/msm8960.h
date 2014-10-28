/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __PLATFORM_MSM8960_H
#define __PLATFORM_MSM8960_H

#include <platform/iomap.h>
#include <platform/irqs.h>
#include <platform/clock.h>
#include <platform/gpio.h>

#define MAX_INT NR_IRQS

uint8_t platform_pmic_type(uint32_t pmic_type);
void apq8064_keypad_gpio_init(void);
void clock_config_mmc(uint32_t interface, uint32_t freq);
void clock_init_mmc(uint32_t interface);
void msm_clocks_init(void);
void mmss_clock_init(void);
void mmss_clock_disable(void);

#if TARGET_MSM8960_ARIES
void mi_display_gpio_init(void);
#endif

#endif

