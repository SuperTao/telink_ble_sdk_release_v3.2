
#include "../../proj/tl_common.h"

#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/ble_phy.h"


#if (REMOTE_IR_ENABLE)
#include "rc_ir.h"
#endif


#if (__PROJECT_8261_BLE_REMOTE__ || __PROJECT_8266_BLE_REMOTE__ || __PROJECT_8267_BLE_REMOTE__ || __PROJECT_8269_BLE_REMOTE__)

extern void user_init();
extern void main_loop (void);
extern void deep_wakeup_proc(void);

_attribute_ram_code_ void irq_handler(void)
{


#if (REMOTE_IR_ENABLE)
		u8 pwm_sta = reg_pwm_irq_sta;
	#if (IR_PWM_SELECT == PWM0_IR_MODE)
		if(pwm_sta & FLD_IRQ_PWM0_PNUM){
			ir_irq_send();
			reg_pwm_irq_sta = FLD_IRQ_PWM0_PNUM;
		}
	#elif (IR_PWM_SELECT == PWM1_IR_MODE)
		if(pwm_sta & FLD_IRQ_PWM1_PNUM){
			ir_irq_send();
			reg_pwm_irq_sta = FLD_IRQ_PWM1_PNUM;
		}
	#endif


	u32 src = reg_irq_src;
	if(src & FLD_IRQ_TMR2_EN){
		ir_repeat_handle();
		reg_tmr_sta = FLD_TMR_STA_TMR2;
	}
#endif




	irq_blt_sdk_handler ();





#if (BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART || BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART )
	extern my_fifo_t hci_rx_fifo;

	unsigned char irqS = reg_dma_rx_rdy0;
    if(irqS & FLD_DMA_UART_RX)	//rx
    {
    	reg_dma_rx_rdy0 = FLD_DMA_UART_RX;
    	u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    	if(w[0]!=0)
    	{
    		my_fifo_next(&hci_rx_fifo);
    		u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    		reg_dma0_addr = (u16)((u32)p);
    	}
    }

    if(irqS & FLD_DMA_UART_TX)	//tx
    {
    	reg_dma_rx_rdy0 = FLD_DMA_UART_TX;
		#if (MCU_CORE_TYPE == MCU_CORE_8266)
				uart_clr_tx_busy_flag();
		#endif
    }
#endif

}

int main (void) {

	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init(CRYSTAL_TYPE);

	set_tick_per_us (CLOCK_SYS_CLOCK_HZ/1000000);
	clock_init();

	gpio_init();

	deep_wakeup_proc();

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
