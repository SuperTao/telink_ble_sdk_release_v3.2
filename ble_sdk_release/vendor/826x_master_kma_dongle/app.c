#include "../../proj/tl_common.h"


#if (    __PROJECT_8261_MASTER_KMA_DONGLE__ || __PROJECT_8266_MASTER_KMA_DONGLE__ \
	  || __PROJECT_8267_MASTER_KMA_DONGLE__ || __PROJECT_8269_MASTER_KMA_DONGLE__ )


#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/ble_smp.h"
#include "../../proj_lib/ble/trace.h"
#include "../../proj/mcu/pwm.h"
#include "../../proj/drivers/audio.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj/drivers/uart.h"

#include "app_pair.h"

#if (HCI_ACCESS==HCI_USE_UART || HCI_ACCESS==HCI_USE_USB)
	MYFIFO_INIT(hci_rx_fifo, 72, 4);
	MYFIFO_INIT(hci_tx_fifo, 80, 8);
#endif


MYFIFO_INIT(blt_rxfifo, 64, 8);

////////////////////////////////////////////////////////////////////
u16	current_connHandle = BLE_INVALID_CONNECTION_HANDLE;	 //	handle of  connection

u32 host_update_conn_param_req = 0;
u16 host_update_conn_min;
u16 host_update_conn_latency;
u16 host_update_conn_timeout;


u8 current_conn_adr_type;
u8 current_conn_address[6];

u32 master_connecting_tick_flag;  //for smp trigger proc
int master_connected_led_on = 0;

int	dongle_pairing_enable = 0;
int dongle_unpair_enable = 0;


int master_auto_connect = 0;
int user_manual_paring;



const u8 	telink_adv_trigger_paring[] = {5, 0xFF, 0x11, 0x02, 0x01, 0x00};
const u8 	telink_adv_trigger_unpair[] = {5, 0xFF, 0x11, 0x02, 0x01, 0x01};



typedef void (*main_service_t) (void);

main_service_t		main_service = 0;



#define SMP_PENDING					1   //security management
#define SDP_PENDING					2   //service discovery

u8	app_host_smp_sdp_pending = 0; 		//security & service discovery


extern u8 read_by_type_req_uuidLen;
extern u8 read_by_type_req_uuid[16];



extern void	att_keyboard (u16 conn, u8 *p);


int main_idle_loop (void);


