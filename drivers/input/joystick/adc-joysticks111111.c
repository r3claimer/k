/*
 * Input driver for resistor ladder connected on ADC
 *
 * Copyright (c) 2016 Alexandre Belloni
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/iio/consumer.h>
#include <linux/iio/types.h>
#include <linux/input-polldev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

static int dbg_enable = 0;

#define DBG(args...) \
	do { \
		if (dbg_enable) { \
			pr_info(args); \
		} \
	} while (0)

struct adc_joystick_state {
	struct iio_channel *channelx;
	struct iio_channel *channely;
	 struct iio_channel *channelz;
	 struct iio_channel *channelrz;
//	int adc_ena_gpios;
//	int adc_enb_gpios;
//	int adc_hall_gpios;
	int num_joystick;
	
	struct input_dev *input;
	unsigned char data[4];
//	unsigned char btn_data[2];
};
enum adc_val {
	RZ_val = 0,
	Z_val,
	X_val,
	Y_val,
};

static unsigned char abs_date[]={128,128,128,128};
//static unsigned char adcval_maxmin[]={2048,2048,2048,2048,0,0,0,0};
//static unsigned char adcval_mid[]={1024,1024,1024,1024};
//static unsigned char btn_date[]={128,128};

//static int adc_to_abs(int val) {
//	int abs_val;
//	// if(val<500)val = val-200;
//	// if(val>1500)val = val+200;
//	val = val+100;
//	if(val<0) val = 0;
//	if(val>1800) val = 1800;
//	abs_val = 255 - val * 255 / 1800;
////	printk("abs_val111 = %d\n",abs_val);
//	if(abs_val >210) abs_val = 255;
//	if(abs_val <45) abs_val = 0;
//	// if(abs_val > (128 - 25) && abs_val < (128 + 20)) abs_val = 128;
//	if(abs_val > (128 - 42) && abs_val < (128 + 47))
//		{
//		abs_val = 128;
////		printk("lqb abs_val2222 = 128");
//	}
//	return abs_val;
//};

static int adc_to_abs(int val,enum adc_val chan) {
	int abs_val;
	switch(chan){
	case RZ_val://chan 2
		if(val<900)
			val +=20;
		abs_val = val*255/1800;
		if(abs_val + 5 > 128 && abs_val - 5 < 128)  abs_val = 128;				
		break;
		
	case Z_val://chan 3
		if(val < 40)val = 40;
		if(val< 1080)		
			val = val -40;
		abs_val = val*255/1800;
		if(abs_val + 5 > 128 && abs_val - 5 < 128)  abs_val = 128;		
		break;
		
	case X_val:
		if(val < 150)val = 150;
		if(val < 1080)
			val = val -150;
		abs_val = val*255/1800;
		if(abs_val + 8 > 128 && abs_val - 8 < 128)  abs_val = 128;
		break;
		
	case Y_val:
		if(val < 250)val = 0;
		if(val< 990&&val > 900)
			val = val -50;
		abs_val = val*255/1800;
		if(abs_val + 8 > 128 && abs_val - 8 < 128)  abs_val = 128;
		break;
	};
	if(abs_val > 250) abs_val = 255;
		else if(abs_val < 5)	abs_val = 0;
	return abs_val;
};


//static void adc_joystick_poll(struct input_polled_dev *dev) {
static void adc_joystick_poll(struct input_dev *dev){

	struct adc_joystick_state *st = input_get_drvdata(dev);
	int value, ret;
	int state = 0;
	DBG("lqbjoy *******************************\n");

	// joystick x
	ret = iio_read_channel_processed(st->channelx, &value);
	if (unlikely(ret < 0)) {
		/* Forcibly release key if any was pressed */
		value = 1024;
	}
	
	st->data[0] = adc_to_abs(value,X_val);
	DBG("lqbjoy read adc_to_abs[x_val] %d,value = %d\n",adc_to_abs(value,X_val),value);
