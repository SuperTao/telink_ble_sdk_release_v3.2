/*
 * emi.c
 *
 *  Created on: 2017-8-23
 *      Author: Administrator
 */
#include "../../proj/tl_common.h"
#include "../../proj/mcu/watchdog_i.h"
#include "../../proj_lib/rf_drv.h"
#include "emi.h"
unsigned char  emi_var[5];
unsigned char  emi_tx[16]  __attribute__ ((aligned (4))) = {0xc,0x00,0x00,0x00,0x00,0x20,0xaa,0xbb};
int state0,state1,state2,state3;
unsigned char depth=1;
#define STATE0		0x1234
#define STATE1		0x5678
#define STATE2		0xabcd
#define STATE3		0xef01
//unsigned char  nordic_tx_packet[16]  __attribute__ ((aligned (4))) = {0x9,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x34,0x56,0x78,0x12};
unsigned char  ble_tx_packet [64]  __attribute__ ((aligned (4))) = {39, 0, 0, 0,0, 37,	0, 1, 2, 3, 4, 5, 6, 7};

unsigned char  rx_packet[128] __attribute__ ((aligned (4)));

unsigned char mode=1;
unsigned char power_level = 0;
unsigned char chn = 2;
unsigned char cmd_now=1;
unsigned char run=0;
unsigned char *packet;
unsigned char tx_mode=0;

unsigned long Adebug_irq=0;
unsigned long Adebug_Rx_irq=0;
unsigned long rssi=0;
/********************************************************
*
*	@brief		Rf_reei_filter
*
*	@param		None
*
*
*	@return		None
*/

unsigned char Rf_rssi_filter(unsigned char* pRfRxBuff){
	#define RSSI_NUM 5

	unsigned char tmp_rssi = pRfRxBuff[4];			//get the RSSI value of this packet
	static unsigned char store_full_flag = 0,filter_first_flag = 0;
	static unsigned char store_rssi_value[RSSI_NUM] = {0};
	static unsigned char sort_rssi_value[RSSI_NUM] = {0};

	static unsigned int rssi_filter = 0,mid_cal_data = 0;
	static unsigned char rssi_index = 0;

	if((rssi_index < RSSI_NUM-1)&& (!store_full_flag)){
		store_rssi_value[rssi_index] = tmp_rssi;
		mid_cal_data = tmp_rssi;
	}
	else{
		store_full_flag = 1;						//receive 5 packets' flag
		store_rssi_value[rssi_index] = tmp_rssi;
		unsigned char copy_index = 0;

		for(copy_index = 0;copy_index<RSSI_NUM;copy_index++){
			sort_rssi_value[copy_index] = store_rssi_value[copy_index];
		}
		unsigned char i = 0,j = 0;
		unsigned char tmp_sort = 0;
		for(i=0;i<RSSI_NUM-1;i++){
			for(j=0;j<RSSI_NUM-1-i;j++){
				if(sort_rssi_value[j] > sort_rssi_value[j+1]){
					tmp_sort = sort_rssi_value[j];
					sort_rssi_value[j] = sort_rssi_value[j+1];
					sort_rssi_value[j+1] = tmp_sort;
				}
			}
		}
		mid_cal_data = sort_rssi_value[RSSI_NUM>>1];
	}

	if(!filter_first_flag){							///filter_first_flag == 0
		filter_first_flag = 1;
		rssi_filter = mid_cal_data;
	}
	else{											//filter_first_flag ==  1
													//+2 to rounding  +2/4
		rssi_filter = (3*rssi_filter + 1*mid_cal_data + 2)>>2;
	}

	rssi_index++;
	rssi_index %= RSSI_NUM;							//from the header restore data
	return rssi_filter;
}


