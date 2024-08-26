/*
 * Wacom Penabled Driver for I2C
 *
 * Copyright (c) 2011-2014 Tatsunosuke Tobita, Wacom.
 * <tobita.tatsunosuke@wacom.co.jp>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version of 2 of the License,
 * or (at your option) any later version.
 */
#include "wacom.h"

#define WACOM_PEN_NAME  "wacom_pen"
struct wacom_i2c_pen *g_pen_data = NULL;

static int wacom_input_config(struct wacom_i2c_pen *pen_data)
{
    struct input_dev *input;
    int rc = 0;
    input = input_allocate_device();
    if (!input) {
        TS_LOG_ERR("%s: Error, failed to allocate input device\n",__func__);
        return -ENOMEM;
    }

    input->name = WACOM_PEN_NAME;

    input->dev.parent = &pen_data->client->dev;

    input->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    __set_bit(ABS_X, input->absbit);
    __set_bit(ABS_Y, input->absbit);
    __set_bit(BTN_TOOL_RUBBER, input->keybit);
    __set_bit(BTN_STYLUS, input->keybit);
    __set_bit(BTN_STYLUS2, input->keybit);
    __set_bit(BTN_TOUCH, input->keybit);
    __set_bit(BTN_TOOL_PEN, input->keybit);
    __set_bit(INPUT_PROP_DIRECT, input->propbit);        
    input_set_abs_params(input, ABS_Y, 0, REPORT_ABS_Y_MAX, 0, 0);
    input_set_abs_params(input, ABS_X, 0, REPORT_ABS_X_MAX, 0, 0);
    input_set_abs_params(input, ABS_PRESSURE, 0, WACOM_ABS_PRESSURE, 0, 0);

    rc = input_register_device(input);
    if (rc < 0){
        TS_LOG_ERR("%s: Error, failed register input device r=%d\n", __func__, rc);
	return rc;
    }
    
    input_set_drvdata(input, pen_data);
    pen_data->input =  input;

    return NO_ERR;
}

void wacom_report_pen_data(struct wacom_i2c_pen *pen_data,struct wacom_i2c *wac_i2c,u8 *data)
{
    struct input_dev *input = pen_data->input;
    struct wacom_id *ids = &wac_i2c->ids;
    unsigned int x, y, pressure;
    unsigned char tsw, f1, f2, ers;
    unsigned char  *battery_cap = &wac_i2c->battery_cap;
    tsw = data[3] & 0x01;
    ers = data[3] & 0x04;
    f1 = data[3] & 0x02;
    f2 = data[3] & 0x10;
    y = le16_to_cpup((__le16 *)&data[4]);
    x = WACOM_PEN_ABS_X_MAX -le16_to_cpup((__le16 *)&data[6]);

    x = (int)(x * REPORT_ABS_X_MAX) / WACOM_PEN_ABS_X_MAX;
    y = (int)(y * REPORT_ABS_Y_MAX) / WACOM_PEN_ABS_Y_MAX;

    pressure = le16_to_cpup((__le16 *)&data[8]);
    *battery_cap = data[17];	

    ids->deviceId = (u16)((data[11] & 0x0f) << 8) | data[10];
    ids->designId = (data[11] & 0xf0) >> 4;
    ids->customerId = (data[12] & 0x1f);
    ids->uniqueId =  (u32)((data[15] << 24 | data[14] << 16 | data[13] << 8 | data[12]) >> 5);
	
    if (!wac_i2c->rdy){
  	    wac_i2c->tool = BTN_TOOL_PEN;
    }
    
    wac_i2c->rdy = data[3] & 0x20;
    if( wac_i2c->tool == BTN_TOOL_PEN ) {
 	if( f2 && tsw ) {
    		//hover
    		input_report_abs(input, ABS_X, x);
    		input_report_abs(input, ABS_Y, y);
    		input_report_abs(input, ABS_PRESSURE, 0);
    		input_sync(input);
    		//pen leave
    		input_report_key(input, BTN_TOOL_PEN, 0);
    		input_report_abs(input, ABS_X, x);
    		input_report_abs(input, ABS_Y, y);
    		input_report_abs(input, ABS_PRESSURE, 0);
    		input_sync(input);
    		//rubber in
    		wac_i2c->tool = BTN_TOOL_RUBBER;
	    }
    }else{
	    if(!(f2 && tsw) ) {
    		//hover
    		input_report_abs(input, ABS_X, x);
    		input_report_abs(input, ABS_Y, y);
    		input_report_abs(input, ABS_PRESSURE, 0);
    		input_sync(input);
    		//rubber leave
    		input_report_key(input, BTN_TOOL_RUBBER, 0);
    		input_report_abs(input, ABS_X, x);
    		input_report_abs(input, ABS_Y, y);
    		input_report_abs(input, ABS_PRESSURE, 0);
    		input_sync(input);
    		//pen in
    		wac_i2c->tool = BTN_TOOL_PEN;
        }
	}
    TS_LOG_DEBUG("report pen coord: x= %d, y= %d, stylus=%d, stylus2=%d\n", x, y, f1, f2);
    input_report_key(input, BTN_STYLUS, f1);
    input_report_abs(input, ABS_X, x);
    input_report_abs(input, ABS_Y, y);
    input_report_abs(input, ABS_PRESSURE, pressure);
    input_report_key(input, BTN_TOUCH, tsw || ers);
    input_report_key(input, wac_i2c->tool, wac_i2c->rdy);
    input_sync(input);
	
    return;    
}


static int wacom_i2c_pen_probe(struct i2c_client *client,
    const struct i2c_device_id *i2c_id)
{
    //struct device_node *np = NULL;
    struct wacom_i2c_pen *pen_data = NULL;
    int ret = 0;
    
    /* get context and debug print buffers */
    pen_data = kzalloc(sizeof(*pen_data), GFP_KERNEL);
    if (!pen_data) {
        TS_LOG_ERR("%s: pen_data kzalloc fail.\n", __func__);
        ret = -ENOMEM;
        goto out;
    }

    pen_data->client = client;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        TS_LOG_ERR("%s: I2C functionality not Supported.\n", __func__);
        goto error_alloc_data;
    }

    wacom_input_config(pen_data);

    g_pen_data = pen_data;

    return 0;

error_alloc_data:
    kfree(pen_data);
out:
    TS_LOG_DEBUG("-.\n");
    return ret;
}

#define WACOM_I2C_NAME "wacom_i2c_ipc"
static const struct i2c_device_id wacom_i2c_id[] = {
    { WACOM_I2C_NAME, 0, },
    { }
};

static struct of_device_id wacom_i2c_of_match[] = {
    { .compatible = "wacom,wacom_i2c_adapter", },
    { }
};
MODULE_DEVICE_TABLE(of, wacom_i2c_of_match);

static struct i2c_driver wacom_i2c_driver = {
    .driver = {
        .name = WACOM_I2C_NAME,
        .owner = THIS_MODULE,
        .of_match_table = wacom_i2c_of_match,
    },
    .probe = wacom_i2c_pen_probe,
    .id_table = wacom_i2c_id,
};

static int __init wacom_i2c_pen_init(void)
{
    int ret = 0;
    
    ret = i2c_add_driver(&wacom_i2c_driver);

    return ret;
}
module_init(wacom_i2c_pen_init);

static void __exit wacom_i2c_pen_exit(void)
{
    i2c_del_driver(&wacom_i2c_driver);
}
module_exit(wacom_i2c_pen_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