//	if(st->data[0] == 127) st->data[0] = 128;
	
	// joystick y
	ret = iio_read_channel_processed(st->channely, &value);
	if (unlikely(ret < 0)) {
		value = 1024;
	}
	
	st->data[1] = 255 - adc_to_abs(value,Y_val);
	 DBG("lqbjoy read adc_to_abs[y_val] %d----value = %d\n",adc_to_abs(value,Y_val),value);

	// joystick rz
	ret = iio_read_channel_processed(st->channelrz, &value);
	if (unlikely(ret < 0)) {
		/* Forcibly release key if any was pressed */
		value = 1024;
	}

	st->data[2] = adc_to_abs(value,RZ_val);
	
	DBG("lqbjoy read adc_to_abs[rz_val] %d----value = %d\n",adc_to_abs(value,RZ_val),value);
//	if(st->data[2] == 127) st->data[2] = 128;
	
	// joystick z
	ret = iio_read_channel_processed(st->channelz, &value);
	if (unlikely(ret < 0)) {
		/* Forcibly release key if any was pressed */
		value = 1024;
	}
	
	st->data[3] = adc_to_abs(value,Z_val);
	
	DBG("lqbjoy read adc_to_abs[z_val] %d----value = %d\n",adc_to_abs(value,Z_val),value);
//	if(st->data[3] == 127) st->data[3] = 128;

	// joystick hall
	
	if(abs_date[0] != st->data[0]) state++;
	if(abs_date[1] != st->data[1]) state++;
	if(abs_date[2] != st->data[2]) state++;
	if(abs_date[3] != st->data[3]) state++;


	if(state > 0){
		abs_date[0] = st->data[0];
		abs_date[1] = st->data[1];
		abs_date[2] = st->data[2];
		abs_date[3] = st->data[3];

	DBG("lqb datax = %d,datay= %d,datarz = %d,dataz= %d\n",abs_date[0],abs_date[1],abs_date[2],abs_date[3]);
		input_report_abs(st->input, ABS_X,  abs_date[0]);
		input_report_abs(st->input, ABS_Y,  abs_date[1]);
		input_report_abs(st->input, ABS_RZ, abs_date[2]);
		input_report_abs(st->input, ABS_Z,  abs_date[3]);
		input_sync(st->input);
		state = 0;
	}
	
	DBG("lqbjoy*******************************\n");
}