/********************************************************
*
*	@brief		read flash parameter
*
*	@param		None
*
*
*	@return		None
*/
//unsigned  char g_tp0;
//unsigned  char g_tp1;
void read_flash_para(void)
{
//	unsigned char  tp0,tp1,cap;
	unsigned char temp=0;
	flash_read_page(EMI_TEST_POWER_LEVEL,1,&temp);
	if( temp != 0xff )
	{
		power_level=temp;
		write_reg8(POWER_ADDR,power_level);
	}

	flash_read_page(EMI_TEST_CHANNEL,1,&temp);
	if( temp != 0xff )
	{
		chn=temp;
		write_reg8(CHANNEL_ADDR,chn);
	}
	flash_read_page(EMI_TEST_MODE,1,&temp);
	if( temp != 0xff )
	{

		mode=temp;
		write_reg8(RF_MODE_ADDR,mode);
	}
	flash_read_page(EMI_TEST_CMD,1,&temp);
	if(temp != 0xff )
	{

		cmd_now=temp;
		write_reg8(TEST_COMMAND_ADDR,cmd_now);
	}
	flash_read_page(EMI_TEST_TX_MODE,1,&temp);
	if( temp != 0xff )
	{
		tx_mode=temp;
		write_reg8(TX_PACKET_MODE_ADDR,tx_mode);
	}
/*
    flash_read_page(CAP_VALUE,1,&cap);
	if(cap != 0xff && cap > 0xbf && cap < 0xe0 )
	{
		WriteAnalogReg(0x81,cap);
	}
	flash_read_page(TP_LOW_VALUE,1,&g_tp0);
	flash_read_page(TP_HIGH_VALUE,1,&g_tp1);
	if( (tp0 != 0xff ) && (tp1 != 0xff))
	{
		g_tp0 = tp0;
		g_tp1 = tp1;
		if(mode == 1)//1M
		{
			if(( tp0 < (0x1d+10)) && (tp0 > (0x1d-10)) && ( tp1 < (0x19+10)) && (tp1 > (0x19-10)))
			{
				Rf_UpdateTpValue(mode ,tp0, tp1);
			}

		}
		else if(mode == 0 || mode == 2)//2M/250K
		{
			if(( tp0 < (0x40+10)) && (tp0 > (0x40-10)) && ( tp1 < (0x39+10)) && (tp1 > (0x39-10)))
			{
				Rf_UpdateTpValue(mode ,tp0, tp1);
			}
		}

	}
*/
}



/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) test  initialization
*
*	@param		None
*
*
*	@return		None
*/

int Rf_EmiInit(void)
{
	// for registers recover.
	emi_var[0] = ReadAnalogReg(0xa5);
	emi_var[1] = read_reg8(0x8004e8);
//	emi_var[2] = read_reg8(0x800408);
//	emi_var[2] = read_reg8(0x800402);
	emi_var[3] = read_reg8(0x80050f);
	emi_var[4] = read_reg8(0x80050e);

	return 1;

}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) test  recovery setting
*
*	@param		None
*
*
*	@return		None
*/
extern int xtalType_rfMode;
int Rf_EmiCarrierRecovery(void)
{
	//reset zb & dma

	write_reg16(0x800060, 0x0480);
	write_reg16(0x800060, 0x0000);
	WriteAnalogReg (0xa5, emi_var[0]);
    write_reg8 (0x8004e8, emi_var[1]);
    if(( xtalType_rfMode == XTAL_12M_RF_2m_MODE ) || (xtalType_rfMode == XTAL_16M_RF_2m_MODE))
	{
    	write_reg8 (0x800402, 0x26);
	}
	else if(( xtalType_rfMode == XTAL_12M_RF_1m_MODE ) || (xtalType_rfMode == XTAL_16M_RF_1m_MODE))
	{
		write_reg8 (0x800402, 0x26);
	}
//	else if(( xtalType_rfMode == XTAL_12M_RF_250k_MODE ) || (xtalType_rfMode == XTAL_16M_RF_250k_MODE))
//	{
//		write_reg8 (0x800402, 0x26);
//	}

	write_reg8(0x80050f, emi_var[3]);
    write_reg8(0x80050e, emi_var[4]);
    return 1;

}

