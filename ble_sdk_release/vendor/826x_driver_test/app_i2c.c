#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/drivers/spi.h"

#if (DRIVER_TEST_MODE == TEST_IIC)

#define  I2C_MASTER_EN        1  //1:dma mode ; ; ; 0: not dma mode

#define  SLAVE_RAM_ADDR       0x8001
#define  SPI_DMA_WRITE_SIZE   11

#define  I2C_SLAVE_MAP_MODE   0
#define  I2C_SLAVE_DMA_MODE   1
#define  I2C_SLAVE_MODE_SEL   I2C_SLAVE_DMA_MODE

volatile unsigned char i2c_read_operation = 0, i2c_write_operation = 0;   // indicate i2c irq

__attribute__((aligned(128))) unsigned char mapping_buf[128]={0x00};

unsigned char i2c_dma_write_data[SPI_DMA_WRITE_SIZE]     = {0x01,0x12,0x34,0x45,0x56,0x67,0x78,0x89,0x90,0x0a,0xab,};
unsigned char i2c_mapping_write_data[SPI_DMA_WRITE_SIZE] = {0x10,0x21,0x43,0x54,0x65,0x76,0x87,0x98,0x09,0xa0,0xba,};
unsigned char i2c_read_buff[SPI_DMA_WRITE_SIZE] = {0x00};

void app_i2c_test_init(void){

#if MCU_CORE_TYPE == MCU_CORE_8266
	i2c_pin_init(I2C_GPIO_GROUP_E7F1);
#else
	i2c_pin_init(I2C_GPIO_GROUP_C0C1);
#endif

#if I2C_MASTER_EN
	i2c_master_init_div(0x5c>>1, 0x14);//para1:ID;para2:DivClock,i2c clock = system_clock/4*DivClock
#else
	#if(I2C_SLAVE_MODE_SEL == I2C_SLAVE_DMA_MODE)
		i2c_slave_init(0x5c>>1,I2C_SLAVE_DMA,NULL); // ID, slave mode,don't care
	#elif(I2C_SLAVE_MODE_SEL == I2C_SLAVE_MAP_MODE)
		//if you config slave like this, the first 64bytes of mapping_buf will get the data master write to slave.the second 64bytes of mapping_buf will be the address master read from slave in.
		i2c_slave_init(0x5c>>1,I2C_SLAVE_MAP,mapping_buf+64);  //if you want read the data master write to slave. slave can be config like this.
	#endif
		I2C_IRQ_EN();
		irq_enable();
#endif

}

void app_i2c_test_start(void){
#if I2C_MASTER_EN
	#if (I2C_SLAVE_MODE_SEL == I2C_SLAVE_DMA_MODE)
		i2c_dma_write_data[0] += 1;
		i2c_dma_write_data[0] &= 0xff;
		i2c_write_dma(SLAVE_RAM_ADDR, 2, i2c_dma_write_data, sizeof(i2c_dma_write_data));///0x0b
		WaitMs(1000);
		i2c_read_dma(SLAVE_RAM_ADDR, 2, i2c_read_buff, sizeof(i2c_read_buff));///0x0b
	#elif (I2C_SLAVE_MODE_SEL == I2C_SLAVE_MAP_MODE)
		i2c_mapping_write_data[0] += 1;
		i2c_mapping_write_data[0] &= 0xff;
		i2c_write_mapping(i2c_mapping_write_data, 0x0b);///0x0b sizeof(i2c_mapping_write_data)
		WaitMs(1000);
		i2c_read_mapping(i2c_read_buff,0x0b);///  sizeof(i2c_read_buff)
	#endif
#else
	WaitMs(100);
#endif
}


_attribute_ram_code_ void app_i2c_test_irq_proc(void){

	I2C_I2CIrqSrcTypeDef i2c_irq_flag = I2C_SlaveIrqGet();//i2c slave can distinguish the operation host write or read.
	I2C_SlaveIrqClr(i2c_irq_flag);
	if(i2c_irq_flag == I2C_IRQ_HOST_READ_ONLY){          /// host read
		i2c_read_operation++;
	}
	else if(i2c_irq_flag == I2C_IRQ_HOST_WRITE_ONLY){    ///host write
		i2c_write_operation++;
	}
	else{
	}
}


#endif
