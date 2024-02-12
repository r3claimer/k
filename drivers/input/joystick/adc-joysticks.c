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
#include <linux/device/driver.h>
#include <linux/sysfs.h>
#include <linux/string.h>


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
	int raw_data[4];
//	bool flag;
};
//enum adc_val {
//	RZ_val = 0,
//	Z_val,
//	X_val,
//	Y_val,
//};

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

//static int adc_to_abs(int val,enum adc_val chan) {
//	int abs_val;
//	switch(chan){
//	case RZ_val://chan 2
//		if(val<900)
//			val +=20;
//		abs_val = val*255/1800;
//		if(abs_val + 5 > 128 && abs_val - 5 < 128)  abs_val = 128;				
//		break;
//		
//	case Z_val://chan 3
//		if(val < 40)val = 40;
//		if(val< 1080)		
//			val = val -40;
//		abs_val = val*255/1800;
//		if(abs_val + 5 > 128 && abs_val - 5 < 128)  abs_val = 128;		
//		break;
//		
//	case X_val:
//		if(val < 150)val = 150;
//		if(val < 1080)
//			val = val -150;
//		abs_val = val*255/1800;
//		if(abs_val + 8 > 128 && abs_val - 8 < 128)  abs_val = 128;
//		break;
//		
//	case Y_val:
//		if(val < 250)val = 0;
//		if(val< 990&&val > 900)
//			val = val -50;
//		abs_val = val*255/1800;
//		if(abs_val + 8 > 128 && abs_val - 8 < 128)  abs_val = 128;
//		break;
//	};
//	if(abs_val > 250) abs_val = 255;
//		else if(abs_val < 5)	abs_val = 0;
//	return abs_val;
//};

static int average_data(struct iio_channel *chan){
	int i,value;
	int sum = 0;
	int ret = 0;
	printk("**********************\n");
	for(i = 0;i<10;i++){
		ret = iio_read_channel_processed(chan, &value);
//		printk("average_data value = %d\n",value);
		sum+=value;
	}
	sum = sum/10;
	printk("average_data**********************sum = %d\n",sum);
	return sum;
}

static int get_raw_data(struct adc_joystick_state *st){

	st->raw_data[0] = average_data(st->channelx);
	printk("lqblqb get_raw_data x = %d\n",st->raw_data[0]);

	st->raw_data[1] = average_data(st->channely);
	printk("lqblqb get_raw_data y = %d\n",st->raw_data[1]);
	
	st->raw_data[2] = average_data(st->channelrz);
	printk("lqblqb get_raw_data rz = %d\n",st->raw_data[2]);

	st->raw_data[3] = average_data(st->channelz);
	printk("lqblqb get_raw_data z = %d\n",st->raw_data[3]);
	return 0;
}


static int cal_data(int val,int data){
	int ret = 0;
	
//	DBG("lqb cal_data00000 val = %d ,data = %d\n",val,data);
	if(val > (data +100))
	{
		ret = 128 + (val-data) * 127/(1700 -data);		
//		DBG("lqb cal_data11111 %d\n",ret);
	}
	else if(val < (data - 100 ))
	{
		ret = val * 127/(data-100);	
//		DBG("lqb cal_data22222 %d\n",ret);
	}
	else {
		ret = 128;
//	 DBG("lqb cal_data33333 %d\n",ret);
	}
	
	return	ret;

}


static int adc_to_abs(int val,int data) {
	int abs_val;
	if(val > 1700) val = 1700;
		else if(val < 100) val = 0;
	abs_val = cal_data(val,data);
	return abs_val;
};