//unsigned char prbs9[128];
void PhyTest_PRBS9_Tx (unsigned char *p, int n,unsigned char words)
{
	//PRBS9: (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100)
	unsigned short x = 0x1ff;
	int i;
	int j;
	for ( i=0; i<n; i++)
	{
		unsigned char d = 0;
		for (j=0; j<8; j++)
		{
			if (x & 1)
			{
				d |= BIT(j);
			}
			x = (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100);
		}
		if(( i<((words+1)*4))&&( i>= (words*4)))
		{
			*p++ = d;
		}

	}
}
void PhyTest_PRBS9 (unsigned char *p, int n)
{
	//PRBS9: (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100)
	unsigned short x = 0x1ff;
	int i;
	int j;
	for ( i=0; i<n; i++)
	{
		unsigned char d = 0;
		for (j=0; j<8; j++)
		{
			if (x & 1)
			{
				d |= BIT(j);
			}
			x = (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100);
		}
		*p++ = d;
	}
}
/********************************************************
*
*	@brief		pnGen
*
*	@param		None
*
*
*	@return		None
*/
int pnGen(int state)
{
	int feed = 0;
	feed = (state&0x4000) >> 1;
	state ^= feed;
	state <<= 1;
	state = (state&0xfffe) + ((state&0x8000)>>15);
	return state;
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) CarrierOnly Test
*
*	@param		power_level: set power level(0~14)
*				rf_chn	   : set tx channel((0~100))
*
*
*	@return		None
*/
void Rf_EmiCarrierOnlyTest(int power_level,signed char rf_chn)
{
	Rf_EmiCarrierRecovery();
	SetTxMode(rf_chn,0);         //0,RF_MODE_TX
	WaitUs(150);				 //wait pllclock

	rf_set_power_level_index(power_level);
	WriteAnalogReg(0xa5,0x44);   // for carrier  mode
	write_reg8 (0x8004e8, 0x04); // for  carrier mode
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) CarrierData Test
*
*	@param		power_level: set power level(0~14)
*				rf_chn	   : set tx channel((0 ~ 100))
*
*
*	@return		None
*/

void Rf_EmiCarrierDataTest(int power_level,signed char rf_chn)
{

	Rf_EmiCarrierRecovery();

	rf_set_power_level_index(power_level);
	SetTxMode(rf_chn,0);		//0,TX MODE
	WaitUs(150);				//wait pllclock

	write_reg8(0x80050e,depth); // this size must small than the beacon_packet dma send length

	state0 = STATE0;
	state1 = STATE1;
	state2 = STATE2;
	state3 = STATE3;
	emi_tx[0] = depth*16-4;
	write_reg8(0x80050f, 0x80);  // must fix to 0x80
	write_reg8(0x800402, 0x21);	 //preamble length=1
	TxPkt(emi_tx);
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) CarrierData Test Data Update
*
*	@param		None
*
*
*	@return		None
*/

void Rf_EmiDataUpdate(void)
{
//	write_reg32((emi_tx+depth*16-4),(state0<<16)+state1); // the last value
//														  //	advance PN generator
//	state0 = pnGen(state0);
//	state1 = pnGen(state1);
//	write_reg32((emi_tx+depth*16-4),0xf0f0f0f0);          // the last value
//	phyTest_PRBS9((emi_tx+12),0x04);
	int i;
	for(i=0;i<64;i++)
	{
		PhyTest_PRBS9_Tx((emi_tx+depth*16-4),64,i);
	}


}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) Rx Test
*
*	@param		addr       :set receiving address pointer
*
*				buffer_size: set power level(0~14)
*				rf_chn	   : set tx channel((0~ 100))
*
*
*	@return		None
*/

