#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/hci/hci.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/service/ble_ll_ota.h"
#include "../../proj/drivers/adc.h"
#include "../../proj_lib/ble/ble_smp.h"
#include "../../proj_lib/ble/ble_phy.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/emi.h"
#include "../common/blt_soft_timer.h"


#if (__PROJECT_8261_FEATURE_TEST__ || __PROJECT_8266_FEATURE_TEST__ || __PROJECT_8267_FEATURE_TEST__ || __PROJECT_8269_FEATURE_TEST__)


#if (HCI_ACCESS==HCI_USE_UART)
#include "../../proj/drivers/uart.h"
#endif

#if (HCI_ACCESS==HCI_USE_UART || FEATURE_TEST_MODE == TEST_BLE_PHY)
	MYFIFO_INIT(hci_rx_fifo, 72, 2);
	MYFIFO_INIT(hci_tx_fifo, 72, 8);
#endif

#if (BLE_LONG_PACKET_ENABLE)
	MYFIFO_INIT(blt_rxfifo, 96, 8);
	MYFIFO_INIT(blt_txfifo, 240, 16);
#else
	MYFIFO_INIT(blt_rxfifo, 64, 8);
	MYFIFO_INIT(blt_txfifo, 40, 16);
#endif



int  app_whilteList_enable;


void	task_connect (u8 e, u8 *p, int n)
{

}

void	task_terminate (u8 e, u8 *p, int n)
{

}

void	task_paring_begin (u8 e, u8 *p, int n)
{

}



u8 paring_result = 0xff;
void	task_paring_end (u8 e, u8 *p, int n)
{
		paring_result = *p;

		if(paring_result == BLE_SUCCESS){

		}
		else{
			// paring_result is fail reason
		}

}

void	task_encryption_done (u8 e, u8 *p, int n)
{
		if(*p == SMP_STANDARD_PAIR){  //first paring

		}
		else if(*p == SMP_FAST_CONNECT){  //auto connect

		}
}




int AA_dbg_suspend;
void  func_suspend_enter (u8 e, u8 *p, int n)
{
	AA_dbg_suspend ++;
}

void  func_suspend_exit (u8 e, u8 *p, int n)
{

}








#if (FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)
	int gpio_test0(void)
	{
		//gpio 0 toggle to see the effect
		//DBG_CHN0_TOGGLE;

		return 0;
	}


	int gpio_test1(void)
	{
		//gpio 1 toggle to see the effect
		//DBG_CHN1_TOGGLE;

		static u8 flg = 0;
		flg = !flg;
		if(flg){
			return 7000;
		}
		else{
			return 17000;
		}

	}

	int gpio_test2(void)
	{
		//gpio 2 toggle to see the effect
		//DBG_CHN2_TOGGLE;

		//timer last for 5 second
		if(clock_time_exceed(0, 5000000)){
			//return -1;
			//blt_soft_timer_delete(&gpio_test2);
		}
		else{

		}

		return 0;
	}

	int gpio_test3(void)
	{
		//gpio 3 toggle to see the effect
		//DBG_CHN3_TOGGLE;

		return 0;
	}

#endif











#if (	 FEATURE_TEST_MODE == TEST_SCANNING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE \
	  || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
#define DBG_ADV_REPORT_ON_RAM 				1
#if (DBG_ADV_REPORT_ON_RAM)  //debug adv report on ram
	#define  RAM_ADV_MAX		64
	u8 AA_advRpt[RAM_ADV_MAX][48];
	u8 AA_advRpt_index = 0;
#endif

int app_event_callback (u32 h, u8 *p, int n)
{

	static u32 event_cb_num;
	event_cb_num++;

	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];
			if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event

				event_adv_report_t *pa = (event_adv_report_t *)p;
				s8 rssi = pa->data[pa->len];

				#if (PRINT_DEBUG_INFO)
					printf("LE advertising report:\n");foreach(i, pa->len + 11){PrintHex(p[i]);}printf("\n");
				#endif

				#if (DBG_ADV_REPORT_ON_RAM)
					if(pa->len > 31){
						pa->len = 31;
					}
					memcpy( (u8 *)AA_advRpt[AA_advRpt_index++],  p, pa->len + 11);
					if(AA_advRpt_index >= RAM_ADV_MAX){
						AA_advRpt_index = 0;
					}
				#endif

				DBG_CHN3_TOGGLE;

			}
		}
	}

}
#endif




