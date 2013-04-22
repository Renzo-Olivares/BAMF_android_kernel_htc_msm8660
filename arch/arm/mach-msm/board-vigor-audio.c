/* linux/arch/arm/mach-msm/board-vigor-audio.c
 *
 * Copyright (C) 2010-2011 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/android_pmem.h>
#include <linux/mfd/pmic8058.h>
#include <linux/delay.h>
#include <linux/pmic8058-othc.h>
#include <linux/spi/spi_aic3254.h>
#include <linux/regulator/consumer.h>

#include <mach/gpio.h>
#include <mach/dal.h>
#include <mach/tpa2051d3.h>
#include "qdsp6v2/snddev_icodec.h"
#include "qdsp6v2/snddev_ecodec.h"
#include "qdsp6v2/snddev_hdmi.h"
#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <sound/apr_audio.h>
#include <sound/q6asm.h>
#include <mach/htc_acoustic_8x60.h>
#include <mach/board_htc.h>

#include "board-vigor.h"

#define PM8058_GPIO_BASE					NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)
#define BIT_SPEAKER    (1 << 0)
#define BIT_HEADSET    (1 << 1)
#define BIT_RECEIVER	(1 << 2)
#define BIT_FM_SPK		(1 << 3)
#define BIT_FM_HS		(1 << 4)

void vigor_snddev_bmic_pamp_on(int en);

static uint32_t msm_aic3254_reset_gpio[] = {
	      GPIO_CFG(VIGOR_AUD_CODEC_RST, 0, GPIO_CFG_OUTPUT,
	      GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
	  };
	  
// static uint32_t msm_snddev_gpio[] = {
// 	GPIO_CFG(108, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
// 	GPIO_CFG(109, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
// 	GPIO_CFG(110, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
// };

// static uint32_t msm_spi_gpio[] = {
// 	GPIO_CFG(VIGOR_SPI_DO,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
// 	GPIO_CFG(VIGOR_SPI_DI,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
// 	GPIO_CFG(VIGOR_SPI_CS,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
// 	GPIO_CFG(VIGOR_SPI_CLK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
// };

static struct regulator *snddev_reg_l11;

void vigor_mic_bias_on(int en)
{
	int rc;
	pr_aud_info("%s\n", __func__);

	if (en) {
		snddev_reg_l11 = regulator_get(NULL, "8058_l11");
		if (IS_ERR(snddev_reg_l11)) {
			pr_aud_err("%s: regulator_get(%s) failed (%ld)\n",
				__func__, "8058_l11", PTR_ERR(snddev_reg_l11));
			return;
		}

		rc = regulator_set_voltage(snddev_reg_l11, 2850000, 2850000);
		if (rc < 0)
			pr_aud_err("%s: regulator_set_voltage(8058_l11) failed (%d)\n",
				__func__, rc);

		rc = regulator_enable(snddev_reg_l11);
		if (rc < 0)
			pr_aud_err("%s: regulator_enable(8058_l11) failed (%d)\n",
				__func__, rc);
	} else {

		if (!snddev_reg_l11)
			return;

		rc = regulator_disable(snddev_reg_l11);
		if (rc < 0)
			pr_aud_err("%s: regulator_disable(8058_l11) failed (%d)\n",
					__func__, rc);
		regulator_put(snddev_reg_l11);

		snddev_reg_l11 = NULL;
	}
}

static struct mutex mic_lock;
static atomic_t q6_effect_mode = ATOMIC_INIT(-1);
static int curr_rx_mode;
static atomic_t aic3254_ctl = ATOMIC_INIT(0);


void vigor_snddev_poweramp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);
	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_AMP_EN), 1);
		set_speaker_amp(1);
		if (!atomic_read(&aic3254_ctl))
       curr_rx_mode |= BIT_SPEAKER;
	} else {
		set_speaker_amp(0);
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_AMP_EN), 0);
		if (!atomic_read(&aic3254_ctl))
       curr_rx_mode &= ~BIT_SPEAKER;
	}
}

void vigor_snddev_usb_headset_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);
	if (en) {
	} else {
	}
}

void vigor_snddev_hsed_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);
	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_AMP_EN), 1);
		if (!atomic_read(&aic3254_ctl))
		  curr_rx_mode |= BIT_HEADSET;
	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_AMP_EN), 0);
		 if (!atomic_read(&aic3254_ctl))
       curr_rx_mode &= ~BIT_HEADSET;
	}
}

void vigor_snddev_hs_spk_pamp_on(int en)
{
	vigor_snddev_poweramp_on(en);
	vigor_snddev_hsed_pamp_on(en);
}

void vigor_snddev_receiver_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);
	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_REC_EN), 1);
	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_REC_EN), 0);
	}
}

/* power on/off externnal mic bias */
void vigor_mic_enable(int en, int shift)
{
	pr_aud_info("%s: %d, shift %d\n", __func__, en, shift);

	mutex_lock(&mic_lock);

	if (en)
		vigor_mic_bias_on(en);
	else
		vigor_mic_bias_on(en);

	mutex_unlock(&mic_lock);
}

