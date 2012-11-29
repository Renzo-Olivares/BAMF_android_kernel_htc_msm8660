/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#include <linux/slab.h>

#ifdef CONFIG_MSM_CAMERA_8X60
// #include <mach/camera-8x60.h>
#elif defined(CONFIG_MSM_CAMERA_7X30)
#include <mach/camera-7x30.h>
#else
#include <mach/camera.h>
#endif
// #include <media/msm_camera_sensor.h>
#include "msm_sensor.h"
#include "msm.h"

#include <mach/gpio.h>
#include <mach/vreg.h>
#include <asm/mach-types.h>
#include "s5k3h2yx.h"

#define SENSOR_NAME "s5k3h2yx"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k3h2yx"

/* CAMIF output resolutions */
/* 816x612, 24MHz MCLK 96MHz PCLK */
#define SENSOR_FULL_SIZE_WIDTH 3280
#define SENSOR_FULL_SIZE_HEIGHT 2464
#define SENSOR_VIDEO_SIZE_WIDTH 3084
#define SENSOR_VIDEO_SIZE_HEIGHT 1736

#if defined(CONFIG_MACH_HOLIDAY) || defined(CONFIG_MACH_RUBY) || defined(CONFIG_MACH_RIDER)
#define SENSOR_VIDEO_SIZE_WIDTH_FAST 1528
#define SENSOR_VIDEO_SIZE_HEIGHT_FAST  860
#else
#define SENSOR_VIDEO_SIZE_WIDTH_FAST 1640
#define SENSOR_VIDEO_SIZE_HEIGHT_FAST  916
#endif
#define SENSOR_VIDEO_SIZE_WIDTH_FAST_7X30 1632 /* 1632 */  /* 1024 */
#define SENSOR_VIDEO_SIZE_HEIGHT_FAST_7X30  768 /* 576 */  /* 768 */
#define SENSOR_QTR_SIZE_WIDTH 1640
#define SENSOR_QTR_SIZE_HEIGHT 1232

#define SENSOR_HRZ_FULL_BLK_PIXELS 190
#define SENSOR_VER_FULL_BLK_LINES 16
#define SENSOR_HRZ_VIDEO_BLK_PIXELS 386
#define SENSOR_VER_VIDEO_BLK_LINES 16

#if defined(CONFIG_MACH_HOLIDAY) || defined(CONFIG_MACH_RUBY) || defined(CONFIG_MACH_RIDER)
#define SENSOR_HRZ_VIDEO_BLK_PIXELS_FAST 1942
#define SENSOR_VER_VIDEO_BLK_LINES_FAST 16
#else
#define SENSOR_HRZ_VIDEO_BLK_PIXELS_FAST 1830
#define SENSOR_VER_VIDEO_BLK_LINES_FAST 16
#endif
#define SENSOR_HRZ_VIDEO_BLK_PIXELS_FAST_7X30 1838 /* 1838 */  /* 2446 */
#define SENSOR_VER_VIDEO_BLK_LINES_FAST_7X30 48 /* 136 */ /* 16 */
#define SENSOR_HRZ_QTR_BLK_PIXELS 1830
#define SENSOR_VER_QTR_BLK_LINES 16

#define S5K3H2YX_MIN_COARSE_INTEGRATION_TIME 4
#define S5K3H2YX_OFFSET 8 /* 4; */     /* kipper */

#define S5K3H2YX_AF_I2C_ADDR 0x18
#define S5K3H2YX_VCM_CODE_MSB 0x04
#define S5K3H2YX_VCM_CODE_LSB 0x05
#define S5K3H2YX_TOTAL_STEPS_NEAR_TO_FAR 42
#define S5K3H2YX_SW_DAMPING_STEP 10
#define S5K3H2YX_MAX_FPS 30

/*=============================================================
 SENSOR REGISTER DEFINES
==============================================================*/

#define S5K3H2YX_REG_MODEL_ID 0x0000
#define S5K3H2YX_MODEL_ID 0x382b

/* Color bar pattern selection */
#define S5K3H2YX_COLOR_BAR_PATTERN_SEL_REG 0x0601

#define REG_LINE_LENGTH_PCK_MSB 0x0342
#define REG_LINE_LENGTH_PCK_LSB 0x0343
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_MSB 0x0204
#define REG_ANALOGUE_GAIN_CODE_GLOBAL_LSB 0x0205
#define REG_COARSE_INTEGRATION_TIME_MSB 0x0202
#define REG_COARSE_INTEGRATION_TIME_LSB 0x0203

#define S5K3H2YX_REG_GROUP_PARAMETER_HOLD 0x0104
#define S5K3H2YX_GROUP_PARAMETER_HOLD 0x01
#define S5K3H2YX_GROUP_PARAMETER_UNHOLD 0x00

/* Mode select register */
#define S5K3H2YX_REG_MODE_SELECT		0x0100
#define S5K3H2YX_MODE_SELECT_STREAM		0x01	/* start streaming */
#define S5K3H2YX_MODE_SELECT_SW_STANDBY	0x00	/* software standby */
/* Read Mode */
#define S5K3H2YX_REG_READ_MODE 0x0101
#define S5K3H2YX_READ_NORMAL_MODE 0x00  /* without mirror/flip */
#define S5K3H2YX_READ_MIRROR_FLIP 0x03  /* with mirror/flip */

#define Q8 0x00000100
#define SENSOR_DEFAULT_CLOCK_RATE 24000000

DEFINE_MUTEX(s5k3h2yx_mut);