#if (BLE_HOST_SIMPLE_SDP_ENABLE)
	extern void host_att_service_disccovery_clear(void);
	int host_att_client_handler (u16 connHandle, u8 *p);
	ble_sts_t  host_att_discoveryService (u16 handle, att_db_uuid16_t *p16, int n16, att_db_uuid128_t *p128, int n128);


	#define				ATT_DB_UUID16_NUM		20
	#define				ATT_DB_UUID128_NUM		8

	u8 	conn_char_handler[8] = {0};


	u8	serviceDiscovery_adr_type;
	u8	serviceDiscovery_address[6];


	extern const u8 my_MicUUID[16];
	extern const u8 my_SpeakerUUID[16];
	extern const u8 my_OtaUUID[16];


	void app_service_discovery ()
	{

		att_db_uuid16_t 	db16[ATT_DB_UUID16_NUM];
		att_db_uuid128_t 	db128[ATT_DB_UUID128_NUM];
		memset (db16, 0, ATT_DB_UUID16_NUM * sizeof (att_db_uuid16_t));
		memset (db128, 0, ATT_DB_UUID128_NUM * sizeof (att_db_uuid128_t));


		if ( IS_CONNECTION_HANDLE_VALID(current_connHandle) && \
			 host_att_discoveryService (current_connHandle, db16, ATT_DB_UUID16_NUM, db128, ATT_DB_UUID128_NUM) == BLE_SUCCESS)	// service discovery OK
		{
			//int h = current_connHandle & 7;
			conn_char_handler[0] = blm_att_findHandleOfUuid128 (db128, my_MicUUID);			//MIC
			conn_char_handler[1] = blm_att_findHandleOfUuid128 (db128, my_SpeakerUUID);		//Speaker
			conn_char_handler[2] = blm_att_findHandleOfUuid128 (db128, my_OtaUUID);			//OTA


			conn_char_handler[3] = blm_att_findHandleOfUuid16 (db16, CHARACTERISTIC_UUID_HID_REPORT,
						HID_REPORT_ID_CONSUME_CONTROL_INPUT | (HID_REPORT_TYPE_INPUT<<8));		//consume report

			conn_char_handler[4] = blm_att_findHandleOfUuid16 (db16, CHARACTERISTIC_UUID_HID_REPORT,
						HID_REPORT_ID_KEYBOARD_INPUT | (HID_REPORT_TYPE_INPUT<<8));				//normal key report

			conn_char_handler[5] = blm_att_findHandleOfUuid16 (db16, CHARACTERISTIC_UUID_HID_REPORT,
						HID_REPORT_ID_MOUSE_INPUT | (HID_REPORT_TYPE_INPUT<<8));				//mouse report

			//module
			//conn_char_handler[6] = blm_att_findHandleOfUuid128 (db128, my_SppS2CUUID);			//notify
			//conn_char_handler[7] = blm_att_findHandleOfUuid128 (db128, my_SppC2SUUID);			//write_cmd



			//save current service discovery conn address
			serviceDiscovery_adr_type = current_conn_adr_type;
			memcpy(serviceDiscovery_address, current_conn_address, 6);

		}

		app_host_smp_sdp_pending = 0;  //service discovery finish

	}

	void app_register_service (void *p)
	{
		main_service = p;
	}


	#define			HID_HANDLE_CONSUME_REPORT			conn_char_handler[3]
	#define			HID_HANDLE_KEYBOARD_REPORT			conn_char_handler[4]
	#define			AUDIO_HANDLE_MIC					conn_char_handler[0]
#else  //no service discovery

	//need define att handle same with slave
	#define			HID_HANDLE_CONSUME_REPORT			25
	#define			HID_HANDLE_KEYBOARD_REPORT			29
	#define			AUDIO_HANDLE_MIC					47

#endif





#if (BLE_HOST_SMP_ENABLE)
int app_host_smp_finish (void)  //smp finish callback
{
	#if (BLE_HOST_SIMPLE_SDP_ENABLE)  //smp finish, start sdp
		if(app_host_smp_sdp_pending == SMP_PENDING)
		{
			//new slave device, should do service discovery again
			if (current_conn_adr_type != serviceDiscovery_adr_type || \
				memcmp(current_conn_address, serviceDiscovery_address, 6))
			{
				app_register_service(&app_service_discovery);
				app_host_smp_sdp_pending = SDP_PENDING; //service discovery busy
			}
			else
			{
				app_host_smp_sdp_pending = 0;  //no need sdp
			}
		}
	#else
		app_host_smp_sdp_pending = 0;  //no sdp
	#endif

	return 0;
}
#endif









