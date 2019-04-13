/*
 * rc_ir.h
 *
 *  Created on: 2015-12-2
 *      Author: Administrator
 */

#ifndef RC_IR_H_
#define RC_IR_H_

#include "../../proj/tl_common.h"
#include "../../proj/mcu/register.h"


enum{
    IR_TYPE_KONKA_KONKA=0,
    IR_TYPE_SUNSUNG_TC9012,
    IR_TYPE_HAIER_TC9012,
    IR_TYPE_CHANGHONG_uPD6121,
    IR_TYPE_HiSense_uPD6121,
    IR_TYPE_TCL_RCA,
    IR_TYPE_SKYWORTH_TC9012,
    IR_TYPE_NEC_TIANZUN,
    IR_TYPE_MAX ,
};


#define IR_MODE_SELECT			    IR_TYPE_NEC_TIANZUN


#define PWM0_IR_MODE					0
#define PWM1_IR_MODE					1



#define IR_PWM_SELECT					PWM0_IR_MODE



#if (IR_PWM_SELECT == PWM0_IR_MODE)
	#define IR_PWM_ID						PWM0_ID
#elif (IR_PWM_SELECT == PWM1_IR_MODE)
	#define IR_PWM_ID						PWM1_ID
#endif


#if (IR_MODE_SWITCH == IR_TYPE_NEC_TIANZUN)
	#define IR_CARRIER_FREQ				38000  	// 1 frame -> 1/38k -> 1000/38 = 26 us
	#define PWM_CYCLE_VALUE				( CLOCK_SYS_CLOCK_HZ/IR_CARRIER_FREQ )  //16M: 421 tick, f = 16000000/421 = 38004,T = 421/16=26.3125 us
	#define PWM_HIGH_VALUE				( PWM_CYCLE_VALUE/3 )   // 1/3 duty
#else


#endif



#define IR_HIGH_CARR_TIME			565			// in us
#define IR_HIGH_NO_CARR_TIME		1685
#define IR_LOW_CARR_TIME			560
#define IR_LOW_NO_CARR_TIME			565
#define IR_INTRO_CARR_TIME			9000
#define IR_INTRO_NO_CARR_TIME		4500

#define IR_SWITCH_CODE              0x0d
#define IR_ADDR_CODE                0x00
#define IR_CMD_CODE                 0xbf

#define IR_REPEAT_INTERVAL_TIME     40500
#define IR_REPEAT_NO_CARR_TIME      2250
#define IR_END_TRANS_TIME			563

//#define IR_CARRIER_FREQ				37917//38222
#define IR_CARRIER_DUTY				3
#define IR_LEARN_SERIES_CNT     	160





enum{
	IR_SEND_TYPE_TIME_SERIES,
	IR_SEND_TYPE_BYTE,
	IR_SEND_TYPE_HALF_TIME_SERIES,
};



typedef struct{
	u32 cycle;
	u16 hich;
	u16 cnt;
}ir_ctrl_t;


typedef struct{
	ir_ctrl_t *ptr_irCtl;
	u8 type;
	u8 start_high;
	u8 ir_number;
	u8 code;
}ir_send_ctrl_data_t;


#define IR_GROUP_MAX		8

typedef struct{
	ir_send_ctrl_data_t	data[IR_GROUP_MAX];
	u8 group_index;
	u8 group_cnt;
	u8 is_sending;
	u8 repeat_timer_enable;

	u8 ir_send_irq_idx;
	u8 ir_send_start_high;
	u8 last_cmd;
	u8 rsvd;

	u16 carrier_cycle;
	u16 carrier_high;
	u32 sending_start_time;
	u32 repeat_time;
}ir_send_ctrl_t;

ir_send_ctrl_t ir_send_ctrl;



typedef struct{
	u8 ir_protocol;
	u8 toshiba_c0flag;
	u8 resv0[2];
	u32 carr_high_tm;
	u32 carr_low_tm;
	u16 series_cnt;
	u16 resv1;
	u8 series_tm[(IR_LEARN_SERIES_CNT/2)*3];
}ir_universal_pattern_t;

typedef struct{
	u8 is_carr;
	u8 ir_protocol;
	u8 toshiba_c0flag;
	u8 learn_timer_started;
	int carr_first;
	u32 carr_switch_start_tm;
	u16 carr_check_cnt;
	u16 series_cnt;
	u32 series_tm[IR_LEARN_SERIES_CNT];
	u32 time_interval;
	u32 last_trigger_tm;
	u32 curr_trigger_tm;
	u32 carr_high_tm;
	u32	carr_low_tm;
	int ir_int_cnt;

}ir_learn_ctrl_t;






void ir_config_carrier(u16 cycle_tick, u16 high_tick);
void ir_config_byte_timing(u32 logic_1_carr, u32 logic_1_none, u32 logic_0_carr, u32 logic_0_none);

void ir_send_add_series_item(u32 *time_series, u8 series_cnt, ir_ctrl_t *pIrCtrl, u8 start_high);
void ir_send_add_byte_item(u8 code, u8 start_high);


void rc_ir_init(void);
void ir_send_release(void);

void ir_irq_send(void);
void ir_repeat_handle(void);







void ir_nec_send(u8 addr1, u8 addr2, u8 cmd);
void ir_nec_send_repeat(void);

void ir_tc9012_send_repeat(u8 addr1);
void ir_rca_send_repeat(void);
void ir_upd6121_send_repeat(void);
void ir_konka_send_repeat(void);
void ir_tc9012_send(u8 addr1, u8 addr2,u8 local_data_code);
void ir_upd6121_send(u8 addr1, u8 addr2,u8 local_data_code);
void ir_konka_send(u8 addr,u8 local_data_code);
void ir_rca_send(u8 addr1, u8 addr2,u8 local_data_code);









#endif /* RC_IR_H_ */
