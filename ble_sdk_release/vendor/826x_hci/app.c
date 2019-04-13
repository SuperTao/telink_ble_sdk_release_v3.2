#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/trace.h"
#include "../../proj/mcu/pwm.h"
#include "../../proj_lib/ble/service/ble_ll_ota.h"
#include "../../proj/drivers/adc.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ble_smp.h"

#if (HCI_ACCESS==HCI_USE_UART)
#include "../../proj/drivers/uart.h"
#endif

MYFIFO_INIT(hci_rx_fifo, 72, 2);
MYFIFO_INIT(hci_tx_fifo, 72, 8);

MYFIFO_INIT(blt_rxfifo, 64, 8);

MYFIFO_INIT(blt_txfifo, 40, 16);
//////////////////////////////////////////////////////////////////////////////
//	Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////
const u8	tbl_advData[] = {
	 0x05, 0x09, 't', 'M', 'o', 'd',
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
};

const u8	tbl_scanRsp [] = {
		 0x07, 0x09, 't', 'M', 'o', 'd', 'u', 'l',	//scan name " tmodul"
	};


void	task_connect (void)
{
	bls_l2cap_requestConnParamUpdate (12, 32, 99, 400);  //interval=10ms latency=99 timeout=4s
}


u32 tick_wakeup;
int	mcu_uart_working;
int	module_uart_working;
int module_task_busy;
int	module_uart_data_flg;

#define UART_TX_BUSY			( (hci_tx_fifo.rptr != hci_tx_fifo.wptr) || uart_tx_is_busy() )
#define UART_RX_BUSY			(hci_rx_fifo.rptr != hci_rx_fifo.wptr)

int app_module_busy ()
{
	mcu_uart_working = gpio_read(GPIO_WAKEUP_MODULE);  //mcu用GPIO_WAKEUP_MODULE指示 是否处于uart数据收发状态
	module_uart_working = UART_TX_BUSY || UART_RX_BUSY; //module自己检查uart rx和tx是否都处理完毕
	module_task_busy = mcu_uart_working || module_uart_working;
	return module_task_busy;
}

void app_suspend_exit ()
{
	GPIO_WAKEUP_MODULE_HIGH;  //module enter working state
	bls_pm_setSuspendMask(SUSPEND_DISABLE);
	tick_wakeup = clock_time () | 1;
}

int app_suspend_enter ()
{
	if (app_module_busy ())
	{
		app_suspend_exit ();
		return 0;
	}
	return 1;
}

void app_power_management ()
{
	// pullup GPIO_WAKEUP_MODULE: exit from suspend
	// pulldown GPIO_WAKEUP_MODULE: enter suspend
#if (BLE_MODULE_PM_ENABLE)

	if (!app_module_busy() && !tick_wakeup)
	{
		bls_pm_setSuspendMask(SUSPEND_ADV | SUSPEND_CONN);
		bls_pm_setWakeupSource(PM_WAKEUP_CORE);  //需要被 GPIO_WAKEUP_MODULE 唤醒
	}

	if (tick_wakeup && clock_time_exceed (tick_wakeup, 500))
	{
		GPIO_WAKEUP_MODULE_LOW;
		tick_wakeup = 0;
	}

#endif
}


#if (HCI_ACCESS==HCI_USE_UART)
int blc_rx_from_uart (void)
{
	if(my_fifo_get(&hci_rx_fifo) == 0)
	{
		return 0;
	}

	u8* p = my_fifo_get(&hci_rx_fifo);
	u32 rx_len = p[0]; //usually <= 255 so 1 byte should be sufficient

	if (rx_len)
	{
		blc_hci_handler(&p[4], rx_len - 4);
		my_fifo_pop(&hci_rx_fifo);
	}
	return 0;
}

