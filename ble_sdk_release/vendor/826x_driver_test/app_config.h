#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif


#if  (__PROJECT_8261_DRIVER_TEST__)
	#define CHIP_TYPE				CHIP_TYPE_8261
#elif (__PROJECT_8266_DRIVER_TEST__)
	#define CHIP_TYPE				CHIP_TYPE_8266
#elif (__PROJECT_8267_DRIVER_TEST__)
	#define CHIP_TYPE				CHIP_TYPE_8267
#else
	#define CHIP_TYPE				CHIP_TYPE_8269
#endif




/////////////////// TEST FEATURE SELECTION /////////////////////////////////

#define	TEST_HW_TIMER									1



#define	TEST_GPIO_IRQ									10



#define	TEST_UART										20


#define TEST_IIC										30


#define TEST_SPI										40


#define TEST_ADC										50


#define TEST_PWM										60



#define DRIVER_TEST_MODE								TEST_IIC





#if (DRIVER_TEST_MODE == TEST_ADC)
	#define BATT_CHECK_ENABLE    0
#endif


/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_TYPE  		CLOCK_TYPE_PLL	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	16000000
//////////////////Extern Crystal Type///////////////////////
#define CRYSTAL_TYPE			XTAL_12M		//  extern 12M crystal


/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE		0
#define WATCHDOG_INIT_TIMEOUT		500  //ms







#define DEBUG_GPIO_ENABLE							1

#if(DEBUG_GPIO_ENABLE)
	#if (__PROJECT_8266_DRIVER_TEST__)
		//define debug GPIO here according to your hardware
		//ch0-ch5: B0 A5 E5 F0 F1 E7
		#define GPIO_CHN0							GPIO_PB0
		#define GPIO_CHN1							GPIO_PA5
		#define GPIO_CHN2							GPIO_PE5
		#define GPIO_CHN3							GPIO_PF0
		#define GPIO_CHN4							GPIO_PF1
		#define GPIO_CHN5							GPIO_PE7

		#define PB0_OUTPUT_ENABLE					1
		#define PA5_OUTPUT_ENABLE					1
		#define PE5_OUTPUT_ENABLE					1
		#define PF0_OUTPUT_ENABLE					1
		#define PF1_OUTPUT_ENABLE					1
		#define PE7_OUTPUT_ENABLE					1
	#else  //8261/8267/8269
	    //define debug GPIO here according to your hardware
		//ch0-ch5: A7 A4 A3 E0 A1 A0
		#define GPIO_CHN0							GPIO_PA7
		#define GPIO_CHN1							GPIO_PA4
		#define GPIO_CHN2							GPIO_PA3
		#define GPIO_CHN3							GPIO_PE0
		#define GPIO_CHN4							GPIO_PA1
		#define GPIO_CHN5							GPIO_PA0

		#define PA7_OUTPUT_ENABLE					1
		#define PA4_OUTPUT_ENABLE					1
		#define PA3_OUTPUT_ENABLE					1
		#define PE0_OUTPUT_ENABLE					1
		#define PA1_OUTPUT_ENABLE					1
		#define PA0_OUTPUT_ENABLE					1
	#endif




	#define DBG_CHN0_LOW		gpio_write(GPIO_CHN0, 0)
	#define DBG_CHN0_HIGH		gpio_write(GPIO_CHN0, 0)
	#define DBG_CHN0_TOGGLE		gpio_toggle(GPIO_CHN0)
	#define DBG_CHN1_LOW		gpio_write(GPIO_CHN1, 0)
	#define DBG_CHN1_HIGH		gpio_write(GPIO_CHN1, 0)
	#define DBG_CHN1_TOGGLE		gpio_toggle(GPIO_CHN1)
	#define DBG_CHN2_LOW		gpio_write(GPIO_CHN2, 0)
	#define DBG_CHN2_HIGH		gpio_write(GPIO_CHN2, 0)
	#define DBG_CHN2_TOGGLE		gpio_toggle(GPIO_CHN2)
	#define DBG_CHN3_LOW		gpio_write(GPIO_CHN3, 0)
	#define DBG_CHN3_HIGH		gpio_write(GPIO_CHN3, 0)
	#define DBG_CHN3_TOGGLE		gpio_toggle(GPIO_CHN3)
	#define DBG_CHN4_LOW		gpio_write(GPIO_CHN4, 0)
	#define DBG_CHN4_HIGH		gpio_write(GPIO_CHN4, 0)
	#define DBG_CHN4_TOGGLE		gpio_toggle(GPIO_CHN4)
	#define DBG_CHN5_LOW		gpio_write(GPIO_CHN5, 0)
	#define DBG_CHN5_HIGH		gpio_write(GPIO_CHN5, 0)
	#define DBG_CHN5_TOGGLE		gpio_toggle(GPIO_CHN5)

#else
	#define DBG_CHN0_LOW
	#define DBG_CHN0_HIGH
	#define DBG_CHN0_TOGGLE
	#define DBG_CHN1_LOW
	#define DBG_CHN1_HIGH
	#define DBG_CHN1_TOGGLE
	#define DBG_CHN2_LOW
	#define DBG_CHN2_HIGH
	#define DBG_CHN2_TOGGLE
	#define DBG_CHN3_LOW
	#define DBG_CHN3_HIGH
	#define DBG_CHN3_TOGGLE
	#define DBG_CHN4_LOW
	#define DBG_CHN4_HIGH
	#define DBG_CHN4_TOGGLE
	#define DBG_CHN5_LOW
	#define DBG_CHN5_HIGH
	#define DBG_CHN5_TOGGLE
#endif  //end of DEBUG_GPIO_ENABLE





#include "../common/default_config.h"

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
