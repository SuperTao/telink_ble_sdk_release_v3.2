#ifndef MYPRINTF_H
#define MYPRINTF_H

#if (PRINT_DEBUG_INFO)

void my_array_printf(char*data, int len);
int mini_printf(const char *format, ...);
void PrintHex(u8 x);

#define printf			mini_printf
#define	printfArray		arrayPrint
//#define	logPrint		mini_printf
//#define	logPrintArray	arrayPrint
#define	arrayPrint(arrayAddr,len)					\
{													\
	mini_printf("\n*********************************\n");		\
	unsigned char	i = 0;							\
	do{												\
		mini_printf(" %x",((unsigned char *)arrayAddr)[i++]);						\
	}while(i<len);										\
	mini_printf("\n*********************************\n");		\
}
#else
#define printf
#define	printfArray
#define	PrintHex
#endif

//#define debugBuffer (*(volatile unsigned char (*)[40])(0x8095d8))
#endif
