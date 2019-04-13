
#include "../../proj/tl_common.h"

#if(__PROJECT_8266_OTA_BOOT__ || __PROJECT_8261_OTA_BOOT__)


#include "../../proj/mcu/watchdog_i.h"
#include "../../vendor/common/user_config.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"

#define  DBG_LED_IND 		0   //for led DEBUG ota boot bin running statas



#if(__PROJECT_8266_OTA_BOOT__)   //8266
	#ifndef			NEW_FW_SIZE
	#define			NEW_FW_SIZE			128    //128k
	#endif

	#ifndef			NEW_FW_ADR
	#define			NEW_FW_ADR			0x20000
	#endif

	#ifndef			OTA_FLG_ADR
	#define			OTA_FLG_ADR			0x73000
	#endif

	//now my remote control  PC2 high LED on, PC2 low LED off
	//see <<ble_gpio_lookuptable>> for GPIO register set
	//notice that: 8266 output setting: OEN = 0 means output enable
	//PC2 default is GPIO

	#define LED_INIT_OUTPUT		do{REG_ADDR8(0x591) &= ~BIT(2); REG_ADDR8(0x592) &= ~BIT(2); }while(0)

	#define LED_HIGH			( REG_ADDR8(0x593) |= BIT(2) )
	#define LED_LOW				( REG_ADDR8(0x593) &= ~BIT(2) )
	#define LED_TOGGLE			( REG_ADDR8(0x593) ^= BIT(2) )

#else // 8261
	#ifndef			NEW_FW_SIZE
	#define			NEW_FW_SIZE			40    //40k
	#endif

	#ifndef			NEW_FW_ADR
	#define			NEW_FW_ADR			0x10000
	#endif

	#ifndef			OTA_FLG_ADR
	#define			OTA_FLG_ADR			0x1B000
	#endif

	//now my remote control  PA4 high LED on, PA4 low LED off
	//see <<gpio_lookuptable>> for GPIO register set
	//notice that: 8261 output setting: OEN = 0 means output enable
	//PA4 default is GPIO

	#define LED_INIT_OUTPUT		do{REG_ADDR8(0x581) &= ~BIT(4); REG_ADDR8(0x582) &= ~BIT(4); }while(0)

	#define LED_HIGH			( REG_ADDR8(0x583) |= BIT(4) )
	#define LED_LOW				( REG_ADDR8(0x583) &= ~BIT(4) )
	#define LED_TOGGLE			( REG_ADDR8(0x583) ^= BIT(4) )

#endif

u8		buff[256];
u8		check_buff[256];


_attribute_ram_code_ void irq_handler(void)
{
}

_attribute_ram_code_ int main (void) {

	//open clk for MCU running
	REG_ADDR8(0x60) = 0x00;
	REG_ADDR8(0x61) = 0x00;
	REG_ADDR8(0x62) = 0x00;
	REG_ADDR8(0x63) = 0xff;
	REG_ADDR8(0x64) = 0xff;

	//enable system tick ( clock_time() )
	REG_ADDR8(0x74f) = 0x01;
	irq_disable ();


#if(DBG_LED_IND) //for debug : indicate that RF transforming OK, ota boot begin
	LED_INIT_OUTPUT;

	LED_HIGH;
	sleep_us(3000000);
	LED_LOW;
#endif


	flash_read_page (NEW_FW_ADR, 256, buff);
	int	n_firmware = *(u32 *)(buff + 0x18);

	if(n_firmware > (NEW_FW_SIZE<<10)){  //firmware too big, err
		#if(DBG_LED_IND)  //for debug : indicate that firmware size ERR
			LED_HIGH;
		#endif
		write_reg16(0x8000,0x55aa);  //for debug
		while(1);
	}



	for (int i=4096; i<n_firmware; i+=256)
	{
		if ((i & 0xfff) == 0)  //new sector begin Addr, need erase
		{
			flash_erase_sector (i);
		}

		flash_read_page (NEW_FW_ADR + i, 256, buff);  //read data  from 0x20000 ~ 0x30000
		flash_write_page (i, 256, buff);			  //write data  to  0x00000 ~ 0x10000
		flash_read_page (i, 256, check_buff);		  //read data to check if write OK
		for(int j=0;j<256;j++){
			if(buff[j] != check_buff[j]){  //write data not OK

				#if(DBG_LED_IND)  //for debug : indicate that flash write ERR happens
					LED_HIGH;
				#endif


				i &= 0xfff000; //back to sector begin adr, to rewrite
				i -= 256;
				break;
			}
		}

	}


	for (int i=0; i<4096; i+=256)   //first 4K
	{
		if ((i & 0xfff) == 0)  //new sector begin Addr, need erase
		{
			flash_erase_sector (i);
		}

		flash_read_page (NEW_FW_ADR + i, 256, buff);  //read data  from 0x20000 ~ 0x30000
		flash_write_page (i, 256, buff);			  //write data  to  0x00000 ~ 0x10000
		flash_read_page (i, 256, check_buff);		  //read data to check if write OK
		for(int j=0;j<256;j++){
			if(buff[j] != check_buff[j]){  //write data not OK

				#if(DBG_LED_IND)  //for debug : indicate that flash write ERR happens
					LED_HIGH;
				#endif


				i &= 0xfff000; //back to sector begin adr, to rewrite
				i -= 256;
				break;
			}
		}
	}

	buff[0] = 0;
	flash_write_page (OTA_FLG_ADR, 1, buff);	//clear OTA flag


#if 1
	for (int i = (n_firmware-1)&0x1f000; i>=0; i-=4096)  //erase data on flash for next OTA
	{
		flash_erase_sector (NEW_FW_ADR + i);
	}
#endif





#if(DBG_LED_IND)  //for debug : indicate that ota boot running OK
	for(int i=0; i< 4; i++){  //1Hz shine for 4 S
		LED_HIGH;
		sleep_us(500000);
		LED_LOW;
		sleep_us(500000);
	}
#endif


	REG_ADDR8(0x6f) = 0x20;   //mcu reboot
	while (1);
}


#endif


