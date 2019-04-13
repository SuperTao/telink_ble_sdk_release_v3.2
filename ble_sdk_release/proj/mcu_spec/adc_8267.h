/**************************************************************************************************
  Filename:       	adc.h
  Author:			junjun.xu@telink-semi.com
  Created Date:	2016/06/05

  Description:    This file contains the enum variable defination details used for adc function


**************************************************************************************************/

#if(__TL_LIB_8267__ || (MCU_CORE_TYPE == MCU_CORE_8267) || \
	__TL_LIB_8261__ || (MCU_CORE_TYPE == MCU_CORE_8261) || \
	__TL_LIB_8269__ || (MCU_CORE_TYPE == MCU_CORE_8269))

#ifndef 	ADC_8267_H_
#define 	ADC_8267_H_

#include "../mcu/register.h"
#include "../common/compatibility.h"
#include "../common/utility.h"

#define battery2audio() (reg_adc_ctrl = MASK_VAL(FLD_ADC_AUD_MODE, MONO_AUDIO, FLD_ADC_AUD_DATAPATH_EN, 1, FLD_ADC_CHNL_AUTO_EN, 1))
#define audio2battery() (reg_adc_ctrl = MASK_VAL(FLD_ADC_CHNM_AUTO_EN, 1, FLD_ADC_AUD_DATAPATH_EN, 1))
//ADC reference voltage
enum ADCRFV{
	RV_1P428,
	RV_AVDD,
	RV_1P224,
};

//ADC resolution
enum ADCRESOLUTION{
	RES7,
	RES9,
	RES10,
	RES11,
	RES12,
	RES13,
	RES14,
};

//ADC Sampling time
enum ADCST{
	S_3,
	S_6,
	S_9,
	S_12,
	S_18,
	S_24,
	S_48,
	S_144,
};

//ADC analog input channel selection enum
enum ADCINPUTCH{
	NOINPUT,
	C0,
	C1,
	C6,
	C7,
	B0,
	B1,
	B2,
	B3,
	B4,
	B5,
	B6,
	B7,
	PGAVOM,
	PGAVOP,
	TEMSENSORN,
	TEMSENSORP,
	AVSS,
	OTVDD,//1/3 voltage division detection
};

//ADC channel input mode
enum ADCINPUTMODE{
	SINGLEEND,
	INVERTB_1,
	INVERTB_3,
	PGAVOPM,
};

//adc clock
enum ADCCLOCK {
	ADC_CLK_4M = 4,
	ADC_CLK_5M = 5,
};

//if select interval 1/3 voltage division
enum ONETHIRD_INPUTCHN {
	Battery_Chn_VCC,
	Battery_Chn_B7,
};


/**********************************************************************
* @brief	1.ADC initiate function, set the ADC clock details (4MHz) and start the ADC clock.
*	        set input channel,set reference voltage, set resolution bits, set sample cycle
*			2.ADC clock relays on PLL, if the FHS isn't selected to 192M PLL (probably modified
*			by other parts codes), adc initiation function will returns error.
*
* @param[in] chn          - enum variable ADCINPUTCH ,acd channel
* @param[in] mode         - enum variable ADCINPUTMODE, select adc mode.
* @param[in] ref_vol      - enum variable ADCRFV,select reference voltage.
* @param[in] resolution   - enum variable ADCRESOLUTION, select resolution
* @param[in] sample_cycle - enum variable ADCST, select sample cycle
*
* @return none
*/
void adc_Init(enum ADCCLOCK adc_clk,enum ADCINPUTCH chn,enum ADCINPUTMODE mode,enum ADCRFV ref_vol,\
		      enum ADCRESOLUTION resolution,enum ADCST sample_cycle);

/********************************************************
*
*	@brief		Initiate function for the battery check function. initial adc clock and select the input channel.
*
*	            NOTICE: the parameter "div_en" indicate whether or not open internal 1/3 voltage division.
*	            if "div_en" is 1, .i.e enable internal 1/3 voltage division, the parameter oneThirdChn is valid while
*	            the parameter notOneThirdChn is invalid.
*	            if "div_en" is 0, .i.e disable internal 1/3 voltage division,the parameter oneThirdChn is invalid while
*	            the parameter notOneThirdChn is valid.
*
*	@param[in]  adc_clk -- set the clock of adc module.
*	@param[in]  div_en  -- whether or not open internal 1/3 voltage division.
*	@param[in]  oneThirdChn-- ONETHIRD_INPUTCHN variable. this parameter is valid when "div_en" is 1.
*	@param[in]  notOneThirdChn -- ADCINPUTCH variable.    this parameter is valid when "div_en" is 0.
*   @param[in] mode         - enum variable ADCINPUTMODE, select adc mode.
*   @param[in] ref_vol      - enum variable ADCRFV,select reference voltage.
*   @param[in] resolution   - enum variable ADCRESOLUTION, select resolution
*   @param[in] sample_cycle - enum variable ADCST, select sample cycle
*
*	@return		None
*/
void adc_BatteryCheckInit(enum ADCCLOCK adc_clk,unsigned char div_en,enum ONETHIRD_INPUTCHN oneThirdChn,enum ADCINPUTCH notOneThirdChn,\
		                  enum ADCINPUTMODE mode,enum ADCRFV ref_vol,enum ADCRESOLUTION resolution,enum ADCST sample_cycle);


/********************************************************
*
*	@brief	Initiate function for the temperature sensor.
*	        temperature adc value = TEMSENSORP_adc_value - TEMSENSORN_adc_value
*           step 1: adc_TemSensorInit(...TEMSENSORP...);
*           step 2: TEMSENSORP_adc_value = adc_SampleValueGet();
*           step 3: adc_TemSensorInit(...TEMSENSORN...);
*           step 4: TEMSENSORN_adc_value = adc_SampleValueGet();
*           step 5: temperature adc value = TEMSENSORP_adc_value - TEMSENSORN_adc_value
*	@param[in]	adc_clk -- set the clock of adc module.
*	@param[in]  chn -- enum variable ADCINPUTCH ,select channel,in fact only two selection:one is TEMSENSORN,the other is TEMSENSORP
*	@param[in]  mode         - enum variable ADCINPUTMODE, select adc mode.
*   @param[in]  ref_vol      - enum variable ADCRFV,select reference voltage.
*   @param[in]  resolution   - enum variable ADCRESOLUTION, select resolution
*   @param[in]  sample_cycle - enum variable ADCST, select sample cycle
*
*	@return		None
*/
void adc_TemSensorInit(enum ADCCLOCK adc_clk,enum ADCINPUTCH chn,enum ADCINPUTMODE mode,enum ADCRFV ref_vol,\
	      enum ADCRESOLUTION resolution,enum ADCST sample_cycle);

/*************************************************************************
*
*	@brief	get adc sampled value
*
*	@param	adc_ch:	adc channel select, MISC or the LCHANNEL, enum variable
*			sample_mode:	adc sample mode, '1' manual mode; '0' auto sample mode
*
*	@return	sampled_value:	raw data
*/
unsigned short adc_SampleValueGet(void);

/********************************************************
*
*	@brief		get the battery value
*
*	@param		None
*
*	@return		unsigned long - return the sampling value multiplex 3
*/
unsigned short adc_BatteryValueGet(void);

#endif

#endif

