/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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

/*---------------------------------------------------------------------------
 * This file is autogenerated file using gcdb parser. Please do not edit it.
 * Update input XML file to add a new entry or update variable in this file
 * VERSION = "1.0"
 *---------------------------------------------------------------------------*/

#ifndef _PANEL_AUO_720P_VIDEO_H_

#define _PANEL_AUO_720P_VIDEO_H_
/*---------------------------------------------------------------------------*/
/* HEADER files                                                              */
/*---------------------------------------------------------------------------*/
#include "panel.h"

/*---------------------------------------------------------------------------*/
/* Panel configuration                                                       */
/*---------------------------------------------------------------------------*/

static struct panel_config auo_720p_video_panel_data = {
  "qcom,mdss_dsi_auo_720p_video", "dsi:0:", "qcom,mdss-dsi-panel",
  10, 0, "DISPLAY_1", 0, 572000000, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel resolution                                                          */
/*---------------------------------------------------------------------------*/
static struct panel_resolution auo_720p_video_panel_res = {
  720, 1280, 125, 47, 5, 0, 7, 10, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel Color Information                                                   */
/*---------------------------------------------------------------------------*/
static struct color_info auo_720p_video_color = {
  24, 0, 0xff, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Panel Command information                                                 */
/*---------------------------------------------------------------------------*/
static char auo_720p_video_on_cmd0[] = {
  0x11, 0, 5, 0x80
};

static char auo_720p_video_on_cmd1[] = {
  0xB0, 4, 0x23, 0x80
};

static char auo_720p_video_on_cmd2[] = {
  7, 0, 0x29, 0xC0, 0xBB, 0xB, 0xF3, 0xF, 1, 0, 0, 0xFF
};

static char auo_720p_video_on_cmd3[] = {
  6, 0, 0x29, 0xC0, 0xB8, 1, 0, 0x3F, 0x18, 0x18, 0xFF
};

static char auo_720p_video_on_cmd4[] = {
  8, 0, 0x29, 0xC0, 0xB9, 0x18, 0, 0x18, 0x18, 0x9F
};

static char auo_720p_video_on_cmd5[32] = {
  0x19, 0, 0x29, 0xC0, 0xBA, 0, 0, 0xC, 0x14, 0x4C, 0xB,
  0x6C, 0xB, 0xC, 0x14, 0, 0xDA, 0x6D, 3, 0xFF, 0xFF,
  0x10, 0xD9, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0, 0xFF,
  0xFF, 0xFF
};

static char auo_720p_video_on_cmd6[] = {
  0xB0, 3, 0x23, 0x80
};

static char auo_720p_video_on_cmd7[] = {
  0x29, 0, 5, 0x80
};




static struct mipi_dsi_cmd auo_720p_video_on_command[] = {
{ 0x4, auo_720p_video_on_cmd0, 0x78 },
{ 0x4, auo_720p_video_on_cmd1, 0 },
{ 0xC, auo_720p_video_on_cmd2, 0 },
{ 0xC, auo_720p_video_on_cmd3, 0 },
{ 0xC, auo_720p_video_on_cmd4, 0 },
{ 0x20, auo_720p_video_on_cmd5, 0 },
{ 0x4, auo_720p_video_on_cmd6, 0 },
{ 0x4, auo_720p_video_on_cmd7, 0x14 },
};
#define AUO_720P_VIDEO_ON_COMMAND 8


static struct command_state auo_720p_video_state = {
  0, 1
};

/*---------------------------------------------------------------------------*/
/* Command mode panel information                                            */
/*---------------------------------------------------------------------------*/

static struct commandpanel_info auo_720p_video_command_panel = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*---------------------------------------------------------------------------*/
/* Video mode panel information                                              */
/*---------------------------------------------------------------------------*/

static struct videopanel_info auo_720p_video_video_panel = {
  1, 0, 0, 0, 1, 1, 0, 0, 9
};

/*---------------------------------------------------------------------------*/
/* Lane Configuration                                                        */
/*---------------------------------------------------------------------------*/

static struct lane_configuration auo_720p_video_lane_config = {
  3, 0, 1, 1, 1, 0
};


/*---------------------------------------------------------------------------*/
/* Panel Timing                                                              */
/*---------------------------------------------------------------------------*/
static const uint32_t auo_720p_video_timings[] = {
  0xc0, 0x90, 0x24, 0x00, 0x9b, 0x98, 0x27, 0x92, 0x28, 0x03, 0x04, 0x00
};



static struct panel_timing auo_720p_video_timing_info = {
  0, 4, 0x19, 0x31
};

static struct panel_reset_sequence auo_720p_video_panel_reset_seq = {
{ 1, 0, 1, 0, 0, }, { 30, 20, 50, 0, 0, }, 2
};

/*---------------------------------------------------------------------------*/
/* Backlight Settings                                                        */
/*---------------------------------------------------------------------------*/

static struct backlight auo_720p_video_backlight = {
  1, 1, 4095, 100, 1, "bl_ctrl_wled"
};


#endif /*_PANEL_AUO_720P_VIDEO_H_*/
