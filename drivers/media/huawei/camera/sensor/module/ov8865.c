

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"

#define I2S(i) container_of(i, sensor_t, intf)

//add ov8865
extern struct hw_csi_pad hw_csi_pad;
static hwsensor_vtbl_t s_ov8865_vtbl;

//#define MODULE_OFG "ov8865"

int ov8865_config(hwsensor_intf_t* si, void  *argp);

static bool is_poweron = false;

struct sensor_power_setting ov8865_power_setting[] = {
    //MINIISP CS
    {
    	.seq_type = SENSOR_CS,
    	.config_val = SENSOR_GPIO_HIGH,
    	.sensor_index = SENSOR_INDEX_INVALID,
    	.delay = 1,
    },

    //MINIISP CORE 1.1V
    {
        .seq_type = SENSOR_SUSPEND,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //MINIISP DVDD 1.1V
    {
        .seq_type = SENSOR_RST2,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //MINIISP IOVDD 1.8V
    {
        .seq_type = SENSOR_IOVDD,
        .data = (void*)"common-iovdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

	//SCAM IOVDD 1.8V
//	{
//		.seq_type = SENSOR_IOVDD,
//		.data = (void*)"common-iovdd",
//		.config_val = LDO_VOLTAGE_1P8V,
//		.sensor_index = SENSOR_INDEX_INVALID,
//		.delay = 1,
//	},

	//MCAM1 OISVDD 2.85V
	{
		.seq_type = SENSOR_OIS2,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 0,
	},

	//MCAM1 AFVDD 2.85V
	{
		.seq_type = SENSOR_VCM_AVDD2,
		.data = (void*)"cameravcm-vcc",
		.config_val = LDO_VOLTAGE_V2P85V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 0,
	},

	//MCAM1 AVDD 2.85V
	{
		.seq_type = SENSOR_AVDD2,
		.data = (void*)"main-sensor-avdd",
		.config_val = LDO_VOLTAGE_V2P85V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 0,
	},

	//MCAM1 DVDD 1.0V
	{
		.seq_type = SENSOR_DVDD2,
		.config_val = LDO_VOLTAGE_1P05V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 0,
	},
	//SCAM AVDD 2.85V
	{
		.seq_type = SENSOR_AVDD,
		.data = (void*)"slave-sensor-avdd",
		.config_val = LDO_VOLTAGE_V2P85V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 0,
	},

	//SCAM DVDD1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_RST,
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},
};

static sensor_t s_ov8865 =
{
    .intf = { .vtbl = &s_ov8865_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(ov8865_power_setting),
            .power_setting = ov8865_power_setting,
     },
};

static const struct of_device_id
s_ov8865_dt_match[] =
{
	{
        .compatible = "huawei,ov8865",
        .data = &s_ov8865.intf,
    },
	{
    },
};

MODULE_DEVICE_TABLE(of, s_ov8865_dt_match);

static struct platform_driver
s_ov8865_driver =
{
	.driver =
    {
		.name = "huawei,ov8865",
		.owner = THIS_MODULE,
		.of_match_table = s_ov8865_dt_match,
	},
};

char const*
ov8865_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}

int
ov8865_power_up(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	if(!is_poweron) {
		ret = hw_sensor_power_up(sensor);
		cam_notice("+++ov8865 power on!+++");
		is_poweron = true;
	} else {
		cam_notice("+++not power on+++");
	}
	return ret;
}

int
ov8865_power_down(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	if(is_poweron) {
		ret = hw_sensor_power_down(sensor);
		cam_notice("---ov8865 power off!---");
		is_poweron = false;
	} else {
		cam_notice("---not power off---");
	}
	return ret;
}

int ov8865_csi_enable(hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);

	ret = hw_csi_pad.hw_csi_enable(sensor->board_info->csi_index, sensor->board_info->csi_lane, sensor->board_info->csi_mipi_clk);
	return ret;
}

int ov8865_csi_disable(hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	ret = hw_csi_pad.hw_csi_disable(sensor->board_info->csi_index);
	return ret;
}