#if (HCI_ACCESS==HCI_USE_UART)
	int rx_from_uart_cb (void)
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


	int tx_to_uart_cb (void)
	{
		static u32 uart_tx_tick = 0;

		u8 *p = my_fifo_get (&hci_tx_fifo);


	#if (ADD_DELAY_FOR_UART_DATA)
		if (p && !uart_tx_is_busy () && clock_time_exceed(uart_tx_tick, 30000))
	#else
		if (p && !uart_tx_is_busy ())
	#endif
		{
			memcpy(&T_txdata_buf.data, p + 2, p[0]+p[1]*256);
			T_txdata_buf.len = p[0]+p[1]*256 ;


			if (uart_Send((u8 *)(&T_txdata_buf)))
			{
				uart_tx_tick = clock_time();

				my_fifo_pop (&hci_tx_fifo);
			}
		}
		return 0;
	}
#elif (HCI_ACCESS==HCI_USE_USB)

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


#if (USB_ADV_REPORT_TO_PC_ENABLE)
	REG_ADDR8(0x74) = 0x53;
	REG_ADDR16(0x7e) = 0x08d1;
	REG_ADDR8(0x74) = 0x00;
	usb_log_init ();
	usb_dp_pullup_en (1);  //open USB enum
#endif


////////////////// BLE stack initialization ////////////////////////////////////
	rf_set_power_level_index (RF_POWER_8dBm);

	u8  tbl_mac [] = {0xe1, 0xe1, 0xe2, 0xe3, 0xe4, 0xc7};
	u32 *pmac = (u32 *) CFG_ADR_MAC;
	if (*pmac != 0xffffffff)
	{
		memcpy (tbl_mac, pmac, 6);
	}
	else{
		tbl_mac[0] = (u8)rand();
		flash_write_page (CFG_ADR_MAC, 6, tbl_mac);
	}

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU(tbl_mac);   //mandatory



#if (FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY)

	blc_ll_initAdvertising_module(tbl_mac);


	u8 tbl_advData[] = {
		 0x08, 0x09, 't', 'e', 's', 't', 'a', 'd', 'v',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x08, 0x09, 'T', 'E', 'S', 'T', 'A', 'D', 'V',	//scan name
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_100MS, ADV_INTERVAL_100MS, \
									ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);


	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}

	blc_ll_setAdvCustomedChannel(37, 38, 39);
	bls_ll_setAdvEnable(1);  //adv enable

#elif (FEATURE_TEST_MODE == TEST_SCANNING_ONLY)

	blc_ll_initScanning_module(tbl_mac);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(app_event_callback);


	////// set scan parameter and scan enable /////
	#if 1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_INTERVAL_90MS,
							  	  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report adv only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_INTERVAL_90MS,
							  	  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
	#endif

	blc_ll_setScanEnable (BLC_SCAN_ENABLE, 0);

#elif (FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE)

	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER ); 	//smp initialization
	//HID_service_on_android7p0_init();  //hid device on android 7.0

///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x09, 0x09, 's', 'l', 'a', 'v', 'e', 'a', 'd', 'v',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x09, 0x09, 'S', 'L', 'A', 'V', 'E', 'A', 'D', 'V',
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}


	bls_ll_setAdvEnable(1);  //adv enable


	//add advertising in connection slave role
	u8 tbl_advData_test[] = {
		 0x09, 0x09, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp_test [] = {
			 0x09, 0x09, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
		};
	blc_ll_addAdvertisingInConnSlaveRole();  //adv in conn slave role
	blc_ll_setAdvParamInConnSlaveRole(  (u8 *)tbl_advData_test, sizeof(tbl_advData_test), \
										(u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test), \
										ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);

#elif (FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE)
	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER ); 	//smp initialization
	//HID_service_on_android7p0_init();  //hid device on android 7.0

///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x0A, 0x09, 's', 'l', 'a', 'v', 'e', 's', 'c', 'a', 'n',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x0A, 0x09, 'S', 'L', 'A', 'V', 'E', 'S', 'C', 'A','N'
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}


	bls_ll_setAdvEnable(1);  //adv enable



	//scan setting
	blc_ll_initScanning_module(tbl_mac);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(app_event_callback);

	#if 1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report adv only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);

	#endif

	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role




	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);


#elif (FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER ); 	//smp initialization
	//HID_service_on_android7p0_init();  //hid device on android 7.0

///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x0A, 0x09, 's', 'l', 'a', 'v', 'e', 's', 'c', 'a', 'n',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x0A, 0x09, 'S', 'L', 'A', 'V', 'E', 'S', 'C', 'A','N'
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}


	bls_ll_setAdvEnable(1);  //adv enable



	//add advertising in connection slave role
	u8 tbl_advData_test[] = {
			 0x09, 0x09, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
			 0x02, 0x01, 0x05,
			};
		u8	tbl_scanRsp_test [] = {
				 0x09, 0x09, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
			};
		blc_ll_addAdvertisingInConnSlaveRole();  //adv in conn slave role
		blc_ll_setAdvParamInConnSlaveRole(  (u8 *)tbl_advData_test, sizeof(tbl_advData_test), \
											(u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test), \
											ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);



	//scan setting
	blc_ll_initScanning_module(tbl_mac);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(app_event_callback);

	#if 0  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report adv only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
	#endif

	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role




	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);

