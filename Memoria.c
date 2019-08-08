/*
/ * Memoria.c
 *
 * Created: 10/10/2016 03:31:57 p.m.
 *  Author: César
 */ 

#include <asf.h>
#include "ATM90E26.h"
#include "ioport.h"
#include "delay.h"
#include "Memoria.h"

extern unsigned char ucHora,ucMinuto,ucSegundo,ucDia,ucMes,ucYear;


void Mem_Busy(void)
    {
    unsigned char ucFlag;
    do
        {
        Mem_Enable();
        SPI_MasterTxRx(0x05); //Comando read status register
        ucFlag=SPI_MasterTxRx(0x20);
        Mem_Disable();
        delay_ms(1);
        }
    while(ucFlag & 0x01);

    }

unsigned char Mem_Read(unsigned long ulAddr)
    {
    unsigned char ucDato,ucAux;
    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x0B); //Comando para read
	ucAux=(ulAddr>>16)&0xFF;
    SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr>>8)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr)&0xFF;
	SPI_MasterTxRx(ucAux);
	SPI_MasterTxRx(0x55);
    ucDato=SPI_MasterTxRx(0x20); //Ese 0x20 es un dato dummy, se manda cualquier cosa para que la memoria devuelva
    Mem_Disable();
    return ucDato;
    }

void Mem_Program(unsigned long ulAddr, unsigned char ucDato)
    {
	unsigned char ucAux;
		
    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x06); //Comando write enable
    Mem_Disable();
    delay_ms(1);
    Mem_Enable();
    SPI_MasterTxRx(0x02); //Comando para byte-program
	ucAux=(ulAddr>>16)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr>>8)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr)&0xFF;
	SPI_MasterTxRx(ucAux);
    SPI_MasterTxRx(ucDato); //Mando el dato a escribir
    Mem_Disable();
    }


void Mem_Read_Paq(unsigned long ulAddr, unsigned char ucCant, unsigned char ucBuf[])
    {
    unsigned char j,ucAux;

    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x03); //Comando para read
	ucAux=(ulAddr>>16)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr>>8)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr)&0xFF;
	SPI_MasterTxRx(ucAux);

    for(j=0; j<ucCant; j++)
        ucBuf[j]=SPI_MasterTxRx(0x20); //Ese 0x20 es un dato dummy, se manda cualquier cosa para que la memoria devuelva

    Mem_Disable();
    }


void Mem_Program_Paq(unsigned long ulAddr, unsigned char ucCant, unsigned char ucBuf[])
    {
    unsigned char ucPos, ucDatosImpar=0,ucAux;

    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x06); //Comando write enable
    Mem_Disable();
    delay_ms(1);
    Mem_Enable();
    SPI_MasterTxRx(0xAD); //Comando para Auto addres Increment write

	ucAux=(ulAddr>>16)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr>>8)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr)&0xFF;
	SPI_MasterTxRx(ucAux);

    SPI_MasterTxRx(ucBuf[0]);
    SPI_MasterTxRx(ucBuf[1]);
    Mem_Disable();

    Mem_Busy();

    if(ucCant%2) //Para cantidad impar de datos, ese último valor impar le agrego un byte FF al vector, ya que siempre se escribe de a dos.
        {
        ucCant--;
        ucDatosImpar=1;
        }

    for(ucPos=2; ucPos<ucCant; ucPos++)
        {
        Mem_Enable();
        SPI_MasterTxRx(0xAD);
        SPI_MasterTxRx(ucBuf[ucPos]);
        ucPos++;
        SPI_MasterTxRx(ucBuf[ucPos]);
        Mem_Disable();
        Mem_Busy();
        }

    if(ucDatosImpar)
        {
        Mem_Enable();
        SPI_MasterTxRx(0xAD);
        SPI_MasterTxRx(ucBuf[ucPos]);
        SPI_MasterTxRx(0xFF);
        Mem_Disable();
        Mem_Busy();
        }

    Mem_Enable();
    SPI_MasterTxRx(0x04); //Comando write disable
    Mem_Disable();

    }

