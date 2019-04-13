/*
 * emi.h
 *
 *  Created on: 2017-8-23
 *      Author: Administrator
 */

#ifndef EMI_H_
#define EMI_H_

#if (MCU_CORE_TYPE == MCU_CORE_8266)
	#define  PA_POWER_ON    0
#else
	#define  PA_POWER_ON    1
#endif

#define RSSI_FILTER	 	1

#define EMI_TEST_TX_MODE  			0x3f005
#define EMI_TEST_RUN  				0x3f006
#define EMI_TEST_CMD  				0x3f007
#define EMI_TEST_POWER_LEVEL  		0x3f008
#define EMI_TEST_CHANNEL  			0x3f009
#define EMI_TEST_MODE  				0x3f00a


#define CAP_VALUE					0x77000
#define TP_LOW_VALUE				0x77040
#define TP_HIGH_VALUE				0x77041

#define TX_PACKET_MODE_ADDR 		0x808005
#define RUN_STATUE_ADDR 			0x808006
#define TEST_COMMAND_ADDR			0x808007
#define POWER_ADDR 					0x808008
#define CHANNEL_ADDR				0x808009
#define RF_MODE_ADDR				0x80800a


extern int pnGen(int state);
extern int Rf_EmiInit(void);
extern void	Rf_EmiCarrierOnlyTest(int power_level,signed char rf_chn);
extern void	Rf_EmiCarrierDataTest(int power_level,signed char rf_chn);
extern void Rf_EmiDataUpdate(void);
extern void Rf_EmiRxTest(unsigned char *addr,signed char rf_chn,int buffer_size,unsigned char  pingpong_en);
extern void Rf_EmiTxInit(int power_level,signed char rf_chn);
extern void Rf_EmiSingleTx(unsigned char *addr,int power_level);
unsigned char Rf_rssi_filter(unsigned char* pRfRxBuff);
void emi_test(void);
#endif /* EMI_H_ */

