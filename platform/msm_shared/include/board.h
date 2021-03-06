/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 * Copyright (c) 2011-2014, Xiaomi Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Fundation, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __BOARD_H
#define __BOARD_H

#include <smem.h>
#include <arch/arm/mmu.h>

#if DEVICE_TREE
#include <dev_tree.h>
#endif

#define LINUX_MACHTYPE_UNKNOWN 0
#define BOARD_SOC_VERSION2     0x20000
#define MAX_PMIC_DEVICES       SMEM_MAX_PMIC_DEVICES

struct board_pmic_data {
	uint32_t pmic_type;
	uint32_t pmic_version;
	uint32_t pmic_target;
};

/* 8960 */
#define LINUX_MACHTYPE_8960_SIM     3230
#define LINUX_MACHTYPE_8960_RUMI3   3231
#define LINUX_MACHTYPE_8960_CDP     3396
#define LINUX_MACHTYPE_8960_MTP     3397
#define LINUX_MACHTYPE_8960_FLUID   3398
#define LINUX_MACHTYPE_8960_APQ     3399
#define LINUX_MACHTYPE_8960_LIQUID  3535
#define LINUX_MACHTYPE_8960_MITWOA  4459

/* 8627 */
#define LINUX_MACHTYPE_8627_CDP     3861
#define LINUX_MACHTYPE_8627_MTP     3862

/* 8930 */
#define LINUX_MACHTYPE_8930_CDP     3727
#define LINUX_MACHTYPE_8930_MTP     3728
#define LINUX_MACHTYPE_8930_FLUID   3729

/* 8064 */
#define LINUX_MACHTYPE_8064_SIM     3572
#define LINUX_MACHTYPE_8064_RUMI3   3679
#define LINUX_MACHTYPE_8064_CDP     3948
#define LINUX_MACHTYPE_8064_MTP     3949
#define LINUX_MACHTYPE_8064_LIQUID  3951
#define LINUX_MACHTYPE_8064_MPQ_CDP 3993
#define LINUX_MACHTYPE_8064_MPQ_HRD 3994
#define LINUX_MACHTYPE_8064_MPQ_DTV 3995
#define LINUX_MACHTYPE_8064_HRD     3994
#define LINUX_MACHTYPE_8064_DTV     3995
#define LINUX_MACHTYPE_8064_MITWO   4180

struct board_data {
	uint32_t platform;
	uint32_t foundry_id;
	uint32_t platform_version;
	uint32_t platform_hw;
	uint32_t platform_subtype;
	uint32_t target;
	uint32_t baseband;
	struct board_pmic_data pmic_info[MAX_PMIC_DEVICES];
	uint32_t platform_hlos_subtype;
};

void board_init(void);
void target_detect(struct board_data *);
void target_baseband_detect(struct board_data *);
uint32_t board_platform_id(void);
uint32_t board_target_id(void);
uint32_t board_baseband(void);
uint32_t board_hardware_id(void);
uint8_t board_pmic_info(struct board_pmic_data *, uint8_t num_ent);
uint32_t board_soc_version(void);
uint32_t board_hardware_subtype(void);
uint32_t board_get_ddr_subtype(void);
uint32_t board_hlos_subtype(void);
uint32_t board_pmic_target(uint8_t num_ent);

/* DDR Subtype Macros
 * Determine the DDR Size on the device and define
 * the below subtype enums based on the DDR size.
 * These defines are matched with that of the DT
 * subtype
 */

#define MB             (1024 * 1024)
#define DDR_512MB      (512 * MB)

enum subtype_ddr {
       SUBTYPE_512MB = 1,
};

#if BOOT_2NDSTAGE
struct original_atags_info {
	char* cmdline;
	uint32_t platform_id;
	uint32_t variant_id;
	uint32_t soc_rev;
#if DEVICE_TREE
	struct original_fdt_property* chosen_props;
	uint32_t num_chosen_props;
#endif
};

// parsed atag info
struct original_atags_info* board_get_original_atags_info(void);
int board_has_original_atags_info(void);
void board_parse_original_atags(void);

// original tags
extern void* original_atags;
#endif

uint32_t board_foundry_id(void);
#endif
