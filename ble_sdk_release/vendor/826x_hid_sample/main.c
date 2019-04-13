
#include "../../proj/tl_common.h"

#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"



#if (__PROJECT_8261_HID_SAMPLE__ || __PROJECT_8266_HID_SAMPLE__ || __PROJECT_8267_HID_SAMPLE__ || __PROJECT_8269_HID_SAMPLE__)

extern void user_init();
extern void main_loop (void);

_attribute_ram_code_ void irq_handler(void)
{

	irq_blt_sdk_handler ();

}

int main (void) {

	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init(CRYSTAL_TYPE);

	set_tick_per_us (CLOCK_SYS_CLOCK_HZ/1000000);
	clock_init();

	gpio_init();

	rf_drv_init(CRYSTAL_TYPE);

	user_init ();

    irq_enable();

	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}



#endif
