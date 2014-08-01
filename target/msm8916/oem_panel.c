/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <err.h>
#include <smem.h>
#include <msm_panel.h>
#include <board.h>
#include <mipi_dsi.h>
#include <target/display.h>

#include "include/panel.h"
#include "panel_display.h"

/*---------------------------------------------------------------------------*/
/* GCDB Panel Database                                                       */
/*---------------------------------------------------------------------------*/
#include "include/panel_jdi_1080p_video.h"
#include "include/panel_nt35590_720p_video.h"
#include "include/panel_nt35590_720p_cmd.h"
#include "include/panel_innolux_720p_video.h"
#include "include/panel_otm8019a_fwvga_video.h"
#include "include/panel_otm1283a_720p_video.h"
#include "include/panel_nt35596_1080p_skuk_video.h"

#define DISPLAY_MAX_PANEL_DETECTION 2
#define OTM8019A_FWVGA_VIDEO_PANEL_ON_DELAY 50

/*---------------------------------------------------------------------------*/
/* static panel selection variable                                           */
/*---------------------------------------------------------------------------*/
static uint32_t auto_pan_loop = 0;

enum {
JDI_1080P_VIDEO_PANEL,
NT35590_720P_VIDEO_PANEL,
NT35590_720P_CMD_PANEL,
INNOLUX_720P_VIDEO_PANEL,
OTM8019A_FWVGA_VIDEO_PANEL,
OTM1283A_720P_VIDEO_PANEL,
NT35596_1080P_VIDEO_PANEL,
UNKNOWN_PANEL
};

/*
 * The list of panels that are supported on this target.
 * Any panel in this list can be selected using fastboot oem command.
 */
static struct panel_list supp_panels[] = {
	{"jdi_1080p_video", JDI_1080P_VIDEO_PANEL},
	{"nt35590_720p_video", NT35590_720P_VIDEO_PANEL},
	{"nt35590_720p_cmd", NT35590_720P_CMD_PANEL},
	{"innolux_720p_video", INNOLUX_720P_VIDEO_PANEL},
	{"otm8019a_fwvga_video", OTM8019A_FWVGA_VIDEO_PANEL},
	{"otm1283a_720p_video", OTM1283A_720P_VIDEO_PANEL},
	{"nt35596_1080p_video", NT35596_1080P_VIDEO_PANEL},
};

static uint32_t panel_id;

int oem_panel_rotation()
{
	return NO_ERROR;
}

int oem_panel_on()
{
	/*
	 *OEM can keep there panel specific on instructions in this
	 *function
	 */
	if (panel_id == OTM8019A_FWVGA_VIDEO_PANEL) {
		/* needs extra delay to avoid unexpected artifacts */
		mdelay(OTM8019A_FWVGA_VIDEO_PANEL_ON_DELAY);
	}

	return NO_ERROR;
}

int oem_panel_off()
{
	/* OEM can keep their panel specific off instructions
	 * in this function
	 */
	return NO_ERROR;
}