int app_l2cap_handler (u16 conn_handle, u8 *raw_pkt)
{

	//l2cap data packeted, make sure that user see complete l2cap data
	rf_packet_l2cap_t *ptrL2cap = blm_l2cap_packet_pack (conn_handle, raw_pkt);
	if (!ptrL2cap)
	{
		return 0;
	}



	//l2cap data channel id, 4 for att, 5 for signal, 6 for smp
	if(ptrL2cap->chanId == L2CAP_CID_ATTR_PROTOCOL)  //att data
	{

		#if (BLE_HOST_SIMPLE_SDP_ENABLE)
			if(app_host_smp_sdp_pending == SDP_PENDING)  //ATT service discovery is ongoing
			{
				//when service discovery function is running, all the ATT data from slave
				//will be processed by it,  user can only send your own att cmd after  service discovery is over
				host_att_client_handler (conn_handle, (u8 *)ptrL2cap); //handle this ATT data by service discovery process
			}
		#endif


		rf_packet_att_t *pAtt = (rf_packet_att_t*)ptrL2cap;
		u16 attHandle = pAtt->handle0 | pAtt->handle1<<8;

		if(pAtt->opcode == ATT_OP_READ_BY_TYPE_RSP)  //slave ack ATT_OP_READ_BY_TYPE_REQ data
		{
			#if (KMA_DONGLE_OTA_ENABLE)
				//when use ATT_OP_READ_BY_TYPE_REQ to find ota atthandle, should get the result
				extern void host_find_slave_ota_attHandle(u8 *p);
				host_find_slave_ota_attHandle( (u8 *)pAtt );
			#endif
			//u16 slave_ota_handle;
		}
		else if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //slave handle notify
		{
			if(attHandle == HID_HANDLE_CONSUME_REPORT)
			{
				static u32 app_key;
				app_key++;
				att_keyboard_media (conn_handle, pAtt->dat);
			}
			else if(attHandle == HID_HANDLE_KEYBOARD_REPORT)
			{
				static u32 app_key;
				app_key++;
				att_keyboard (conn_handle, pAtt->dat);

			}
			else if(attHandle == AUDIO_HANDLE_MIC)
			{
				static u32 app_mic;
				app_mic	++;
				att_mic (conn_handle, pAtt->dat);
			}
			else
			{

			}
		}
		else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
		{

		}

	}
	else if(ptrL2cap->chanId == L2CAP_CID_SIG_CHANNEL)  //signal
	{
		if(ptrL2cap->opcode == L2CAP_CMD_CONN_UPD_PARA_REQ)  //slave send conn param update req on l2cap
		{
			rf_packet_l2cap_connParaUpReq_t  * req = (rf_packet_l2cap_connParaUpReq_t *)ptrL2cap;

			u32 interval_us = req->min_interval*1250;  //1.25ms unit
			u32 timeout_us = req->timeout*10000; //10ms unit
			u32 long_suspend_us = interval_us * (req->latency+1);

			//interval < 200ms
			//long suspend < 11S
			// interval * (latency +1)*2 <= timeout
			if( interval_us < 200000 && long_suspend_us < 20000000 && (long_suspend_us*2<=timeout_us) )
			{
				//when master host accept slave's conn param update req, should send a conn param update response on l2cap
				//with CONN_PARAM_UPDATE_ACCEPT; if not accpet,should send  CONN_PARAM_UPDATE_REJECT
				blc_l2cap_SendConnParamUpdateResponse(current_connHandle, CONN_PARAM_UPDATE_ACCEPT);  //send SIG Connection Param Update Response


				//if accept, master host should mark this, add will send  update conn param req on link layer later
				//set a flag here, then send update conn param req in mainloop
				host_update_conn_param_req = clock_time() | 1 ; //in case zero value
				host_update_conn_min = req->min_interval;  //backup update param
				host_update_conn_latency = req->latency;
				host_update_conn_timeout = req->timeout;
			}
			else
			{
				blc_l2cap_SendConnParamUpdateResponse(current_connHandle, CONN_PARAM_UPDATE_REJECT);  //send SIG Connection Param Update Response
			}
		}


	}
	else if(ptrL2cap->chanId == L2CAP_CID_SMP) //smp
	{
		#if (BLE_HOST_SMP_ENABLE)
			if(app_host_smp_sdp_pending == SMP_PENDING)
			{
				blm_host_smp_handler(conn_handle, (u8 *)ptrL2cap);
			}

		#endif
	}
	else
	{

	}


	return 0;
}