void vigor_snddev_imic_pamp_on(int en)
{
	int ret;

	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		ret = pm8058_micbias_enable(OTHC_MICBIAS_0, OTHC_SIGNAL_ALWAYS_ON);
		if (ret)
			pr_aud_err("%s: Enabling int mic power failed\n", __func__);

	} else {
		ret = pm8058_micbias_enable(OTHC_MICBIAS_0, OTHC_SIGNAL_OFF);
		if (ret)
			pr_aud_err("%s: Enabling int mic power failed\n", __func__);

	}
}

void vigor_snddev_bmic_pamp_on(int en)
{
	int ret;
	
	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		ret = pm8058_micbias_enable(OTHC_MICBIAS_1, OTHC_SIGNAL_ALWAYS_ON);
		if (ret)
			pr_aud_err("%s: Enabling back mic power failed\n", __func__);

		/* select external mic path */
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_MIC_SEL), 0);

	} else {
		ret = pm8058_micbias_enable(OTHC_MICBIAS_1, OTHC_SIGNAL_OFF);
		if (ret)
			pr_aud_err("%s: Enabling back mic power failed\n", __func__);

		/* select external mic path */
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_MIC_SEL), 0);

	}
}

void vigor_snddev_stereo_mic_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		vigor_snddev_imic_pamp_on(en);
		vigor_snddev_bmic_pamp_on(en);
	} else {
		vigor_snddev_imic_pamp_on(en);
		vigor_snddev_bmic_pamp_on(en);
	}
}

void vigor_snddev_emic_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		/* select external mic path */
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_MIC_SEL), 1);

	} else {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_MIC_SEL), 0);

	}
}

void vigor_snddev_fmspk_pamp_on(int en)
{
	vigor_snddev_poweramp_on(en);
}

void vigor_snddev_fmhs_pamp_on(int en)
{
	pr_aud_info("%s %d\n", __func__, en);

	if (en) {
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_HP_EN), 1);
		set_headset_amp(1);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode |= BIT_FM_HS;
	} else {
		set_headset_amp(0);
		gpio_set_value(PM8058_GPIO_PM_TO_SYS(VIGOR_AUD_HP_EN), 0);
		if (!atomic_read(&aic3254_ctl))
			curr_rx_mode &= ~BIT_FM_HS;
	}
}

static struct regulator *snddev_reg_ncp;

void vigor_voltage_on (int en)
{
	int rc;
	pr_aud_info("%s\n", __func__);

	if (en) {
		snddev_reg_ncp = regulator_get(NULL, "8058_ncp");
		if (IS_ERR(snddev_reg_ncp)) {
			pr_aud_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
				"ncp", PTR_ERR(snddev_reg_ncp));
			return;
		}

		rc = regulator_set_voltage(snddev_reg_ncp, 1800000, 1800000);
		if (rc < 0)
			pr_aud_err("%s: regulator_set_voltage(ncp) failed (%d)\n",
				__func__, rc);

		rc = regulator_enable(snddev_reg_ncp);
		if (rc < 0)
			pr_aud_err("%s: regulator_enable(ncp) failed (%d)\n",
				__func__, rc);
	} else {

		if (!snddev_reg_ncp)
			return;

		rc = regulator_disable(snddev_reg_ncp);
		if (rc < 0)
			pr_aud_err("%s: regulator_disable(ncp) failed (%d)\n",
					__func__, rc);
		regulator_put(snddev_reg_ncp);

		snddev_reg_ncp = NULL;
	}
}

