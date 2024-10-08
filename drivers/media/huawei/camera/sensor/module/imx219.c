


#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"

#define I2S(i) container_of(i, sensor_t, intf)

extern struct hw_csi_pad hw_csi_pad;
static hwsensor_vtbl_t s_imx219_vtbl;

int imx219_config(hwsensor_intf_t* si, void  *argp);

struct sensor_power_setting hw_imx219_power_setting[] = {

	{
		.seq_type = SENSOR_AVDD,
		.data = (void*)"main-sensor-avdd",
		.config_val = LDO_VOLTAGE_2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_IOVDD,
		.data = (void*)"common-iovdd",
		.config_val = LDO_VOLTAGE_1P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_DVDD,
		.config_val = LDO_VOLTAGE_1P2V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VCM_AVDD,
		.data = (void*)"cameravcm-vcc",
		.config_val = LDO_VOLTAGE_2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VCM_PWDN,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 2,
	},
	{
		.seq_type = SENSOR_I2C,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
};

static sensor_t s_imx219 =
{
    .intf = { .vtbl = &s_imx219_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_imx219_power_setting),
            .power_setting = hw_imx219_power_setting,
     },
};

static const struct of_device_id
s_imx219_dt_match[] =
{
	{
        .compatible = "huawei,imx219",
        .data = &s_imx219.intf,
    },
    {
    },
};

MODULE_DEVICE_TABLE(of, s_imx219_dt_match);

static struct platform_driver
s_imx219_driver =
{
	.driver =
    {
		.name = "huawei,imx219",
		.owner = THIS_MODULE,
		.of_match_table = s_imx219_dt_match,
	},
};

char const*
imx219_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}

int
imx219_power_up(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	ret = hw_sensor_power_up(sensor);
	return ret;
}

int
imx219_power_down(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	ret = hw_sensor_power_down(sensor);
	return ret;
}

int imx219_csi_enable(hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);

	ret = hw_csi_pad.hw_csi_enable(sensor->board_info->csi_index, sensor->board_info->csi_lane, sensor->board_info->csi_mipi_clk);
	return ret;
}

int imx219_csi_disable(hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	ret = hw_csi_pad.hw_csi_disable(sensor->board_info->csi_index);
	return ret;
}

static int imx219_i2c_read (hwsensor_intf_t* intf, void * data)
{
	sensor_t* sensor = NULL;
	int ret = 0;

	sensor= I2S(intf);
	ret = hw_sensor_i2c_read(sensor,data);

	return ret;
}

static int imx219_i2c_read_otp (hwsensor_intf_t* intf, void * data)
{
	sensor_t* sensor = NULL;
	int ret = 0;

	sensor= I2S(intf);
	ret = hw_sensor_i2c_read_otp(sensor,data);

	return ret;
}

static int
imx219_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = I2S(si);
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;
    struct sensor_cfg_data cdata_h;
    struct sensor_cfg_data cdata_l;
    u16 sensor_id = 0;
    int rc = 0;
    int32_t module_id = 0;
    char *sensor_name [] = {"imx219_liteon","imx219_ofilm","imx219"};

    cdata_h.cfg.reg.subaddr = 0x0000;
    cdata_h.cfg.reg.value = 0;

    cdata_l.cfg.reg.subaddr = 0x0001;
    cdata_l.cfg.reg.value = 0;

    imx219_i2c_read(si, &cdata_h);
    imx219_i2c_read(si, &cdata_l);

    sensor_id = (cdata_h.cfg.reg.value) << 8 | (cdata_l.cfg.reg.value);

    cam_notice( "%s, line %d, sensor id: 0x%x", __func__, __LINE__, sensor_id);

    if (0x219 == sensor_id) {
        cam_info("%s succeed to match id.", __func__);
        rc = gpio_request(sensor->board_info->gpios[FSIN].gpio, NULL);
        if(rc < 0) {
            cam_err("%s failed to request gpio[%d]", __func__, sensor->board_info->gpios[FSIN].gpio);
            return rc;
        }

        cam_info("%s gpio[%d].", __func__, sensor->board_info->gpios[FSIN].gpio);

        rc = gpio_direction_input(sensor->board_info->gpios[FSIN].gpio);
        if (rc < 0) {
            cam_err("%s failed to config gpio(%d) input.\n",
                __func__, sensor->board_info->gpios[FSIN].gpio);
        }

        module_id = gpio_get_value_cansleep(sensor->board_info->gpios[FSIN].gpio);
        if (module_id < 0) {
            cam_err("%s failed to get gpio(%d) value(%d).\n",
                    __func__, sensor->board_info->gpios[FSIN].gpio, module_id);
        }

        cam_info("%s module_id = %d", __func__,module_id);

        gpio_free(sensor->board_info->gpios[FSIN].gpio);

        if (0 == module_id) {
            cam_info("%s module_id == 0",__func__);
            strncpy(cdata->cfg.name, sensor_name[0], DEVICE_NAME_SIZE-1);
        } else if(1 == module_id) {
            cam_info("%s module_id == 1", __func__);
            strncpy(cdata->cfg.name, sensor_name[1], DEVICE_NAME_SIZE-1);
        }else{
            cam_info("%s module_id < 0", __func__);
            strncpy(cdata->cfg.name, sensor_name[2], DEVICE_NAME_SIZE-1);
	}

    } else {
        cam_info("%s failed to match id.", __func__);
        sensor->board_info->sensor_index = CAMERA_SENSOR_INVALID;
    }

    cdata->data = sensor->board_info->sensor_index;
    hwsensor_writefile(sensor->board_info->sensor_index,cdata->cfg.name);

    return 0;
}

