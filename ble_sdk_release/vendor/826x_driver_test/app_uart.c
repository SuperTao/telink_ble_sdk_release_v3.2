#include "../../proj/tl_common.h"
#include "../../proj_lib/rf_drv.h"
#include "../../proj_lib/pm.h"
#include "../../proj/drivers/adc.h"
#include "../../proj/drivers/uart.h"
#include "../../proj/drivers/i2c.h"
#include "../../proj/drivers/spi.h"

#if (DRIVER_TEST_MODE == TEST_UART)

#define UART_DMA_MODE_EN     1  //1:dma mode ; ; ; 0: not dma mode

#if UART_DMA_MODE_EN
	unsigned char uart_rx_irq = 0, uart_tx_irq = 0;
	#define UART_RX_BUFF_SIZE      16
	#define UART_TX_BUFF_SIZE      16

	__attribute__((aligned(4))) unsigned char uart_rec_buff[UART_RX_BUFF_SIZE] = {0x00,0x00,0x00,0x00,}; // the first four byte is length to receive data.
	__attribute__((aligned(4))) unsigned char uart_tx_buff[UART_TX_BUFF_SIZE]  = {0x0b,0x00,0x00,0x00,'t','e','l','i','n','k','-','s','e','m','i'}; // the first four byte is length to send data.
#else
	unsigned char uart_no_dma_rev_data = 0, uart_no_dma_rev_flag = 0;
#endif

void app_uart_test_init(void){

#if UART_DMA_MODE_EN
	//	uart_Init(9,13,PARITY_NONE,STOP_BIT_ONE); //set baud rate, parity bit and stop bit
	//	uart_DmaModeInit(UART_DMA_TX_IRQ_EN, UART_DMA_RX_IRQ_EN);   // enable tx and rx interrupt
	CLK16M_UART115200;

	uart_RecBuffInit(uart_rec_buff, UART_RX_BUFF_SIZE);  //set uart rev buffer and buffer size
	uart_txBuffInit(UART_TX_BUFF_SIZE);

	#if ((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))
		UART_GPIO_CFG_PC2_PC3();  //enable uart function and enable input
	#elif CHIP_TYPE == CHIP_TYPE_8266
		UART_GPIO_CFG_PC6_PC7();
	#endif

#else
	uart_Init(9, 13, PARITY_NONE, STOP_BIT_ONE); //set baud rate, parity bit and stop bit.(9,13,115200) other baud rate can get from tool
	uart_notDmaModeInit(1, 0, UART_NODMA_RX_IRQ_EN, UART_NODMA_TX_IRQ_DIS);

	#if ((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))
		UART_GPIO_CFG_PC2_PC3();  //enable uart function and enable input
	#elif CHIP_TYPE == CHIP_TYPE_8266
		UART_GPIO_CFG_PC6_PC7();
	#endif

#endif
	irq_enable();
}


void app_uart_test_start(void){

#if UART_DMA_MODE_EN
	if(uart_rx_irq){
		uart_rx_irq = 0;
		/*receive buffer,the first four bytes is the length information of received data.send the received data*/
		while(!uart_Send(uart_rec_buff));
		/*transmit buffer, the first four bytes is the length information of transmitting data.the DMA module will send the data based on the length.
		* so the useful data start from the fifth byte and start to send to other device from the fifth byte.*/
		while(!uart_Send(uart_tx_buff));
	}
#else
	if(uart_no_dma_rev_flag == 1){
		uart_no_dma_rev_flag = 0;
		uart_notDmaModeSendByte(uart_no_dma_rev_data);
	}
#endif

}


_attribute_ram_code_ void app_uart_test_irq_proc(void){
	static unsigned char irqS;
	static unsigned char idx = 0;
	#if UART_DMA_MODE_EN

		irqS = uart_IRQSourceGet(); // get the irq source and clear the irq.
		if(irqS & UARTRXIRQ){
			uart_rx_irq = 1;
		}
		if(irqS & UARTTXIRQ){
			uart_tx_irq++;
		}
	#else
		irqS = GET_UART_NOT_DMA_IRQ();  ///get the status of uart irq.
		if(irqS){
			uart_no_dma_rev_data = uart_notDmaModeRevData();//cycle the four registers 0x90 0x91 0x92 0x93,in addition reading will clear the irq.
			idx++;
			idx &= 0x03;// cycle the four registers 0x90 0x91 0x92 0x93, it must be done like this for the design of SOC.

			uart_no_dma_rev_flag = 1;
		}
	#endif
}

#endif