#elif (FEATURE_TEST_MODE == TEST_POWER_ADV)

	sleep_us(3000000);

	blc_ll_initAdvertising_module(tbl_mac);

	u8 tbl_advData[] = {
		 0x08, 0x09, 't', 'e', 's', 't', 'a', 'd', 'v',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x08, 0x09, 'T', 'E', 'S', 'T', 'A', 'D', 'V',	//scan name
		};

	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	//1S 1 channel   30uA
	//u8 status = bls_ll_setAdvParam( ADV_INTERVAL_1S, ADV_INTERVAL_1S, \
									ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);  //no scan, no connect

	//1S 3 channel   54uA
	//u8 status = bls_ll_setAdvParam( ADV_INTERVAL_1S, ADV_INTERVAL_1S, \
									ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);  //no scan, no connect

	//400mS 3 channel   122uA
	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_400MS, ADV_INTERVAL_400MS, \
									ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);  //no scan, no connect


	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}

	bls_ll_setAdvEnable(1);  //adv enable

	bls_pm_enableAdvMcuStall(1);
	rf_set_power_level_index (RF_POWER_0dBm);




#elif ( FEATURE_TEST_MODE == TEST_SMP_PASSKEY_ENTRY  )

	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,

	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization

	#if(FEATURE_TEST_MODE == TEST_SMP_PASSKEY_ENTRY)
		bls_smp_enableParing (SMP_PARING_CONN_TRRIGER ); 	//smp initialization
		blc_smp_enableAuthMITM (1, 123456);//pincode
		blc_smp_setIoCapability (IO_CAPABLITY_DISPLAY_ONLY);
	#else
		bls_smp_enableParing (SMP_PARING_CONN_TRRIGER ); 	//smp initialization
	#endif


///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x08, 0x09, 't', 'e', 's', 't', 'S', 'M', 'P',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x08, 0x09, 't', 'e', 's', 't', 'S', 'M', 'P',
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS,
						ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
						0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);

	bls_ll_setAdvEnable(1);  //adv enable

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);


#elif(FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)

	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,

	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER ); 	//smp initialization


///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x08, 0x09, 't', 'e', 's', 't', 'T', 'I', 'M',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x08, 0x09, 't', 'e', 's', 't', 'T', 'I', 'M',
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);

	bls_ll_setAdvEnable(1);  //adv enable

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);


	//////////////// TEST  /////////////////////////
	//common/blt_soft_timer.h   #define		BLT_SOFTWARE_TIMER_ENABLE				1
	blt_soft_timer_init();
	blt_soft_timer_add(&gpio_test0, 23000);
	blt_soft_timer_add(&gpio_test1, 7000);
	blt_soft_timer_add(&gpio_test2, 13000);
	blt_soft_timer_add(&gpio_test3, 27000);

#elif(FEATURE_TEST_MODE == TEST_WHITELIST)

	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,

	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER ); 	//smp initialization




///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x05, 0x09, 't', 'e', 's', 't',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x05, 0x09, 't', 'e', 's', 't',
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));



	smp_param_save_t  bondInfo;
	u8 bond_number = blc_smp_param_getCurrentBondingDeviceNumber();  //get bonded device number
	if(bond_number)   //get latest device info
	{
		blc_smp_param_loadByIndex( bond_number - 1, &bondInfo);  //get the latest bonding device (index: bond_number-1 )
	}


	ll_whiteList_reset(); 	  //clear whitelist
	ll_resolvingList_reset(); //clear resolving list


	if(bond_number)  //use whitelist to filter master device
	{
		app_whilteList_enable = 1;

		//if master device use RPA(resolvable private address), must add irk to resolving list
		if( IS_RESOLVABLE_PRIVATE_ADDR(bondInfo.peer_addr_type, bondInfo.peer_addr) ){
			//resolvable private address, should add peer irk to resolving list
			ll_resolvingList_add(bondInfo.peer_id_adrType, bondInfo.peer_id_addr, bondInfo.peer_irk, NULL);  //no local IRK
			ll_resolvingList_setAddrResolutionEnable(1);
		}
		else{
			//if not resolvable random address, add peer address to whitelist
			ll_whiteList_add(bondInfo.peer_addr_type, bondInfo.peer_addr);
		}


		bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
							ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
							0,  NULL, BLT_ENABLE_ADV_37, ADV_FP_ALLOW_SCAN_WL_ALLOW_CONN_WL);

	}
	else{

		bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS,
							ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
							0,  NULL, BLT_ENABLE_ADV_37, ADV_FP_NONE);
	}



	bls_ll_setAdvEnable(1);  //adv enable



	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);

	bls_app_registerEventCallback (BLT_EV_FLAG_PAIRING_BEGIN, &task_paring_begin);
	bls_app_registerEventCallback (BLT_EV_FLAG_PAIRING_END, &task_paring_end);
	bls_app_registerEventCallback (BLT_EV_FLAG_ENCRYPTION_CONN_DONE, &task_encryption_done);