void Rf_EmiRxTest(unsigned char *addr,signed char rf_chn,int buffer_size,unsigned char  pingpong_en)
{
	reg_dma_rf_rx_addr = (u16)(u32) (addr);
	reg_dma2_ctrl = FLD_DMA_WR_MEM | (buffer_size>>4);   // rf rx buffer enable & size£¬0x0108
//	Rf_RxBufferSet(addr,buffer_size,pingpong_en);
	SetRxMode(rf_chn,0);							   	 //0,RX mode
	WaitUs(200);										 //wait pllclock
	Rf_EmiCarrierRecovery();

//	Rf_BaseBandReset();
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) Tx Test initialization
*
*	@param		power_level: set power level(0~14)
*				rf_chn	   : set tx channel((0 ~ 100))
*
*
*	@return		None
*/

void Rf_EmiTxInit(int power_level,signed char rf_chn)
{
	rf_set_power_level_index(power_level);
	SetTxMode(rf_chn,0);
	WaitUs(200);					//wait pllclock
//	Rf_BaseBandReset();
	Rf_EmiCarrierRecovery();

}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) CarrierData Test
*
*	@param		addr       :set tx address pointer
*
*
*	@return		None
*/

void Rf_EmiSingleTx(unsigned char *addr,int power_level)
{
#if	PA_POWER_ON
	rf_set_power_level_index(power_level);
	WriteAnalogReg(0xa5,0x04);   				// for carrier  mode
	write_reg8 (0x8004e8, 0x04); 				// for  carrier mode//tx_cyc1
#endif
	TxPkt(addr);
	while((read_reg8(0x800f20) & 0x02)==0);
	write_reg8 (0x800f20, read_reg8(0x800f20)|0x02);
#if	PA_POWER_ON
//	WaitUs(100);
//	Rf_PowerLevelSet(RF_POWER_6dBm);
//	WaitUs(100);
//	Rf_PowerLevelSet(RF_POWER_5dBm);
//	WaitUs(100);
//	Rf_PowerLevelSet(RF_POWER_3P5dBm);
//	WaitUs(100);
//	Rf_PowerLevelSet(RF_POWER_1P65dBm);
//	WaitUs(100);
//	Rf_PowerLevelSet(RF_POWER_m0P6dBm);
//	//WaitUs(100);
//	Rf_PowerLevelSet(RF_POWER_m4P3dBm);
//	/Rf_PowerLevelSet(RF_POWER_m9P5dBm);
//	Rf_PowerLevelSet(RF_POWER_m13P6dBm);
//	Rf_PowerLevelSet(RF_POWER_m18P8dBm);
//	Rf_PowerLevelSet(RF_POWER_m23P3dBm);
//	Rf_PowerLevelSet(RF_POWER_m27P5dBm);
	rf_set_power_level_index(7);			     //30dbm
//	Rf_PowerLevelSet(RF_POWER_m37dBm);
//	Rf_PowerLevelSet(RF_POWER_OFF);
#endif
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) carrier mode
*
*	@param		None
*
*
*	@return		None
*/
void EmiCarrierOnly(int power_level,signed char rf_chn)
{
	Rf_EmiCarrierOnlyTest(power_level,rf_chn);
}

