
#pragma once

#if(__TL_LIB_8267__ || (MCU_CORE_TYPE == MCU_CORE_8267) || \
	__TL_LIB_8261__ || (MCU_CORE_TYPE == MCU_CORE_8261) || \
	__TL_LIB_8269__ || (MCU_CORE_TYPE == MCU_CORE_8269)	)

typedef void (*pm_optimize_handler_t)(void);

static inline void usb_dp_pullup_en (int en)
{
	unsigned char dat = ReadAnalogReg(0x08);
	if (en) {
		dat = (dat & 0x3f) | BIT(7);
	}
	else {
		dat = (dat & 0x3f) | BIT(6);
	}

	WriteAnalogReg (0x08, dat);
}



#define SUSPEND_MODE	0
#define DEEPSLEEP_MODE	1


//8267 analog register 0x34-0x3e can store infomation when MCU in deepsleep mode
//store your information in these ana_regs before deepsleep by calling analog_write function
//when MCU wakeup from deepsleep, read the information by by calling analog_read function

//these five below are stable
#define DEEP_ANA_REG0    0x3a
#define DEEP_ANA_REG1    0x3b
#define DEEP_ANA_REG2    0x3c
#define DEEP_ANA_REG3    0x3d

//these six below may have some problem when user enter deepsleep but ERR wakeup
// for example, when set a GPIO PAD high wakeup deepsleep, but this gpio is high before
// you call func cpu_sleep_wakeup, then deepsleep will be ERR wakeup, these 6 register
//   infomation loss.
#define DEEP_ANA_REG5    0x34
#define DEEP_ANA_REG6    0x35
#define DEEP_ANA_REG7    0x36
#define DEEP_ANA_REG8    0x37
#define DEEP_ANA_REG9    0x38
#define DEEP_ANA_REG10   0x39


#define ADV_DEEP_FLG	 0x01
#define CONN_DEEP_FLG	 0x02



#define SYS_DEEP_ANA_REG	0x3e  //ana_3e system use for external 32k mode, user can not use






#define 	BLT_RESET_WAKEUP_TIME_2000		1

#if (BLT_RESET_WAKEUP_TIME_2000)
	#define RESET_TIME_US	    	  2000
	#define EARLYWAKEUP_TIME_US       2100
	#define EMPTYRUN_TIME_US       	  2400
#elif(BLT_RESET_WAKEUP_TIME_2200)
	#define RESET_TIME_US	    	  2200
	#define EARLYWAKEUP_TIME_US       2300
	#define EMPTYRUN_TIME_US       	  2600
#elif(BLT_RESET_WAKEUP_TIME_2400)
	#define RESET_TIME_US	    	  2400
	#define EARLYWAKEUP_TIME_US       2500
	#define EMPTYRUN_TIME_US       	  2800
#elif(BLT_RESET_WAKEUP_TIME_2600)
	#define RESET_TIME_US	    	  2600
	#define EARLYWAKEUP_TIME_US       2700
	#define EMPTYRUN_TIME_US       	  3000
#elif(BLT_RESET_WAKEUP_TIME_2800)
	#define RESET_TIME_US	    	  2800
	#define EARLYWAKEUP_TIME_US       2900
	#define EMPTYRUN_TIME_US       	  3200
#else
#endif



#define SWAP_BIT0_BIT6(x)     ( ((x)&0xbe) | ( ((x)&0x01)<<6 ) | ( ((x)&0x40)>>6 )  )



typedef int (*suspend_handler_t)(void);
extern suspend_handler_t func_before_suspend;



void cpu_wakeup_init(int);



typedef int (*cpu_pm_handler_t)(int,int, u32);
extern cpu_pm_handler_t cpu_sleep_wakeup;
//int cpu_sleep_wakeup (int deepsleep, int wakeup_src, u32 wakeup_tick);

int cpu_sleep_wakeup_32krc (int deepsleep, int wakeup_src, u32 wakeup_tick);
int cpu_sleep_wakeup_32kpad (int deepsleep, int wakeup_src, u32 wakeup_tick);


void blt_pm_ext32k_crystal_init(void);


//set wakeup source
enum {
	 PM_WAKEUP_PAD   = BIT(4),
	 PM_WAKEUP_CORE  = BIT(5),
	 PM_WAKEUP_TIMER = BIT(6),
	 PM_WAKEUP_COMP  = BIT(7),
};


//wakeup status from return value of "cpu_sleep_wakeup"
enum {
	 WAKEUP_STATUS_COMP   = BIT(0),  //wakeup by comparator
	 WAKEUP_STATUS_TIMER  = BIT(1),
	 WAKEUP_STATUS_CORE   = BIT(2),
	 WAKEUP_STATUS_PAD    = BIT(3),

	 STATUS_GPIO_ERR_NO_ENTER_PM  = BIT(7),
};

#define 	WAKEUP_STATUS_TIMER_CORE	( WAKEUP_STATUS_TIMER | WAKEUP_STATUS_CORE)







/******************************* User Interface  ************************************/


void blc_pm_select_internal_32k_crystal(void);
void blc_pm_select_external_32k_crystal(void);

void bls_pm_registerFuncBeforeSuspend (suspend_handler_t func );

void cpu_set_gpio_wakeup (int pin, int pol, int en);



void cpu_stall_wakeup_by_timer0(u32 tick_stall);
void cpu_stall_wakeup_by_timer1(u32 tick_stall);
void cpu_stall_wakeup_by_timer2(u32 tick_stall);




#endif