static int init_panel_data(struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	int pan_type = PANEL_TYPE_DSI;

	switch (panel_id) {
	case JDI_1080P_VIDEO_PANEL:
		panelstruct->paneldata    = &jdi_1080p_video_panel_data;
		panelstruct->paneldata->panel_with_enable_gpio = 1;
		panelstruct->panelres     = &jdi_1080p_video_panel_res;
		panelstruct->color        = &jdi_1080p_video_color;
		panelstruct->videopanel   = &jdi_1080p_video_video_panel;
		panelstruct->commandpanel = &jdi_1080p_video_command_panel;
		panelstruct->state        = &jdi_1080p_video_state;
		panelstruct->laneconfig   = &jdi_1080p_video_lane_config;
		panelstruct->paneltiminginfo
			= &jdi_1080p_video_timing_info;
		panelstruct->panelresetseq
					 = &jdi_1080p_video_panel_reset_seq;
		panelstruct->backlightinfo = &jdi_1080p_video_backlight;
		pinfo->mipi.panel_cmds
			= jdi_1080p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
			= JDI_1080P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
			jdi_1080p_video_timings, TIMING_SIZE);
		pinfo->mipi.signature 	= JDI_1080P_VIDEO_SIGNATURE;
		break;
	case NT35590_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &nt35590_720p_video_panel_data;
		panelstruct->panelres     = &nt35590_720p_video_panel_res;
		panelstruct->color        = &nt35590_720p_video_color;
		panelstruct->videopanel   = &nt35590_720p_video_video_panel;
		panelstruct->commandpanel = &nt35590_720p_video_command_panel;
		panelstruct->state        = &nt35590_720p_video_state;
		panelstruct->laneconfig   = &nt35590_720p_video_lane_config;
		panelstruct->paneltiminginfo
					 = &nt35590_720p_video_timing_info;
		panelstruct->panelresetseq
					 = &nt35590_720p_video_panel_reset_seq;
		panelstruct->backlightinfo = &nt35590_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= nt35590_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= NT35590_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35590_720p_video_timings, TIMING_SIZE);
		pinfo->mipi.signature 	= NT35590_720P_VIDEO_SIGNATURE;
		break;
	case NT35590_720P_CMD_PANEL:
		panelstruct->paneldata    = &nt35590_720p_cmd_panel_data;
		panelstruct->panelres     = &nt35590_720p_cmd_panel_res;
		panelstruct->color        = &nt35590_720p_cmd_color;
		panelstruct->videopanel   = &nt35590_720p_cmd_video_panel;
		panelstruct->commandpanel = &nt35590_720p_cmd_command_panel;
		panelstruct->state        = &nt35590_720p_cmd_state;
		panelstruct->laneconfig   = &nt35590_720p_cmd_lane_config;
		panelstruct->paneltiminginfo = &nt35590_720p_cmd_timing_info;
		panelstruct->panelresetseq
					= &nt35590_720p_cmd_panel_reset_seq;
		panelstruct->backlightinfo = &nt35590_720p_cmd_backlight;
		pinfo->mipi.panel_cmds
					= nt35590_720p_cmd_on_command;
		pinfo->mipi.num_of_panel_cmds
					= NT35590_720P_CMD_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35590_720p_cmd_timings, TIMING_SIZE);
		pinfo->mipi.signature 	= NT35590_720P_CMD_SIGNATURE;
		break;
	case INNOLUX_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &innolux_720p_video_panel_data;
		panelstruct->panelres     = &innolux_720p_video_panel_res;
		panelstruct->color        = &innolux_720p_video_color;
		panelstruct->videopanel   = &innolux_720p_video_video_panel;
		panelstruct->commandpanel = &innolux_720p_video_command_panel;
		panelstruct->state        = &innolux_720p_video_state;
		panelstruct->laneconfig   = &innolux_720p_video_lane_config;
		panelstruct->paneltiminginfo
					= &innolux_720p_video_timing_info;
		panelstruct->panelresetseq
					= &innolux_720p_video_reset_seq;
		panelstruct->backlightinfo = &innolux_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= innolux_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= INNOLUX_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				innolux_720p_video_timings, TIMING_SIZE);
		break;
	case OTM8019A_FWVGA_VIDEO_PANEL:
		panelstruct->paneldata    = &otm8019a_fwvga_video_panel_data;
		panelstruct->panelres     = &otm8019a_fwvga_video_panel_res;
		panelstruct->color        = &otm8019a_fwvga_video_color;
		panelstruct->videopanel   = &otm8019a_fwvga_video_video_panel;
		panelstruct->commandpanel = &otm8019a_fwvga_video_command_panel;
		panelstruct->state        = &otm8019a_fwvga_video_state;
		panelstruct->laneconfig   = &otm8019a_fwvga_video_lane_config;
		panelstruct->paneltiminginfo
					= &otm8019a_fwvga_video_timing_info;
		panelstruct->panelresetseq
					= &otm8019a_fwvga_video_reset_seq;
		panelstruct->backlightinfo = &otm8019a_fwvga_video_backlight;
		pinfo->mipi.panel_cmds
					= otm8019a_fwvga_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= OTM8019A_FWVGA_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				otm8019a_fwvga_video_timings, TIMING_SIZE);
		break;
	case OTM1283A_720P_VIDEO_PANEL:
		panelstruct->paneldata    = &otm1283a_720p_video_panel_data;
		panelstruct->panelres     = &otm1283a_720p_video_panel_res;
		panelstruct->color        = &otm1283a_720p_video_color;
		panelstruct->videopanel   = &otm1283a_720p_video_video_panel;
		panelstruct->commandpanel = &otm1283a_720p_video_command_panel;
		panelstruct->state        = &otm1283a_720p_video_state;
		panelstruct->laneconfig   = &otm1283a_720p_video_lane_config;
		panelstruct->paneltiminginfo
					= &otm1283a_720p_video_timing_info;
		panelstruct->panelresetseq
					= &otm1283a_720p_video_reset_seq;
		panelstruct->backlightinfo = &otm1283a_720p_video_backlight;
		pinfo->mipi.panel_cmds
					= otm1283a_720p_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= OTM1283A_720P_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				otm1283a_720p_video_timings, TIMING_SIZE);
		break;
	case NT35596_1080P_VIDEO_PANEL:
		panelstruct->paneldata    = &nt35596_1080p_skuk_video_panel_data;
		panelstruct->panelres     = &nt35596_1080p_skuk_video_panel_res;
		panelstruct->color        = &nt35596_1080p_skuk_video_color;
		panelstruct->videopanel   = &nt35596_1080p_skuk_video_video_panel;
		panelstruct->commandpanel = &nt35596_1080p_skuk_video_command_panel;
		panelstruct->state        = &nt35596_1080p_skuk_video_state;
		panelstruct->laneconfig   = &nt35596_1080p_skuk_video_lane_config;
		panelstruct->paneltiminginfo
					= &nt35596_1080p_skuk_video_timing_info;
		panelstruct->panelresetseq
					= &nt35596_1080p_skuk_video_reset_seq;
		panelstruct->backlightinfo = &nt35596_1080p_skuk_video_backlight;
		pinfo->mipi.panel_cmds
					= nt35596_1080p_skuk_video_on_command;
		pinfo->mipi.num_of_panel_cmds
					= NT35596_1080P_SKUK_VIDEO_ON_COMMAND;
		memcpy(phy_db->timing,
				nt35596_1080p_skuk_video_timings, TIMING_SIZE);
		break;
	case UNKNOWN_PANEL:
	default:
		memset(panelstruct, 0, sizeof(struct panel_struct));
		memset(pinfo->mipi.panel_cmds, 0, sizeof(struct mipi_dsi_cmd));
		pinfo->mipi.num_of_panel_cmds = 0;
		memset(phy_db->timing, 0, TIMING_SIZE);
		pan_type = PANEL_TYPE_UNKNOWN;
		break;
	}
	return pan_type;
}