static struct msm_camera_i2c_reg_conf s5k3h2yx_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_mipi_settings[] = {
/*	{0x0101, 0x00},*/
	{0x3065, 0x35},
	{0x310E, 0x00},
	{0x3098, 0xAB},
	{0x30C7, 0x0A},
	{0x309A, 0x01},
	{0x310D, 0xC6},
	{0x30C3, 0x40},
	{0x30BB, 0x02},
	{0x30BC, 0x38},
	{0x30BD, 0x40},
	{0x3110, 0x70},
	{0x3111, 0x80},
	{0x3112, 0x7B},
	{0x3113, 0xC0},
	{0x30C7, 0x1A},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_pll_settings[] = {
	{0x0305, 0x04},/*pre_pll_clk_div = 4*/
	{0x0306, 0x00},/*pll_multiplier*/
	{0x0307, 0x98},/*pll_multiplier  = 152*/
	{0x0303, 0x01},/*vt_sys_clk_div = 1*/
	{0x0301, 0x05},/*vt_pix_clk_div = 5*/
	{0x030B, 0x01},/*op_sys_clk_div = 1*/
	{0x0309, 0x05},/*op_pix_clk_div = 5*/
	{0x30CC, 0xE0},/*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
	{0x31A1, 0x5A},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_prev_settings[] = {
	/* PLL setting*/
	{0x0305, 0x04},/*PRE_PLL_CLK_DIV*/
	{0x0306, 0x00},/*PLL_MULTIPLIER*/
	{0x0307, 0x6C},/*PLL_MULTIPLIER*/
	{0x0303, 0x01},/*VT_SYS_CLK_DIV*/
	{0x0301, 0x05},/*VT_PIX_CLK_DIV*/
	{0x030B, 0x01},/*OP_SYS_CLK_DIV*/
	{0x0309, 0x05},/*OP_PIX_CLK_DIV*/
	{0x30CC, 0xB0},/*DPHY_BAND_CTRL*/
	{0x31A1, 0x56},/*BINNING*/

	/*Timing configuration*/
	{0x0200, 0x02},/*FINE_INTEGRATION_TIME_*/
	{0x0201, 0x50},
	{0x0202, 0x04},/*COARSE_INTEGRATION_TIME*/
	{0x0203, 0xDB},
	{0x0204, 0x00},/*ANALOG_GAIN*/
	{0x0205, 0x20},
	{0x0342, 0x0D},/*LINE_LENGTH_PCK*/
	{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
	{0x0340, 0x04},/*FRAME_LENGTH_LINES 1268*/
	{0x0341, 0xF4},
#else
	{0x0340, 0x04},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xE0},
#endif
	/*Output Size (1640x1232)*/
	{0x0344, 0x00},/*X_ADDR_START*/
	{0x0345, 0x00},
	{0x0346, 0x00},/*Y_ADDR_START*/
	{0x0347, 0x00},
	{0x0348, 0x0C},/*X_ADDR_END*/
	{0x0349, 0xCD},
	{0x034A, 0x09},/*Y_ADDR_END*/
	{0x034B, 0x9F},
	{0x0381, 0x01},/*X_EVEN_INC*/
	{0x0383, 0x03},/*X_ODD_INC*/
	{0x0385, 0x01},/*Y_EVEN_INC*/
	{0x0387, 0x03},/*Y_ODD_INC*/
	{0x0401, 0x00},/*DERATING_EN*/
	{0x0405, 0x10},
	{0x0700, 0x05},/*FIFO_WATER_MARK_PIXELS*/
	{0x0701, 0x30},
	{0x034C, 0x06},/*X_OUTPUT_SIZE*/
	{0x034D, 0x68},
	{0x034E, 0x04},/*Y_OUTPUT_SIZE*/
	{0x034F, 0xD0},
	/*Manufacture Setting*/
	{0x300E, 0xED},
	{0x301D, 0x80},
	{0x301A, 0x77},
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_video_settings[] = {
	/* PLL setting*/
	{0x0305, 0x04},/*pre_pll_clk_div = 4*/
	{0x0306, 0x00},/*pll_multiplier*/
	{0x0307, 0x98},/*pll_multiplier  = 152*/
	{0x0303, 0x01},/*vt_sys_clk_div = 1*/
	{0x0301, 0x05},/*vt_pix_clk_div = 5*/
	{0x030B, 0x01},/*op_sys_clk_div = 1*/
	{0x0309, 0x05},/*op_pix_clk_div = 5*/
	{0x30CC, 0xE0},/*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
	{0x31A1, 0x5A},

	{ 0x0344 , 0x00 }, /* X addr start 98d */
	{ 0x0345 , 0x62 },
	{ 0x0346 , 0x01 }, /* Y addr start 364d */
	{ 0x0347 , 0x6C },
	{ 0x0348 , 0x0C }, /* X addr end 3181d */
	{ 0x0349 , 0x6D },
	{ 0x034A , 0x08 }, /* Y addr end 2099d */
	{ 0x034B , 0x33 },
	{ 0x0381 , 0x01 }, /* x_even_inc = 1 */
	{ 0x0383 , 0x01 }, /* x_odd_inc = 1 */
	{ 0x0385 , 0x01 }, /* y_even_inc = 1 */
	{ 0x0387 , 0x01 }, /* y_odd_inc = 1 */
	{ 0x0105 , 0x01 }, /* skip corrupted frame - for preview flash when doing hjr af */
	{ 0x0401 , 0x00 }, /* Derating_en  = 0 (disable) */
	{ 0x0405 , 0x10 },
	{ 0x0700 , 0x05 }, /* fifo_water_mark_pixels = 1328 */
	{ 0x0701 , 0x30 },
	{ 0x034C , 0x0C }, /* x_output_size = 3084 */
	{ 0x034D , 0x0C },
	{ 0x034E , 0x06 }, /* y_output_size = 1736 */
	{ 0x034F , 0xC8 },
	{ 0x0200 , 0x02 }, /* fine integration time */
	{ 0x0201 , 0x50 },
	{ 0x0202 , 0x04 }, /* Coarse integration time */
	{ 0x0203 , 0xDB },
	{ 0x0204 , 0x00 }, /* Analog gain */
	{ 0x0205 , 0x20 },
	{ 0x0342 , 0x0D }, /* Line_length_pck 3470d */
	{ 0x0343 , 0x8E },
#ifdef CONFIG_RAWCHIP
	{ 0x0340 , 0x06 }, /* Frame_length_lines 1772d */
	{ 0x0341 , 0xEC },
#else
	{ 0x0340 , 0x06 }, /* Frame_length_lines 1752d */
	{ 0x0341 , 0xD8 },
#endif

	/*Manufacture Setting*/
	{ 0x300E , 0x29 }, /* Reserved  For 912Mbps */
	{ 0x31A3 , 0x00 }, /* Reserved  For 912Mbps */
	{ 0x301A , 0x77 }, /* Reserved */
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_fast_video_settings[] = {
#if 0
  {0x0305, 0x04}, /*pre_pll_clk_div = 4*/
  {0x0306, 0x00}, /*pll_multiplier*/
  {0x0307, 0x98}, /*pll_multiplier  = 152*/
  {0x0303, 0x01}, /*vt_sys_clk_div = 1*/
  {0x0301, 0x05}, /*vt_pix_clk_div = 5*/
  {0x030B, 0x01}, /*op_sys_clk_div = 1*/
  {0x0309, 0x05}, /*op_pix_clk_div = 5*/
  {0x30CC, 0xE0}, /*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
  {0x31A1, 0x5A}, /*"DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0]= 912*/
  /*Readout*/
  /*Address Data  Comment*/
  {0x0344, 0x00}, /*X addr start 112d*/
  {0x0345, 0x70},
  {0x0346, 0x01}, /*Y addr start 372d*/
  {0x0347, 0x74},
  {0x0348, 0x0C}, /*X addr end 3165d*/
  {0x0349, 0x5D},
  {0x034A, 0x08}, /*Y addr end 2091d*/
  {0x034B, 0x2B},

  {0x0381, 0x01}, /*x_even_inc = 1*/
  {0x0383, 0x03}, /*x_odd_inc = 3*/
  {0x0385, 0x01}, /*y_even_inc = 1*/
  {0x0387, 0x03}, /*y_odd_inc = 3*/

  {0x0401, 0x00}, /*Derating_en  = 0 (disable)*/
  {0x0405, 0x10},
  {0x0700, 0x05}, /*fifo_water_mark_pixels = 1328*/
  {0x0701, 0x30},

  {0x034C, 0x05}, /*x_output_size = 1528*/
  {0x034D, 0xF8},
  {0x034E, 0x03}, /*y_output_size = 860*/
  {0x034F, 0x5C},

  {0x0200, 0x02}, /*fine integration time*/
  {0x0201, 0x50},
  {0x0202, 0x02}, /*Coarse integration time*/
  {0x0203, 0x5c},
  {0x0204, 0x00}, /*Analog gain*/
  {0x0205, 0x20},
  {0x0342, 0x0D}, /*Line_length_pck 3470d*/
  {0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
  /*Frame_length_lines 960d*/
  {0x0340, 0x03},
  {0x0341, 0xC0},
#else
  /*Frame_length_lines 876d*/
  {0x0340, 0x03},
  {0x0341, 0x6C},
#endif

  /*Manufacture Setting*/
  /*Address Data  Comment*/
  {0x300E, 0x2D},
  {0x31A3, 0x40},
  {0x301A, 0xA7},
  {0x3053, 0xCB}, /*CF for full ,CB for preview/HD/FHD/QVGA120fps*/
 #else
//100fps
{0x0305, 0x04},	//pre_pll_clk_div = 4
{0x0306, 0x00},	//pll_multiplier
{0x0307, 0x98},	//pll_multiplier  = 152
{0x0303, 0x01},	//vt_sys_clk_div = 1
{0x0301, 0x05},	//vt_pix_clk_div = 5
{0x030B, 0x01},	//op_sys_clk_div = 1
{0x0309, 0x05},	//op_pix_clk_div = 5
{0x30CC, 0xE0},	//DPHY_band_ctrl 870 MHz ~ 950 MHz
{0x31A1, 0x5A},	//"DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0]= 912Mhz / 10 = 91.2Mhz[7:4]

{0x0344, 0x00},	//X addr start 0d
{0x0345, 0x00},
{0x0346, 0x00},	//Y addr start 212d
{0x0347, 0xD4},
{0x0348, 0x0C},	//X addr end 3277d
{0x0349, 0xCD},
{0x034A, 0x08},	//Y addr end 2251d
{0x034B, 0xCB},

{0x0381, 0x01},	//x_even_inc = 1
{0x0383, 0x03},	//x_odd_inc = 3
{0x0385, 0x01},	//y_even_inc = 1
{0x0387, 0x07},	//y_odd_inc = 7

{0x0401, 0x00},	//Derating_en  = 0 (disable)
{0x0405, 0x10},
{0x0700, 0x05},	//fifo_water_mark_pixels = 1328
{0x0701, 0x30},

{0x034C, 0x06},	//x_output_size = 1640
{0x034D, 0x68},
{0x034E, 0x01},	//y_output_size = 510
{0x034F, 0xFE},

{0x0200, 0x02},	//fine integration time
{0x0201, 0x50},
{0x0202, 0x01},	//Coarse integration time
{0x0203, 0x39},
{0x0204, 0x00},	//Analog gain
{0x0205, 0x20},
{0x0342, 0x0D},	//Line_length_pck 3470d
{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
  {0x0340, 0x02},	//Frame_length_lines 324d
  {0x0341, 0x22},
#else
  {0x0340, 0x02},	//Frame_length_lines 324d
  {0x0341, 0x0E},
#endif

{0x300E, 0x2D},
{0x31A3, 0x40},
{0x301A, 0xA7},
{0x3053, 0xCB}, //CF for full ,CB for preview/HD/FHD/QVGA120fps
#endif //ori
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_snap_settings[] = {
	/* PLL setting*/
	{0x0305, 0x04},/*pre_pll_clk_div = 4*/
	{0x0306, 0x00},/*pll_multiplier*/
	{0x0307, 0x98},/*pll_multiplier  = 152*/
	{0x0303, 0x01},/*vt_sys_clk_div = 1*/
	{0x0301, 0x05},/*vt_pix_clk_div = 5*/
	{0x030B, 0x01},/*op_sys_clk_div = 1*/
	{0x0309, 0x05},/*op_pix_clk_div = 5*/
	{0x30CC, 0xE0},/*DPHY_band_ctrl 870 MHz ~ 950 MHz*/
	{0x31A1, 0x5A},

	/*Timing configuration*/
	{0x0200, 0x02},/*FINE_INTEGRATION_TIME_*/
	{0x0201, 0x50},
	{0x0202, 0x04},/*COARSE_INTEGRATION_TIME*/
	{0x0203, 0xE7},
	{0x0204, 0x00},/*ANALOG_GAIN*/
	{0x0205, 0x20},
	{0x0342, 0x0D},/*LINE_LENGTH_PCK*/
	{0x0343, 0x8E},
#ifdef CONFIG_RAWCHIP
	{0x0340, 0x09},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xC4},
#else
	{0x0340, 0x09},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xC0},
#endif
	/*Output Size (3280x2464)*/
	{0x0344, 0x00},/*X_ADDR_START*/
	{0x0345, 0x00},
	{0x0346, 0x00},/*Y_ADDR_START*/
	{0x0347, 0x00},
	{0x0348, 0x0C},/*X_ADDR_END*/
	{0x0349, 0xCF},
	{0x034A, 0x09},/*Y_ADDR_END*/
	{0x034B, 0x9F},
	{0x0381, 0x01},/*X_EVEN_INC*/
	{0x0383, 0x01},/*X_ODD_INC*/
	{0x0385, 0x01},/*Y_EVEN_INC*/
	{0x0387, 0x01},/*Y_ODD_INC*/
	{0x0105, 0x01 }, /* skip corrupted frame - for preview flash when doing hjr af */
	{0x0401, 0x00},/*DERATING_EN*/
	{0x0405, 0x10},
	{0x0700, 0x05},/*FIFO_WATER_MARK_PIXELS*/
	{0x0701, 0x30},
	{0x034C, 0x0C},/*X_OUTPUT_SIZE*/
	{0x034D, 0xD0},
	{0x034E, 0x09},/*Y_OUTPUT_SIZE*/
	{0x034F, 0xA0},

	/*Manufacture Setting*/
	{ 0x300E , 0x29 }, /* Reserved  For 912Mbps */
	{ 0x31A3 , 0x00 }, /* Reserved  For 912Mbps */
	{ 0x301A , 0x77 }, /* Reserved */
};


static struct msm_camera_i2c_reg_conf s5k3h2yx_snap_wide_settings[] = {

	{0x0305, 0x04},	//pre_pll_clk_div = 4
	{0x0306, 0x00},	//pll_multiplier
	{0x0307, 0x98},	//pll_multiplier  = 152
	{0x0303, 0x01},	//vt_sys_clk_div = 1
	{0x0301, 0x05},	//vt_pix_clk_div = 5
	{0x030B, 0x01},	//op_sys_clk_div = 1
	{0x0309, 0x05},	//op_pix_clk_div = 5
	{0x30CC, 0xE0},	//DPHY_band_ctrl 870 MHz ~ 950 MHz
	{0x31A1, 0x5A},	//"DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0])

	{0x0344, 0x00},	//X addr start 0d
	{0x0345, 0x00},
	{0x0346, 0x01},	//Y addr start 304d
	{0x0347, 0x30},
	{0x0348, 0x0C},	//X addr end 3279d
	{0x0349, 0xCF},
	{0x034A, 0x08},	//Y addr end 2159d
	{0x034B, 0x6F},

	{0x0381, 0x01},	//x_even_inc = 1
	{0x0383, 0x01},	//x_odd_inc = 1
	{0x0385, 0x01},	//y_even_inc = 1
	{0x0387, 0x01},	//y_odd_inc = 1

	{0x0105, 0x01 }, /* skip corrupted frame - for preview flash when doing hjr af */
	{0x0401, 0x00},	//Derating_en  = 0 (disable)
	{0x0405, 0x10},
	{0x0700, 0x05},	//fifo_water_mark_pixels = 1328
	{0x0701, 0x30},

	{0x034C, 0x0C},	//x_output_size = 3280
	{0x034D, 0xD0},
	{0x034E, 0x07},	//y_output_size = 1856
	{0x034F, 0x40},

	{0x0200, 0x02},	//fine integration time
	{0x0201, 0x50},
	{0x0202, 0x04},	//Coarse integration time
	{0x0203, 0xDB},
	{0x0204, 0x00},	//Analog gain
	{0x0205, 0x20},
	{0x0342, 0x0D},	//Line_length_pck 3470d
	{0x0343, 0x8E},
#if 0
	{0x0340, 0x07},	//Frame_length_lines 1872d
	{0x0341, 0x50},
#else
	{0x0340, 0x09},	//Frame_length_lines 1872d
	{0x0341, 0x60},
#endif

	{0x300E, 0x29},
	{0x31A3, 0x00},
	{0x301A, 0xA7},
	{0x3053, 0xCB},	//CF for full/preview/ ,CB for HD/FHD/QVGA120fps
};

static struct msm_camera_i2c_reg_conf s5k3h2yx_night_settings[] = {
  {0x0305, 0x04},	//pre_pll_clk_div = 4
  {0x0306, 0x00},	//pll_multiplier
  {0x0307, 0x98},	//pll_multiplier  = 152
  {0x0303, 0x01},	//vt_sys_clk_div = 1
  {0x0301, 0x05},	//vt_pix_clk_div = 5
  {0x030B, 0x01},	//op_sys_clk_div = 1
  {0x0309, 0x05},	//op_pix_clk_div = 5
  {0x30CC, 0xE0},	//DPHY_band_ctrl 870 MHz ~ 950 MHz
  {0x31A1, 0x5A},	//"DBR_CLK = PLL_CLK / DIV_DBR(s31A1[3:0]) = 912Mhz / 10 = 91.2Mhz[7:4] must be same as vt_pix_clk_div (s0301)"

  {0x0344, 0x00},	//X addr start 0d
  {0x0345, 0x00},	//
  {0x0346, 0x00},	//Y addr start 0d
  {0x0347, 0x00},	//
  {0x0348, 0x0C},	//X addr end 3279d
  {0x0349, 0xCD},	//
  {0x034A, 0x09},	//Y addr end 2463d
  {0x034B, 0x9F},	//

  {0x0381, 0x01},	//x_even_inc = 1
  {0x0383, 0x03},	//x_odd_inc = 3
  {0x0385, 0x01},	//y_even_inc = 1
  {0x0387, 0x03},	//y_odd_inc = 3

  {0x0401, 0x00},	//Derating_en  = 0 (disable)
  {0x0405, 0x10},	//
  {0x0700, 0x05},	//fifo_water_mark_pixels = 1328
  {0x0701, 0x30},	//

  {0x034C, 0x06},	//x_output_size = 1640
  {0x034D, 0x68},	//
  {0x034E, 0x04},	//y_output_size = 1232
  {0x034F, 0xD0},	//

  {0x0200, 0x02},	//fine integration time
  {0x0201, 0x50},	//
  {0x0202, 0x04},	//Coarse integration time
  {0x0203, 0xDB},	//
  {0x0204, 0x00},	//Analog gain
  {0x0205, 0x20},	//
  {0x0342, 0x0D},	//Line_length_pck 3470d
  {0x0343, 0x8E},	//
  {0x0340, 0x04},	//Frame_length_lines 1248d
  {0x0341, 0xE0},

#ifdef CONFIG_RAWCHIP
  {0x0340, 0x04},	//Frame_length_lines 1248d
  {0x0341, 0xF4},
#else
  {0x0340, 0x04},	//Frame_length_lines 1248d
  {0x0341, 0xE0},
#endif


  {0x300E, 0x2D},	//Hbinnning[2] : 1b enale / 0b disable
  {0x31A3, 0x40},	//Vbinning enable[6] : 1b enale / 0b disable
  {0x301A, 0xA7},	//"In case of using the Vt_Pix_Clk more than 137Mhz, sA7h should be adopted! "
  {0x3053, 0xCF},
};


static struct msm_camera_i2c_reg_conf s5k3h2yx_recommend_settings[] = {
	{0x3000, 0x08},
	{0x3001, 0x05},
	{0x3002, 0x0D},
	{0x3003, 0x21},
	{0x3004, 0x62},
	{0x3005, 0x0B},
	{0x3006, 0x6D},
	{0x3007, 0x02},
	{0x3008, 0x62},
	{0x3009, 0x62},
	{0x300A, 0x41},
	{0x300B, 0x10},
	{0x300C, 0x21},
	{0x300D, 0x04},
	{0x307E, 0x03},
	{0x307F, 0xA5},
	{0x3080, 0x04},
	{0x3081, 0x29},
	{0x3082, 0x03},
	{0x3083, 0x21},
	{0x3011, 0x5F},
	{0x3156, 0xE2},
	{0x3027, 0x0E},
	{0x300f, 0x02},
	{0x3010, 0x10},
	{0x3017, 0x74},
	{0x3018, 0x00},
	{0x3020, 0x02},
	{0x3021, 0x24},
	{0x3023, 0x80},
	{0x3024, 0x04},
	{0x3025, 0x08},
	{0x301C, 0xD4},
	{0x315D, 0x00},
	/*Manufacture Setting*/
	{0x300E, 0x29},
	{0x31A3, 0x00},
	{0x301A, 0xA7},
	{0x3053, 0xCF},
	{0x3054, 0x00},
	{0x3055, 0x35},
	{0x3062, 0x04},
	{0x3063, 0x38},
	{0x31A4, 0x04},
	{0x3016, 0x54},
	{0x3157, 0x02},
	{0x3158, 0x00},
	{0x315B, 0x02},
	{0x315C, 0x00},
	{0x301B, 0x05},
	{0x3028, 0x41},
	{0x302A, 0x00},
	{0x3060, 0x01},
	{0x302D, 0x19},
	{0x302B, 0x04},
	{0x3072, 0x13},
	{0x3073, 0x21},
	{0x3074, 0x82},
	{0x3075, 0x20},
	{0x3076, 0xA2},
	{0x3077, 0x02},
	{0x3078, 0x91},
	{0x3079, 0x91},
	{0x307A, 0x61},
	{0x307B, 0x28},
	{0x307C, 0x31},
};
static struct v4l2_subdev_info s5k3h2yx_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k3h2yx_init_conf[] = {
	{&s5k3h2yx_mipi_settings[0],
	ARRAY_SIZE(s5k3h2yx_mipi_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_recommend_settings[0],
	ARRAY_SIZE(s5k3h2yx_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_pll_settings[0],
	ARRAY_SIZE(s5k3h2yx_pll_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array s5k3h2yx_confs[] = {
	{&s5k3h2yx_snap_settings[0],
	ARRAY_SIZE(s5k3h2yx_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_prev_settings[0],
	ARRAY_SIZE(s5k3h2yx_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_video_settings[0],
	ARRAY_SIZE(s5k3h2yx_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_fast_video_settings[0],
	ARRAY_SIZE(s5k3h2yx_fast_video_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_night_settings[0],
	ARRAY_SIZE(s5k3h2yx_night_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2yx_snap_wide_settings[0],
	ARRAY_SIZE(s5k3h2yx_snap_wide_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t s5k3h2yx_dimensions[] = {
	{/*full size*/
		.x_output = 0xCD0,
		.y_output = 0x9A0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x9C4,
#else
		.frame_length_lines = 0x9C0,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{/*Q size*/
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x4F4,
#else
		.frame_length_lines = 0x4E0,
#endif
		.vt_pixel_clk = 129600000,
		.op_pixel_clk = 129600000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{/*video size*/
		.x_output = 0xC0C,
		.y_output = 0x6C8,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x6EC,
#else
		.frame_length_lines = 0x6D8,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0x062,
		.y_addr_start = 0x16C,
		.x_addr_end = 0xC6D,
		.y_addr_end = 0x833,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{/*fast video size*/
#if 0
		.x_output = 0x5F8,
		.y_output = 0x35C,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x3C0,
#else
		.frame_length_lines = 0x36C,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0x070,
		.y_addr_start = 0x174,
		.x_addr_end = 0xC5D,
		.y_addr_end = 0x82B,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
#else
//100 fps
		.x_output = 0x668,
		.y_output = 0x1FE,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x222,
#else
		.frame_length_lines = 0x20E,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x0D4,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x8CB,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 7,
		.binning_rawchip = 0x22,
#endif //ori
	},
	{/*night mode size*/
		.x_output = 0x668,
		.y_output = 0x4D0,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x4F4,
#else
		.frame_length_lines = 0x4E0,
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0xCCD,
		.y_addr_end = 0x99F,
		.x_even_inc = 1,
		.x_odd_inc = 3,
		.y_even_inc = 1,
		.y_odd_inc = 3,
		.binning_rawchip = 0x22,
	},
	{/*wide full size*/
		.x_output = 0xCD0,
		.y_output = 0x740,
		.line_length_pclk = 0xD8E,
#ifdef CONFIG_RAWCHIP
		.frame_length_lines = 0x960,  //0x764
#else
		.frame_length_lines = 0x960,  //0x750
#endif
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0x0130,
		.x_addr_end = 0xCCF,
		.y_addr_end = 0x86F,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
};

#ifdef CONFIG_ARCH_MSM8X60

static struct msm_camera_csi_params s5k3h2yx_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x2a,
};

static struct msm_camera_csi_params *s5k3h2yx_csi_params_array[] = {
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
};
#else
static struct msm_camera_csid_vc_cfg s5k3h2yx_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k3h2yx_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = s5k3h2yx_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};

static struct msm_camera_csi2_params *s5k3h2yx_csi_params_array[] = {
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params,
	&s5k3h2yx_csi_params
};
#endif
static struct msm_sensor_output_reg_addr_t s5k3h2yx_reg_addr = {
	.x_output = 0x34C,
	.y_output = 0x34E,
	.line_length_pclk = 0x342,
	.frame_length_lines = 0x340,
};

static struct msm_sensor_id_info_t s5k3h2yx_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x382B,
};

static struct msm_sensor_exp_gain_info_t s5k3h2yx_exp_gain_info = {
	.coarse_int_time_addr = 0x202,
	.global_gain_addr = 0x204,
	.vert_offset = 16,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
	.sensor_max_linecount = 65519, /* sensor max linecount = max unsigned value of linecount register size - vert_offset */ /* HTC ben 20120229 */
};

/*============================================================================
 TYPE DECLARATIONS
============================================================================*/
static struct msm_sensor_ctrl_t s5k3h2yx_s_ctrl;


/* 16bit address - 8 bit context register structure */
struct reg_addr_val_pair_struct {
        uint16_t reg_addr;
        uint8_t reg_val;
};

struct awb_lsc_struct_type {
       unsigned int caBuff[8];  /*awb_calibartion*/
	struct reg_addr_val_pair_struct LSC_table[150];  /*lsc_calibration*/
	uint32_t LSC_table_CRC;
};

enum s5k3h2yx_test_mode_t {
	TEST_OFF,
	TEST_1,
	TEST_2,
	TEST_3
};

enum s5k3h2yx_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	QVGA_SIZE,
	VIDEO_SIZE,
	FAST_VIDEO_SIZE,
	INVALID_SIZE
};

enum s5k3h2yx_reg_update_t{
	REG_INIT,
	REG_PERIODIC
};

static int preview_frame_count = 0;

static struct wake_lock s5k3h2yx_wake_lock;

static inline void init_suspend(void)
{
	wake_lock_init(&s5k3h2yx_wake_lock, WAKE_LOCK_IDLE, "s5k3h2yx");
}

static inline void deinit_suspend(void)
{
	wake_lock_destroy(&s5k3h2yx_wake_lock);
}

static inline void prevent_suspend(void)
{
	wake_lock(&s5k3h2yx_wake_lock);
}

static inline void allow_suspend(void)
{
	wake_unlock(&s5k3h2yx_wake_lock);
}

/*============================================================================
DATA DECLARATIONS
============================================================================*/

/*  96MHz PCLK @ 24MHz MCLK inc*/


/* FIXME: Changes from here */
struct s5k3h2yx_work {
  struct work_struct work;
};

static struct  i2c_client *s5k3h2yx_client;
static uint16_t s5k3h2yx_pos_tbl[S5K3H2YX_TOTAL_STEPS_NEAR_TO_FAR + 1];

struct s5k3h2yx_ctrl_t {
  const struct  msm_camera_sensor_info *sensordata;

  uint32_t sensormode;
  uint32_t fps_divider; /* init to 1 * 0x00000400 */
  uint32_t pict_fps_divider; /* init to 1 * 0x00000400 */
  uint16_t fps;

  int16_t  curr_lens_pos;
  uint16_t curr_step_pos;
  uint16_t init_curr_lens_pos;
  uint16_t my_reg_gain;
  uint32_t my_reg_line_count;
  uint16_t total_lines_per_frame;

  enum s5k3h2yx_resolution_t prev_res;
  enum s5k3h2yx_resolution_t pict_res;
  enum s5k3h2yx_resolution_t curr_res;
  enum s5k3h2yx_test_mode_t set_test;
  enum s5k3h2yx_reg_update_t reg_update;

  unsigned short imgaddr;
};


static struct s5k3h2yx_ctrl_t *s5k3h2yx_ctrl;
static struct platform_device *s5k3h2yx_pdev;

struct s5k3h2yx_waitevent{
	uint32_t waked_up;
	wait_queue_head_t event_wait;
};

static DECLARE_WAIT_QUEUE_HEAD(s5k3h2yx_wait_queue);
DEFINE_SEMAPHORE(s5k3h2yx_sem);


/*=============================================================*/

static int s5k3h2yx_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr  = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(s5k3h2yx_client->adapter, msgs, 2) < 0) {
		pr_err("[CAM]s5k3h2yx_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}
static int32_t s5k3h2yx_i2c_txdata(unsigned short saddr,
				 unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};
	if (i2c_transfer(s5k3h2yx_client->adapter, msg, 1) < 0) {
		pr_err("[CAM]s5k3h2yx_i2c_txdata failed 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

static int32_t s5k3h2yx_i2c_read_b(unsigned short saddr, unsigned short raddr,
	unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = s5k3h2yx_i2c_rxdata(saddr, buf, 1);
	if (rc < 0)
		return rc;

	*rdata = buf[0];

	if (rc < 0)
		pr_info("[CAM]s5k3h2yx_i2c_read failed!\n");

	return rc;
}


static int32_t s5k3h2yx_i2c_read(unsigned short raddr,
				unsigned short *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[2];
	int count = 0;
	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
retry:
	rc = s5k3h2yx_i2c_rxdata(s5k3h2yx_client->addr, buf, rlen);

	if (rc < 0) {
		pr_err("[CAM]s5k3h2yx_i2c_read 0x%x failed!\n", raddr);
		pr_err("[CAM] starting read retry policy count:%d\n", count);
		udelay(10);
		count++;
		if (count < 20) {
			if (count > 10)
				udelay(100);
		} else
			return rc;
		goto retry;
	}

	*rdata = (rlen == 2 ? buf[0] << 8 | buf[1] : buf[0]);
	return rc;
}


static int32_t s5k3h2yx_i2c_write_b(unsigned short saddr,
  unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];
	int count = 0;

	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;

retry:
	rc = s5k3h2yx_i2c_txdata(saddr, buf, 3);

	if (rc < 0) {
		pr_err("[CAM]i2c_write_b failed, addr = 0x%x, val = 0x%x\n",
			waddr, bdata);
		pr_err("[CAM]starting read retry policy count:%d\n", count);
		udelay(10);
		count++;
		if (count < 20) {
			if (count > 10)
			udelay(100);
		} else
			return rc;
		goto retry;
	}

	return rc;
}

static int s5k3h2yx_vreg_enable(struct platform_device *pdev)
{
  struct msm_camera_sensor_info *sdata = pdev->dev.platform_data;
  int rc;
	pr_info("[CAM]%s camera vreg on\n", __func__);

  if (sdata->camera_power_on == NULL) {
    pr_err("[CAM]sensor platform_data didnt register\n");
    return -EIO;
  }
  rc = sdata->camera_power_on();
  return rc;
}

static int s5k3h2yx_vreg_disable(struct platform_device *pdev)
{
  struct msm_camera_sensor_info *sdata = pdev->dev.platform_data;
  int rc;
	pr_info("[CAM]%s camera vreg off\n", __func__);

  if (sdata->camera_power_off == NULL) {
    pr_err("[CAM]sensor platform_data didnt register\n");
    return -EIO;
  }
  rc = sdata->camera_power_off();
  return rc;
}

static int s5k3h2yx_common_deinit(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;

	pr_info("[CAM]%s\n", __func__);

	if (data->sensor_pwd >= 0) {
		rc = gpio_request(data->sensor_pwd, "s5k3h2yx");
		if (!rc)
			gpio_direction_output(data->sensor_pwd, 0);
		else
			pr_err("[CAM]GPIO (%d) request failed\n", data->sensor_pwd);

		gpio_free(data->sensor_pwd);

	} else {
		pr_info("[CAM] RST pin on pm8058\n");
		if (data->camera_pm8058_power != NULL)
			if (data->camera_pm8058_power(0) < 0)
				pr_err("[CAM]camera_pm8058_power(0): request failed\n");
	}

	mdelay(1);

	if (data->vcm_pwd) {
	  if (data->gpio_set_value_force) {/* force to set gpio */
		  gpio_set_value(data->vcm_pwd, 0);
	  } else {
		rc = gpio_request(data->vcm_pwd, "s5k3h2yx");
		if (!rc)
			gpio_direction_output(data->vcm_pwd, 0);
		else
			pr_err("[CAM]GPIO (%d) request faile\n", data->vcm_pwd);
		gpio_free(data->vcm_pwd);
	  }
	}
	mdelay(1);

	data->pdata->camera_gpio_off();
	mdelay(1);

	if (!data->power_down_disable)
		s5k3h2yx_vreg_disable(s5k3h2yx_pdev);

	return 0;
}

static int s5k3h2yx_common_init(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	uint16_t chipid = 0;
	int count = 0;

	pr_info("[CAM]%s\n", __func__);

	pr_info("[CAM]%s  add open retry policy !!!\n", __func__);
retry:
	s5k3h2yx_vreg_enable(s5k3h2yx_pdev);

	/* switch MCLK to Main cam */
	if (data->camera_clk_switch != NULL)
		data->camera_clk_switch();

	data->pdata->camera_gpio_on();
	mdelay(1);

	if (data->sensor_pwd >= 0) {
		rc = gpio_request(data->sensor_pwd, "s5k3h2yx");
		if (!rc) {
			gpio_direction_output(data->sensor_pwd, 1);
		} else {
			pr_err("[CAM]GPIO (%d) request failed\n", data->sensor_pwd);
			goto init_fail;
		}
		gpio_free(data->sensor_pwd);

	} else {
		pr_info("[CAM] RST pin on pm8058\n");
		if (data->camera_pm8058_power != NULL)
			if (data->camera_pm8058_power(1) < 0)
				goto init_fail;
	}

	mdelay(1);

	if (data->vcm_pwd) {
	  if (data->gpio_set_value_force) {/* force to set gpio */
		gpio_set_value(data->vcm_pwd, 1);
	  } else {
		rc = gpio_request(data->vcm_pwd, "s5k3h2yx");
		if (!rc) {
			gpio_direction_output(data->vcm_pwd, 1);
		} else {
			pr_err("[CAM]GPIO (%d) request failed\n", data->vcm_pwd);
			goto init_fail;
		}
		gpio_free(data->vcm_pwd);
	  }
	}

	/* msleep(1); */
	/* msm_camio_probe_on(s5k3h2yx_pdev); */

	msleep(1);

	/* Read sensor Model ID: */
	rc = s5k3h2yx_i2c_read(S5K3H2YX_REG_MODEL_ID, &chipid, 2);
	if (rc < 0) {
		pr_err("[CAM]read sensor id fail\n");

		pr_err("[CAM]starting open retry policy count:%d\n", count);
		mdelay(10);
		count++;
		if (count < 10) {
			s5k3h2yx_common_deinit(data);
			mdelay(10);
		} else
			goto init_fail;
		goto retry;
	}

	/* Compare sensor ID to S5K3H2YX ID: */
	pr_info("[CAM]%s, Expected id=0x%x\n", __func__, S5K3H2YX_MODEL_ID);
	pr_info("[CAM]%s, Read id=0x%x\n", __func__, chipid);

	if (chipid != S5K3H2YX_MODEL_ID) {
		pr_err("[CAM]sensor model id is incorrect\n");
		rc = -ENODEV;
		goto init_fail;
	}

	pr_info("[CAM]s5k3h2yx_common_init done\n");
	goto init_done;

init_fail:
	pr_err("[CAM]s5k3h2yx_common_init failed\n");
init_done:
	return rc;
}

#if 0	/* HTC linear led 20111011 */
static int s5k3h2yx_i2c_read_focus_offest(uint16_t *pValue)
{
	int32_t  rc = 0;
	int page = 16;
	int addr_msb = 20;
	int addr_lsb = 21;
	unsigned short vcm_msb = 0, vcm_lsb = 0;

	/*Read Page 16*/
	rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A02, page);
	if (rc < 0) {
		pr_info("[CAM]%s: i2c_write_b 0x0A02 (select page %d) fail\n",
			__func__, page);
		goto get_done;
	}

	/* Set Read Mode */
	rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A00, 0x01);
	if (rc < 0) {
		pr_info("[CAM]%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);
		goto get_done;
	}

	rc = s5k3h2yx_i2c_read_b(s5k3h2yx_client->addr,
		(0x0A04 + addr_msb), &vcm_msb);
	if (rc < 0) {
		pr_info("[CAM]%s: i2c_read_b 0x%x fail\n",
			__func__, (0x0A04 + addr_msb));
		goto get_done;
	}

	rc = s5k3h2yx_i2c_read_b(s5k3h2yx_client->addr,
		(0x0A04 + addr_lsb), &vcm_lsb);
	if (rc < 0) {
		pr_info("[CAM]%s: i2c_read_b 0x%x fail\n",
			__func__, (0x0A04 + addr_lsb));
		goto get_done;
	}

	pr_info("[CAM] OTP VCM value: 0x%04X:[0x%X] | 0x%04X:[0x%X] = (%d)\n",
		(0x0A04 + addr_msb), vcm_msb, (0x0A04 + addr_lsb), vcm_lsb,
		((vcm_msb << 8) | vcm_lsb));

/*
	{
		int i = 0;
		unsigned short otp_data = 0;

		for (i = 0; i <= 63; i++) {
			rc = s5k3h2yx_i2c_read_b(s5k3h2yx_client->addr, (0x0A04 + i),
				&otp_data);
			if (rc < 0)
				pr_info("[CAM]%s: i2c_read_b 0x%x fail\n", __func__,
					(0x0A04 + i));
			pr_info("[CAM]%s: page:%d-data:%02d-addr:0x%04X = 0x%02X \n",
				__func__, 16, i, (0x0A04 + i), otp_data);
			otp_data = 0;
		}
	}
*/

get_done:
	rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A00, 0x00);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x0A00 (Stop) fail\n", __func__);

	if (rc == 0)
		*pValue = (uint16_t)((vcm_msb << 8) | vcm_lsb);
	else
		*pValue = 0;

	return rc;
}
#endif

static void s5k3h2yx_setup_af_tbl(void)
{
	uint32_t i;
	uint16_t s5k3h2yx_nl_region_boundary1 = 3;
	uint16_t s5k3h2yx_nl_region_boundary2 = 5;
	uint16_t s5k3h2yx_nl_region_code_per_step1 = 40;
	uint16_t s5k3h2yx_nl_region_code_per_step2 = 20;
	uint16_t s5k3h2yx_l_region_code_per_step = 16;
#if 0	/* HTC linear led 20111011 */
	uint16_t s5k3h2yx_focus_correction = 256;
	uint16_t s5k3h2yx_focus_offset = 0;

	s5k3h2yx_i2c_read_focus_offest(&s5k3h2yx_focus_offset);
	s5k3h2yx_pos_tbl[0] = (s5k3h2yx_focus_offset > s5k3h2yx_focus_correction) ?
		(s5k3h2yx_focus_offset - s5k3h2yx_focus_correction) : 0;
#else
	s5k3h2yx_pos_tbl[0] = 0;
#endif

	for (i = 1; i <= S5K3H2YX_TOTAL_STEPS_NEAR_TO_FAR; i++) {
		if (i <= s5k3h2yx_nl_region_boundary1)
			s5k3h2yx_pos_tbl[i] = s5k3h2yx_pos_tbl[i-1] +
			s5k3h2yx_nl_region_code_per_step1;
		else if (i <= s5k3h2yx_nl_region_boundary2)
			s5k3h2yx_pos_tbl[i] = s5k3h2yx_pos_tbl[i-1] +
				s5k3h2yx_nl_region_code_per_step2;
		else
			s5k3h2yx_pos_tbl[i] = s5k3h2yx_pos_tbl[i-1] +
				s5k3h2yx_l_region_code_per_step;
		/*
		pr_info("[CAM]%s: s5k3h2yx_pos_tbl[%02d@%02d] = (%03d)\n",
			__func__, i, (S5K3H2YX_TOTAL_STEPS_NEAR_TO_FAR - i),
			s5k3h2yx_pos_tbl[i]);
		*/
	}
}

static int32_t
s5k3h2yx_go_to_position(uint32_t lens_pos, uint8_t mask)
{
	int32_t rc = 0;
	unsigned char buf[2];
	uint8_t vcm_code_msb, vcm_code_lsb;

	vcm_code_msb = (lens_pos >> 8) & 0x3;
	vcm_code_lsb = lens_pos & 0xFF;

	buf[0] = S5K3H2YX_VCM_CODE_MSB;
	buf[1] = vcm_code_msb;

	rc = s5k3h2yx_i2c_txdata(S5K3H2YX_AF_I2C_ADDR >> 1, buf, 2);

	if (rc < 0)
		pr_err("[CAM]i2c_write failed, saddr = 0x%x addr = 0x%x, val =0x%x!\n", S5K3H2YX_AF_I2C_ADDR >> 1, buf[0], buf[1]);

	buf[0] = S5K3H2YX_VCM_CODE_LSB;
	buf[1] = vcm_code_lsb;

	rc = s5k3h2yx_i2c_txdata(S5K3H2YX_AF_I2C_ADDR >> 1, buf, 2);

	if (rc < 0)
		pr_err("[CAM]i2c_write failed, saddr = 0x%x addr = 0x%x, val =0x%x!\n", S5K3H2YX_AF_I2C_ADDR >> 1, buf[0], buf[1]);

	return rc;
}

static struct v4l2_subdev_core_ops s5k3h2yx_subdev_core_ops;
static struct v4l2_subdev_video_ops s5k3h2yx_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k3h2yx_subdev_ops = {
	.core = &s5k3h2yx_subdev_core_ops,
	.video  = &s5k3h2yx_subdev_video_ops,
};

static int s5k3h2yx_i2c_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{

	int32_t  rc;
	int page = 0;
	unsigned short info_value = 0, info_index = 0;
	unsigned short  OTP[10] = {0};

	pr_info("[CAM]%s: sensor OTP information:\n", __func__);
	/* testmode disable */
	rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x3A1C, 0x00);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x3A1C fail\n", __func__);

	/* Initialize */
	rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A00, 0x04);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x0A00 (Start) fail\n", __func__);

	mdelay(4);

	/*Read Page 20 to Page 16*/
	for (info_index = 0; info_index < 10; info_index++) {
		for (page = 20; page >= 16; page--) {
			rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A02, page);
			if (rc < 0)
				pr_info("[CAM]%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

			/* Set Read Mode */
			rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A00, 0x01);
			if (rc < 0)
				pr_info("[CAM]%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

			/* 0x0A04~0x0A0D: read Information 0~9 according to SPEC*/
			rc = s5k3h2yx_i2c_read_b(s5k3h2yx_client->addr, (0x0A04 + info_index), &info_value);
			if (rc < 0)
				pr_info("[CAM]%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

			 /* some values of fuseid are maybe zero */
			if (((info_value&0x0F) != 0) || page == 0)
				break;
		}
		OTP[info_index] = (short)(info_value&0x0F);
		info_value = 0;
	}

	if (OTP[0] != 0 && OTP[1] != 0) {
		pr_info("[CAM] Get Fuseid from Page20 to Page16\n");
		goto get_done;
	}

	/*Read Page 4 to Page 0*/
	memset(OTP, 0, sizeof(OTP));
	for (info_index = 0; info_index < 10; info_index++) {
		for (page = 4; page >= 0; page--) {
			rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A02, page);
			if (rc < 0)
				pr_info("[CAM]%s: i2c_write_b 0x0A02 (select page %d) fail\n", __func__, page);

			/* Set Read Mode */
			rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A00, 0x01);
			if (rc < 0)
				pr_info("[CAM]%s: i2c_write_b 0x0A00: Set read mode fail\n", __func__);

			/* 0x0A04~0x0A0D: read Information 0~9 according to SPEC*/
			rc = s5k3h2yx_i2c_read_b(s5k3h2yx_client->addr, (0x0A04 + info_index), &info_value);
			if (rc < 0)
				pr_info("[CAM]%s: i2c_read_b 0x%x fail\n", __func__, (0x0A04 + info_index));

			 /* some values of fuseid are maybe zero */
			if (((info_value & 0x0F) != 0) || page == 0)
				break;
		}
		OTP[info_index] = (short)(info_value&0x0F);
		info_value = 0;
	}

get_done:
	/* interface disable */
	rc = s5k3h2yx_i2c_write_b(s5k3h2yx_client->addr, 0x0A00, 0x00);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_b 0x0A00 (Stop) fail\n", __func__);

	pr_info("[CAM]%s: VenderID=%x,LensID=%x,SensorID=%x%x\n", __func__,
		OTP[0], OTP[1], OTP[2], OTP[3]);
	pr_info("[CAM]%s: ModuleFuseID= %x%x%x%x%x%x\n", __func__,
		OTP[4], OTP[5], OTP[6], OTP[7], OTP[8], OTP[9]);

    cdata->cfg.fuse.fuse_id_word1 = 0;
    cdata->cfg.fuse.fuse_id_word2 = 0;
	cdata->cfg.fuse.fuse_id_word3 = (OTP[0]);
	cdata->cfg.fuse.fuse_id_word4 =
		(OTP[4]<<20) |
		(OTP[5]<<16) |
		(OTP[6]<<12) |
		(OTP[7]<<8) |
		(OTP[8]<<4) |
		(OTP[9]);

	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word1:%d\n",
		cdata->cfg.fuse.fuse_id_word1);
	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word2:%d\n",
		cdata->cfg.fuse.fuse_id_word2);
	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word3:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word3);
	pr_info("[CAM]s5k3h2yx: fuse->fuse_id_word4:0x%08x\n",
		cdata->cfg.fuse.fuse_id_word4);
	return 0;
}

static int s5k3h2yx_sensor_config(void __user *argp)
{
        return msm_sensor_config(&s5k3h2yx_s_ctrl, argp);
}

static int s5k3h2yx_sensor_release(void)
{
        int     rc = 0;
        rc = msm_sensor_release(&s5k3h2yx_s_ctrl);
        return rc;
}

static int s5k3h2yx_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;

	pr_info("[CAM]Calling s5k3h2yx_sensor_open_init\n");

	down(&s5k3h2yx_sem);

	if (data == NULL) {
		pr_info("[CAM]data is a NULL pointer\n");
		goto init_fail;
	}

	s5k3h2yx_ctrl = kzalloc(sizeof(struct s5k3h2yx_ctrl_t), GFP_KERNEL);

	if (!s5k3h2yx_ctrl) {
		rc = -ENOMEM;
		goto init_fail;
	}

	s5k3h2yx_ctrl->curr_lens_pos = -1;
	s5k3h2yx_ctrl->fps_divider = 1 * 0x00000400;
	s5k3h2yx_ctrl->pict_fps_divider = 1 * 0x00000400;
	s5k3h2yx_ctrl->fps = 30 * Q8;
	s5k3h2yx_ctrl->set_test = TEST_OFF;
	s5k3h2yx_ctrl->prev_res = QTR_SIZE;
	s5k3h2yx_ctrl->pict_res = FULL_SIZE;
	s5k3h2yx_ctrl->curr_res = INVALID_SIZE;
	s5k3h2yx_ctrl->reg_update = REG_INIT;
	s5k3h2yx_ctrl->sensordata = data;
	s5k3h2yx_ctrl->my_reg_gain = 0;
	s5k3h2yx_ctrl->my_reg_line_count = 0;

	rc = s5k3h2yx_common_init(data);

	if (rc < 0)
		goto init_fail;

	if (rc < 0)
		goto init_fail;

	/* rc = s5k3h2yx_i2c_write_b( s5k3h2yx_client->addr, S5K3H2YX_REG_MODE_SELECT, S5K3H2YX_MODE_SELECT_STREAM);
	if (rc < 0)
		goto init_fail; */

	/* set up lens position table */
	s5k3h2yx_setup_af_tbl();
	s5k3h2yx_go_to_position(0, 0);
	s5k3h2yx_ctrl->curr_lens_pos = 0;
	s5k3h2yx_ctrl->curr_step_pos = 0;

	pr_info("[CAM]s5k3h2yx_sensor_open_init done\n");
		goto init_done;

init_fail:
	pr_err("[CAM]s5k3h2yx_sensor_open_init failed\n");
init_done:
	up(&s5k3h2yx_sem);
	return rc;
} /* end of s5k3h2yx_sensor_open_init */

static const struct i2c_device_id s5k3h2yx_i2c_id[] = {
        {SENSOR_NAME, (kernel_ulong_t)&s5k3h2yx_s_ctrl},
        { }
};

static int __exit s5k3h2yx_i2c_remove(struct i2c_client *client)
{
	struct s5k3h2yx_work_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	deinit_suspend();
	s5k3h2yx_client = NULL;
	kfree(sensorw);
	sensorw = NULL;
	return 0;
}

static const char *S5K3H2YXVendor = "samsung";
static const char *S5K3H2YXNAME = "S5K3H2YX";
static const char *S5K3H2YXSize = "8M";


static ssize_t sensor_vendor_show(struct device *dev,
  struct device_attribute *attr, char *buf)
{
  ssize_t ret = 0;

  sprintf(buf, "%s %s %s\n", S5K3H2YXVendor, S5K3H2YXNAME, S5K3H2YXSize);
  ret = strlen(buf) + 1;

  return ret;
}


static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);


static struct kobject *android_s5k3h2yx = NULL;

static int s5k3h2yx_sysfs_init(void)
{
  int ret = 0;
  pr_info("[CAM]s5k3h2yx:kobject creat and add\n");
  android_s5k3h2yx = kobject_create_and_add("android_camera", NULL);
  if (android_s5k3h2yx == NULL) {
    pr_info("[CAM]s5k3h2yx_sysfs_init: subsystem_register failed\n");
    ret = -ENOMEM;
    return ret ;
  }
  pr_info("[CAM]s5k3h2yx:sysfs_create_file\n");
  ret = sysfs_create_file(android_s5k3h2yx, &dev_attr_sensor.attr);
  if (ret) {
    pr_info("[CAM]s5k3h2yx_sysfs_init: sysfs_create_file failed\n");
    ret = -EFAULT;
    goto error;
  }

  return ret;

error:
  kobject_del(android_s5k3h2yx);
  return ret;
}

uint8_t s5k3h2yx_preview_skip_frame(void)
{
	if (s5k3h2yx_ctrl->sensormode == SENSOR_PREVIEW_MODE
		&& preview_frame_count < 2) {
		preview_frame_count++;
		return 1;
	}
	return 0;
}

static struct msm_camera_i2c_client s5k3h2yx_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static struct i2c_driver s5k3h2yx_i2c_driver = {
        .id_table = s5k3h2yx_i2c_id,
        .probe  = msm_sensor_i2c_probe,
        .driver = {
                .name = SENSOR_NAME,
        },
};

static int s5k3h2yx_sensor_probe(struct msm_sensor_ctrl_t *s_ctrl, const struct msm_camera_sensor_info *info,
  struct msm_sensor_ctrl *s)
{
	int rc = 0;
	pr_info("[CAM]s5k3h2yx_sensor_probe()\n");

	if (info == NULL) {
		pr_info("[CAM]info is a NULL pointer\n");
		goto probe_fail;
	}

	rc = i2c_add_driver(&s5k3h2yx_i2c_driver);
	if (rc < 0 || s5k3h2yx_client == NULL) {
		rc = -ENOTSUPP;
		/*goto probe_fail;*/
		pr_err("[CAM]__s5k3h2yx_probe, rc < 0 or s5k3h2yx_client == NULL\n");
		return rc;
	}

	rc = s5k3h2yx_common_init(info);
	if (rc < 0)
		goto probe_fail;

	init_suspend();
	s->s_init = s5k3h2yx_sensor_open_init;
	s->s_release = s5k3h2yx_sensor_release;
	s->s_config  = s5k3h2yx_sensor_config;
	s5k3h2yx_sysfs_init();

	pr_info("[CAM]s5k3h2yx_sensor_probe done\n");
	goto probe_done;

probe_fail:
	pr_err("[CAM]s5k3h2yx_sensor_probe failed\n");
probe_done:
	if (info)
		s5k3h2yx_common_deinit(info);
	return rc;
}

static int s5k3h2yx_sensor_v4l2_probe(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = -EINVAL;

	rc = msm_sensor_v4l2_probe(&s5k3h2yx_s_ctrl, info, sdev, s);

	return rc;
}

static int s5k3h2yx_probe(struct platform_device *pdev)
{
	int	rc = 0;

	pr_info("%s\n", __func__);

	rc = msm_sensor_register(pdev, s5k3h2yx_sensor_v4l2_probe);
	if(rc >= 0)
		s5k3h2yx_sysfs_init();
	return rc;
}

struct platform_driver s5k3h2yx_driver = {
	.probe = s5k3h2yx_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	return platform_driver_register(&s5k3h2yx_driver);
}

static struct msm_sensor_fn_t s5k3h2yx_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
#ifdef CONFIG_ARCH_MSM8X60
	.sensor_setting = msm_sensor_setting1,
#else
	.sensor_setting = msm_sensor_setting,
#endif
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = s5k3h2yx_sensor_config,
	.sensor_open_init = s5k3h2yx_sensor_open_init,
	.sensor_release = s5k3h2yx_sensor_release,
	.sensor_power_up = s5k3h2yx_common_init,
	.sensor_power_down = s5k3h2yx_common_deinit,
	.sensor_probe = s5k3h2yx_sensor_probe,
	.sensor_i2c_read_fuseid = s5k3h2yx_i2c_read_fuseid,
	/* HTC_START Awii 20120306 */
//	.sensor_i2c_read_vcm_clib = s5k3h2yx_read_vcm_clib,
	/* HTC_END*/
};


static struct msm_sensor_reg_t s5k3h2yx_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k3h2yx_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k3h2yx_start_settings),
	.stop_stream_conf = s5k3h2yx_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k3h2yx_stop_settings),
	.group_hold_on_conf = s5k3h2yx_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k3h2yx_groupon_settings),
	.group_hold_off_conf = s5k3h2yx_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k3h2yx_groupoff_settings),
	.init_settings = &s5k3h2yx_init_conf[0],
	.init_size = ARRAY_SIZE(s5k3h2yx_init_conf),
	.mode_settings = &s5k3h2yx_confs[0],
	.output_settings = &s5k3h2yx_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k3h2yx_confs),
};


static struct msm_sensor_ctrl_t s5k3h2yx_s_ctrl = {
	.msm_sensor_reg = &s5k3h2yx_regs,
	.sensor_i2c_client = &s5k3h2yx_sensor_i2c_client,
	.sensor_i2c_addr = 0x20,
	.sensor_output_reg_addr = &s5k3h2yx_reg_addr,
	.sensor_id_info = &s5k3h2yx_id_info,
	.sensor_exp_gain_info = &s5k3h2yx_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
#ifdef CONFIG_ARCH_MSM8X60
	.csic_params = &s5k3h2yx_csi_params_array[0],
#else
	.csi_params = &s5k3h2yx_csi_params_array[0],
#endif
	.msm_sensor_mutex = &s5k3h2yx_mut,
	.sensor_i2c_driver = &s5k3h2yx_i2c_driver,
	.sensor_v4l2_subdev_info = s5k3h2yx_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k3h2yx_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k3h2yx_subdev_ops,
	.func_tbl = &s5k3h2yx_func_tbl,
};

module_init(msm_sensor_init_module);

