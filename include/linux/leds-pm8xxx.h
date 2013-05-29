/* Copyright (c) 2010-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LEDS_PM8XXX_H__
#define __LEDS_PM8XXX_H__


#define PM8XXX_LEDS_DEV_NAME	"pm8xxx-led"

#include <linux/android_alarm.h>
#include <linux/leds.h>

enum pm8xxx_blink_type {
	BLINK_STOP = -1,
	BLINK_UNCHANGE = 0,
	BLINK_64MS_PER_2SEC,
	BLINK_64MS_ON_310MS_PER_2SEC,
	BLINK_64MS_ON_2SEC_PER_2SEC,
	BLINK_1SEC_PER_2SEC,
};

enum pm8xxx_leds {
	PM8XXX_ID_GPIO24 = 0,
	PM8XXX_ID_GPIO25,
	PM8XXX_ID_GPIO26,
	PM8XXX_ID_LED_KB_LIGHT,
	PM8XXX_ID_LED_2,
	PM8XXX_ID_LED_1,
	PM8XXX_ID_LED_0,
	PM8XXX_ID_FLASH_LED_0,
	PM8XXX_ID_FLASH_LED_1,
};

enum pm8xxx_led_modes {
	PM8XXX_LED_MODE_MANUAL = 0,
	PM8XXX_LED_MODE_PWM1,
	PM8XXX_LED_MODE_PWM2,
	PM8XXX_LED_MODE_PWM3,
	PM8XXX_LED_MODE_DTEST1,
	PM8XXX_LED_MODE_DTEST2,
	PM8XXX_LED_MODE_DTEST3,
	PM8XXX_LED_MODE_DTEST4
};

int pm8xxx_led_config(enum pm8xxx_leds led_id,
		enum pm8xxx_led_modes led_mode, int max_current);

/* over voltage protection threshold */
enum wled_ovp_threshold {
	WLED_OVP_35V,
	WLED_OVP_32V,
	WLED_OVP_29V,
	WLED_OVP_27V,
};

/**
 *  wled_config_data - wled configuration data
 *  @num_strings - number of wled strings supported
 *  @ovp_val - over voltage protection threshold
 *  @boost_curr_lim - boot current limit
 *  @cp_select - high pole capacitance
 *  @ctrl_delay_us - delay in activation of led
 *  @dig_mod_gen_en - digital module generator
 *  @cs_out_en - current sink output enable
 *  @op_fdbck - selection of output as feedback for the boost
 *  @cabc_en - enable cabc for backlight pwm control
 */
struct wled_config_data {
	u8	num_strings;
	u8	ovp_val;
	u8	boost_curr_lim;
	u8	cp_select;
	u8	ctrl_delay_us;
	u16	comp_res_val;
	bool	dig_mod_gen_en;
	bool	cs_out_en;
	bool	op_fdbck;
	bool	cabc_en;
};

struct pm8xxx_led_platform_data {
	int				num_leds;
	struct pm8xxx_led_configure	*leds;
};

struct pm8xxx_led_data {
	struct led_classdev			cdev;
	struct pwm_device 		*pwm_led;
	int							  id;
	int							bank;
	int				  function_flags;
	int					   period_us;
	int 				duty_time_ms;
	int 				 start_index;
	int 				 duites_size;
	int 					lut_flag;
	int					 out_current;
	int 				     *duties;
	int 					led_sync;
	u8			             	 reg;
	struct device				*dev;
	struct delayed_work		blink_delayed_work;
	struct delayed_work 	fade_delayed_work;
	struct work_struct 		led_work;
	struct alarm		   led_alarm;
	void (*gpio_status_switch)(bool);
	int 	(*lpm_power)(int on);
};
void pm8xxx_led_current_set_for_key(int brightness_key);

#endif 
