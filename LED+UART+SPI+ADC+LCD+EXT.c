/****** SET LCD header VPBDIV=0x02 , dont use lcd timer*******/
/****** OR vpbdiv=0x01 uart dll=0x6E dlm=0x01 ********/

#include <LPC214x.h>
#include <string.h>
#include <stdlib.h>
#include "LCD7TDMI.h"
//void adc_int(void);

char str[20];
unsigned int datax,final_data;
char str_ext[10];
float volt;
int arr[4];


///////////////////////UART/////////////////////////////
void uart_init()
{
	PINSEL0|=(1<<16)|(1<<18);						//enable the txd and rxd
	U1LCR=0x83;													//8bit, no parity, 1 stop bit, dlab=1
	U1DLL=0x6E;
	U1DLM=0x01;		//Baud Rate  9600
	U1FDR=0xF1;													//MULVAL =15 DIVADDVAL=1
	U1LCR=0x03;													//8bit, no parity, 1 stop bit, dlab=0
}

void rx( char *p)
{
	char *q=p;
	while(*(q-1)!='\r')
	{
		while(!(U1LSR & 0x01));
		*q=U1RBR;
		q++;
	}
	*(q-1)='\0';
}

void tx( char *p)
{
	while((*p)!='\0')
	{
		while(!(U1LSR & 0x20));
		U1THR=*p;
		p++;
	}
}

void tx_char(char ch)
{
	while(!(U1LSR & 0x20));
	U1THR=ch;
}
void led_toggle()
{
	static int i=0;
	if(i==0)
	{
		i=1;
		IO0CLR|=(1<<31);
	}
	else
	{
		i=0;
		IO0SET|=(1<<31);
	}
}

//////////////////////////ADC/////////////////////////////////

void adc_init()
{
	PINSEL1|=(1<<28); 							//p0.30 as AD0.3
	AD0CR=0x00;
	AD0CR|=(1<<3)|(1<<16)|(1<<21);				//channel 3 select,burst conv,no power down 
	AD0CR|=(0x0000FF00);									//bit 8:15 clock div val

}



void adc_print()
{
		int i,j;
		while(!(AD0DR3&(1<<31)));
		datax=AD0DR3;
		datax=(datax>>6)&(0x3FF);
		volt=(datax*3.3)/1023;				//convertion to volts formula
		final_data=volt*1000;							//mV in integer
	
			for(i=0;i<4;i++)
		{
			arr[i]=(final_data%10)+48;
			final_data/=10;
		}
	
		for(i=3;i>=0;i--)
		{
			tx_char(arr[i]);
			if(i==3) tx(".");
		}
		for(i=0;i<300;i++)
			for(j=0;j<100;j++);
		tx("V");
		tx(" ");
		tx("\n");
		tx("\r");
}

///////////////////////SPI/////////////////////////////////
void spi_init()
{
	
		PINSEL0|=(1<<8)|(1<<12);									//setMODE  sck  and mosi
		IO0DIR|=(1<<7)|(1<<6)|(1<<4); 	//set sck mosi and ss(gpio) as output	
		S0SPCR=(1<<5)|(1<<6); //|(1<<8) | (1<<2);						//spi control reg 5=master mode, 7=spi int enable
																				//	,6=(1) msb first,11=8 BIT FORMAT,2=ENABLE
		S0SPCCR=0x0E;						//spi_data clk= pclk/sppr (greater than 8 and must be even)
	
		IO0CLR|=(1<<7);		//low
		S0SPDR=0x00;
		IO0SET|=(1<<7);		//high
	int j=S0SPSR;
}

void spi_send()
{
	static int i=0;
	int j;
	if(i==0)
	{
		i=1;
		IO0CLR|=(1<<7);		//low
		while(!(S0SPSR&(1<<7)));
		S0SPDR=0x55;			//data lock
		IO0SET|=(1<<7);		//high

	}
	else{
		i=0;
		IO0CLR|=(1<<7);		//low
		while(!(S0SPSR&(1<<7)));
		S0SPDR=0xAA;			//data lock
		IO0SET|=(1<<7);		//high
	
		
	}
		j=S0SPSR;
		j=S0SPDR;
}

///////////////////LCD/////////////////////////
void uart_to_lcd()
{
	int i=0;
	char lcd[]="\n\rEnter string to be displayed on LCD\n\r";
	send_data('A');
	tx(lcd);
	rx(str);
	send_cmd(0x01);
	send_cmd(0x80);
	while(str[i]!='\0')
	{	send_data(str[i++]);
		//user_delay(2000);
	}
}

//////////// External Int to LCD///////////////////

void External_int(void)__irq
{
	
	int j=0;
	send_cmd(0x01);
	send_cmd(0x80);
	while(str_ext[j]!='\0')
	{	send_data(str_ext[j++]);
		//user_delay(2000);
	}
	EXTINT |= (1<<2);
	VICVectAddr = 0x00;
}

void ext_init()
{
	PINSEL0|= (1<<31);
	EXTMODE|= (1<<2);
	EXTPOLAR|= (1<<2);
	EXTINT|= (1<<2);
	
	VICIntEnable = (1<<16);
	VICVectCntl0 = 0x20 | 0x10;
	VICVectAddr0 = (unsigned)External_int;
}

void extint_to_lcd()
{
	char lcd[]="\n\rEnter string to be displayed on LCD\n\r";
	send_data('A');
	tx(lcd);
	rx(str_ext);
	
}
//*********************************************************************************************//
int main()
{
	VPBDIV=0x02;											//div Plck by two i.e., 60Mhz/2=30MHz=Pclk
	IO0DIR|=(1<<31);
	IO0SET|=(1<<31);
	adc_init();
	spi_init();
	init_lcd();
	ext_init();
//	timer_init();
	char w[]="Home Automation\r\n";
	char str1[]="\n\r1.Led Toggle\n\r2.ADC value on Uart\n\r3.Send Data on SPI\n\r";
	char str2[]="4.Enter string to be displayed on LCD\n\r5.Display on LCD with Ext Interrupt\n\r";
	char error[]="\n\rInvalid. Enter correct choice\n\r";
	uart_init();
	tx(w);
	U1THR=0x0A;

	//tx(str1);
	while(1)
	{
		tx(str1);
		tx(str2);
		rx(str);
		
		switch(str[0]){
			case '1':
							led_toggle();
							break;
				
			case '2':
							adc_print();
							break;
				
			case '3':
							
							spi_send();
							break;
			case '4':
							uart_to_lcd();
							break;
			case '5':
				
							extint_to_lcd();
							break;
							
				
			default:
							
							tx(error);
							break;
		}
	}
}