void Mem_Erase_Block(unsigned long ulAddr, unsigned char ucTam)
    {
    //ucTam: Tamaño del bloque puede valer 4,32,64. Define tamaño de bloque en KB a borrar
    unsigned char ucCommand,ucAux;

    Mem_Busy();
    ucCommand=0x03; //Por defecto, si tiene mal pasado el parámetro de tamaño, no borra nada
    if(ucTam==4) ucCommand=0x20; //Comando para bloque de 4KB
    if(ucTam==32) ucCommand=0x52; //Comando para bloque de 32KB
    if(ucTam==64) ucCommand=0xD8; //Comando para bloque de 64KB

    Mem_Enable();
    SPI_MasterTxRx(0x06); //Comando write enable
    Mem_Disable();
    delay_ms(1);
    Mem_Enable();
    SPI_MasterTxRx(ucCommand); //Comando para borrar sector

	ucAux=(ulAddr>>16)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr>>8)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr)&0xFF;
	SPI_MasterTxRx(ucAux);

    Mem_Disable();
    }

void Mem_Erase_Chip(void)
    {
    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x06); //Comando write enable
    Mem_Disable();
    delay_ms(1);
    Mem_Enable();
    SPI_MasterTxRx(0x60); //Comando para borrar chip
    Mem_Disable();
    }

void Mem_Unprotect_All(void)
    {
    //Escribe el registro de estados para dejar la memoria desprotegida
    //Por defecto, en el start-up, esta protegida
    Mem_Enable();
    SPI_MasterTxRx(0x50); //Comando enable write status register
    Mem_Disable();
    delay_ms(1);
    Mem_Enable();
    SPI_MasterTxRx(0x01); //Comando para escribir status register
    SPI_MasterTxRx(0x00); //Dejo toda la memoria utilizable para escribir
    Mem_Disable();
    }

unsigned char Mem_Read_Status (void)
    {
    unsigned char ucDato;
    Mem_Enable();
    SPI_MasterTxRx(0x05); //Comando write enable
    ucDato=SPI_MasterTxRx(0x20);
    Mem_Disable();
    return(ucDato);
    }


/***************************************************************************************************
Para programar muchos datos con Auto Address Increment (AAI) tengo que llamar a estas 3 funciones:
        Primero seteo la dirección y los primeros 2 datos.
        Luego mando de a 2 datos y automaticamente va a ir incrementando la dirección.
        Para terminar la operación tengo que mandar un comando de Fin de AAI.
****************************************************************************************************/
void Mem_Program_AAI_Ini(unsigned long ulAddr, unsigned char ucDato1, unsigned char ucDato2)
    {
	unsigned char ucAux;	
		
    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x06); //Comando write enable
    Mem_Disable();
    delay_ms(1);
    Mem_Enable();
    SPI_MasterTxRx(0xAD); //Comando para Auto addres Increment write

	ucAux=(ulAddr>>16)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr>>8)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr)&0xFF;
	SPI_MasterTxRx(ucAux);

    SPI_MasterTxRx(ucDato1);
    SPI_MasterTxRx(ucDato2);
    Mem_Disable();
    }

void Mem_Program_AAI(unsigned char ucDato1, unsigned char ucDato2)
    {
    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0xAD);
    SPI_MasterTxRx(ucDato1);
    SPI_MasterTxRx(ucDato2);
    Mem_Disable();
    }

void Mem_Program_AAI_Fin(void)
    {
    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x04); //Comando write disable
    Mem_Disable();
    }
/***************************************************************************************************/


/***************************************************************************************************
Para leer muchos datos con Auto Address Increment (AAI) tengo que llamar a estas 3 funciones:
        Primero seteo la dirección.
        Luego leo de a 1 byte y automaticamente va a ir incrementando la dirección.
        Para terminar la operación tengo que deshabilitar la memoria.
****************************************************************************************************/
void Mem_Read_AAI_Ini(unsigned long ulAddr)
    {
	unsigned char ucAux;	
		
    Mem_Busy();
    Mem_Enable();
    SPI_MasterTxRx(0x03); //Comando para read
	ucAux=(ulAddr>>16)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr>>8)&0xFF;
	SPI_MasterTxRx(ucAux);
	ucAux=(ulAddr)&0xFF;
	SPI_MasterTxRx(ucAux);
    }
unsigned char Mem_Read_AAI(void)
    {
    return(SPI_MasterTxRx(0x20)); //Ese 0x20 es un dato dummy
    }

void Mem_Read_AAI_Fin(void)
    {
    Mem_Disable();
    }


