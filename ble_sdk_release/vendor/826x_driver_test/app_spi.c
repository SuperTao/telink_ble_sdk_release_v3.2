#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/drivers/spi.h"

#if (DRIVER_TEST_MODE == TEST_SPI)

#define SPI_MASTER_EN        1  //1:dma mode , 0: not dma mode

volatile unsigned char spi_interrupt_flag = 0;

#if SPI_MASTER_EN
	#define  SPI_CS_PIN         GPIO_PC0
	#define  elementNum(v)      (sizeof(v)/sizeof(v[0]))

	unsigned char spi_write_buff[10]= {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
	unsigned char spi_read_buff[10] = {0x00};

	#define  SLAVE_REG_ADD_H     0x80
	#define  SLAVE_REG_ADD_L     0x00
	#define  SPI_READ_CMD        0x80   // refer to the read format of spi in datasheet.
	#define  SPI_WRITE_CMD       0x00   // refer to the write format of spi in datasheet.
	unsigned char   slaveRegAddr_WriteCMD[] = {SLAVE_REG_ADD_H,SLAVE_REG_ADD_L,SPI_WRITE_CMD};
	unsigned char   slaveRegAddr_ReadCMD[]  = {SLAVE_REG_ADD_H,SLAVE_REG_ADD_L,SPI_READ_CMD};
#endif


void app_spi_test_init(void){

#if(MCU_CORE_TYPE == MCU_CORE_8266)
	#if SPI_MASTER_EN
		spi_master_init(0x0f,SPI_MODE0);
		spi_master_pin_init(SPI_CS_PIN);//  //GPIO_PE6  GPIO_PC0
	#else
		spi_slave_init(SPI_MODE0);     //SPI_MODE0
		SPI_IRQ_EN();
		irq_enable();
	#endif
#elif((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))
	#if SPI_MASTER_EN
		spi_master_init(0x0f,SPI_MODE0);  //SPI clock = System clock / ((div_clk+1)*2);
		spi_master_pin_init(SPI_PIN_GROUPA, SPI_CS_PIN);//  //GPIO_PA5  GPIO_PC0
	#else
		spi_slave_init(SPI_PIN_GROUPA, SPI_MODE0);     //SPI_MODE0
		SPI_IRQ_EN();
		irq_enable();
	#endif
#endif

}

void app_spi_test_start(void){
#if SPI_MASTER_EN
	spi_write_buff[0] += 1;
	spi_write_buff[0] &= 0xff;

	// pls refer to the datasheet for the write and read format of spi.
	spi_write(slaveRegAddr_WriteCMD,3,spi_write_buff,elementNum(spi_write_buff),SPI_CS_PIN);
	spi_read(slaveRegAddr_ReadCMD,3,spi_read_buff,elementNum(spi_read_buff),SPI_CS_PIN);

	WaitMs(1000);
#else
	WaitMs(2000);
#endif
}

_attribute_ram_code_ void app_spi_test_irq_proc(void){
	if(SPI_IRQ_GET()){
		SPI_IRQ_CLR(); //clear spi irq flag
		spi_interrupt_flag++; //only test. we can read data from buffer master write in.
	}
}


#endif