void vigor_rx_amp_enable(int en)
{
	if (curr_rx_mode != 0) {
		atomic_set(&aic3254_ctl, 1);
		pr_aud_info("%s: curr_rx_mode 0x%x, en %d\n",
			__func__, curr_rx_mode, en);
		if (curr_rx_mode & BIT_SPEAKER)
			vigor_snddev_poweramp_on(en);
		if (curr_rx_mode & BIT_HEADSET)
			vigor_snddev_hsed_pamp_on(en);
		if (curr_rx_mode & BIT_RECEIVER)
			vigor_snddev_receiver_pamp_on(en);
		if (curr_rx_mode & BIT_FM_SPK)
			vigor_snddev_fmspk_pamp_on(en);
		if (curr_rx_mode & BIT_FM_HS)
			vigor_snddev_fmhs_pamp_on(en);
		atomic_set(&aic3254_ctl, 0);
	}
}

int vigor_get_speaker_channels(void)
{
	/* 1 - Mono, 2 - Stereo */
	return 1;
}

int vigor_support_beats(void)
{
	/* HW revision support 1V output from headset */
	if (get_engineerid() > 2)
		return 1;
	else
		return 0;
}

int vigor_support_adie(void)
{
	return 0;
}

int vigor_support_back_mic(void)
{
	return 0;
}

void vigor_enable_beats(int en)
{
	pr_aud_info("%s: %d\n", __func__, en);
	set_beats_on(en);
}

void vigor_reset_3254(void)
{
	gpio_tlmm_config(msm_aic3254_reset_gpio[0], GPIO_CFG_ENABLE);
	gpio_set_value(VIGOR_AUD_CODEC_RST, 0);
	mdelay(1);
	gpio_set_value(VIGOR_AUD_CODEC_RST, 1);
}

void vigor_set_q6_effect_mode(int mode)
{
	pr_aud_info("%s: mode %d\n", __func__, mode);
	atomic_set(&q6_effect_mode, mode);
}

int vigor_get_q6_effect_mode(void)
{
	int mode = atomic_read(&q6_effect_mode);
	pr_aud_info("%s: mode %d\n", __func__, mode);
	return mode;
}

static struct q6v2audio_analog_ops ops = {
	.speaker_enable	        = vigor_snddev_poweramp_on,
	.headset_enable	        = vigor_snddev_hsed_pamp_on,
	.handset_enable	        = vigor_snddev_receiver_pamp_on,
	.headset_speaker_enable	= vigor_snddev_hs_spk_pamp_on,
	.int_mic_enable         = vigor_snddev_imic_pamp_on,
	.back_mic_enable        = vigor_snddev_bmic_pamp_on,
	.ext_mic_enable         = vigor_snddev_emic_pamp_on,
	.fm_headset_enable      = vigor_snddev_fmhs_pamp_on,
	.fm_speaker_enable      = vigor_snddev_fmspk_pamp_on,
	.stereo_mic_enable      = vigor_snddev_stereo_mic_pamp_on,
	.usb_headset_enable     = vigor_snddev_usb_headset_pamp_on,
	.voltage_on             = vigor_voltage_on,
};

static struct aic3254_ctl_ops cops = {
	.rx_amp_enable        = vigor_rx_amp_enable,
	.reset_3254           = vigor_reset_3254,
};

static struct acoustic_ops acoustic = {
	.enable_mic_bias = vigor_mic_enable,
	.support_adie = vigor_support_adie,
	.support_back_mic = vigor_support_back_mic,
	.get_speaker_channels = vigor_get_speaker_channels,
	.support_beats = vigor_support_beats,
	.enable_beats = vigor_enable_beats,
	.set_q6_effect = vigor_set_q6_effect_mode,
};

void vigor_aic3254_set_mode(int config, int mode)
{
	aic3254_set_mode(config, mode);
}

static struct q6v2audio_aic3254_ops aops = {
       .aic3254_set_mode = vigor_aic3254_set_mode,
};

static struct q6asm_ops qops = {
	.get_q6_effect = vigor_get_q6_effect_mode,
};

void __init vigor_audio_init(void)
{
	mutex_init(&mic_lock);

	pr_aud_info("%s\n", __func__);
	htc_8x60_register_analog_ops(&ops);
	htc_register_q6asm_ops(&qops);
	acoustic_register_ops(&acoustic);
	htc_8x60_register_aic3254_ops(&aops);
	msm_set_voc_freq(8000, 8000);	
	aic3254_register_ctl_ops(&cops);

	vigor_reset_3254();
}