int blc_hci_tx_to_uart ()
{
	uart_data_t T_txdata_buf;
	u8 *p = my_fifo_get (&hci_tx_fifo);
	if (p && !uart_tx_is_busy ())
	{
		memcpy(&T_txdata_buf.data, p + 2, p[0]+p[1]*256);
		T_txdata_buf.len = p[0]+p[1]*256 ;
		if (uart_Send((u8 *)(&T_txdata_buf)))
		{
			my_fifo_pop (&hci_tx_fifo);
		}
	}
	return 0;
}
#else

int app_hci_cmd_from_usb (void)
{
	u8 buff[72];
	extern int usb_bulk_out_get_data (u8 *p);
	int n = usb_bulk_out_get_data (buff);
	if (n)
	{
		blc_hci_handler (buff, n);
	}
	return 0;
}


#endif

void user_init()
{
	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value and tp value

	REG_ADDR8(0x74) = 0x53;
	REG_ADDR16(0x7e) = 0x08d1;
	REG_ADDR8(0x74) = 0x00;

	usb_log_init ();
	usb_dp_pullup_en (1);  //open USB enum


	////////////////// BLE stack initialization ////////////////////////////////////
	u8  tbl_mac [] = {0xe1, 0xe1, 0xe2, 0xe3, 0xe4, 0xc7};
	//u32 *pmac = (u32 *) (CFG_ADR_MAC + 10);
	u32 *pmac = (u32 *) CFG_ADR_MAC;
	if (*pmac != 0xffffffff)
	{
	    memcpy (tbl_mac, pmac, 6);
	}
    else
    {
        //TODO : should write mac to flash after pair OK
        tbl_mac[0] = (u8)rand();
    }

	///////////// Controller Link Layer Initialization ////////////////
	////// Controller Initialization  //////////
	blc_ll_initBasicMCU(tbl_mac);   //mandatory

	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();			//slave module: 	 mandatory for BLE slave,

	//l2cap initialization
	blc_l2cap_register_handler (blc_hci_sendACLData2Host); 		//send l2cap 2 uart
	blc_hci_registerControllerEventHandler(blc_hci_send_data);		//register event callback


	rf_set_power_level_index (RF_POWER_8dBm);

	////////////////// HCI interface initialization ///////////////////////////////////
	#if (HCI_ACCESS==HCI_USE_USB)
		usb_bulk_drv_init (0);
		blc_register_hci_handler (app_hci_cmd_from_usb, blc_hci_tx_to_usb);
	#else	//uart
		//one gpio should be configured to act as the wakeup pin if in power saving mode; pending
		gpio_set_input_en(GPIO_PB2, 1);
		gpio_set_input_en(GPIO_PB3, 1);
		gpio_setup_up_down_resistor(GPIO_PB2, PM_PIN_PULLUP_1M);
		gpio_setup_up_down_resistor(GPIO_PB3, PM_PIN_PULLUP_1M);
		uart_io_init(UART_GPIO_8267_PB2_PB3);

		reg_dma_rx_rdy0 = FLD_DMA_UART_RX | FLD_DMA_UART_TX; //clear uart rx/tx status
		CLK16M_UART115200;
		uart_BuffInit(hci_rx_fifo_b, hci_rx_fifo.size, hci_tx_fifo_b);
		blc_register_hci_handler (blc_rx_from_uart, blc_hci_tx_to_uart);		//default handler
	#endif

	bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);//(SUSPEND_DISABLE);//
#if (BLE_MODULE_PM_ENABLE)
	//mcu 可以通过拉高GPIO_WAKEUP_MODULE将 module从低低功耗唤醒
	gpio_set_wakeup		(GPIO_WAKEUP_MODULE, 1, 1);  // core(gpio) high wakeup suspend
	cpu_set_gpio_wakeup (GPIO_WAKEUP_MODULE, 1, 1);  // pad high wakeup deepsleep

	bls_pm_registerFuncBeforeSuspend( &app_suspend_enter );
#endif
}


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
void main_loop ()
{
	static u32 tick_loop;

	tick_loop ++;

	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();

	////////////////////////////////////// UI entry /////////////////////////////////
	//  add spp UI task
	app_power_management ();
}
