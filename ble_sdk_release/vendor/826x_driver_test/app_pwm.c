/*
 * app_pwm.c
 *
 *  Created on: 2017-8-25
 *      Author: Administrator
 */

#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/drivers/spi.h"



#if (DRIVER_TEST_MODE == TEST_PWM)


void app_pwm_test(void)
{
	pwm_set_clk(CLOCK_SYS_CLOCK_HZ, CLOCK_SYS_CLOCK_HZ);


#if (MCU_CORE_TYPE == MCU_CORE_8266)  //8266 PWM
	//PC0 PWM0  1ms cycle  1/2 duty
	gpio_set_func(GPIO_PC0, AS_PWM);
	pwm_set_mode(PWM0_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM0_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM0_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (500 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM0_ID);


	//PC3 PWM1  1ms cycle  1/3 duty
	gpio_set_func(GPIO_PC3, AS_PWM);
	pwm_set_mode(PWM1_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM1_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM1_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (333 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM1_ID);


	//PC5 PWM2_N   1ms cycle  1/3 duty
	gpio_set_func(GPIO_PC5, AS_PWM);
	pwm_set_mode(PWM2_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM2_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM2_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (666 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM2_ID);


	//PA4 PWM3_N  1ms cycle  4/5 duty
	gpio_set_func(GPIO_PA4, AS_PWM);
	pwm_set_mode(PWM3_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM3_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM3_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM3_ID);


	//PA5 PWM4  1ms cycle  1/5 duty
	gpio_set_func(GPIO_PA5, AS_PWM);
	pwm_set_mode(PWM4_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM4_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM4_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM4_ID);


	//PB0 PWM5  1ms cycle  1/5 duty
	gpio_set_func(GPIO_PB0, AS_PWM);
	pwm_set_mode(PWM5_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM5_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM5_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM5_ID);

#else //8261/8267/8269 PWM

	//PA0 PWM0  1ms cycle  1/2 duty
	gpio_set_func(GPIO_PA0, AS_PWM);
	pwm_set_mode(PWM0_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM0_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM0_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (500 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM0_ID);


	//PA3 PWM1  1ms cycle  1/3 duty
	gpio_set_func(GPIO_PA3, AS_PWM);
	pwm_set_mode(PWM1_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM1_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM1_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (333 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM1_ID);


	//PA5 PWM2_N   1ms cycle  1/3 duty
	gpio_set_func(GPIO_PA5, AS_PWM);
	pwm_set_mode(PWM2_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM2_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM2_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (666 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM2_ID);


	//PB3 PWM3_N  1ms cycle  4/5 duty
	gpio_set_func(GPIO_PB3, AS_PWM);
	pwm_set_mode(PWM3_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM3_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM3_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM3_ID);

	//PB4 PWM4  1ms cycle  1/5 duty
	gpio_set_func(GPIO_PB4, AS_PWM);
	pwm_set_mode(PWM4_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM4_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM4_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM4_ID);


	//PB6 PWM5  1ms cycle  1/5 duty
	gpio_set_func(GPIO_PB6, AS_PWM);
	pwm_set_mode(PWM5_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM5_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM5_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM5_ID);

#endif

}









#endif