void  MemInit_Test (void)
	{
	unsigned char ucVec[3];	
		
	//Leer ID de memoria
	Mem_Enable();
	SPI_MasterTxRx(0x9F);
	//SPI_MasterTxRx(0x00);
	//SPI_MasterTxRx(0x00);
	//SPI_MasterTxRx(0x00);
	ucVec[0]=SPI_MasterTxRx(0x55);
	ucVec[1]=SPI_MasterTxRx(0x55);
	ucVec[2]=SPI_MasterTxRx(0x55);
	//while(ioport_get_pin_level(BUTTON_0_PIN));
	Mem_Disable();
	printf("La Memoria SPI devuelve %02X %02X %02X\r\n",	ucVec[0], ucVec[1],ucVec[2]);
	delay_ms(1);


	Mem_Enable();
	SPI_MasterTxRx(0x05);
	ucVec[0]=SPI_MasterTxRx(0x55);
	Mem_Disable();
	printf("2 La Memoria SPI devuelve %02X \r\n",	ucVec[0]);



	Mem_Busy();
	Mem_Enable();
	SPI_MasterTxRx(0x06); //Comando write enable
	Mem_Disable();
	delay_ms(10);
	Mem_Enable();
	SPI_MasterTxRx(0x02); //Comando para byte-program
	SPI_MasterTxRx(0x00);
	SPI_MasterTxRx(0x00);
	SPI_MasterTxRx(0x12);
	SPI_MasterTxRx(0xA0); //Mando el dato a escribir
	Mem_Disable();
	delay_ms(10);
	Mem_Enable();
	SPI_MasterTxRx(0x03);
	SPI_MasterTxRx(0x00);
	SPI_MasterTxRx(0x00);
	SPI_MasterTxRx(0x12);
	ucVec[0]=SPI_MasterTxRx(0x55); //Mando el dato a escribir
	Mem_Disable();

	delay_ms(10);
	Mem_Program(0x000010,0x55);
	delay_ms(10);
	ucVec[1]=Mem_Read(0x000010);


	printf("La Memoria SPI devuelve %02X %02X\r\n",	ucVec[0],ucVec[1]);
	
	}


/***************************************************************************************************/

/*RTC*/

/***************************************************************************************************/

void RTC_TWI_Init (void)
	{
	twi_options_t opt;

	ioport_set_pin_peripheral_mode(TWI3_SDA_GPIO, TWI3_SDA_FLAGS);
	ioport_set_pin_peripheral_mode(TWI3_SCL_GPIO, TWI3_SCL_FLAGS);


	flexcom_enable(BOARD_FLEXCOM_TWI);
	flexcom_set_opmode(BOARD_FLEXCOM_TWI, FLEXCOM_TWI);

	twi_enable_master_mode(BOARD_BASE_TWI_RTC);

	opt.master_clk = sysclk_get_peripheral_hz();
	opt.speed      = TWI_CLK;

	twi_master_init(BOARD_BASE_TWI_RTC, &opt);

	
	}
	
	
	
	

unsigned char RTC_Read(unsigned char Addr, unsigned char * ucBuf)
    {
    unsigned char ucRet=0;
	twi_packet_t packet_tx,packet_rx;
    
	/* Configure the data packet to be transmitted */
	packet_tx.chip        = RTC_RAM_ADDRESS;
	packet_tx.addr[0]     = Addr;
	packet_tx.addr_length = 1;
	packet_tx.buffer      = ucBuf;
	packet_tx.length      = 0;

	/* Configure the data packet to be received */
	packet_rx.chip        = RTC_RAM_ADDRESS|1;
	packet_rx.addr[0]     = packet_tx.addr[0];
	packet_rx.addr_length = packet_tx.addr_length;
	packet_rx.buffer      = ucBuf;
	packet_rx.length      = 1;

	ucRet=twi_master_read(BOARD_BASE_TWI_RTC, &packet_rx);
	
    return(ucRet);
    }

unsigned char RTC_Write(unsigned char Addr, unsigned char ucDato)
    {
    unsigned char ucRet;
    twi_packet_t packet_tx;
    
    /* Configure the data packet to be transmitted */
    packet_tx.chip        = RTC_RAM_ADDRESS;
    packet_tx.addr[0]     = Addr;
    packet_tx.addr_length = 1;
    packet_tx.buffer      = (uint8_t*)(&ucDato);
    packet_tx.length      = 1;

    ucRet=twi_master_write(BOARD_BASE_TWI_RTC, &packet_tx);
    
    delay_us(10);
    
    return(ucRet);
    }