uint32_t oem_panel_max_auto_detect_panels()
{
        return target_panel_auto_detect_enabled() ?
                        DISPLAY_MAX_PANEL_DETECTION : 0;
}

int oem_panel_select(const char *panel_name, struct panel_struct *panelstruct,
			struct msm_panel_info *pinfo,
			struct mdss_dsi_phy_ctrl *phy_db)
{
	uint32_t hw_id = board_hardware_id();
	uint32_t hw_subtype = board_hardware_subtype();
	int32_t panel_override_id;
	uint32_t target_id, plat_hw_ver_major;

	if (panel_name) {
		panel_override_id = panel_name_to_id(supp_panels,
				ARRAY_SIZE(supp_panels), panel_name);

		if (panel_override_id < 0) {
			dprintf(CRITICAL, "Not able to search the panel:%s\n",
					 panel_name + strspn(panel_name, " "));
		} else if (panel_override_id < UNKNOWN_PANEL) {
			/* panel override using fastboot oem command */
			panel_id = panel_override_id;

			dprintf(INFO, "OEM panel override:%s\n",
					panel_name + strspn(panel_name, " "));
			goto panel_init;
		}
	}

	switch (hw_id) {
	case HW_PLATFORM_MTP:
		panel_id = JDI_1080P_VIDEO_PANEL;
		break;
	case HW_PLATFORM_SURF:
		panel_id = JDI_1080P_VIDEO_PANEL;
		switch (auto_pan_loop) {
		case 0:
			panel_id = JDI_1080P_VIDEO_PANEL;
			break;
		case 1:
			panel_id = NT35590_720P_VIDEO_PANEL;
			break;
		default:
			panel_id = UNKNOWN_PANEL;
			dprintf(CRITICAL, "Unknown panel\n");
			return PANEL_TYPE_UNKNOWN;
		}
		auto_pan_loop++;
		break;
	case HW_PLATFORM_QRD:
		if (platform_is_msm8939()) {
			switch (hw_subtype) {
			case HW_PLATFORM_SUBTYPE_SKUK:
				panel_id = NT35596_1080P_VIDEO_PANEL;
				break;
			default:
				dprintf(CRITICAL, "Invalid subtype id %d for QRD HW\n",
					hw_subtype);
				return PANEL_TYPE_UNKNOWN;
			}
		} else {
			switch (hw_subtype) {
			case HW_PLATFORM_SUBTYPE_SKUH:
				target_id = board_target_id();
				plat_hw_ver_major = ((target_id >> 16) & 0xFF);

				/* qrd fan-out hw ? */
				if ((plat_hw_ver_major >> 4) == 0x1)
					panel_id = OTM1283A_720P_VIDEO_PANEL;
				else
					panel_id = INNOLUX_720P_VIDEO_PANEL;
				break;
			case HW_PLATFORM_SUBTYPE_SKUI:
				panel_id = OTM8019A_FWVGA_VIDEO_PANEL;
				break;
			default:
				dprintf(CRITICAL, "Invalid subtype id %d for QRD HW\n",
					hw_subtype);
				return PANEL_TYPE_UNKNOWN;
			}
		}
		break;
	default:
		dprintf(CRITICAL, "Display not enabled for %d HW type\n",
			hw_id);
		return PANEL_TYPE_UNKNOWN;
	}

panel_init:
	/* Set LDO mode */
	if (platform_is_msm8939() || (hw_id == HW_PLATFORM_QRD))
		phy_db->regulator_mode = DSI_PHY_REGULATOR_LDO_MODE;

	pinfo->pipe_type = MDSS_MDP_PIPE_TYPE_RGB;
	return init_panel_data(panelstruct, pinfo, phy_db);
}