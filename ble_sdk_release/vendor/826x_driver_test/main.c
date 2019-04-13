#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj/drivers/uart.h"

#if (__PROJECT_8261_DRIVER_TEST__ || __PROJECT_8266_DRIVER_TEST__ || __PROJECT_8267_DRIVER_TEST__ || __PROJECT_8269_DRIVER_TEST__)

extern my_fifo_t hci_rx_fifo;
extern void user_init();
extern void main_loop (void);

extern void app_i2c_test_irq_proc(void);
extern void app_uart_test_irq_proc(void);
extern void app_spi_test_irq_proc(void);
extern void app_timer_test_irq_proc(void);
extern void app_gpio_irq_test_proc(void);


_attribute_ram_code_ void irq_handler(void)
{

/***timer demo***/
#if (DRIVER_TEST_MODE == TEST_HW_TIMER)
	app_timer_test_irq_proc();

/***uart demo***/
#elif (DRIVER_TEST_MODE == TEST_UART)
	app_uart_test_irq_proc();

/***i2c demo***/
#elif (DRIVER_TEST_MODE == TEST_IIC)
	app_i2c_test_irq_proc();

/***spi demo ***/
#elif (DRIVER_TEST_MODE == TEST_SPI)
	app_spi_test_irq_proc();

#elif (DRIVER_TEST_MODE == TEST_GPIO_IRQ)

	app_gpio_irq_test_proc();

#endif

}

int main (void) {
	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init(CRYSTAL_TYPE);

	set_tick_per_us (CLOCK_SYS_CLOCK_HZ/1000000);
	clock_init();

	gpio_init();

	rf_drv_init(CRYSTAL_TYPE);

	user_init ();

	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}




#endif   //end of __PROJECT_826x_DRIVER_TEST__
