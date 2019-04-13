/*
 * blt_led.h
 *
 *  Created on: 2016-1-29
 *      Author: Administrator
 */

#ifndef BLT_LED_H_
#define BLT_LED_H_

#include "../../proj/tl_common.h"


//led management
typedef struct{
	unsigned short onTime_ms;
	unsigned short offTime_ms;

	unsigned char  repeatCount;  //0xff special for long on(offTime_ms=0)/long off(onTime_ms=0)
	unsigned char  priority;     //0x00 < 0x01 < 0x02 < 0x04 < 0x08 < 0x10 < 0x20 < 0x40 < 0x80
} led_cfg_t;

typedef struct {
	unsigned char  isOn;
	unsigned char  polar;
	unsigned char  repeatCount;
	unsigned char  priority;


	unsigned short onTime_ms;
	unsigned short offTime_ms;

	unsigned int gpio_led;
	unsigned int startTick;
}device_led_t;

extern device_led_t device_led;

#define  DEVICE_LED_BUSY	(device_led.repeatCount)

extern void led_proc(void);
extern void device_led_init(u32 gpio,u8 polarity);
int device_led_setup(led_cfg_t led_cfg);

static inline void device_led_process(void)
{
	if(DEVICE_LED_BUSY){
		led_proc();
	}
}




#endif /* BLT_LED_H_ */
