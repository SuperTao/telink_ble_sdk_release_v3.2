/******************************************************************
 *Filename:    myprintf.c
 *Author:	 junjun.xu@telink-semi.com
 *Created Date:	2015/11/13
 *Description:	This is a minimal printf() implementation that doesn't depend on any other libraries.
 *				Mainly designed for Telink BLT stack debug usages. It supports %x and %d format string
 *				specifiers only. One can easily extend mini_printf.c for additional format strings
 */
#include <stdarg.h>
#include "../../proj/tl_common.h"  
#include "myprintf.h"
#if(PRINT_DEBUG_INFO)

#define			DECIMAL_OUTPUT		10
#define			OCTAL_OUTPUT		8
#define			HEX_OUTPUT			16


#define va_start(ap,v)    (ap = (char *)((int)&v + sizeof(v)))
#define va_arg(ap,t)      ((t *)(ap += sizeof(t)))[-1]

#ifndef		BIT_INTERVAL
#define		BIT_INTERVAL	(CLOCK_SYS_CLOCK_HZ/PRINT_BAUD_RATE)
#endif

_attribute_ram_code_ static void uart_put_char(u8 byte){
	u8 j = 0;
	u32 t1 = 0,t2 = 0;
	
	REG_ADDR8(0x582+((DEBUG_INFO_TX_PIN>>8)<<3)) &= ~(DEBUG_INFO_TX_PIN & 0xff) ;//Enable output


	u32 pcTxReg = (0x583+((DEBUG_INFO_TX_PIN>>8)<<3));//register GPIO output
	u8 tmp_bit0 = read_reg8(pcTxReg) & (~(DEBUG_INFO_TX_PIN & 0xff));
	u8 tmp_bit1 = read_reg8(pcTxReg) | (DEBUG_INFO_TX_PIN & 0xff);


	u8 bit[10] = {0};
	bit[0] = tmp_bit0;
	bit[1] = (byte & 0x01)? tmp_bit1 : tmp_bit0;
	bit[2] = ((byte>>1) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[3] = ((byte>>2) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[4] = ((byte>>3) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[5] = ((byte>>4) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[6] = ((byte>>5) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[7] = ((byte>>6) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[8] = ((byte>>7) & 0x01)? tmp_bit1 : tmp_bit0;
	bit[9] = tmp_bit1;

	//u8 r = irq_disable();
	t1 = read_reg32(0x740);
	for(j = 0;j<10;j++)
	{
		t2 = t1;
		while(t1 - t2 < BIT_INTERVAL){
			t1  = read_reg32(0x740);
		}
		write_reg8(pcTxReg,bit[j]);        //send bit0
	}
	//irq_restore(r);
}

static int puts(char *s){
	while((*s != '\0')){
		uart_put_char(*s++);
	}
}

static void puti(unsigned int num, int base){
	char re[]="0123456789ABCDEF";

	char buf[50];

	char *addr = &buf[49];

	*addr = '\0';

	do{
		*--addr = re[num%base];
		num/=base;
	}while(num!=0);

	puts(addr);
}


int mini_printf(const char *format, ...){

	char span;
	unsigned long j;
	char *s;
	//char *msg;
	va_list arg_ptr;
	va_start(arg_ptr, format);

	while((span = *(format++))){
		if(span != '%'){
			uart_put_char(span);
		}else{
			span = *(format++);
			if(span == 'c'){
				j = va_arg(arg_ptr,int);//get value of char
				uart_put_char(j);
			}else if(span == 'd'){
				j = va_arg(arg_ptr,int);//get value of char
				if(j<0){
					uart_put_char('-');
					j = -j;
				}
				puti(j,DECIMAL_OUTPUT);
			}else if(span == 's'){
				s = va_arg(arg_ptr,char *);//get string value
				puts(s);
			}else if(span == 'o'){
				j = va_arg(arg_ptr,unsigned int);//get octal value
				puti(j,OCTAL_OUTPUT);
			}else if(span == 'x'){
					j = va_arg(arg_ptr,unsigned int);//get hex value
					puti(j,HEX_OUTPUT);
			}else if(span == 0){
				break;
			}else{
				uart_put_char(span);
			}
		}

	}
	va_end(arg_ptr);
}

u8 HexTable[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
void PrintHex(u8 x)
{
	uart_put_char('0');
	uart_put_char('x');
	uart_put_char(HexTable[x>>4]);
	uart_put_char(HexTable[x&0xf]);
	uart_put_char(' ');
}

#endif