//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
int app_event_callback (u32 h, u8 *p, int n)
{


	static u32 event_cb_num;
	event_cb_num++;

	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		//------------ disconnect -------------------------------------
		if(evtCode == HCI_CMD_DISCONNECTION_COMPLETE)  //connection terminate
		{
			event_disconnection_t	*pd = (event_disconnection_t *)p;

			//terminate reason
			//connection timeout
			if(pd->reason == HCI_ERR_CONN_TIMEOUT){

			}
			//peer device(slave) send terminate cmd on link layer
			else if(pd->reason == HCI_ERR_REMOTE_USER_TERM_CONN){

			}
			//master host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
			else if(pd->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){

			}
			 //master create connection, send conn_req, but did not received acked packet in 6 connection event
			else if(pd->reason == HCI_ERR_CONN_FAILED_TO_ESTABLISH){
				//when controller is in initiating state, find the specified device, send connection request to slave,
				//but slave lost this rf packet, there will no ack packet from slave, after 6 connection events, master
				//controller send a disconnect event with reason HCI_ERR_CONN_FAILED_TO_ESTABLISH
				//if slave got the connection request packet and send ack within 6 connection events, controller
				//send connection establish event to host(telink defined event)


			}
			else{

			}

			#if (UI_LED_ENABLE)
				//led show none connection state
				if(master_connected_led_on){
					master_connected_led_on = 0;
					gpio_write(GPIO_LED_WHITE, LED_ON_LEVAL);   //white on
					gpio_write(GPIO_LED_RED, !LED_ON_LEVAL);    //red off
				}
			#endif


			current_connHandle = BLE_INVALID_CONNECTION_HANDLE;  //when disconnect, clear conn handle

			master_connecting_tick_flag = 0;

			//if previous connection smp&sdp not finished, clear this flag
			if(app_host_smp_sdp_pending){
				app_host_smp_sdp_pending = 0;
			}

			host_update_conn_param_req = 0; //when disconnect, clear update conn flag

			extern void host_att_data_clear(void);
			host_att_data_clear();

			//should set scan mode again to scan slave adv packet
			blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
									  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
			blc_ll_setScanEnable (BLC_SCAN_ENABLE, 0);


		}
#if (BLE_HOST_SMP_ENABLE)
		else if(evtCode == HCI_EVT_ENCRYPTION_CHANGE)
		{
			event_enc_change_t *pe = (event_enc_change_t *)p;
			blm_smp_encChangeEvt(pe->status, pe->handle, pe->enc_enable);
		}
		else if(evtCode == BLM_CONN_ENC_REFRESH)
		{
			event_enc_refresh_t *pe = (event_enc_refresh_t *)p;
			blm_smp_encChangeEvt(pe->status, pe->handle, 1);
		}
#endif
		else if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];

			//------------le connection complete event-------------------------------------
			if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)	// connection complete
			{
				//after controller is set to initiating state by host (blc_ll_createConnection(...) )
				//it will scan the specified device(adr_type & mac), when find this adv packet, send a connection request packet to slave
				//and enter to connection state, send connection complete evnet. but notice that connection complete not
				//equals to connection establish. connection complete measn that master controller set all the ble timing
				//get ready, but has not received any slave packet, if slave rf lost the connection request packet, it will
				//not send any packet to master controller



			}
			else if(subEvt_code == HCI_SUB_EVT_LE_CONNECTION_ESTABLISH)  //connection establish(telink private event)
			{
				//notice that: this connection event is defined by telink, not a standard ble controller event
				//after master controller send connection request packet to slave, when slave received this packet
				//and enter to connection state, send a ack packet within 6 connection event, master will send
				//connection establish event to host(HCI_SUB_EVT_LE_CONNECTION_ESTABLISH)

				event_connection_complete_t *pc = (event_connection_complete_t *)p;
				if (pc->status == BLE_SUCCESS)	// status OK
				{
					#if (UI_LED_ENABLE)
						//led show connection state
						master_connected_led_on = 1;
						gpio_write(GPIO_LED_RED, LED_ON_LEVAL);     //red on
						gpio_write(GPIO_LED_WHITE, !LED_ON_LEVAL);  //white off
					#endif


					current_connHandle = pc->handle;   //mark conn handle, in fact this equals to BLM_CONN_HANDLE

					master_connecting_tick_flag = clock_time() | 1;  //none zero value

					//save current connect address type and address
					current_conn_adr_type = pc->peer_adr_type;
					memcpy(current_conn_address, pc->mac, 6);


					#if (BLE_HOST_SMP_ENABLE)
						app_host_smp_sdp_pending = SMP_PENDING; //pair & security first
					#else
						// if this connection establish is a new device manual paring, should add this device to slave table
						if(user_manual_paring && !master_auto_connect){
							user_tbl_slave_mac_add(pc->peer_adr_type, pc->mac);
						}

						//new slave device, should do service discovery again
						if (current_conn_adr_type != serviceDiscovery_adr_type || \
							memcmp(current_conn_address, serviceDiscovery_address, 6))
						{
							app_register_service(&app_service_discovery);
							app_host_smp_sdp_pending = SDP_PENDING;  //service discovery busy
						}
						else{
							app_host_smp_sdp_pending = 0;  //no need sdp
						}
					#endif
				}
			}
			//------------ le ADV report event ------------------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event

				event_adv_report_t *pa = (event_adv_report_t *)p;
				s8 rssi = pa->data[pa->len];


				 //if previous connection smp&sdp not finish, can not create a new connection
				if(app_host_smp_sdp_pending){
					return 1;
				}


				//user design manual paring methods
				user_manual_paring = dongle_pairing_enable && (rssi > -56);  //button trigger pairing(rssi threshold, short distance)
				if(!user_manual_paring){  //special adv pair data can also trigger pairing
					user_manual_paring = (memcmp(pa->data, telink_adv_trigger_paring, sizeof(telink_adv_trigger_paring)) == 0) && (rssi > -56);
				}

				master_auto_connect = 0;
				#if (BLE_HOST_SMP_ENABLE)
					master_auto_connect = tbl_bond_slave_search(pa->adr_type, pa->mac);
				#else
					//search in slave mac table to find whether this device is an old device which has already paired with master
					master_auto_connect = user_tbl_slave_mac_search(pa->adr_type, pa->mac);
				#endif

				if(master_auto_connect || user_manual_paring)
				{
					//send create connection cmd to controller, trigger it switch to initiating state, after this cmd,
					//controller will scan all the adv packets it received but not report to host, to find the specified
					//device(adr_type & mac), then send a connection request packet after 150us, enter to connection state
					// and send a connection complete event(HCI_SUB_EVT_LE_CONNECTION_COMPLETE)
					u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, INITIATE_FP_ADV_SPECIFY,  \
											 pa->adr_type, pa->mac, BLE_ADDR_PUBLIC, \
											 CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, \
											 0, 0);

					if(status != BLE_SUCCESS)  //create connection fail
					{

					}
				}

			}
			//------------le connection update complete event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)	// connection update
			{
				//after master host send update conn param req cmd to controller( blm_ll_updateConnection(...) ),
				//when update take effect, controller send update complete event to host

				event_connection_update_t *pc = (event_connection_update_t *)p;

				#if (KMA_DONGLE_OTA_ENABLE)
					extern void host_ota_update_conn_complete(u16, u16, u16);
					host_ota_update_conn_complete( pc->interval, pc->latency, pc->timeout );
				#endif


			}
		}
	}


}




