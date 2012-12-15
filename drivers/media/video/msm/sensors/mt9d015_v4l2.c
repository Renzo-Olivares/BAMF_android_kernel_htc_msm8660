#include "msm_sensor.h"
#include "msm.h"

#define SENSOR_NAME "mt9d015"
#define PLATFORM_DRIVER_NAME "msm_camera_mt9d015"
#define mt9d015_obj mt9d015_##obj

DEFINE_MUTEX(mt9d015_mut);
DEFINE_MUTEX(mt9d015_sensor_init_mut); //CC120826,
static struct msm_sensor_ctrl_t mt9d015_s_ctrl;

static struct msm_camera_i2c_reg_conf mt9d015_start_settings[] = {
    {0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf mt9d015_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf mt9d015_groupon_settings[] = {
	{0x104, 0x01},
};

static struct msm_camera_i2c_reg_conf mt9d015_groupoff_settings[] = {
	{0x104, 0x00},
};

static struct msm_camera_i2c_reg_conf mt9d015_prev_settings[] = {
	/*Timing configuration*/
	{0x0200, 0x01E5},/*FINE_INTEGRATION_TIME_*/
	{0x0202, 0x0543},/*COARSE_INTEGRATION_TIME*/
	{0x3010, 0x0094},/*FINE_CORRECTION*//*0426 by Micro FAE*/
	{0x0340, 0x054B},/*FRAME_LENGTH_LINES*/
	{0x0342, 0x096C},/*LINE_LENGTH_PCK*/
	/*Output Size (1612x1208)*/
	{0x0344, 0x0000},/*X_ADDR_START*/
	{0x0346, 0x0000},/*Y_ADDR_START*/
	{0x0348, 0x064B},/*X_ADDR_END*/
	{0x034A, 0x04B7},/*Y_ADDR_END*/
	{0x034C, 0x064C},/*X_OUTPUT_SIZE*/
	{0x034E, 0x04B8},/*Y_OUTPUT_SIZE*/
	{0x0400, 0x0000},/*SCALING_MODE*/
	{0x0402, 0x0000},/*SPATIAL_SAMPLING_BAYER*/
	{0x0404, 0x0010},/*SCALE_M*/
};

static struct msm_camera_i2c_reg_conf mt9d015_snap_settings[] = {
	/*Timing configuration*/
	{0x0200, 0x01E5},/*FINE_INTEGRATION_TIME_*/
	{0x0202, 0x0543},/*COARSE_INTEGRATION_TIME*/
	{0x3010, 0x0094},/*FINE_CORRECTION*//*0426 by Micro FAE*/
	{0x0340, 0x054B},/*FRAME_LENGTH_LINES*/
	{0x0342, 0x096C},/*LINE_LENGTH_PCK*/
	/*Output Size (1612x1208*/
	{0x0344, 0x0000},/*X_ADDR_START*/
	{0x0346, 0x0000},/*Y_ADDR_START*/
	{0x0348, 0x064B},/*X_ADDR_END*/
	{0x034A, 0x04B7},/*Y_ADDR_END*/
	{0x034C, 0x064C},/*X_OUTPUT_SIZE*/
	{0x034E, 0x04B8},/*Y_OUTPUT_SIZE*/
	{0x0400, 0x0000},/*SCALING_MODE*/
	{0x0402, 0x0000},/*SPATIAL_SAMPLING_BAYER*/
	{0x0404, 0x0010},/*SCALE_M*/
};

static struct msm_camera_i2c_reg_conf mt9d015_recommend_settings[] = {
    {0x3E00, 0x0430},
	{0x3E02, 0x3FFF},
	{0x3E1E, 0x67CA},
	{0x3E2A, 0xCA67},
	{0x3E2E, 0x8054},
	{0x3E30, 0x8255},
	{0x3E32, 0x8410},
	{0x3E36, 0x5FB0},
	{0x3E38, 0x4C82},
	{0x3E3A, 0x4DB0},
	{0x3E3C, 0x5F82},
	{0x3E3E, 0x1170},
	{0x3E40, 0x8055},
	{0x3E42, 0x8061},
	{0x3E44, 0x68D8},
	{0x3E46, 0x6882},
	{0x3E48, 0x6182},
	{0x3E4A, 0x4D82},
	{0x3E4C, 0x4C82},
	{0x3E4E, 0x6368},
	{0x3E50, 0xD868},
	{0x3E52, 0x8263},
	{0x3E54, 0x824D},
	{0x3E56, 0x8203},
	{0x3E58, 0x9D66},
	{0x3E5A, 0x8045},
	{0x3E5C, 0x4E7C},
	{0x3E5E, 0x0970},
	{0x3E60, 0x8072},
	{0x3E62, 0x5484},
	{0x3E64, 0x2037},
	{0x3E66, 0x8216},
	{0x3E68, 0x0486},
	{0x3E6A, 0x1070},
	{0x3E6C, 0x825E},
	{0x3E6E, 0xEE54},
	{0x3E70, 0x825E},
	{0x3E72, 0x8212},
	{0x3E74, 0x7086},
	{0x3E76, 0x1404},
	{0x3E78, 0x8220},
	{0x3E7A, 0x377C},
	{0x3E7C, 0x6170},
	{0x3E7E, 0x8082},
	{0x3E80, 0x4F82},
	{0x3E82, 0x4E82},
	{0x3E84, 0x5FCA},
	{0x3E86, 0x5F82},
	{0x3E88, 0x4E82},
	{0x3E8A, 0x4F81},
	{0x3E8C, 0x7C7F},
	{0x3E8E, 0x7000},
	{0x30D4, 0xE200}, /*{0x30D4, 0xC200},*/
	{0x3174, 0x8000},
	{0x3EE0, 0x0020},
	{0x3EE2, 0x0016},
	{0x3F00, 0x0002},
	{0x3F02, 0x0028},
	{0x3F0A, 0x0300},
	{0x3F0C, 0x1008},
	{0x3F10, 0x0405},
	{0x3F12, 0x0101}, /*{0x3F12, 0x021E},*/
	{0x3F14, 0x0000},

};

static struct msm_sensor_output_info_t mt9d015_dimensions[] = {
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C2E,
		.frame_length_lines = 0x0495,
		.vt_pixel_clk = 109714286,
		.op_pixel_clk = 76800000,//76800000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x07A7-0x0020,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
	{
		.x_output = 0x0788,
		.y_output = 0x0440,
		.line_length_pclk = 0x0C2E,
		.frame_length_lines = 0x0495,
		.vt_pixel_clk = 109714286,
		.op_pixel_clk = 76800000,//76800000,
		.binning_factor = 1,
		.x_addr_start = 0,
		.y_addr_start = 0,
		.x_addr_end = 0x07A7-0x0020,
		.y_addr_end = 0x0440-1,
		.x_even_inc = 1,
		.x_odd_inc = 1,
		.y_even_inc = 1,
		.y_odd_inc = 1,
		.binning_rawchip = 0x11,
	},
};

static struct msm_sensor_output_reg_addr_t mt9d015_reg_addr = {
	.x_output = 0xC86C,
	.y_output = 0xC86E,
	.line_length_pclk = 0xC814,
	.frame_length_lines = 0xC812,
};
static struct msm_sensor_exp_gain_info_t mt9d015_exp_gain_info = {
	.coarse_int_time_addr = 0x3012,
	.global_gain_addr = 0x305E,
	.vert_offset = 4,
	.min_vert = 4, /* min coarse integration time */ /* HTC Angie 20111019 - Fix FPS */
};


static struct v4l2_subdev_info mt9d015_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array mt9d015_init_conf[] = {
	{&mt9d015_recommend_settings[0],
	ARRAY_SIZE(mt9d015_recommend_settings), 0, MSM_CAMERA_I2C_WORD_DATA}
};

static struct msm_camera_i2c_conf_array mt9d015_confs[] = {
	{&mt9d015_snap_settings[0],
	ARRAY_SIZE(mt9d015_snap_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
	{&mt9d015_prev_settings[0],
	ARRAY_SIZE(mt9d015_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};


static struct msm_camera_csi_params mt9d015_csi_params = {
		.data_format = CSI_10BIT,
		.lane_cnt = 1,
		.lane_assign = 0xe4,
		.dpcm_scheme = 0,
                .settle_cnt = 20,
		.mipi_driving_strength = 0,
		.hs_impedence = 0x0F,
};

static struct msm_camera_csi_params *mt9d015_csi_params_array[] = {
	&mt9d015_csi_params,
	&mt9d015_csi_params,
};

static struct msm_sensor_id_info_t mt9d015_id_info = {
	.sensor_id_reg_addr = 0x0,
	.sensor_id = 0x1501,
};

#if 0	/* HTC_START for i2c operation */

static struct i2c_client *mt9d015_client;

#define MAX_I2C_RETRIES 20
#define CHECK_STATE_TIME 100

enum mt9d015_width {
	WORD_LEN,
	BYTE_LEN
};


static int i2c_transfer_retry(struct i2c_adapter *adap,
			struct i2c_msg *msgs,
			int len)
{
	int i2c_retry = 0;
	int ns; /* number sent */

	while (i2c_retry++ < MAX_I2C_RETRIES) {
		ns = i2c_transfer(adap, msgs, len);
		if (ns == len)
			break;
		pr_err("%s: try %d/%d: i2c_transfer sent: %d, len %d\n",
			__func__,
			i2c_retry, MAX_I2C_RETRIES, ns, len);
		msleep(10);
	}

	return ns == len ? 0 : -EIO;
}

static int mt9d015_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer_retry(mt9d015_client->adapter, msg, 1) < 0) {
		pr_err("mt9d015_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int mt9d015_i2c_write(unsigned short saddr,
				 unsigned short waddr, unsigned short wdata,
				 enum mt9d015_width width)
{
	int rc = -EIO;
	unsigned char buf[4];
	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN:{
			/*pr_info("i2c_write, WORD_LEN, addr = 0x%x, val = 0x%x!\n",waddr, wdata);*/

			buf[0] = (waddr & 0xFF00) >> 8;
			buf[1] = (waddr & 0x00FF);
			buf[2] = (wdata & 0xFF00) >> 8;
			buf[3] = (wdata & 0x00FF);

			rc = mt9d015_i2c_txdata(saddr, buf, 4);
		}
		break;

	case BYTE_LEN:{
			/*pr_info("i2c_write, BYTE_LEN, addr = 0x%x, val = 0x%x!\n",waddr, wdata);*/

			buf[0] = waddr;
			buf[1] = wdata;
			rc = mt9d015_i2c_txdata(saddr, buf, 2);
		}
		break;

	default:
		break;
	}

	if (rc < 0)
		pr_info("i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		     waddr, wdata);

	return rc;
}

#if 0
static int mt9d015_i2c_write_table(struct mt9d015_i2c_reg_conf
				       *reg_conf_tbl, int num_of_items_in_table)
{
	int i;
	int rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = mt9d015_i2c_write(mt9d015_client->addr,
				       reg_conf_tbl->waddr, reg_conf_tbl->wdata,
				       reg_conf_tbl->width);
		if (rc < 0) {
		pr_err("%s: num_of_items_in_table=%d\n", __func__,
			num_of_items_in_table);
			break;
		}
		if (reg_conf_tbl->mdelay_time != 0)
			mdelay(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}
#endif

static int mt9d015_i2c_rxdata(unsigned short saddr,
			      unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = 2,  /* .len = 1, */
		 .buf = rxdata,
		 },
		{
		 .addr = saddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxdata,
		 },
	};

	if (i2c_transfer_retry(mt9d015_client->adapter, msgs, 2) < 0) {
		pr_err("mt9d015_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

/*read 2 bytes data from sensor via I2C */
static int32_t msm_camera_i2c_read_b(unsigned short saddr, unsigned short raddr,
	unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = mt9d015_i2c_rxdata(saddr, buf, 2);
	if (rc < 0)
		return rc;

	*rdata = buf[0] << 8 | buf[1];

	if (rc < 0)
		CDBG("msm_camera_i2c_read_b failed!\n");

	return rc;
}

#if 0
static int mt9d015_i2c_read(unsigned short saddr,
				unsigned short raddr, unsigned char *rdata)
{
	int rc = 0;
	unsigned char buf[1];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));
	buf[0] = raddr;
	rc = mt9d015_i2c_rxdata(saddr, buf, 1);
	if (rc < 0)
		return rc;
	*rdata = buf[0];
	if (rc < 0)
		pr_info("mt9d015_i2c_read failed!\n");

	return rc;
}
#endif
static int mt9d015_i2c_write_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, unsigned short state)
{
	int rc;
	unsigned short check_value;
	unsigned short check_bit;

	if (state)
		check_bit = 0x0001 << bit;
	else
		check_bit = 0xFFFF & (~(0x0001 << bit));
	pr_info("mt9d015_i2c_write_bit check_bit:0x%4x", check_bit);
	rc = msm_camera_i2c_read_b(saddr, raddr, &check_value);
	if (rc < 0)
	  return rc;

	pr_info("%s: mt9d015: 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);
	if (state)
		check_value = (check_value | check_bit);
	else
		check_value = (check_value & check_bit);

	pr_info("%s: mt9d015: Set to 0x%4x reg value = 0x%4x\n", __func__,
		raddr, check_value);

	rc = mt9d015_i2c_write(saddr, raddr, check_value,
		WORD_LEN);
	return rc;
}

static int mt9d015_i2c_check_bit(unsigned short saddr, unsigned short raddr,
unsigned short bit, int check_state)
{
	int k;
	unsigned short check_value;
	unsigned short check_bit;
	check_bit = 0x0001 << bit;
	for (k = 0; k < CHECK_STATE_TIME; k++) {/* retry 100 times */
		msm_camera_i2c_read_b(mt9d015_client->addr,
			      raddr, &check_value);
		if (check_state) {
			if ((check_value & check_bit))
			break;
		} else {
			if (!(check_value & check_bit))
			break;
		}
		msleep(1);
	}
	if (k == CHECK_STATE_TIME) {
		pr_err("%s failed addr:0x%2x data check_bit:0x%2x",
			__func__, raddr, check_bit);
		return -1;
	}
	return 1;
}


#endif	/* HTC_END */

static int mt9d015_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	if (data->sensor_platform_info)
		mt9d015_s_ctrl.mirror_flip = data->sensor_platform_info->mirror_flip;

	return 0;

#if 0	/* HTC_START for i2c operation */
	if (mt9d015_s_ctrl.sensor_i2c_client && mt9d015_s_ctrl.sensor_i2c_client->client)
		mt9d015_client = mt9d015_s_ctrl.sensor_i2c_client->client;
	#if 1	//FIXME:	test only and never run here
	mt9d015_i2c_check_bit(0, 0, 0, 0);
	mt9d015_i2c_write_bit(0, 0, 0, 0);
	#endif
#endif	/* HTC_END */
}

static int mt9d015_sensor_config(void __user *argp)
{
	return msm_sensor_config(&mt9d015_s_ctrl, argp);
}

static const char *mt9d015Vendor = "aptina";
static const char *mt9d015NAME = "mt9d015";
static const char *mt9d015Size = "2.0M";

static ssize_t sensor_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "%s %s %s\n", mt9d015Vendor, mt9d015NAME, mt9d015Size);
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(sensor, 0444, sensor_vendor_show, NULL);

static struct kobject *android_mt9d015;

static int mt9d015_sysfs_init(void)
{
	int ret ;
	pr_info("mt9d015:kobject creat and add\n");
	android_mt9d015 = kobject_create_and_add("android_camera2", NULL);
	if (android_mt9d015 == NULL) {
		pr_info("mt9d015_sysfs_init: subsystem_register " \
		"failed\n");
		ret = -ENOMEM;
		return ret ;
	}
	pr_info("mt9d015:sysfs_create_file\n");
	ret = sysfs_create_file(android_mt9d015, &dev_attr_sensor.attr);
	if (ret) {
		pr_info("mt9d015_sysfs_init: sysfs_create_file " \
		"failed\n");
		kobject_del(android_mt9d015);
	}

	return 0 ;
}

static struct msm_camera_i2c_client mt9d015_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int mt9d015_sensor_v4l2_probe(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = -EINVAL;

	rc = msm_sensor_v4l2_probe(&mt9d015_s_ctrl, info, sdev, s);

	return rc;
}

#define CAM2_RSTz       (2)//GPIO(2)
#define CAM_PIN_GPIO_CAM2_RSTz	CAM2_RSTz

int32_t mt9d015_power_up(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	pr_info("%s\n", __func__);

	if (sdata->camera_power_on == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	if (!sdata->use_rawchip) {
		rc = msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0) {
			pr_err("%s: msm_camio_sensor_clk_on failed:%d\n",
			 __func__, rc);
			goto enable_mclk_failed;
		}
	}

	rc = sdata->camera_power_on();
	if (rc < 0) {
		pr_err("%s failed to enable power\n", __func__);
		goto enable_power_on_failed;
	}

	rc = msm_sensor_power_up(sdata);
	if (rc < 0) {
		pr_err("%s msm_sensor_power_up failed\n", __func__);
		goto enable_sensor_power_up_failed;
	}

	return rc;

enable_sensor_power_up_failed:
	if (sdata->camera_power_off == NULL)
		pr_err("sensor platform_data didnt register\n");
	else
		sdata->camera_power_off();
enable_power_on_failed:
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
enable_mclk_failed:
	return rc;
}

int32_t mt9d015_power_down(const struct msm_camera_sensor_info *sdata)
{
	int rc;
	pr_info("%s\n", __func__);

	if (sdata->camera_power_off == NULL) {
		pr_err("sensor platform_data didnt register\n");
		return -EIO;
	}

	rc = sdata->camera_power_off();
	if (rc < 0)
		pr_err("%s failed to disable power\n", __func__);

	rc = msm_sensor_power_down(sdata);
	if (rc < 0)
		pr_err("%s msm_sensor_power_down failed\n", __func__);

	if (!sdata->use_rawchip) {
		msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
		if (rc < 0)
			pr_err("%s: msm_camio_sensor_clk_off failed:%d\n",
				 __func__, rc);
	}

	return rc;
}

int32_t mt9d015_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int	rc = 0;
	pr_info("%s\n", __func__);
	rc = msm_sensor_i2c_probe(client, id);
	pr_info("%s: rc(%d)\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id mt9d015_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&mt9d015_s_ctrl},
	{ }
};

static struct i2c_driver mt9d015_i2c_driver = {
	.id_table = mt9d015_i2c_id,
	.probe  =  mt9d015_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static int mt9d015_read_fuseid(struct sensor_cfg_data *cdata,
	struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t  rc;
	uint8_t  reg_status = 0;
	uint8_t  fuseid[4] = {0};

	struct msm_camera_i2c_client *mt9d015_msm_camera_i2c_client = s_ctrl->sensor_i2c_client;

	pr_info("[CAM]%s: sensor OTP information:\n", __func__);
	/* Read a word value from 0x301A  */
	rc = msm_camera_i2c_read_seq(mt9d015_msm_camera_i2c_client, (0x301A), &reg_status, 2);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_read value from 0x301A fail\n", __func__);
	/* OR 0x0020 with 0x301A[5] as 1 */
	reg_status = (reg_status|0x0020);
	pr_info("[CAM]%s: reg_status = %x\n", __func__, reg_status);

	/* set reg_status to 0x301A */
	rc = msm_camera_i2c_write_b(mt9d015_msm_camera_i2c_client, 0x301A, reg_status);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_w_sensor 0x301A fail\n", __func__);

	/* Read fuseid value from 0x31F4, 0x31F6,0x31F8,0x31FA */
	rc = msm_camera_i2c_read_seq(mt9d015_msm_camera_i2c_client, (0x31F4), &fuseid[0], 2);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_w_sensor 0x31F4 fail\n", __func__);

	rc = msm_camera_i2c_read_seq(mt9d015_msm_camera_i2c_client, (0x31F6), &fuseid[1], 2);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_w_sensor 0x31F6 fail\n", __func__);

	rc = msm_camera_i2c_read_seq(mt9d015_msm_camera_i2c_client, (0x31F8), &fuseid[2], 2);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_w_sensor 0x318 fail\n", __func__);

	rc = msm_camera_i2c_read_seq(mt9d015_msm_camera_i2c_client, (0x31FA), &fuseid[3], 2);
	if (rc < 0)
		pr_info("[CAM]%s: i2c_write_w_sensor 0x31A fail\n", __func__);

	pr_info("[CAM]%s: fuseid: %x%x%x%x\n",
		__func__, fuseid[3], fuseid[2], fuseid[1], fuseid[0]);
	/* return user space */
	cdata->cfg.fuse.fuse_id_word1 = 0;
	cdata->cfg.fuse.fuse_id_word2 = 0;
	cdata->cfg.fuse.fuse_id_word3 =
    (fuseid[3]<<16)|(fuseid[2]);/* High value*/
	cdata->cfg.fuse.fuse_id_word4 =
    (fuseid[1]<<16)|(fuseid[0]);/* Low value*/

	pr_info("[CAM]mt9d015: fuse->fuse_id_word1:%d\n",
            cdata->cfg.fuse.fuse_id_word1);
	pr_info("[CAM]mt9d015: fuse->fuse_id_word2:%d\n",
            cdata->cfg.fuse.fuse_id_word2);
	pr_info("[CAM]mt9d015: fuse->fuse_id_word3:0x%08x\n",
            cdata->cfg.fuse.fuse_id_word3);
	pr_info("[CAM]mt9d015: fuse->fuse_id_word4:0x%08x\n",
            cdata->cfg.fuse.fuse_id_word4);
	return 0;

}
/* HTC_END*/

static int mt9d015_probe(struct platform_device *pdev)
{
	int	rc = 0;

	pr_info("%s\n", __func__);

	rc = msm_sensor_register(pdev, mt9d015_sensor_v4l2_probe);
	if(rc >= 0)
		mt9d015_sysfs_init();
	return rc;
}

struct platform_driver mt9d015_driver = {
	.probe = mt9d015_probe,
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_sensor_init_module(void)
{
	pr_info("%s\n", __func__);
//	return i2c_add_driver(&mt9d015_i2c_driver);
	return platform_driver_register(&mt9d015_driver);
}

static struct v4l2_subdev_core_ops mt9d015_subdev_core_ops;

static struct v4l2_subdev_video_ops mt9d015_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops mt9d015_subdev_ops = {
	.core = &mt9d015_subdev_core_ops,
	.video  = &mt9d015_subdev_video_ops,
};

static struct msm_sensor_fn_t mt9d015_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain1,
	.sensor_setting = msm_sensor_setting1,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = mt9d015_sensor_config,
	.sensor_open_init = mt9d015_sensor_open_init,
	.sensor_power_up = mt9d015_power_up,
	.sensor_power_down = mt9d015_power_down,
	.sensor_probe = msm_sensor_probe,
	.sensor_i2c_read_fuseid = mt9d015_read_fuseid,
};

static struct msm_sensor_reg_t mt9d015_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = mt9d015_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(mt9d015_start_settings),
	.stop_stream_conf = mt9d015_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(mt9d015_stop_settings),
	.group_hold_on_conf = mt9d015_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(mt9d015_groupon_settings),
	.group_hold_off_conf = mt9d015_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(mt9d015_groupoff_settings),
	.init_settings = &mt9d015_init_conf[0],
	.init_size = ARRAY_SIZE(mt9d015_init_conf),
	.mode_settings = &mt9d015_confs[0],
	.output_settings = &mt9d015_dimensions[0],
	.num_conf = ARRAY_SIZE(mt9d015_confs),
};

static struct msm_sensor_ctrl_t mt9d015_s_ctrl = {
	.msm_sensor_reg = &mt9d015_regs,
	.sensor_i2c_client = &mt9d015_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.sensor_output_reg_addr = &mt9d015_reg_addr,
	.sensor_id_info = &mt9d015_id_info,
	.sensor_exp_gain_info = &mt9d015_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &mt9d015_csi_params_array[0],
	.msm_sensor_mutex = &mt9d015_mut,
	.sensor_i2c_driver = &mt9d015_i2c_driver,
	.sensor_v4l2_subdev_info = mt9d015_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(mt9d015_subdev_info),
	.sensor_v4l2_subdev_ops = &mt9d015_subdev_ops,
	.func_tbl = &mt9d015_func_tbl,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Aptina 2.0 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