/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) carrierdata mode
*
*	@param		None
*
*
*	@return		None
*/
void EmiCarrierData(int power_level,signed char rf_chn)
{
	unsigned char run_t     = read_reg8(RUN_STATUE_ADDR);  // get the run state!
	unsigned char cmd_now_t = read_reg8(TEST_COMMAND_ADDR) ;
	unsigned char power_t   = read_reg8(POWER_ADDR);
	unsigned char chn_t     = read_reg8(CHANNEL_ADDR);
	unsigned char mode_t    = read_reg8(RF_MODE_ADDR);
	unsigned char tx_mode_temp_t = read_reg8(TX_PACKET_MODE_ADDR);

	Rf_EmiCarrierDataTest(power_level,rf_chn);
	while( ((read_reg8(RUN_STATUE_ADDR)) == run_t ) &&  ((read_reg8(TEST_COMMAND_ADDR)) == cmd_now_t )\
			&& ((read_reg8(POWER_ADDR)) == power_t ) &&  ((read_reg8(CHANNEL_ADDR)) == chn_t )\
			&& ((read_reg8(RF_MODE_ADDR)) == mode_t )&&  ((read_reg8(TX_PACKET_MODE_ADDR)) == tx_mode_temp_t )) //// if not new command
	{
		Rf_EmiDataUpdate();
	}
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) Tx prbs9 mode
*
*	@param		None
*
*
*	@return		None
*/
void EmiTxPrbs9(int power_level,signed char rf_chn)
{
	unsigned char run_t     = read_reg8(RUN_STATUE_ADDR);  // get the run state!
	unsigned char cmd_now_t = read_reg8(TEST_COMMAND_ADDR) ;
	unsigned char power_t   = read_reg8(POWER_ADDR);
	unsigned char chn_t     = read_reg8(CHANNEL_ADDR);
	unsigned char mode_t    = read_reg8(RF_MODE_ADDR);
	unsigned char tx_mode_temp_t = read_reg8(TX_PACKET_MODE_ADDR);
	unsigned int tx_cnt=0;
	Rf_EmiTxInit(power_level,rf_chn);
	while( ((read_reg8(RUN_STATUE_ADDR)) == run_t ) &&  ((read_reg8(TEST_COMMAND_ADDR)) == cmd_now_t )\
			&& ((read_reg8(POWER_ADDR)) == power_t ) &&  ((read_reg8(CHANNEL_ADDR)) == chn_t )\
			&& ((read_reg8(RF_MODE_ADDR)) == mode_t )&&  ((read_reg8(TX_PACKET_MODE_ADDR)) == tx_mode_temp_t )) //// if not new command
	{
		packet[4] = 0;//type
		PhyTest_PRBS9(packet + 6, 37);
		Rf_EmiSingleTx(packet,power_level);
		if(tx_mode==1)
		{
			tx_cnt++;
			if(tx_cnt>=1000)
			{
				break;
			}
		}
	}
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) Tx 55 mode
*
*	@param		None
*
*
*	@return		None
*/
void EmiTx55(int power_level,signed char rf_chn)
{
	unsigned char run_t     = read_reg8(RUN_STATUE_ADDR);  // get the run state!
	unsigned char cmd_now_t = read_reg8(TEST_COMMAND_ADDR) ;
	unsigned char power_t   = read_reg8(POWER_ADDR);
	unsigned char chn_t     = read_reg8(CHANNEL_ADDR);
	unsigned char mode_t    = read_reg8(RF_MODE_ADDR);
	unsigned char tx_mode_temp_t = read_reg8(TX_PACKET_MODE_ADDR);
	unsigned int tx_cnt=0;
	Rf_EmiTxInit(power_level,rf_chn);
	while( ((read_reg8(RUN_STATUE_ADDR)) == run_t ) &&  ((read_reg8(TEST_COMMAND_ADDR)) == cmd_now_t )\
			&& ((read_reg8(POWER_ADDR)) == power_t ) &&  ((read_reg8(CHANNEL_ADDR)) == chn_t )\
			&& ((read_reg8(RF_MODE_ADDR)) == mode_t )&&  ((read_reg8(TX_PACKET_MODE_ADDR)) == tx_mode_temp_t )) //// if not new command
	{
		int i;
		packet[4] = 2;//type
		for( i=0;i<37;i++)
		{
			packet[6+i]=0x55;
		}
		Rf_EmiSingleTx(packet,power_level);
		if(tx_mode==1)
		{
			tx_cnt++;
			if(tx_cnt>=1000)
			{
				break;
			}
		}
	}

}

/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) Tx ff mode
*
*	@param		None
*
*
*	@return		None
*/

void EmiTxff(int power_level,signed char rf_chn)
{
	unsigned char run_t     = read_reg8(RUN_STATUE_ADDR);  // get the run state!
	unsigned char cmd_now_t = read_reg8(TEST_COMMAND_ADDR) ;
	unsigned char power_t   = read_reg8(POWER_ADDR);
	unsigned char chn_t     = read_reg8(CHANNEL_ADDR);
	unsigned char mode_t    = read_reg8(RF_MODE_ADDR);
	unsigned char tx_mode_temp_t = read_reg8(TX_PACKET_MODE_ADDR);
	unsigned int tx_cnt=0;
	Rf_EmiTxInit(power_level,rf_chn);
	while( ((read_reg8(RUN_STATUE_ADDR)) == run_t ) &&  ((read_reg8(TEST_COMMAND_ADDR)) == cmd_now_t )\
			&& ((read_reg8(POWER_ADDR)) == power_t ) &&  ((read_reg8(CHANNEL_ADDR)) == chn_t )\
			&& ((read_reg8(RF_MODE_ADDR)) == mode_t )&&  ((read_reg8(TX_PACKET_MODE_ADDR)) == tx_mode_temp_t )) //// if not new command
	{
		int i;
		packet[4] = 1;//type
		for( i=0;i<37;i++)
		{
			packet[6+i]=0x0f;
		}
		Rf_EmiSingleTx(packet,power_level);
		if(tx_mode==1)
		{
			tx_cnt++;
			if(tx_cnt>=1000)
			{
				break;
			}
		}
	}
}
/********************************************************
*
*	@brief		Emi ( Electro Magnetic Interference ) Rx mode
*
*	@param		None
*
*
*	@return		None
*/

void EmiRx(int power_level,signed char rf_chn)
{
//	irq_enable();
	irq_set_mask( FLD_IRQ_ZB_RT_EN );
	reg_rf_irq_status = FLD_RF_IRQ_RX;
	reg_rf_irq_mask = FLD_RF_IRQ_RX;//rf irq en
	Rf_EmiRxTest(rx_packet,rf_chn,128,0);
}


// Test List
struct  test_list_s {
	unsigned char  cmd_id;
	void	 (*func)(int power_level,signed char rf_chn);
};
struct  test_list_s  ate_list[] = {
		{0x01,EmiCarrierOnly},        // osc test
		{0x02,EmiCarrierData},

		{0x03,EmiRx},
		{0x04,EmiTxPrbs9},
		{0x05,EmiTx55},
		{0x06,EmiTxff},
};


//unsigned char dug_irq_status = 0;
void EmiRxProc(void)
{
	Adebug_irq++;
	if(reg_rf_irq_status & FLD_RF_IRQ_RX)
	{
		reg_rf_irq_status = FLD_RF_IRQ_RX;//clr rf irq
		if((rx_packet[0] >= 15) && RF_PACKET_LENGTH_OK (rx_packet) && RF_PACKET_CRC_OK(rx_packet))
		{
			Adebug_Rx_irq++;
#if RSSI_FILTER
			rssi = Rf_rssi_filter(rx_packet)-110;//filter RSSI
#else
			if(Adebug_Rx_irq==1)
			{
				rssi = rx_packet[4]-110;
			}
			else
			{
				rssi = (rssi +rx_packet[4]-110)/2;

			}
#endif
			write_reg8(0x808004,rssi);
			write_reg32(0x80800c,Adebug_Rx_irq);
		}
		rx_packet[0]=1;
	}
}

/******************************************************************************
 * 									EMI
******************************************************************************/
void emi_test(void)
{
#if (MCU_CORE_TYPE == MCU_CORE_8266)
	analog_write(0x86, analog_read(0x86) & 0xfe);
#endif

//#if (CLOCK_SYS_CLOCK_HZ == 16000000)
//	write_reg8(0xb2,0x01);
//#endif

	static int first_flg = 0;
	unsigned char i;
	Rf_EmiInit();
	//usb
#if (MCU_CORE_TYPE == MCU_CORE_8267)
	WriteAnalogReg (0x88, 0x0f);
	WriteAnalogReg (0x05, 0x60);
	write_reg8(0x80013c,0x10);					 // print buffer size set
#endif
	write_reg8(RUN_STATUE_ADDR,run);			 //run
	write_reg8(TEST_COMMAND_ADDR,cmd_now);	     //cmd
	write_reg8(POWER_ADDR,power_level);			 //power
	write_reg8(CHANNEL_ADDR,chn);				 //chn
	write_reg8(RF_MODE_ADDR,mode);				 //mode
	write_reg8(TX_PACKET_MODE_ADDR,tx_mode);	 //tx_mode
	read_flash_para();
//	Rf_Init(OSC_SEL_FLAG,RF_MODE_BLE_1M);
	//accesscode: 1001-0100 1000-0010 0110-1110 1000-1110   29 41 76 71
	write_reg32 (0x800408, 0x29417671);
	PhyTest_PRBS9(ble_tx_packet + 6, 37);
	while(1)
	{
	   run = read_reg8(0x808006);  			  	  // get the run state!
	   if(!first_flg || run!=0)
	   {
			if(!first_flg){
				first_flg = 1;
			}
			else{
			   power_level = read_reg8(POWER_ADDR);
			   chn = read_reg8(CHANNEL_ADDR);
			   mode=read_reg8(RF_MODE_ADDR);
			   cmd_now = read_reg8(TEST_COMMAND_ADDR);  	   // get the command!
			   tx_mode	= read_reg8(TX_PACKET_MODE_ADDR);
			}
			irq_disable();
			Adebug_irq=0;
			Adebug_Rx_irq=0;
			write_reg32(0x80800c,0);
			write_reg8(0x808004,0);
			rssi=0;
//			if(mode==1)
//			{
//			    rf_drv_1m();
//			    write_reg32 (0x800408, 0x29417671);
//				packet = ble_tx_packet;
//			}
//			else if(mode==0)
//			{
//				rf_drv_2m();
//				write_reg32 (0x800408, 0x29417671);
//				packet = nordic_tx_packet;
//			}
//			if(cmd_now == 0x01)				//EmiCarrierOnly
//			{
//				EmiCarrierOnly(power_level,chn);
//			}
//			else if(cmd_now == 0x02)		//EmiCarrierData
//			{
//				EmiCarrierData(power_level,chn);
//			}
//			else if(cmd_now == 0x03)		//EmiRx
//			{
//				EmiRx(power_level,chn);
//			}
//			else if(cmd_now == 0x04)		//EmiTx_prbs9
//			{
//				EmiTxPrbs9(power_level,chn);
//			}
//			else if(cmd_now == 0x05)		//EmiTx_0x55
//			{
//				EmiTx55(power_level,chn);
//			}
//			else if(cmd_now == 0x06)		//EmiTx_0xff
//			{
//				EmiTxff(power_level,chn);
//			}
			for (i=0; i<sizeof (ate_list)/sizeof (struct test_list_s); i++)
			{
				if(cmd_now == ate_list[i].cmd_id)
				{
					if(mode==1)
					{
						rf_drv_1m();
						write_reg8 (0x800401, 0x01);
					//	write_reg8 (0x800402,  0x20|(preamble&0x1f));
						write_reg32 (0x800408, 0x29417671);	//accesscode: 1001-0100 1000-0010 0110-1110 1000-1110   29 41 76 71
						packet = ble_tx_packet;
					}
					else if(mode==0)
					{
						rf_drv_2m();
						write_reg8 (0x800401, 0x01);
						//write_reg8 (0x800402,  0x20|(preamble&0x1f));
						write_reg32 (0x800408, 0x29417671);	//accesscode: 1001-0100 1000-0010 0110-1110 1000-1110   29 41 76 71
						packet = ble_tx_packet;
					}
					ate_list[i].func(power_level,chn);
					break;

				}
			}

			run = 0;
			write_reg8(RUN_STATUE_ADDR, 0);
	   }
	   if(cmd_now == 0x03){
		   EmiRxProc();
	   }
	}
}



