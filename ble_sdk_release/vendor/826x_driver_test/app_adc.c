#include "../../proj/tl_common.h"

#if((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))
#include "../../proj/mcu_spec/adc_8267.h"
#include "../../proj_lib/pm_8267.h"
#elif(MCU_CORE_TYPE == MCU_CORE_8266)
#include "../../proj/mcu_spec/adc_8266.h"
#include "../../proj_lib/pm_8266.h"
#endif

#if (DRIVER_TEST_MODE == TEST_ADC)

unsigned int app_adc_test_Vol;
unsigned char ref_vol = 0;


void app_adc_test_init(void){

#if((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))
	#if BATT_CHECK_ENABLE
		adc_BatteryCheckInit(ADC_CLK_4M, 1, Battery_Chn_VCC, 0, SINGLEEND, RV_1P428, RES14, S_3);
		ref_vol = RV_1P428;
	#else
		adc_Init(ADC_CLK_4M, B6, SINGLEEND, RV_AVDD, RES14, S_3);
		ref_vol = RV_AVDD;
	#endif
#elif(MCU_CORE_TYPE == MCU_CORE_8266)
	adc_Init(ADC_CLK_4M, ADC_CHN_D2, SINGLEEND, ADC_REF_VOL_AVDD, ADC_SAMPLING_RES_14BIT, ADC_SAMPLING_CYCLE_6);
	ref_vol = ADC_REF_VOL_AVDD;
#endif
}


/***
 * the function can filter the not good data from the adc.
 * remove the maximum data and minimum data from the adc data.
 * then average the rest data to calculate the voltage of battery.
 **/
static unsigned short app_adc_test_filter_data(unsigned short* pData,unsigned char len){
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
#define  ADC_SAMPLE_CNT  8
void app_adc_test_start(void){
	static u32 battCheckTick = 0;
	int adc_idx = 0;
	unsigned short adcValue[ADC_SAMPLE_CNT] = {0};
	unsigned short average_data;


	if(clock_time_exceed(battCheckTick, 100000)){
		battCheckTick = clock_time();
	}
	else{
		return;
	}

	for(adc_idx=0;adc_idx<ADC_SAMPLE_CNT;adc_idx++){
		adcValue[adc_idx] = adc_SampleValueGet();
	}

	average_data = app_adc_test_filter_data(adcValue,ADC_SAMPLE_CNT);

	if(average_data < 128)
		average_data = 128;

	#if((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))

			switch(ref_vol){
			case RV_1P428:
				#if (BATT_CHECK_ENABLE)
					app_adc_test_Vol = 3*(1428*(average_data-128)/(16383-256)); //2^14 - 1 = 16383;
				#else
					app_adc_test_Vol = 1428*(average_data-128)/(16383-256); //2^14 - 1 = 16383;
				#endif
				break;
			case RV_AVDD:
				#if (BATT_CHECK_ENABLE)
					app_adc_test_Vol = 3*(3300*(average_data-128)/(16383-256)); //2^14 - 1 = 16383;
				#else
					app_adc_test_Vol = 3300*(average_data-128)/(16383-256); //2^14 - 1 = 16383;
				#endif
				break;
			case RV_1P224:
				#if (BATT_CHECK_ENABLE)
					app_adc_test_Vol = 3*(1224*(average_data-128)/(16383-256)); //2^14 - 1 = 16383;
				#else
					app_adc_test_Vol = 1224*(average_data-128)/(16383-256); //2^14 - 1 = 16383;
				#endif
				break;
			}

	#elif(MCU_CORE_TYPE == MCU_CORE_8266)
			switch(ref_vol){
			case ADC_REF_VOL_1V3:
				app_adc_test_Vol = ((1300*average_data)>>14);
				break;
			case ADC_REF_VOL_AVDD:
				app_adc_test_Vol = ((3300*average_data)>>14);
				break;
			}
	#endif
//	if(app_adc_test_Vol < 1900){  //when battery voltage is lower than 1.9v, chip will enter deep sleep mode
//		cpu_sleep_wakeup(1, PM_WAKEUP_PAD, 0);  // chip enter deep sleep mode
//	}
}


#endif
