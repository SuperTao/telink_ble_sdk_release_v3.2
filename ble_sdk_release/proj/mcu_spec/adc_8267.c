/*
 * adc_8267.c
 *
 *  Created on: 2015-12-10
 *      Author: Telink
 */
#include "../tl_common.h"
#if(__TL_LIB_8267__ || (MCU_CORE_TYPE == MCU_CORE_8267) || \
	__TL_LIB_8261__ || (MCU_CORE_TYPE == MCU_CORE_8261) || \
	__TL_LIB_8269__ || (MCU_CORE_TYPE == MCU_CORE_8269))


#include "adc_8267.h"

/**************************************************************************************************
  Filename:       adc.c
  Author:		  qiuwei.chen@telink-semi.com
  Created Date:	  2017/04/19

  Description:    This file contains the adc driver functions for the Telink 8267.
                  It provided some sample applications like battery check and temperature check

**************************************************************************************************/

enum {
	ADC_DONE_SIGNAL_RISING = 1,
	ADC_DONE_SIGNAL_FALLING,
};

//Check adc busy status bit
#define     CHECKADCSTATUS      ((reg_adc_chn0_input & FLD_ADC_BUSY_FLAG) ? 1:0)

//enable ADC clock
#define     EN_ADCCLK           (reg_adc_clk_en |= FLD_ADC_MOD_H_CLK)

//Select ADC auto mode
#define     EN_ADC_AUTO         do{\
	                            	reg_adc_ctrl = 0x00;\
	                            	reg_adc_ctrl |= MASK_VAL(FLD_ADC_CHNM_AUTO_EN,1,FLD_ADC_DONE_SIGNAL,ADC_DONE_SIGNAL_FALLING);\
                                }while(0)

//Read sampling data
#define		READOUTPUTDATA		reg_adc_dat_byp_outp


static inline void adc_SetClkFreq(u8 mhz){
	reg_adc_step_l = mhz*4;
	reg_adc_mod = MASK_VAL(FLD_ADC_MOD, 192*4, FLD_ADC_CLK_EN, 1);
}

/********************************************************
*
*	@brief		set ADC reference voltage for the Misc
*
*	@param		adcCha - enum variable adc channel.
*				adcRF - enum variable of adc reference voltage.
*
*	@return		None
*/
static inline void adc_RefVoltageSet(enum ADCRFV adcRF){
	unsigned char ref_vol;

	ref_vol = (unsigned char)adcRF;
	BM_CLR(reg_adc_ref,FLD_ADC_REF_M);
	reg_adc_ref |= MASK_VAL(FLD_ADC_REF_M,ref_vol);
}
/********************************************************
*
*	@brief		set ADC resolution for channel Misc
*
*	@param		adcRes - enum variable adc resolution.
*
*	@return		None
*/
static inline void adc_ResSet(enum ADCRESOLUTION adcRes){
	unsigned char resN;
	resN = (unsigned char )adcRes;
	BM_CLR(reg_adc_samp_res,FLD_ADC_CHNM_SAMP_RESOL);
	reg_adc_samp_res |= MASK_VAL(FLD_ADC_CHNM_SAMP_RESOL,resN);
}

/********************************************************
*
*	@brief		set ADC sample time(the number of adc clocks for each sample)
*
*	@param		adcCha - enum variable adc channel.
*				adcST - enum variable of adc sample time.
*
*	@return		None
*/

static inline void adc_SampleTimeSet( enum ADCST adcST){

	unsigned char st;
	st = (unsigned char)adcST;
	BM_CLR(reg_adc_samp_res,FLD_ADC_CHNM_SAMP_CYCLE);
	reg_adc_samp_res |= MASK_VAL(FLD_ADC_CHNM_SAMP_CYCLE,st);
}


/********************************************************
*
*	@brief		set ADC analog input channel
*
*	@param		adcCha - enum variable adc channel.
*				adcInCha - enum variable of adc input channel.
*
*	@return		None
*/
static inline void adc_AnaChSet(enum ADCINPUTCH adcInCha){
	unsigned char cnI;

	cnI = (unsigned char)adcInCha;
	BM_CLR(reg_adc_chn_m_sel,FLD_ADC_CHN_SEL);
	reg_adc_chn_m_sel |= MASK_VAL(FLD_ADC_CHN_SEL,cnI);
}

/********************************************************
*
*	@brief		set ADC input channel mode - signle-end or differential mode
*
*	@param		adcCha - enum variable adc channel.
*				inM - enum variable of ADCINPUTMODE.
*
*	@return		None
*/
static inline void adc_AnaModeSet( enum ADCINPUTMODE inM){
	unsigned char cnM;

	cnM = (unsigned char)inM;
	BM_CLR(reg_adc_chn_m_sel,FLD_ADC_DIFF_CHN_SEL);
	reg_adc_chn_m_sel |= MASK_VAL(FLD_ADC_DIFF_CHN_SEL,cnM);
}



