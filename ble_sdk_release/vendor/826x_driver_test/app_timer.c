#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/drivers/spi.h"

#if (DRIVER_TEST_MODE == TEST_HW_TIMER)

int timer0_irq_cnt = 0;
int timer1_irq_cnt = 0;
int timer2_irq_cnt = 0;


void app_timer_test_init(void){
	//timer0 10ms interval irq
	reg_irq_mask |= FLD_IRQ_TMR0_EN;
	reg_tmr0_tick = 0; //claer counter
	reg_tmr0_capt = 10 * CLOCK_SYS_CLOCK_1MS;
	reg_tmr_sta = FLD_TMR_STA_TMR0; //clear irq status
	reg_tmr_ctrl |= FLD_TMR0_EN;  //start timer

	//timer1 15ms interval irq
	reg_irq_mask |= FLD_IRQ_TMR1_EN;
	reg_tmr1_tick = 0; //claer counter
	reg_tmr1_capt = 15 * CLOCK_SYS_CLOCK_1MS;
	reg_tmr_sta = FLD_TMR_STA_TMR1; //clear irq status
	reg_tmr_ctrl |= FLD_TMR1_EN;  //start timer


	//timer1 20ms interval irq
	reg_irq_mask |= FLD_IRQ_TMR2_EN;
	reg_tmr2_tick = 0; //claer counter
	reg_tmr2_capt = 20 * CLOCK_SYS_CLOCK_1MS;
	reg_tmr_sta = FLD_TMR_STA_TMR2; //clear irq status
	reg_tmr_ctrl |= FLD_TMR2_EN;  //start timer

	irq_enable();
}


_attribute_ram_code_ void app_timer_test_irq_proc(void){
	if(reg_tmr_sta & FLD_TMR_STA_TMR0){
		reg_tmr_sta = FLD_TMR_STA_TMR0; //clear irq status
		timer0_irq_cnt ++;
		DBG_CHN0_TOGGLE;
	}

	if(reg_tmr_sta & FLD_TMR_STA_TMR1){
		reg_tmr_sta = FLD_TMR_STA_TMR1; //clear irq status
		timer1_irq_cnt ++;
		DBG_CHN1_TOGGLE;
	}

	if(reg_tmr_sta & FLD_TMR_STA_TMR2){
		reg_tmr_sta = FLD_TMR_STA_TMR2; //clear irq status
		timer2_irq_cnt ++;
		DBG_CHN2_TOGGLE;
	}
}


#endif
