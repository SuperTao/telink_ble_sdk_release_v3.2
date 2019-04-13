/*
 * app_button.c
 *
 *  Created on: 2017-5-5
 *      Author: Administrator
 */

#include "../../proj/tl_common.h"

#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/hci/hci.h"
#include "../../proj_lib/ble/ble_smp.h"
#include "../../proj/drivers/keyboard.h"
#include "../common/tl_audio.h"
#include "../common/rf_frame.h"
#include "../../proj_lib/ble/trace.h"
#include "../../proj/mcu/pwm.h"
#include "../../proj_lib/ble/service/ble_ll_ota.h"
#include "../../proj/drivers/audio.h"
#include "../../proj/drivers/adc.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj/drivers/uart.h"


#if (UI_BUTTON_ENABLE)

extern int 	dongle_pairing_enable;
extern int  dongle_unpair_enable;
extern int 	master_ota_test_mode;

/////////////////////////////////////////////////////////////////////
	#define MAX_BTN_SIZE			2
	#define BTN_VALID_LEVEL			0
	#define BTN_PAIR				0x01
	#define BTN_UNPAIR				0x02

	u32 ctrl_btn[] = {SW1_GPIO, SW2_GPIO};
	u8 btn_map[MAX_BTN_SIZE] = {BTN_PAIR, BTN_UNPAIR};

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

	//		if(btn_status.btn_filter_last == 0){
	//			btn_status.btn_not_release = 1;
	//		}
	//		else if(btn_status.btn_filter_last != 0 && btn_status.btn_history[0] == 0){
	//			btn_status.btn_not_release = 0;
	//		}

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


		static u32 button_history = 0;
		static u32 last_singleKey_press_tick;

		static int button0_press_flag;
		static u32 button0_press_tick;
		static int button1_press_flag;
		static u32 button1_press_tick;

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
				#if (KMA_DONGLE_OTA_ENABLE)
					if(!master_ota_test_mode && !clock_time_exceed(last_singleKey_press_tick, 2000000))
					{
						button_history = button_history<<1 | (key0 == BTN_PAIR);
						if( (button_history & 0x0f) == 0x0f)
						{
							master_ota_test_mode = 1;
							extern u32 ota_mode_begin_tick;
							ota_mode_begin_tick = clock_time();
						}
					}
					else{
						button_history = 0;
					}

					last_singleKey_press_tick = clock_time();
				#endif


				if(key0 == BTN_PAIR)
				{
					if(!master_ota_test_mode){
						dongle_pairing_enable = 1;
					}
					button0_press_flag = 1;
					button0_press_tick = clock_time();

				}
				else if(key0 == BTN_UNPAIR)
				{
					if(!master_ota_test_mode){
						dongle_unpair_enable = 1;
					}
					button1_press_flag = 1;
					button1_press_tick = clock_time();
				}
			}
			else{  //release
				if(dongle_pairing_enable){
					dongle_pairing_enable = 0;
				}

				if(dongle_unpair_enable){
					dongle_unpair_enable = 0;
				}


				#if (KMA_DONGLE_OTA_ENABLE)  //ota cmd trigger
					extern void host_button_trigger_ota_start(int , int );
					if(master_ota_test_mode == 2){
						host_button_trigger_ota_start(button0_press_flag, button1_press_flag);
					}
				#endif

				button0_press_flag = 0;
				button1_press_flag = 0;
			}

		}


	}
#endif   //end of UI_BUTTON_ENABLE
