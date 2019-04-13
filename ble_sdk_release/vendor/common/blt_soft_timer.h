/*
 * blt_soft_timer.h
 *
 *  Created on: 2016-10-28
 *      Author: Administrator
 */

#ifndef BLT_SOFT_TIMER_H_
#define BLT_SOFT_TIMER_H_


//user define
#define		BLT_SOFTWARE_TIMER_ENABLE				0   //enable or disable


#define 	MAX_TIMER_NUM							4   //timer max number


#define		MAINLOOP_ENTRY							0
#define 	CALLBACK_ENTRY							1



//if t1 < t2  return 1
#define		TIME_COMPARE_SMALL(t1,t2)   ( (u32)((t2) - (t1)) < BIT(30)  )

// if t1 > t2 return 1
#define		TIME_COMPARE_BIG(t1,t2)   ( (u32)((t1) - (t2)) < BIT(30)  )


#define		BLT_TIMER_SAFE_MARGIN_PRE	  (CLOCK_SYS_CLOCK_1US<<8)  //256 us
#define		BLT_TIMER_SAFE_MARGIN_POST	  (CLOCK_SYS_CLOCK_1S<<2)   // 4S
static int inline blt_is_timer_expired(u32 t, u32 now) {
	//return ((u32)(now - t) < BLT_TIMER_SAFE_MARGIN_POST);
	return ((u32)(now + BLT_TIMER_SAFE_MARGIN_PRE - t) < BLT_TIMER_SAFE_MARGIN_POST);
}




typedef int (*blt_timer_callback_t)(void);




typedef struct blt_time_event_t {
	blt_timer_callback_t    cb;
	u32                     t;
	u32                     interval;
} blt_time_event_t;


// timer table managemnt
typedef struct blt_soft_timer_t {
	blt_time_event_t	timer[MAX_TIMER_NUM];  //timer0 - timer3
	u8					currentNum;  //total valid timer num
} blt_soft_timer_t;




//////////////////////// USER  INTERFACE ///////////////////////////////////
//return 0 means Fail, others OK
int 	blt_soft_timer_add(blt_timer_callback_t func, u32 interval_us);
int 	blt_soft_timer_delete(blt_timer_callback_t func);




//////////////////////// SOFT TIMER MANAGEMENT  INTERFACE ///////////////////////////////////
void 	blt_soft_timer_init(void);
void  	blt_soft_timer_process(int type);
int 	blt_soft_timer_delete_by_index(u8 index);


int is_timer_expired(blt_timer_callback_t *e);


#endif /* BLT_SOFT_TIMER_H_ */