#if (HCI_ACCESS == HCI_USE_UART)
	uart_data_t T_txdata_buf;

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
			blm_hci_handler(&p[4], rx_len - 4);
			my_fifo_pop(&hci_rx_fifo);
		}


		return 0;
	}

	int blc_hci_tx_to_uart ()
	{
		static u32 uart_tx_tick = 0;

		u8 *p = my_fifo_get (&hci_tx_fifo);



	#if 1 //(ADD_DELAY_FOR_UART_DATA)
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
#endif

///////////////////////////////////////////
void user_init()
{
	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value and tp value

	//set USB ID
	usb_log_init ();
	REG_ADDR8(0x74) = 0x53;
	REG_ADDR16(0x7e) = 0x08d0;
	REG_ADDR8(0x74) = 0x00;

	//////////////// config USB ISO IN/OUT interrupt /////////////////
	reg_usb_mask = BIT(7);			//audio in interrupt enable
	reg_irq_mask |= FLD_IRQ_IRQ4_EN;
	reg_usb_ep6_buf_addr = 0x80;
	reg_usb_ep7_buf_addr = 0x60;
	reg_usb_ep_max_size = (256 >> 3);

	usb_dp_pullup_en (1);  //open USB enum

	///////////////// SDM /////////////////////////////////
#if (AUDIO_SDM_ENBALE)
	u16 sdm_step = config_sdm  (buffer_sdm, TL_SDM_BUFFER_SIZE, 16000, 4);
#endif

///////////// BLE stack Initialization ////////////////
	u8  tbl_mac [6];
	if (*(u32 *) CFG_ADR_MAC == 0xffffffff){
		u16 * ps = (u16 *) tbl_mac;
		ps[0] = REG_ADDR16(0x448);  //random
		ps[1] = REG_ADDR16(0x448);
		ps[2] = REG_ADDR16(0x448);
		flash_write_page (CFG_ADR_MAC, 6, tbl_mac);  //store master address
	}
	else{
		memcpy (tbl_mac, (u8 *) CFG_ADR_MAC, 6);  //copy from flash
	}


	////// Controller Initialization  //////////
	blc_ll_initBasicMCU(tbl_mac);   //mandatory

	blc_ll_initScanning_module(tbl_mac); 	//scan module: 		 mandatory for BLE master,
	blc_ll_initInitiating_module();			//initiate module: 	 mandatory for BLE master,
	blc_ll_initMasterRoleSingleConn_module();			//master module: 	 mandatory for BLE master,


	//// controller hci event mask config ////
	//bluetooth event
	blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE | HCI_EVT_MASK_ENCRYPTION_CHANGE);
	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd(		HCI_LE_EVT_MASK_CONNECTION_COMPLETE  \
									|	HCI_LE_EVT_MASK_ADVERTISING_REPORT \
									|   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE \
									|   HCI_LE_EVT_MASK_CONNECTION_ESTABLISH);  //connection establish: telink private event



	////// Host Initialization  //////////
#if (HCI_ACCESS == HCI_NONE)
	blc_l2cap_register_handler (app_l2cap_handler);  //controller data to host(l2cap data) all processed in this func
	blc_hci_registerControllerEventHandler(app_event_callback); //controller hci event to host all processed in this func


	#if (BLE_HOST_SMP_ENABLE)
		blm_host_smp_init(FLASH_ADR_PARING);
		blm_smp_registerSmpFinishCb(app_host_smp_finish);

		//default smp trigger by slave
		//blm_host_smp_setSecurityTrigger(MASTER_TRIGGER_SMP_FIRST_PAIRING | MASTER_TRIGGER_SMP_AUTO_CONNECT);
	#else  //telink referenced paring&bonding
		user_master_host_pairing_management_init();
	#endif

	host_att_register_idle_func (main_idle_loop);

#else

	blc_l2cap_register_handler (blc_hci_sendACLData2Host);
	blc_hci_registerControllerEventHandler(blc_hci_send_data);

	#if(HCI_ACCESS == HCI_USE_UART)
		gpio_set_input_en(GPIO_PB2, 1);
		gpio_set_input_en(GPIO_PB3, 1);
		gpio_setup_up_down_resistor(GPIO_PB2, PM_PIN_PULLUP_1M);
		gpio_setup_up_down_resistor(GPIO_PB3, PM_PIN_PULLUP_1M);
		uart_io_init(UART_GPIO_8267_PB2_PB3);
		CLK32M_UART115200;

		reg_dma_rx_rdy0 = FLD_DMA_UART_RX | FLD_DMA_UART_TX; //clear uart rx/tx status
		uart_BuffInit(hci_rx_fifo_b, hci_rx_fifo.size, hci_tx_fifo_b);
		blc_register_hci_handler (blc_rx_from_uart, blc_hci_tx_to_uart);
	#endif
#endif



///////////// USER Initialization ////////////////
	rf_set_power_level_index (RF_POWER_8dBm);
	ll_whiteList_reset();  //clear whitelist



	//set scan paramter and scan enable
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
							  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, 0);

}