#elif (FEATURE_TEST_MODE == TEST_BLE_PHY)

	blc_phy_initPhyTest_module();
	blc_phy_setPhyTestEnable( BLC_PHYTEST_ENABLE );


	#if	(BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART)
			#if (MCU_CORE_TYPE == MCU_CORE_8266)
				//8266 only PC6/PC7 uart function
				gpio_set_input_en(GPIO_PC6, 1);
				gpio_set_input_en(GPIO_PC7, 1);
				gpio_setup_up_down_resistor(GPIO_PC6, PM_PIN_PULLUP_1M);
				gpio_setup_up_down_resistor(GPIO_PC7, PM_PIN_PULLUP_1M);
				gpio_set_func(GPIO_PC6, AS_UART);
				gpio_set_func(GPIO_PC7, AS_UART);
			#else
				//8261/8267/8269 demo code use PB2/PB3, user can change them to PC2/PC3 or PA6/PA7
				gpio_set_input_en(GPIO_PB2, 1);
				gpio_set_input_en(GPIO_PB3, 1);
				gpio_setup_up_down_resistor(GPIO_PB2, PM_PIN_PULLUP_1M);
				gpio_setup_up_down_resistor(GPIO_PB3, PM_PIN_PULLUP_1M);
				gpio_set_func(GPIO_PB2, AS_UART);
				gpio_set_func(GPIO_PB3, AS_UART);
			#endif

			reg_dma_rx_rdy0 = FLD_DMA_UART_RX | FLD_DMA_UART_TX; //clear uart rx/tx status
			#if (CLOCK_SYS_CLOCK_HZ == 32000000)
				CLK32M_UART115200;
			#elif(CLOCK_SYS_CLOCK_HZ == 16000000)
				CLK16M_UART115200;
			#else
				need config uart clock here
			#endif

			uart_BuffInit(hci_rx_fifo_b, hci_rx_fifo.size, hci_tx_fifo_b);

			blc_register_hci_handler (phy_test_2_wire_rx_from_uart, phy_test_2_wire_tx_to_uart);
	#endif
#elif (FEATURE_TEST_MODE == TEST_EMI)
			emi_test();
#endif



#if (BLE_PM_ENABLE)
	blc_ll_initPowerManagement_module();
	bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);

	#if (FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
		bls_pm_setSuspendMask (SUSPEND_ADV);
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif

	//bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_ENTER, &func_suspend_enter);
	//bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &func_suspend_exit);
#endif







#if (HCI_ACCESS==HCI_NONE)

#elif (HCI_ACCESS==HCI_USE_USB)
	usb_bulk_drv_init (0);
#elif (HCI_ACCESS==HCI_USE_UART)
	gpio_set_input_en(GPIO_PB2, 1);
	gpio_set_input_en(GPIO_PB3, 1);
	gpio_setup_up_down_resistor(GPIO_PB2, PM_PIN_PULLUP_1M);
	gpio_setup_up_down_resistor(GPIO_PB3, PM_PIN_PULLUP_1M);
	uart_io_init(UART_GPIO_8267_PB2_PB3);

	reg_dma_rx_rdy0 = FLD_DMA_UART_RX | FLD_DMA_UART_TX; //clear uart rx/tx status
	#if (CLOCK_SYS_CLOCK_HZ == 32000000)
		CLK32M_UART115200;
	#else
		CLK16M_UART115200;
	#endif

	uart_BuffInit(hci_rx_fifo_b, hci_rx_fifo.size, hci_tx_fifo_b);
#endif



}


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32 tick_wakeup;
void main_loop (void)
{
	static u32 tick_loop;

	tick_loop ++;


#if (FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)
	blt_soft_timer_process(MAINLOOP_ENTRY);
#endif

	blt_sdk_main_loop();
}




#endif  // end of __PROJECT_826x_FEATURE_TEST__