/***************************************************************************
*
*	@brief	set IO power supply for the 1/3 voltage division detection, there are two input sources of the
*			IO input battery voltage, one through the VDDH and the other through the  ANA_B<7> pin
*
*	@param	IOp - input power source '1' is the VDDH; '2' is the ANA_B<7>.
*
*	@return	'1' setting success; '0' set error
*/
static unsigned char adc_IOPowerSupplySet(unsigned char IOp){
	unsigned char vv1;
	if(IOp>2||IOp<1){

		return 0;
	}
	else{
		vv1 = ReadAnalogReg(0x02);
		vv1 = vv1 & 0xcf;
		vv1 = vv1 | (IOp<<4);
		WriteAnalogReg(0x02,vv1);
		return 1;
	}
}

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
		      enum ADCRESOLUTION resolution,enum ADCST sample_cycle){
	/***1.set adc's clock and enable adc clock***/
	adc_SetClkFreq(adc_clk);

	/***2.set the analog input pin***/
	adc_AnaChSet(chn);

	/***3.set ADC mode,signle-end or differential mode***/
	adc_AnaModeSet(mode);///default is single-end

	/***4.set reference voltage***/
	adc_RefVoltageSet(ref_vol);

	/***5.set resolution***/
	adc_ResSet(resolution);

	/***6.set sample cycle**/
	adc_SampleTimeSet(sample_cycle);

	/***7.set misc channel sample and convert period***/
//	reg_adc_period_chn0 = (0xE2<<2);//set M channel period with 0xE2, the adc convert frequency is: system_clock/(4*0xE2);

	/***enable adc auto mode***/
	EN_ADC_AUTO;
}

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
		                  enum ADCINPUTMODE mode,enum ADCRFV ref_vol,enum ADCRESOLUTION resolution,enum ADCST sample_cycle)
{
	/***1.set adc's clock and enable adc clock***/
	adc_SetClkFreq(adc_clk);

	/***1.set adc input***/
	if(div_en){              //if enable internal 1/3 voltage division.
		adc_AnaChSet(OTVDD); //select "1/3 voltage division detection" as ADC input
		if(!oneThirdChn)
			adc_IOPowerSupplySet(1); //AVDD is the input channel
		else
			adc_IOPowerSupplySet(2); //B7   is the input channel
	}
	else{                    //if disable internal 1/3 voltage division.
		adc_AnaChSet(notOneThirdChn);
	}

	/***2.set adc mode***/
	adc_AnaModeSet(mode);

	/***3.set adc reference voltage***/
	adc_RefVoltageSet(ref_vol);     //Set reference voltage (V_REF)as  1.428V

	/***4.set adc resultion***/
	adc_ResSet(resolution);               //Set adc resolution to 14 bits, bit[14] to bit bit[1]

	/***5.set adc sample time***/
	adc_SampleTimeSet(sample_cycle);          //set sample time

	/***7.set misc channel sample and convert period***/
//	reg_adc_period_chn0 = (0xE2<<2);//set M channel period with 0xE2, the adc convert frequency is: system_clock/(4*0xE2);

	/***enable adc auto mode***/
	EN_ADC_AUTO;
}

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
	      enum ADCRESOLUTION resolution,enum ADCST sample_cycle){
	/***1.set adc's clock and enable adc clock***/
	adc_SetClkFreq(adc_clk);

	/***1.set adc mode and input***/
	adc_AnaChSet(chn);

	/***2.conft adc mode***/
	adc_AnaModeSet(mode);

	/***3. set adc reference voltage***/
	adc_RefVoltageSet(ref_vol);

	/***4.set adc resultion***/
	adc_ResSet(resolution);

	/***5.set adc sample time***/
	adc_SampleTimeSet(sample_cycle);

	/***6.set misc channel sample and convert period***/
//	reg_adc_period_chn0 = (0xE2<<2);//set M channel period with 0xE2, the adc convert frequency is: system_clock/(4*0xE2);

	/***7.enable manual mode***/
	EN_ADC_AUTO;
}

/*************************************************************************
*
*	@brief	get adc sampled value
*
*	@param	none
*
*	@return	sampled_value:	raw data
*/
unsigned short adc_SampleValueGet(void){
	unsigned short sampledValue;

	while(!CHECKADCSTATUS);

	while(CHECKADCSTATUS);

	sampledValue = READOUTPUTDATA & 0x3FFF;

	return sampledValue;
}

/********************************************************
*
*	@brief		get the battery value
*
*	@param		None
*
*	@return		unsigned long - return the sampling value
*/
unsigned short adc_BatteryValueGet(void){

	unsigned short sampledValue;

	while(!CHECKADCSTATUS);

	while(CHECKADCSTATUS);

	sampledValue = READOUTPUTDATA & 0x3FFF;

	return sampledValue;
}

#endif