void LeerHoraRTC(void)
    {
    unsigned char ucAux,ucVec[6];

    //RTC_Write(0,0x80); //Arrancar Oscilador borra el valor actual de los segundos
    //ucAux=0x80 |(RTC_Read(0));
    //RTC_Write(0,ucAux);

    //SEGUNDOS
    RTC_Read(0,&ucAux);
	if(!(ucAux&0x80)) RTC_Write(0,0x80|ucAux);
    ucAux=(((ucAux & 0x70) >> 4) * 10) + (ucAux & 0x0F);
    if (ucAux>59) ucVec[0]=0;
	else ucVec[0]=ucAux;
	
	

    //MINUTOS
    RTC_Read(1,&ucAux);
    ucAux=(((ucAux & 0x70) >> 4) * 10) + (ucAux & 0x0F);
    if (ucAux>59) ucVec[1]=0;
	else ucVec[1]=ucAux;

    //HORA
    RTC_Read(2,&ucAux);
	ucVec[2]=ucAux&0x0F;
	if(ucAux&0x10) ucVec[2]+=10;
	else
		{
		if(ucAux&0x20) ucVec[2]+=20;
		}
    if(ucVec[2]>23) ucVec[2]=0;

    //DIA
    RTC_Read(4,&ucAux);
    ucAux=10*((ucAux&0x30)>>4)+(ucAux&0x0F);
    if(!ucAux || ucAux>31) ucVec[3]=1;
	else ucVec[3]=ucAux;
	
    //MES
    RTC_Read(5,&ucAux);
	ucVec[4]=ucAux&0x0F;
	if(ucAux&0x10) ucVec[4]+=10;
    if(!ucAux || ucAux>12) ucVec[4]=1;
    //else ucMes=ucAux;

    //AÑO
    RTC_Read(6,&ucAux);
    ucAux=10*((ucAux&0xF0)>>4)+(ucAux&0x0F);
    if(ucAux>99) ucVec[5]=17;
    else ucVec[5]=ucAux;
	
	
	/*Analiza si actualizar hora o grabar la que esta en el micro*/
	if(ucVec[5]>=16)
		{
		ucSegundo=ucVec[0];
		ucMinuto=ucVec[1];
		ucHora=ucVec[2];
		ucDia=ucVec[3];
		ucMes=ucVec[4];
		ucYear=ucVec[5];
		}
	else
		{
		GrabarHoraRTC();
		}
	
    }


void GrabarHoraRTC(void)
{
	unsigned char ucAux;

	//RTC_Write(0,0x80); //Arrancar Oscilador borra el valor actual de los segundos
	//ucAux=0x80 |(RTC_Read(0));
	//RTC_Write(0,ucAux);

	//SEGUNDOS
	if(ucSegundo>59) ucSegundo=0;
	ucAux=((ucSegundo/10) << 4) + (ucSegundo%10);
	RTC_Write(0,ucAux|0x80);

	//MINUTOS
	if(ucMinuto>59) ucMinuto=0;
	ucAux=((ucMinuto/10) << 4) + (ucMinuto%10);
	RTC_Write(1,ucAux);

	//HORA
	if(ucHora>23) ucHora=0;
	ucAux=ucHora%10;
	if(ucHora>=10 && ucHora<20) ucAux|=0x10;
	if(ucHora>=20) ucAux|=0x20;
	RTC_Write(2,ucAux);

	//VBAT ENABLE
	RTC_Write(3,0x19);


	//DIA
	if(ucDia>31) ucDia=1;
	ucAux=((ucDia/10) << 4) + (ucDia%10);
	RTC_Write(4,ucAux);
	
	//MES
	if(ucMes>12) ucMes=1;
	ucAux=ucMes%10;
	if(ucMes>=10) ucMes|=0x10;
	//if( (ucYear&0x03) == 0x00) ucMes|=0x20; //Año bisiesto
	RTC_Write(5,ucAux);

	//AÑO
	if(ucYear>99) ucYear=17;
	ucAux=((ucYear/10) << 4) + (ucYear%10);
	RTC_Write(6,ucAux);
	
	
}
	