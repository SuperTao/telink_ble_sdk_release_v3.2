#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ll/ll.h"

extern my_fifo_t hci_rx_fifo;
extern void user_init();

_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler ();

#if (HCI_ACCESS==HCI_USE_UART)
	unsigned char irqS = reg_dma_rx_rdy0;
    // 刚开始接受数据就会进入中断
	if(irqS & FLD_DMA_UART_RX)	//rx
    {
    	// 清中断
    	reg_dma_rx_rdy0 = FLD_DMA_UART_RX;
    	// 获取fifo的地址
    	u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    	// 判断fifo中数据是否为空
    	if(w[0]!=0)
    	{
    		my_fifo_next(&hci_rx_fifo);
    		u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    		// 把fifo中数据的地址放到DMA里面，接收的数据从外设保存到DMA指定的FIFO中
    		// 后面接受的数据就会保存在fifo里面
    		reg_dma0_addr = (u16)((u32)p);
    	}
    }

    if(irqS & FLD_DMA_UART_TX)	//tx
    {
    	// 清中断
    	reg_dma_rx_rdy0 = FLD_DMA_UART_TX;
#if __PROJECT_8266_MODULE__
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