void host_unpair_proc(void)
{
	//terminate and unpair proc
	static int master_disconnect_flag;
	if(dongle_unpair_enable){
		if(!master_disconnect_flag && blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){
			if( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS){
				master_disconnect_flag = 1;
				dongle_unpair_enable = 0;

				#if (BLE_HOST_SMP_ENABLE)
					tbl_bond_slave_unpair_proc(current_conn_adr_type, current_conn_address); //by telink stack host smp
				#else
					user_tbl_salve_mac_unpair_proc();
				#endif
			}
		}
	}
	if(master_disconnect_flag && blc_ll_getCurrentState() != BLS_LINK_STATE_CONN){
		master_disconnect_flag = 0;
	}
}



extern void usb_handle_irq(void);
extern void proc_button (void);
extern void proc_audio (void);
/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
int main_idle_loop (void)
{
	static u32 tick_loop;
	tick_loop ++;


	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();


	///////////////////////////////////// proc usb cmd from host /////////////////////
	usb_handle_irq();



	/////////////////////////////////////// HCI ///////////////////////////////////////
	blc_hci_proc ();


	static u32 tick_bo;
	if (REG_ADDR8(0x125) & BIT(0))
	{
		tick_bo = clock_time ();
	}
	else if (clock_time_exceed (tick_bo, 200000))
	{
		REG_ADDR8(0x125) = BIT(0);

	}




	////////////////////////////////////// UI entry /////////////////////////////////
#if (UI_BUTTON_ENABLE)
	static u8 button_detect_en = 0;
	if(!button_detect_en && clock_time_exceed(0, 2000000)){// proc button 2 second later after power on
		button_detect_en = 1;
	}
	if(button_detect_en){
		proc_button();  //button triggers pair & unpair  and OTA
	}
#endif


	////////////////////////////////////// proc audio ////////////////////////////////
#if (UI_AUDIO_ENABLE)
	proc_audio();
#endif


	host_unpair_proc();


#if(KMA_DONGLE_OTA_ENABLE)
	extern void proc_ota (void);
	proc_ota();
#endif


	//proc master update
	//at least 50ms later and make sure smp/sdp is finished
	if( host_update_conn_param_req && clock_time_exceed(host_update_conn_param_req, 50000) && !app_host_smp_sdp_pending)
	{
		host_update_conn_param_req = 0;

		if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){  //still in connection state
			blm_ll_updateConnection (current_connHandle,
					host_update_conn_min, host_update_conn_min, host_update_conn_latency,  host_update_conn_timeout,
											  0, 0 );
		}
	}


#if (KMA_DONGLE_OTA_ENABLE)
	extern u8 host_ota_update_pending;
	if(host_ota_update_pending == 1 && !host_update_conn_param_req && !app_host_smp_sdp_pending)
	{

		if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){  //still in connection state
			//10ms interval,  latency = 0   timeout = 2S
			blm_ll_updateConnection (current_connHandle,
										8, 8, 0,  200,
											  0, 0 );

			host_ota_update_pending = 2;

		}
	}
#endif



#if (BLE_HOST_SMP_ENABLE)
	if( blm_host_smp_getSecurityTrigger() )
	{
		if(	master_connecting_tick_flag && clock_time_exceed(master_connecting_tick_flag, 30000) )
		{
			master_connecting_tick_flag = 0;
			blm_host_smp_procSecurityTrigger(BLM_CONN_HANDLE);
		}
	}
#endif



	return 0;
}




void main_loop (void)
{
#if (HCI_ACCESS==HCI_USE_UART || HCI_ACCESS==HCI_USE_USB)
	blt_sdk_main_loop();
#else
	main_idle_loop ();

	if (main_service)
	{
		main_service ();
		main_service = 0;
	}
#endif
}






#endif  //end of __PROJECT_826X_MASTER_KMA_DONGLE__
