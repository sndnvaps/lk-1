/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
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
#ifndef __PLATFORM_MSM_SHARED_H
#define __PLATFORM_MSM_SHARED_H

#define REBOOT_MODE_NORMAL     0x77665501
#define REBOOT_MODE_RECOVERY   0x77665502
#define REBOOT_MODE_FASTBOOT   0x77665500

#if WITH_XIAOMI_DUALBOOT
#define REBOOT_MODE_BOOT0      0x6f656d10
#define REBOOT_MODE_BOOT1      0x6f656d11
#endif

#endif

