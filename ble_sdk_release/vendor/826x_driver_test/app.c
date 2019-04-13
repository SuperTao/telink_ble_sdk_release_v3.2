#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/drivers/spi.h"


#if (__PROJECT_8261_DRIVER_TEST__ || __PROJECT_8266_DRIVER_TEST__ || __PROJECT_8267_DRIVER_TEST__ || __PROJECT_8269_DRIVER_TEST__)

extern void app_i2c_test_init(void);
extern void app_i2c_test_start(void);

extern void app_spi_test_init(void);
extern void app_spi_test_start(void);

extern void app_uart_test_init(void);
extern void app_uart_test_start(void);

extern void app_adc_test_init(void);
extern void app_adc_test_start(void);

extern void app_pwm_test(void);

extern void app_gpio_irq_test_init(void);

extern void app_led_init(void);

void user_init()
{

#if (DRIVER_TEST_MODE == TEST_HW_TIMER)

	app_timer_test_init();

#elif (DRIVER_TEST_MODE == TEST_GPIO_IRQ)

	app_gpio_irq_test_init();
	app_led_init();

#elif (DRIVER_TEST_MODE == TEST_UART)

	app_uart_test_init();

#elif (DRIVER_TEST_MODE == TEST_IIC)

	app_i2c_test_init();

#elif (DRIVER_TEST_MODE == TEST_SPI)

	app_spi_test_init();

#elif (DRIVER_TEST_MODE == TEST_ADC)

	app_adc_test_init();

#elif (DRIVER_TEST_MODE == TEST_PWM)

	app_pwm_test();

#else


#endif

}


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32 tick_wakeup;
void main_loop (void)
{
#if (DRIVER_TEST_MODE == TEST_UART)

	app_uart_test_start();

#elif (DRIVER_TEST_MODE == TEST_IIC)

	app_i2c_test_start();

#elif (DRIVER_TEST_MODE == TEST_SPI)

	app_spi_test_start();

#elif (DRIVER_TEST_MODE == TEST_ADC)
	app_adc_test_start();
#else

#endif

}




#endif  // end of __PROJECT_826x_DRIVER_TEST__
