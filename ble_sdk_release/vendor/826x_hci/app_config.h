#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif
#if		(__PROJECT_8261_HCI__)
	#define CHIP_TYPE				CHIP_TYPE_8261
#elif   (__PROJECT_8267_HCI__)
	#define CHIP_TYPE				CHIP_TYPE_8267
#elif   (__PROJECT_8269_HCI__)
	#define CHIP_TYPE				CHIP_TYPE_8269
#endif



/////////////////////HCI ACCESS OPTIONS///////////////////////////////////////
#define HCI_USE_UART	1
#define HCI_USE_USB		0
#define HCI_ACCESS		HCI_USE_UART

/////////////////// Configure /////////////////////////////////
#define BLE_MODULE_PM_ENABLE				0

//////////////////////////// MODULE PM GPIO	/////////////////////////////////
#define GPIO_WAKEUP_MODULE					GPIO_PC6   //mcu wakeup module
#define	PC5_FUNC							AS_GPIO
#define PC5_INPUT_ENABLE					1
#define	PC5_OUTPUT_ENABLE					0
#define	PC5_DATA_OUT						0
#define GPIO_WAKEUP_MODULE_HIGH				gpio_setup_up_down_resistor(GPIO_PC5, PM_PIN_PULLUP_10K);
#define GPIO_WAKEUP_MODULE_LOW				gpio_setup_up_down_resistor(GPIO_PC5, PM_PIN_PULLDOWN_100K);


#define GPIO_WAKEUP_MCU						GPIO_PC7   //module wakeup mcu
#define	PC3_FUNC							AS_GPIO
#define PC3_INPUT_ENABLE					1
#define	PC3_OUTPUT_ENABLE					1
#define	PC3_DATA_OUT						0
#define GPIO_WAKEUP_MCU_HIGH				do{gpio_set_output_en(GPIO_PC3, 1); gpio_write(GPIO_PC3, 1);}while(0)
#define GPIO_WAKEUP_MCU_LOW					do{gpio_set_output_en(GPIO_PC3, 1); gpio_write(GPIO_PC3, 0);}while(0)
#define GPIO_WAKEUP_MCU_FLOAT				do{gpio_set_output_en(GPIO_PC3, 0); gpio_write(GPIO_PC3, 0);}while(0)

//////////////////////////// KEYSCAN/MIC  GPIO //////////////////////////////////
#define	PULL_WAKEUP_SRC_PC5		PM_PIN_PULLUP_10K
#define	PULL_WAKEUP_SRC_PD2		PM_PIN_PULLUP_10K
#define PC5_INPUT_ENABLE		1
#define PD2_INPUT_ENABLE		1


#define	PULL_WAKEUP_SRC_PB1		PM_PIN_PULLDOWN_100K
#define	PULL_WAKEUP_SRC_PB2		PM_PIN_PULLDOWN_100K
#define	PULL_WAKEUP_SRC_PB3		PM_PIN_PULLDOWN_100K
#define	PULL_WAKEUP_SRC_PB6		PM_PIN_PULLDOWN_100K
#define PB1_INPUT_ENABLE		1
#define PB2_INPUT_ENABLE		1
#define PB3_INPUT_ENABLE		1
#define PB6_INPUT_ENABLE		1


/////////////////////// POWER OPTIMIZATION  AT SUSPEND ///////////////////////
//notice that: all setting here aims at power optimization ,they depends on
//the actual hardware design.You should analyze your hardware board and then
//find out the io leakage

//shut down the input enable of some gpios, to lower io leakage at suspend state
//for example:  #define PA2_INPUT_ENABLE   0



/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_TYPE  		CLOCK_TYPE_PLL	//  one of the following:  CLOCK_TYPE_PLL, CLOCK_TYPE_OSC, CLOCK_TYPE_PAD, CLOCK_TYPE_ADC
#define CLOCK_SYS_CLOCK_HZ  	16000000

//////////////////Extern Crystal Type///////////////////////
#define CRYSTAL_TYPE			XTAL_12M		//  extern 12M crystal


/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE		0
#define WATCHDOG_INIT_TIMEOUT		500  //ms



/////////////open SWS digital pullup to prevent MCU err, this is must ////////////
#define PB0_DATA_OUT					1

/////////////////// set default   ////////////////
#include "../common/default_config.h"

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif
