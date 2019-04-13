/*
 * rc_ir.c
 *
 *  Created on: 2015-12-2
 *      Author: Administrator
 */

#include "../../proj/tl_common.h"
#include "../../proj/mcu/pwm.h"

#include "rc_ir.h"
#include "app_config.h"


#if (REMOTE_IR_ENABLE)




/////////////  NEC  protocol  /////////////////////////////////////////////
#define IR_INTRO_CARR_TIME_NEC          9000
#define IR_INTRO_NO_CARR_TIME_NEC       4500
#define IR_END_TRANS_TIME_NEC           563  // user define
#define IR_REPEAT_CARR_TIME_NEC          9000
#define IR_REPEAT_NO_CARR_TIME_NEC          2250
#define IR_REPEAT_LOW_CARR_TIME_NEC			560

#define IR_HIGH_CARR_TIME_NEC	        560
#define IR_HIGH_NO_CARR_TIME_NEC		1690
#define IR_LOW_CARR_TIME_NEC			560
#define IR_LOW_NO_CARR_TIME_NEC         560





#define NEC_FRAME_CYCLE				(108*1000)
#define GD25Q40B_FLASH_PAGE_SIZE 		0x100 //256bytes
#define IR_LEARN_MAX_INTERVAL			(WATCHDOG_INIT_TIMEOUT * CLOCK_SYS_CLOCK_1MS)
#define IR_STORED_INDEX_ADDRESS			(0xa000)
#define IR_STORED_SERIES_ADDRESS 		(0xa200)
#define IR_LEARN_NONE_CARR_MIN			(200*CLOCK_SYS_CLOCK_1US)//old is 80
#define IR_LEARN_CARR_GLITCH_MIN		(3*CLOCK_SYS_CLOCK_1US)
#define IR_LEARN_CARR_MIN				(7 * CLOCK_SYS_CLOCK_1US)
#define IR_CARR_CHECK_CNT				10
#define IR_LEARN_START_MINLEN			(300*CLOCK_SYS_CLOCK_1US)
#define IR_MAX_INDEX_TABLE_LEN			32
#define NEC_LEAD_CARR_MIN_INTERVAL			(8700*CLOCK_SYS_CLOCK_1US)
#define NEC_LEAD_CARR_MAX_INTERVAL			(9300*CLOCK_SYS_CLOCK_1US)
#define NEC_LEAD_NOCARR_MIN_INTERVAL		(4200*CLOCK_SYS_CLOCK_1US)
#define NEC_LEAD_NOCARR_MAX_INTERVAL		(4800*CLOCK_SYS_CLOCK_1US)
#define TOSHIBA_LEAD_MIN_INTERVAL			(4200*CLOCK_SYS_CLOCK_1US)
#define TOSHIBA_LEAD_MAX_INTERVAL			(4800*CLOCK_SYS_CLOCK_1US)
#define FRAXEL_LEAD_CARR_MIN_INTERVAL		(2100*CLOCK_SYS_CLOCK_1US)
#define FRAXEL_LEAD_CARR_MAX_INTERVAL		(2700*CLOCK_SYS_CLOCK_1US)
#define FRAXEL_LEAD_NOCARR_MIN_INTERVAL		(900*CLOCK_SYS_CLOCK_1US)
#define FRAXEL_LEAD_NOCARR_MAX_INTERVAL		(1500*CLOCK_SYS_CLOCK_1US)
#define IR_NEC_TYPE							1
#define IR_TOSHIBA_TYPE 					2
#define IR_FRAXEL_TYPE						3
#define IR_HIGH_LOW_MIN_INTERVAL			(1000*CLOCK_SYS_CLOCK_1US)
#define IR_HIGH_LOW_MAX_INTERVAL			(2000*CLOCK_SYS_CLOCK_1US)
#define TC9012_FRAME_CYCLE					(108*1000)
#define FRAXEL_LEVEL_NUM 					19
#define NEC_TOSHIBA_LEVEL_NUM 				67



int ir_sending_check(void);
void ir_send_ctrl_clear(void);
void ir_send_ctrl_start(int need_repeat);






ir_ctrl_t  ir_bit_1_controll[2];
ir_ctrl_t  ir_bit_0_controll[2];



const u32 ir_lead_times[] 	  = {	IR_INTRO_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US,
									IR_INTRO_NO_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US};

const u32 ir_stop_bit_times[] = {	IR_END_TRANS_TIME_NEC * CLOCK_SYS_CLOCK_1US,
									500 * CLOCK_SYS_CLOCK_1US};  //all low, no high value

const u32 ir_repeat_times[]   = {	IR_REPEAT_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US,
									IR_REPEAT_NO_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US,
									IR_REPEAT_LOW_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US,
									500 * CLOCK_SYS_CLOCK_1US}; //all low, no high value


ir_ctrl_t	nec_start[ARRAY_SIZE(ir_lead_times)];
ir_ctrl_t	nec_stop[ARRAY_SIZE(ir_stop_bit_times)];
ir_ctrl_t	nec_repeat[ARRAY_SIZE(ir_repeat_times)];









void ir_nec_send(u8 addr1, u8 addr2, u8 cmd)
{

	if(ir_send_ctrl.last_cmd != cmd)
	{
		if(ir_sending_check())
		{
			return;
		}


		ir_send_ctrl.last_cmd = cmd;


		ir_send_ctrl_clear();

		ir_send_add_series_item(ir_lead_times, ARRAY_SIZE(ir_lead_times), &nec_start, 1);

		ir_send_add_byte_item(addr1, 1);
		ir_send_add_byte_item(addr2, 1);
		ir_send_add_byte_item(cmd, 1);
		ir_send_add_byte_item(~cmd, 1);

		ir_send_add_series_item(ir_stop_bit_times, ARRAY_SIZE(ir_stop_bit_times), &nec_stop, 1);

		ir_send_ctrl_start(1);
	}
}



void ir_nec_send_repeat(void)
{
	if(ir_sending_check()){
		return;
	}


	ir_send_ctrl_clear();
	ir_send_add_series_item((u32 *)ir_repeat_times, ARRAY_SIZE(ir_repeat_times), &nec_repeat, 1);
	ir_send_ctrl_start(1);
}




void ir_send_ctrl_clear(void)
{
	ir_send_ctrl.is_sending = ir_send_ctrl.group_cnt = ir_send_ctrl.group_index = 0;
}


void ir_config_carrier(u16 cycle_tick, u16 high_tick)
{
	ir_send_ctrl.carrier_cycle = cycle_tick;
	ir_send_ctrl.carrier_high = high_tick;
}



void ir_config_byte_timing(	u32 tick_logic_1_carr, u32 tick_logic_1_none,
							u32 tick_logic_0_carr, u32 tick_logic_0_none)
{

	ir_bit_1_controll[0].cycle = ir_send_ctrl.carrier_cycle;
	ir_bit_1_controll[0].hich = ir_send_ctrl.carrier_high;
	ir_bit_1_controll[0].cnt = ( tick_logic_1_carr + (ir_send_ctrl.carrier_cycle>>1) )/ir_send_ctrl.carrier_cycle;

	ir_bit_1_controll[1].cycle = tick_logic_1_none;
	ir_bit_1_controll[1].hich = 0;
	ir_bit_1_controll[1].cnt = 1;


	ir_bit_0_controll[0].cycle = ir_send_ctrl.carrier_cycle;
	ir_bit_0_controll[0].hich = ir_send_ctrl.carrier_high;
	ir_bit_0_controll[0].cnt = ( tick_logic_0_carr + (ir_send_ctrl.carrier_cycle>>1) )/ir_send_ctrl.carrier_cycle;

	ir_bit_0_controll[1].cycle = tick_logic_0_none;
	ir_bit_0_controll[1].hich = 0;
	ir_bit_0_controll[1].cnt = 1;
}


static inline ir_ctrl_t* ir_get_byte_control(u8 code, u8 index, u8 start_high)
{


	u8 b = index / 2;
	u8 r = index & 1;
	if(code & BIT(b)){
		if(!r){
			return &ir_bit_1_controll[!start_high];  //start_high=1, ir_bit_1_controll[0]
		}else{
			return &ir_bit_1_controll[!start_high];

		}
	}else{
		if(!r){
			return &ir_bit_0_controll[!start_high];
		}else{
			return &ir_bit_0_controll[!start_high];
		}
	}
}



void ir_send_add_series_item(u32 *time_series, u8 series_cnt, ir_ctrl_t *ir_control, u8 start_high)
{
	u8 i = ir_send_ctrl.group_cnt;
	ir_send_ctrl.data[i].type = IR_SEND_TYPE_TIME_SERIES;
	ir_send_ctrl.data[i].ir_number = series_cnt;
	ir_send_ctrl.data[i].start_high = start_high;
	ir_send_ctrl.data[i].ptr_irCtl = ir_control;

	ir_ctrl_t *pIrCtrl = ir_control;


	u8 high_flag = start_high;
	u32 cur_ir_tick;
	for(int j=0; j<series_cnt; j++ )
	{
		cur_ir_tick = time_series[j];

		if(high_flag){  //high
			pIrCtrl->cycle = ir_send_ctrl.carrier_cycle;
			pIrCtrl->hich = ir_send_ctrl.carrier_high;
			//pIrCtrl->cnt = cur_ir_tick/ir_send_ctrl.carrier_cycle ;
			pIrCtrl->cnt = ( cur_ir_tick + (ir_send_ctrl.carrier_cycle>>1) )/ir_send_ctrl.carrier_cycle ; //rounded to the nearest integer
		}
		else{  //low
			pIrCtrl->cycle = cur_ir_tick>>4;
			pIrCtrl->hich = 0;
			pIrCtrl->cnt = 16;
		}

		pIrCtrl ++;
		high_flag = !high_flag;
	}


	++ir_send_ctrl.group_cnt;
}


void ir_send_add_byte_item(u8 code, u8 start_high)
{
	u8 i = ir_send_ctrl.group_cnt;
	ir_send_ctrl.data[i].type = IR_SEND_TYPE_BYTE;
	ir_send_ctrl.data[i].ir_number = 16;
	ir_send_ctrl.data[i].start_high = start_high;
	ir_send_ctrl.data[i].code = code;

	++ir_send_ctrl.group_cnt;
}


void ir_send_add_half_byte_item(u8 code, u8 start_high){
	u8 i = ir_send_ctrl.group_cnt;
	ir_send_ctrl.data[i].type = IR_SEND_TYPE_HALF_TIME_SERIES;
	ir_send_ctrl.data[i].ir_number = 16;
	ir_send_ctrl.data[i].start_high = start_high;
	ir_send_ctrl.data[i].code = code;

	++ir_send_ctrl.group_cnt;
}






static inline void ir_set_repeat_timer(void)
{
	if(!ir_send_ctrl.repeat_timer_enable){

        ir_send_ctrl.repeat_timer_enable = 1;
		ir_send_ctrl.repeat_time = clock_time();

		//set timer2 cap 108 ms
		reg_irq_mask |= FLD_IRQ_TMR2_EN;
		reg_tmr2_tick = 0;
		reg_tmr2_capt = CLOCK_SYS_CLOCK_1US * 1000 * 108 ;
		reg_tmr_ctrl |= FLD_TMR2_EN;
	}
}


static inline void ir_release_repeat_timer(){
	if(ir_send_ctrl.repeat_timer_enable){
		ir_send_ctrl.repeat_timer_enable = 0;
		reg_irq_mask &= ~FLD_IRQ_TMR2_EN;
		reg_tmr_ctrl  &=  ~FLD_TMR2_EN;  //stop timer2
	}
}




int ir_is_sending()
{
	if(ir_send_ctrl.is_sending && clock_time_exceed(ir_send_ctrl.sending_start_time, 300*1000))
	{
		ir_send_ctrl.is_sending = 0;
		pwm_stop(IR_PWM_ID);

	}

	return ir_send_ctrl.is_sending;
}

int ir_sending_check(void)
{
	u8 r = irq_disable();
	if(ir_is_sending()){
		irq_restore(r);
		return 1;
	}
	irq_restore(r);
	return 0;
}



void ir_send_ctrl_start(int need_repeat)
{
	if(need_repeat){
		ir_set_repeat_timer();
	}
	else{
		ir_release_repeat_timer();
	}

	ir_send_ctrl.ir_send_irq_idx = 0;
	ir_send_ctrl.group_index = 0;
	ir_send_ctrl.sending_start_time = clock_time();


	ir_irq_send();  //set and run IR task0
	ir_irq_send();	//set IR task1
}

void ir_send_release(void){
	ir_send_ctrl.last_cmd = 0xff;
	ir_release_repeat_timer();
}






int ir_send_repeat_timer(void *data)
{
	#if (IR_MODE_SELECT == IR_TYPE_NEC_TIANZUN)
		ir_nec_send_repeat();
	#elif(IR_MODE_SELECT == IR_TYPE_SKYWORTH_TC9012)
		ir_tc9012_send_repeat(0x0E);
	#elif(IR_MODE_SELECT == IR_TYPE_CHANGHONG_uPD6121)
		ir_upd6121_send_repeat();
	#elif(IR_MODE_SELECT == IR_TYPE_KONKA_KONKA)
		ir_konka_send_repeat();
	#elif(IR_MODE_SELECT == IR_TYPE_SUNSUNG_TC9012)
		ir_tc9012_send_repeat(0x07);
	#elif(IR_MODE_SELECT == IR_TYPE_HiSense_uPD6121)
		ir_upd6121_send_repeat();
	#elif(IR_MODE_SELECT == IR_TYPE_HAIER_TC9012)
		ir_tc9012_send_repeat(0x18);
	#elif(IR_MODE_SELECT == IR_TYPE_TCL_RCA)
		ir_rca_send_repeat();
	#endif


		return 0;
}




#if (REMOTE_IR_ENABLE)
_attribute_ram_code_
#endif
void ir_irq_send(void)
{
	if(ir_send_ctrl.group_index >= ir_send_ctrl.group_cnt){  //pwm ir mode stop
		ir_send_ctrl.is_sending = 0;
		pwm_stop(IR_PWM_ID);

		#if (IR_PWM_SELECT == PWM0_IR_MODE)
			reg_pwm_irq_mask &= ~FLD_IRQ_PWM0_PNUM;
		#elif (IR_PWM_SELECT == PWM1_IR_MODE)
			reg_pwm_irq_mask &= ~FLD_IRQ_PWM1_PNUM;
		#endif

		return;
	}



	u8 i = ir_send_ctrl.group_index;
	ir_ctrl_t	*cur_ir_ctl;


	if(0 == ir_send_ctrl.ir_send_irq_idx){		// start
		ir_send_ctrl.ir_send_start_high = ir_send_ctrl.data[i].start_high;
	}


	if(IR_SEND_TYPE_TIME_SERIES == ir_send_ctrl.data[i].type)
	{
		cur_ir_ctl = &ir_send_ctrl.data[i].ptr_irCtl[ir_send_ctrl.ir_send_irq_idx];
	}
	else if(IR_SEND_TYPE_BYTE == ir_send_ctrl.data[i].type)
	{
		cur_ir_ctl = ir_get_byte_control(ir_send_ctrl.data[i].code, ir_send_ctrl.ir_send_irq_idx, ir_send_ctrl.ir_send_start_high);
	}


	pwm_set_cycle_and_duty(IR_PWM_ID, cur_ir_ctl->cycle, cur_ir_ctl->hich);  //set cycle and high
	pwm_set_pulse_num(IR_PWM_ID, cur_ir_ctl->cnt);



	++ ir_send_ctrl.ir_send_irq_idx;
	if(ir_send_ctrl.ir_send_irq_idx == ir_send_ctrl.data[i].ir_number){
		ir_send_ctrl.ir_send_irq_idx = 0;
		ir_send_ctrl.group_index ++;
	}



	if(!ir_send_ctrl.is_sending){ //pwm ir mode start
		ir_send_ctrl.is_sending = 1;

		#if (IR_PWM_SELECT == PWM0_IR_MODE)
			reg_pwm_irq_sta = FLD_IRQ_PWM0_PNUM;
			reg_pwm_irq_mask |= FLD_IRQ_PWM0_PNUM;
		#elif (IR_PWM_SELECT == PWM1_IR_MODE)
			reg_pwm_irq_sta = FLD_IRQ_PWM1_PNUM;
			reg_pwm_irq_mask |= FLD_IRQ_PWM1_PNUM;
		#endif

		pwm_start(IR_PWM_ID);
	}


	//ir_send_ctrl.ir_send_start_high = !ir_send_ctrl.ir_send_start_high;
	ir_send_ctrl.ir_send_start_high ^= 1;

}





#if (REMOTE_IR_ENABLE)
_attribute_ram_code_
#endif
void ir_repeat_handle(void)
{
    if(ir_send_ctrl.repeat_timer_enable){
         ir_send_repeat_timer(0);
    }
}





void rc_ir_init(void)
{

#if (IR_PWM_SELECT == PWM0_IR_MODE)
	gpio_set_func(GPIO_PA0, AS_PWM);
#elif (IR_PWM_SELECT == PWM1_IR_MODE)
	gpio_set_func(GPIO_PA3, AS_PWM);
#endif

	pwm_set_clk(CLOCK_SYS_CLOCK_HZ, CLOCK_SYS_CLOCK_HZ);
	pwm_set_mode(IR_PWM_ID, PWM_IR_MODE);  //pwm0 count mode
	pwm_set_phase(IR_PWM_ID, 0);   //no phase at pwm beginning

	reg_irq_mask |= FLD_IRQ_SW_PWM_EN;


	//nec IR, config carrier and logic data ir timing
	ir_config_carrier(PWM_CYCLE_VALUE, PWM_HIGH_VALUE);

	ir_config_byte_timing(	IR_HIGH_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US,
							IR_HIGH_NO_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US,
							IR_LOW_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US,
							IR_LOW_NO_CARR_TIME_NEC * CLOCK_SYS_CLOCK_1US);


	ir_send_ctrl.last_cmd = 0xff; //must
}








#endif
