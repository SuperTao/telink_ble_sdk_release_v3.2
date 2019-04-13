
#include "../tl_common.h"
#include "battery.h"
#include "adc.h"

#include "../../proj_lib/pm.h"
#if(BATT_CHECK_ENABLE)






/***
 * the function can filter the not good data from the adc.
 * remove the maximum data and minimum data from the adc data.
 * then average the rest data to calculate the voltage of battery.
 **/
unsigned short filter_data(unsigned short* pData,unsigned char len)
{
	unsigned char index = 0,loop = 0;
	unsigned short temData = 0;
	//bubble sort,user can use your algorithm.it is just an example.
	for(loop = 0;loop <(len-1); loop++){
		for(index = 0;index <(len-loop-1); index++){
			if(pData[index]>pData[index+1]){
				temData = pData[index];
				pData[index] = pData[index+1];
				pData[index+1] = temData;
			}
		}
	}

	//remove the maximum and minimum data, then average the rest data.
	unsigned int data_sum = 0;
	unsigned char s_index = 0;
	for(s_index=1;s_index <(len-1);s_index++){
		data_sum += pData[s_index];
	}
	return (data_sum/(len-2));
}


/****
 * the function is used to check the voltage of battery.
 * when the battery voltage is less than 1.96v,
 * let the chip enter deep sleep mode.
 **/
#define  BATT_CHECK_CNT  8
void battery_power_check(void)
{
	static u32 battCheckTick = 0;
	if(clock_time_exceed(battCheckTick, 100000)){
		battCheckTick = clock_time();
	}
	else{
		return;
	}



	int adc_idx = 0;
	unsigned short adcValue[BATT_CHECK_CNT] = {0};

	for(adc_idx=0;adc_idx<BATT_CHECK_CNT;adc_idx++){
		adcValue[adc_idx] = adc_SampleValueGet();
	}

	unsigned short average_data;
	average_data = filter_data(adcValue,BATT_CHECK_CNT);


	unsigned int tem_batteryVol;         //2^14 - 1 = 16383;
#if((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))
	tem_batteryVol = 3*(1428*(average_data-128)/(16383-256)); //2^14 - 1 = 16383;
#elif(MCU_CORE_TYPE == MCU_CORE_8266)
	tem_batteryVol = 3*((1300*average_data)>>14);
#endif

	if(tem_batteryVol < 2000){  //when battery voltage is lower than 2.0v, chip will enter deep sleep mode
		cpu_sleep_wakeup(1, PM_WAKEUP_PAD, 0);
	}
}





#endif