static int
ov8865_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = I2S(si);
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;
    uint16_t sensor_id = 0;
    uint8_t modue_id = 0;
    uint8_t retry = 0;

    cam_info("%s TODO.", __func__);
    for(retry = 0;retry < 2; retry++){
        misp_get_module_info(sensor->board_info->sensor_index,&sensor_id,&modue_id);
        if(sensor_id==0){
            cam_info("%s try to read camera id again",__func__);
            continue;
        }else{
            break;
        }
    }

    if(sensor_id == 0x8865) {
        cdata->data = sensor->board_info->sensor_index;
        hwsensor_writefile(sensor->board_info->sensor_index,
                           sensor->board_info->name);
    } else {
        cdata->data = SENSOR_INDEX_INVALID;
    }

    cam_info("%s TODO.  cdata->data=%d", __func__, cdata->data);

    return 0;
}

static hwsensor_vtbl_t
s_ov8865_vtbl =
{
	.get_name = ov8865_get_name,
	.config = ov8865_config,
	.power_up = ov8865_power_up,
	.power_down = ov8865_power_down,
	.match_id = ov8865_match_id,
	.csi_enable = ov8865_csi_enable,
	.csi_disable = ov8865_csi_disable,
	.match_id = ov8865_match_id,
};

int
ov8865_config(
        hwsensor_intf_t* si,
        void  *argp)
{
	struct sensor_cfg_data *data;

	int ret =0;
    static bool ov8865_power_on = false;
	static bool csi_enable = false;
    data = (struct sensor_cfg_data *)argp;
    cam_debug("ov8865 cfgtype = %d",data->cfgtype);
    switch(data->cfgtype){
		case SEN_CONFIG_POWER_ON:
            if(!ov8865_power_on){
                ret = si->vtbl->power_up(si);
                ov8865_power_on = true;
            }
            break;
		case SEN_CONFIG_POWER_OFF:
            if(ov8865_power_on){
                ret = si->vtbl->power_down(si);
                ov8865_power_on = false;
            }
            break;
		case SEN_CONFIG_WRITE_REG:
			break;
		case SEN_CONFIG_READ_REG:
			break;
		case SEN_CONFIG_WRITE_REG_SETTINGS:
			break;
		case SEN_CONFIG_READ_REG_SETTINGS:
			break;
		case SEN_CONFIG_ENABLE_CSI:
            if(ov8865_power_on && !csi_enable){
                ret = si->vtbl->csi_enable(si);
                csi_enable = true;
            }
            break;
        case SEN_CONFIG_DISABLE_CSI:
            if(ov8865_power_on && csi_enable){
                ret = si->vtbl->csi_disable(si);
                csi_enable = false;
            }
            break;
		case SEN_CONFIG_MATCH_ID:
			ret = si->vtbl->match_id(si,argp);
			break;
		default:
                cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
			break;
	}
	cam_debug("%s exit",__func__);
	return ret;
}

static int32_t
ov8865_platform_probe(
        struct platform_device* pdev)
{
	int rc = 0;
	cam_debug("enter %s",__func__);

	if (pdev->dev.of_node) {
		rc = hw_sensor_get_dt_data(pdev, &s_ov8865);
		if (rc < 0) {
			cam_err("%s failed line %d\n", __func__, __LINE__);
			goto ov8865_sensor_probe_fail;
		}
	} else {
		cam_err("%s ov8865 of_node is NULL.\n", __func__);
		goto ov8865_sensor_probe_fail;
	}

	rc = hwsensor_register(pdev, &s_ov8865.intf);
ov8865_sensor_probe_fail:
	return rc;
}

static int __init
ov8865_init_module(void)
{
	cam_info("Enter: %s", __func__);
    return platform_driver_probe(&s_ov8865_driver,
            ov8865_platform_probe);
}

static void __exit
ov8865_exit_module(void)
{
    hwsensor_unregister(&s_ov8865.intf);
    platform_driver_unregister(&s_ov8865_driver);
}

module_init(ov8865_init_module);
module_exit(ov8865_exit_module);
MODULE_DESCRIPTION("ov8865");
MODULE_LICENSE("GPL v2");

