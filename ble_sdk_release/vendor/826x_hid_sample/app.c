#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj/drivers/keyboard.h"
#include "../common/tl_audio.h"
#include "../common/blt_led.h"
#include "../../proj_lib/ble/trace.h"
#include "../../proj/mcu/pwm.h"
#include "../../proj_lib/ble/service/ble_ll_ota.h"
#include "../../proj/drivers/audio.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/battery.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/ble_smp.h"
#include "../../proj_lib/ble/ble_phy.h"
#include "../../proj/drivers/uart.h"

#if (__PROJECT_8261_HID_SAMPLE__ || __PROJECT_8266_HID_SAMPLE__ || __PROJECT_8267_HID_SAMPLE__ || __PROJECT_8269_HID_SAMPLE__)



#define     MY_APP_ADV_CHANNEL					BLT_ENABLE_ADV_ALL

#define 	MY_ADV_INTERVAL_MIN					ADV_INTERVAL_30MS
#define 	MY_ADV_INTERVAL_MAX					ADV_INTERVAL_35MS



MYFIFO_INIT(blt_rxfifo, 64, 8);
MYFIFO_INIT(blt_txfifo, 40, 16);


u32			tick_led_config;
//////////////////////////////////////////////////////////////////////////////
//	 Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////
const u8	tbl_advData[] = {
	 0x05, 0x09, 't', 'H', 'I', 'D',
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8	tbl_scanRsp [] = {
		 0x05, 0x09, 't', 'H', 'I', 'D',
	};


u32 interval_update_tick = 0;
int device_in_connection_state;


u32		advertise_begin_tick;



//////////////////// key type ///////////////////////
#define IDLE_KEY	   			0
#define CONSUMER_KEY   	   		1
#define KEYBOARD_KEY   	   		2
#define IR_KEY   	   			3

u8 		key_type;
u8 		user_key_mode;

u8 		key_buf[8] = {0};

int 	key_not_released;

int 	ir_not_released;

u32 	latest_user_event_tick;

u8 		user_task_flg;
u8 		sendTerminate_before_enterDeep = 0;
u8 		ota_is_working = 0;



void 	ble_remote_terminate(u8 e,u8 *p, int n) //*p is terminate reason
{

	if(*p == HCI_ERR_CONN_TIMEOUT){

	}
	else if(*p == HCI_ERR_REMOTE_USER_TERM_CONN){  //0x13

	}
	else if(*p == HCI_ERR_CONN_TERM_MIC_FAILURE){

	}
	else{

	}


	gpio_write(GPIO_LED_RED, LED_OFF_LEVAL); //red led off

	tick_led_config = clock_time() | 1;  //white led start to blink
}

void	task_connect (u8 e, u8 *p, int n)
{

	tick_led_config = 0;
	gpio_write(GPIO_LED_WHITE, LED_OFF_LEVAL);  //white led off

	gpio_write(GPIO_LED_RED, LED_ON_LEVAL);  //red led on


	bls_l2cap_requestConnParamUpdate (6, 15, 0, 400);

}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
#define MAX_BTN_SIZE			2
#define BTN_VALID_LEVEL			0

#define USER_BTN_1				0x01
#define USER_BTN_2				0x02

u32 ctrl_btn[] = {SW1_GPIO, SW2_GPIO};
u8 btn_map[MAX_BTN_SIZE] = {USER_BTN_1, USER_BTN_2};

typedef	struct{
	u8 	cnt;				//count button num
	u8 	btn_press;
	u8 	keycode[MAX_BTN_SIZE];			//6 btn
}vc_data_t;
vc_data_t vc_event;

typedef struct{
	u8  btn_history[4];		//vc history btn save
	u8  btn_filter_last;
	u8	btn_not_release;
	u8 	btn_new;					//new btn  flag
}btn_status_t;
btn_status_t 	btn_status;


u8 btn_debounce_filter(u8 *btn_v)
{
	u8 change = 0;

	for(int i=3; i>0; i--){
		btn_status.btn_history[i] = btn_status.btn_history[i-1];
	}
	btn_status.btn_history[0] = *btn_v;

	if(  btn_status.btn_history[0] == btn_status.btn_history[1] && btn_status.btn_history[1] == btn_status.btn_history[2] && \
		btn_status.btn_history[0] != btn_status.btn_filter_last ){
		change = 1;

		btn_status.btn_filter_last = btn_status.btn_history[0];
	}

	return change;
}

u8 vc_detect_button(int read_key)
{
	u8 btn_changed, i;
	memset(&vc_event,0,sizeof(vc_data_t));			//clear vc_event
	//vc_event.btn_press = 0;

	for(i=0; i<MAX_BTN_SIZE; i++){
		if(BTN_VALID_LEVEL != !gpio_read(ctrl_btn[i])){
			vc_event.btn_press |= BIT(i);
		}
	}

	btn_changed = btn_debounce_filter(&vc_event.btn_press);


	if(btn_changed && read_key){
		for(i=0; i<MAX_BTN_SIZE; i++){
			if(vc_event.btn_press & BIT(i)){
				vc_event.keycode[vc_event.cnt++] = btn_map[i];
			}
		}

		return 1;
	}

	return 0;
}

void proc_button (void)
{
	static u32 button_det_tick;
	if(clock_time_exceed(button_det_tick, 5000))
	{
		button_det_tick = clock_time();
	}
	else{
		return;
	}


//	static u32 button_history = 0;
//	static u32 last_singleKey_press_tick;

	static int button1_press_flag;
	static u32 button1_press_tick;
	static int button2_press_flag;
	static u32 button2_press_tick;

	static int consumer_report = 0;

	int det_key = vc_detect_button (1);

	if (det_key)  //key change: press or release
	{
		u8 key0 = vc_event.keycode[0];
		u8 key1 = vc_event.keycode[1];

		if(vc_event.cnt == 2)  //two key press
		{

		}
		else if(vc_event.cnt == 1) //one key press
		{
			if(key0 == USER_BTN_1)
			{
				button1_press_flag = 1;
				button1_press_tick = clock_time();

				u16 consumer_key = MKEY_VOL_UP;
				bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
				consumer_report = 1;
			}
			else if(key0 == USER_BTN_2)
			{
				button2_press_flag = 1;
				button2_press_tick = clock_time();

				u16 consumer_key = MKEY_VOL_DN;
				bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
				consumer_report = 1;
			}
		}
		else{  //release

			button1_press_flag = 0;
			button2_press_flag = 0;

			if(consumer_report){
				consumer_report = 0;
				u16 consumer_key = 0;
				bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
			}
		}

	}


}






void user_init()
{
	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value and tp value


////////////////// BLE stack initialization ////////////////////////////////////
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
	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,
	blc_ll_initPowerManagement_module();        //pm module:      	 optional



	////// Host Initialization  //////////
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization


 	//// smp initialization ////
#if (BLE_REMOTE_SECURITY_ENABLE)
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER );
#else
	bls_smp_enableParing (SMP_PARING_DISABLE_TRRIGER );
#endif


///////////////////// USER application initialization ///////////////////
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));



	u8 status = bls_ll_setAdvParam(  MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
									 0,  NULL,
									 MY_APP_ADV_CHANNEL,
									 ADV_FP_NONE);
	if(status != BLE_SUCCESS) { write_reg8(0x8000, 0x11); 	while(1); }  //debug: adv setting err


	bls_ll_setAdvEnable(1);  //adv enable
	rf_set_power_level_index (RF_POWER_8dBm);

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &ble_remote_terminate);


	///////////////////// Power Management initialization///////////////////
#if(BLE_PM_ENABLE)
	blc_ll_initPowerManagement_module();
	bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif


	tick_led_config = clock_time () | 1;

}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32 tick_loop;
//unsigned short battValue[20];


void main_loop (void)
{
	tick_loop ++;


	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();



	////////////////////////////////////// UI entry /////////////////////////////////
	proc_button();


	if (tick_led_config && clock_time_exceed (tick_led_config, 300000))
	{
		tick_led_config = clock_time () | 1;
		gpio_toggle (GPIO_LED_WHITE);
	}


}


#endif  //end of __PROJECT_826x_HID_SAMPLE