static int imx219_i2c_write (hwsensor_intf_t* intf, void * data)
{
	sensor_t* sensor = NULL;
	int ret = 0;

	sensor= I2S(intf);
	ret = hw_sensor_i2c_write(sensor,data);

	return ret;
}

static int imx219_i2c_read_seq (hwsensor_intf_t* intf, void * data)
{
	sensor_t* sensor = NULL;
	int ret = 0;

	sensor= I2S(intf);
	ret = hw_sensor_i2c_read_seq(sensor,data);

	return ret;
}

static int imx219_i2c_write_seq (hwsensor_intf_t* intf, void * data)
{
	sensor_t* sensor = NULL;
	int ret = 0;

	sensor= I2S(intf);
	ret = hw_sensor_i2c_write_seq(sensor,data);

	return ret;
}

static hwsensor_vtbl_t
s_imx219_vtbl =
{
	.get_name = imx219_get_name,
	.config = imx219_config,
	.power_up = imx219_power_up,
	.power_down = imx219_power_down,
	.i2c_read = imx219_i2c_read,
	.i2c_write = imx219_i2c_write,
	.i2c_read_seq = imx219_i2c_read_seq,
	.i2c_write_seq = imx219_i2c_write_seq,
	.i2c_read_otp = imx219_i2c_read_otp,
	//.ioctl = imx219_ioctl,
	.match_id = imx219_match_id,
	//.set_expo_gain = imx219_set_expo_gain,
	//.apply_expo_gain = imx219_apply_expo_gain,
	//.suspend_eg_task = imx219_suspend_eg_task,
	//.set_hts = imx219_set_hts,
	//.set_vts = imx219_set_vts,
	.csi_enable = imx219_csi_enable,
	.csi_disable = imx219_csi_disable,
};


/*
SEN_CONFIG_POWER_ON = 0,
SEN_CONFIG_POWER_OFF = 1,
SEN_CONFIG_WRITE_REG = 2,
SEN_CONFIG_READ_REG = 3,
SEN_CONFIG_WRITE_REG_SETTINGS = 4,
SEN_CONFIG_READ_REG_SETTINGS = 5,
SEN_CONFIG_ENABLE_CSI = 6,
SEN_CONFIG_DISABLE_CSI = 7,
SEN_CONFIG_MATCH_ID = 8,
SEN_CONFIG_MAX_INDEX
*/

/*
   上层调用的顺序是 1  0  6 8 7 1
*/

int
imx219_config(
        hwsensor_intf_t* si,
        void  *argp)
{
	struct sensor_cfg_data *data;

	int ret =0;
	static bool imx219_power_on = false;
	static bool csi_enable = false;
	data = (struct sensor_cfg_data *)argp;
	cam_debug("imx219 cfgtype = %d",data->cfgtype);
	
    if(!imx219_power_on && (data->cfgtype != SEN_CONFIG_POWER_ON))
    {
        cam_err("%s POWER_ON must be done before other CMD %d",__func__,data->cfgtype);
        return ret;
    }
    switch(data->cfgtype){
		case SEN_CONFIG_POWER_ON:  // 0
			if (!imx219_power_on) {
				ret = si->vtbl->power_up(si);
				imx219_power_on = true;
			}
			break;
		case SEN_CONFIG_POWER_OFF:   // 1
			if (imx219_power_on) {
				ret = si->vtbl->power_down(si);
				imx219_power_on = false;
			}
			break;
		case SEN_CONFIG_WRITE_REG:    // 2
			ret = si->vtbl->i2c_write(si,argp);
			break;
		case SEN_CONFIG_READ_REG:     // 3 
			ret = si->vtbl->i2c_read(si,argp);
			break;
		case SEN_CONFIG_WRITE_REG_SETTINGS:  // 4 
			ret = si->vtbl->i2c_write_seq(si,argp);
			break;
		case SEN_CONFIG_READ_REG_SETTINGS:  // 5 
			ret = si->vtbl->i2c_read_seq(si,argp);
			break;
		case SEN_CONFIG_ENABLE_CSI:   // 6
			if(!csi_enable)
			{
				ret = si->vtbl->csi_enable(si);
				csi_enable = true;
			}
			break;
		case SEN_CONFIG_DISABLE_CSI:  // 7 
			if(csi_enable)
			{
				ret = si->vtbl->csi_disable(si);
				csi_enable = false;
			}
			break;
		case SEN_CONFIG_MATCH_ID:     // 8
			ret = si->vtbl->match_id(si,argp);
			break;
		case SEN_CONFIG_READ_REG_OTP:     // 9
			ret = si->vtbl->i2c_read_otp(si,argp);
			break;
		default:
			break;
	}
	cam_debug("%s exit",__func__);
	return ret;
}



static int32_t
imx219_platform_probe(
        struct platform_device* pdev)
{
	int rc = 0;
	cam_notice("enter %s",__func__);

	if (pdev->dev.of_node) {
		rc = hw_sensor_get_dt_data(pdev, &s_imx219);
		if (rc < 0) {
			cam_err("%s failed line %d\n", __func__, __LINE__);
			goto imx219_sensor_probe_fail;
		}
	} else {
		cam_err("%s imx219 of_node is NULL.\n", __func__);
		goto imx219_sensor_probe_fail;
	}

	rc = hwsensor_register(pdev, &s_imx219.intf);
imx219_sensor_probe_fail:
	return rc;
}

static int __init
imx219_init_module(void)
{
    cam_notice("enter %s",__func__);
    return platform_driver_probe(&s_imx219_driver,
            imx219_platform_probe);
}

static void __exit
imx219_exit_module(void)
{
    platform_driver_unregister(&s_imx219_driver);
}

module_init(imx219_init_module);
module_exit(imx219_exit_module);
MODULE_DESCRIPTION("imx219");
MODULE_LICENSE("GPL v2");