static int adc_joystick_probe(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
	struct adc_joystick_state *st;
//	struct input_polled_dev *poll_dev;
	struct input_dev *input;
	enum iio_chan_type type;
	int value;
	int error;
	printk("adc_joystick_probe start \n");
	st = devm_kzalloc(dev, sizeof(*st), GFP_KERNEL);
	if (!st) return -ENOMEM;

	//joystick x
	printk("adc_joystick_probe joystick_x \n");
	st->channelx = devm_iio_channel_get(dev, "joystick_x");
	if (IS_ERR(st->channelx))
		return PTR_ERR(st->channelx);

	if (!st->channelx->indio_dev)
		return -ENXIO;

	error = iio_get_channel_type(st->channelx, &type);
	if (error < 0)
		return error;
		
	if (type != IIO_VOLTAGE) {
		dev_err(dev, "Incompatible channel type %d\n", type);
		return -EINVAL;
	}

	//joystick y
	printk("adc_joystick_probe joystick_y \n");
	st->channely = devm_iio_channel_get(dev, "joystick_y");
	if (IS_ERR(st->channely))
		return PTR_ERR(st->channely);

	if (!st->channely->indio_dev)
		return -ENXIO;

	error = iio_get_channel_type(st->channely, &type);
	if (error < 0)
		return error;
		
	if (type != IIO_VOLTAGE) {
		dev_err(dev, "Incompatible channel type %d\n", type);
		return -EINVAL;
	}

	//joystick z
	printk("adc_joystick_probe joystick_z \n");
	st->channelz = devm_iio_channel_get(dev, "joystick_z");
	if (IS_ERR(st->channelz))
		return PTR_ERR(st->channelz);

	if (!st->channelz->indio_dev)
		return -ENXIO;
	printk("lqb adc_joystick_probe\n");
	error = iio_get_channel_type(st->channelz, &type);
	if (error < 0)
		return error;
		
	if (type != IIO_VOLTAGE) {
		dev_err(dev, "Incompatible channel type %d\n", type);
		return -EINVAL;
	}


	//joystick rz
	printk("adc_joystick_probe joystick_rz \n");
	st->channelrz = devm_iio_channel_get(dev, "joystick_rz");
	if (IS_ERR(st->channelrz))
		return PTR_ERR(st->channelrz);

	if (!st->channelrz->indio_dev)
		return -ENXIO;

	error = iio_get_channel_type(st->channelrz, &type);
	if (error < 0)
		return error;
		
	if (type != IIO_VOLTAGE) {
		dev_err(dev, "Incompatible channel type %d\n", type);
		return -EINVAL;
	}


	input = devm_input_allocate_device(dev);
	if (!input) {
		dev_err(dev, "Unable to allocate input device\n");
		return -ENOMEM;
	}
	error = input_setup_polling(input, adc_joystick_poll);
	if (error) {
		dev_err(dev, "Unable to set up polling: %d\n", error);
		return error;
	}
		if (!device_property_read_u32(dev, "poll-interval", &value))
	{
		input_set_poll_interval(input, value);
	}
	st->input = input;

//	input = poll_dev->input;

	input->name = pdev->name;
	input->phys = "adc-joysticks/input0";

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0810;
	input->id.product = 0x0002;
	input->id.version = 0x0100;
	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	__set_bit(EV_ABS, input->evbit);
	__set_bit(EV_KEY, input->evbit);
	__set_bit(KEY_UP, input->keybit);
	__set_bit(KEY_DOWN, input->keybit);
	__set_bit(KEY_LEFT, input->keybit);
	__set_bit(KEY_RIGHT, input->keybit);
	__set_bit(BTN_A, input->keybit);
	__set_bit(BTN_B, input->keybit);
	__set_bit(BTN_X, input->keybit);
	__set_bit(BTN_Y, input->keybit);
//	__set_bit(BTN_TL2, input->keybit);
//	__set_bit(BTN_TR2, input->keybit);

	set_bit(ABS_X, input->absbit);
	set_bit(ABS_Y, input->absbit);
	set_bit(ABS_RZ, input->absbit);
	set_bit(ABS_Z, input->absbit);
//	set_bit(ABS_BRAKE, input->absbit); 
//	set_bit(ABS_GAS, input->absbit);
	input_set_abs_params(input, ABS_X, 0, 255, 0, 15);
	input_set_abs_params(input, ABS_Z, 0, 255, 0, 15);
	input_set_abs_params(input, ABS_RZ, 0, 255, 0, 15);
	input_set_abs_params(input, ABS_Y, 0, 255, 0, 15);

	/* Enable auto repeat feature of Linux input subsystem */
	if (device_property_read_bool(dev, "autorepeat"))
		__set_bit(EV_REP, input->evbit);
		
	input_set_drvdata(input, st);
	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device\n");
		return error;
	}

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id adc_joystick_of_match[] = {
	{ .compatible = "adc-joysticks", },
	{ }
};
MODULE_DEVICE_TABLE(of, adc_joystick_of_match);
#endif

static struct platform_driver __refdata adc_joystick_driver = {
	.driver = {
		.name = "adc_joysticks",
		.of_match_table = of_match_ptr(adc_joystick_of_match),
	},
	.probe = adc_joystick_probe,
};
module_platform_driver(adc_joystick_driver);

MODULE_AUTHOR("Alexandre Belloni <alexandre.belloni@free-electrons.com>");
MODULE_DESCRIPTION("Input driver for resistor ladder connected on ADC");
MODULE_LICENSE("GPL v2");