//static void adc_joystick_poll(struct input_polled_dev *dev) {
static void adc_joystick_poll(struct input_dev *dev){

	struct adc_joystick_state *st = input_get_drvdata(dev);
	int value, ret;
	int state = 0;
	DBG("lqbjoy *******************************\n");
//	if(st->flag){
//		get_raw_data(st);
//		st->flag = false;	
//	}
	
	// joystick x
	ret = iio_read_channel_processed(st->channelx, &value);
	if (unlikely(ret < 0)) {
		/* Forcibly release key if any was pressed */
		value = 1024;
	}
	
	st->data[0] = adc_to_abs(value,st->raw_data[0]);
	DBG("lqbjoy read adc_to_abs[x_val] %d,value = %d\n",adc_to_abs(value,st->raw_data[0]),value);
//	if(st->data[0] == 127) st->data[0] = 128;
	
	// joystick y
	ret = iio_read_channel_processed(st->channely, &value);
	if (unlikely(ret < 0)) {
		value = 1024;
	}
	
	st->data[1] = 255 - adc_to_abs(value,st->raw_data[1]);
	 DBG("lqbjoy read adc_to_abs[y_val] %d----value = %d\n",adc_to_abs(value,st->raw_data[1]),value);

	// joystick rz
	ret = iio_read_channel_processed(st->channelrz, &value);
	if (unlikely(ret < 0)) {
		/* Forcibly release key if any was pressed */
		value = 1024;
	}

	st->data[2] = adc_to_abs(value,st->raw_data[2]);
	
	DBG("lqbjoy read adc_to_abs[rz_val] %d----value = %d\n",adc_to_abs(value,st->raw_data[2]),value);
//	if(st->data[2] == 127) st->data[2] = 128;
	
	// joystick z
	ret = iio_read_channel_processed(st->channelz, &value);
	if (unlikely(ret < 0)) {
		/* Forcibly release key if any was pressed */
		value = 1024;
	}
	
	st->data[3] = adc_to_abs(value,st->raw_data[3]);
	
	DBG("lqbjoy read adc_to_abs[z_val] %d----value = %d\n",adc_to_abs(value,st->raw_data[3]),value);
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


static ssize_t peng_show(struct device *pdev,
			struct device_attribute *attr, char *buf){
	struct adc_joystick_state *st;
	struct platform_device *dev;
		printk("lqb peng_show	\n");
		
		dev = to_platform_device(pdev);
		st = dev_get_drvdata(&dev->dev);		
		printk("st->raw_data[0] = %d,st->raw_data[0] = %d,st->raw_data[0] = %d,st->raw_data[0] = %d\n",st->raw_data[0],st->raw_data[1],st->raw_data[2],st->raw_data[3]);
	return sprintf(buf, "%d,%d,%d,%d\n", st->raw_data[0],st->raw_data[1],st->raw_data[2],st->raw_data[3]);
//return sprintf(buf, "%d\n", st->raw_data[0]);
//return 1;
}

static ssize_t peng_store(struct device *pdev,
			struct device_attribute *attr, const char *buf, size_t count){
	struct adc_joystick_state *st;
	struct platform_device *dev;
	char *token;
	int err;
	int cou = 0;
	char str[30];
	int val[4];
	char *cur = str;
	
	memset(val,0,sizeof(val));
	printk("peng_store lqb buf:%s count:%d\n",buf,count);
//	printk();
	strcpy(str,buf);
	
	dev = to_platform_device(pdev);
	printk("peng_store str =  %s\n",str);
	st = dev_get_drvdata(&dev->dev);
	if(buf == NULL) return -1;
	if(count < 3)
	{
		printk("peng_store buf == 1,get_raw_data");
		get_raw_data(st);	
	}
	else{
		//token = strtok();strsep
		token = strsep(&cur,",");
		while( token && cou < 4){
			err = kstrtoint(token,10,&val[cou]);
			printk("peng_store %d Conversion result is %d\n",cou,val[cou]);
		    if (err) {
        		pr_err("peng_store Failed to parse string: %s\n", str);
		        return err;
   			}
			st->raw_data[cou] = val[cou];
			token = strsep(&cur,",");
			cou++;
		}
	}
	
//		if(NULL == buf || count >255 || count == 0 || strnchr(buf, count, 0x20))
//		return -1;	
	return count;
}	


static DEVICE_ATTR_RW(peng);


static struct attribute *peng_attrs[] = {
        &dev_attr_peng.attr,
        NULL
 };


static const struct attribute_group  peng_group_attrs = {
	.attrs = peng_attrs,
};
//ATTRIBUTE_GROUPS(peng);


static int adc_joystick_probe(struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
	struct adc_joystick_state *st;
//	struct input_polled_dev *poll_dev;
//	const struct platform_device_id *id;
//	struct device_driver *driver = NULL;

	struct input_dev *input;
	enum iio_chan_type type;
	int value;
	int error;
	
	st = devm_kzalloc(dev, sizeof(*st), GFP_KERNEL);
	if (!st) return -ENOMEM;
	
	//joystick x
	dev_set_drvdata(&pdev->dev,st);
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
error = sysfs_create_group(&pdev->dev.kobj,&peng_group_attrs);

//	id = platform_get_device_id(pdev);
//	
//	// 通过设备 ID 获取对应的 device_driver 指针
//	if (id) {
//		// 根据设备 ID 在驱动模块的 id_table 中查找匹配的 device_driver
//		driver = driver_find("adc_joysticks", pdev->dev.bus);
//	}
//
//	error = driver_create_file(driver, &driver_attr_peng);

	//joystick y
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
	st->channelz = devm_iio_channel_get(dev, "joystick_z");
	if (IS_ERR(st->channelz))
		return PTR_ERR(st->channelz);

	if (!st->channelz->indio_dev)
		return -ENXIO;
	
	error = iio_get_channel_type(st->channelz, &type);
	if (error < 0)
		return error;
		
	if (type != IIO_VOLTAGE) {
		dev_err(dev, "Incompatible channel type %d\n", type);
		return -EINVAL;
	}


	//joystick rz
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
//	set_bit(ABS_RX, input->absbit); 
//	set_bit(ABS_RY, input->absbit);
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

