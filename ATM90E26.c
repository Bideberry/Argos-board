/**
 * Medidor de consumo eléctrico
 *
 *	Sensor de medicion - ATM90E26
 *
 */


/*-----------------------------
LIBRERIAS
-------------------------------*/
#include <asf.h>
#include "ATM90E26.h"
#include "Memoria.h"

/*-----------------------------
DEFINES
-------------------------------*/
#define cs_enable() ioport_set_pin_level(IOPORT_CREATE_PIN(PIOA, 20),0);
#define cs_disable() ioport_set_pin_level(IOPORT_CREATE_PIN(PIOA, 20),1);

/*-----------------------------
VARIABLES GLOBALES
-------------------------------*/
extern unsigned int uiTension,uiCorriente,uiPotencia,uiFrecuencia,uiCosPhi;
extern unsigned int uiFlag1,uiFlag2;
extern unsigned char ucFlagData;
extern unsigned char ucMinuto,ucHora,ucDia,ucMes,ucYear;
extern unsigned long ulDirMemoria,ulDirPuntero,ulPunteroFin;
extern uint32_t gs_ul_spi_clock;



/*-----------------------------
IMPLEMENTACIÓN DE FUNCIONES
-------------------------------*/
void SPI_MasterInit(void)
	{
	//Configurar pines
	ioport_set_pin_dir(IOPORT_CREATE_PIN(PIOB, 0), IOPORT_DIR_OUTPUT); //ATM90E26
	ioport_set_pin_dir(IOPORT_CREATE_PIN(PIOA, 20), IOPORT_DIR_OUTPUT); //ATM90E26
	ioport_set_pin_peripheral_mode(SPI4_MISO_GPIO, SPI4_MISO_FLAGS);
	ioport_set_pin_peripheral_mode(SPI4_MOSI_GPIO, SPI4_MOSI_FLAGS);
	ioport_set_pin_peripheral_mode(SPI4_SPCK_GPIO, SPI4_SPCK_FLAGS);

	cs_disable();
	Mem_Disable();

	/* Enable the peripheral and set SPI mode. */
	flexcom_enable(BOARD_FLEXCOM_SPI_4);
	flexcom_set_opmode(BOARD_FLEXCOM_SPI_4, FLEXCOM_SPI);
	//spi_enable_clock(SPI_MASTER_BASE);
	spi_disable(SPI_MASTER_BASE);
	spi_reset(SPI_MASTER_BASE);
	spi_set_lastxfer(SPI_MASTER_BASE);
	spi_set_master_mode(SPI_MASTER_BASE);
	spi_disable_mode_fault_detect(SPI_MASTER_BASE);
	spi_set_clock_polarity(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_POLARITY);
	spi_set_clock_phase(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_PHASE);
	spi_set_bits_per_transfer(SPI_MASTER_BASE, SPI_CHIP_SEL,SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL,sysclk_get_peripheral_hz()/gs_ul_spi_clock);
	spi_set_transfer_delay(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_DLYBS,SPI_DLYBCT);
	spi_enable(SPI_MASTER_BASE);

	}

unsigned char SPI_MasterTxRx(unsigned char cData)
	{

	// Start transmission 
	SPI_MASTER_BASE->SPI_TDR=cData;
	// Wait for transmission complete 
	while (!( (SPI_MASTER_BASE->SPI_SR) & SPI_SR_RDRF ) );
	// Lectura del registro 
	cData = SPI_MASTER_BASE->SPI_RDR;
	
	return cData;
	}




unsigned int ReadReg90E26 (unsigned char address)
	{
	unsigned int data;
	unsigned char aux;

	//Encender bit7 de address
	address|=0x80;
	
	cs_enable();
	SPI_MasterTxRx(address);
	aux=SPI_MasterTxRx(0x55);
	data=aux;
	data<<=8;
	aux=SPI_MasterTxRx(0x55);
	data|=aux;
	cs_disable();
		
	return data;	
	}
	
void WriteReg90E26 (unsigned char address,unsigned int data)
	{
	unsigned int data2;
	unsigned char aux;

	//Apagar bit7 de address
	address&=0x7F;
	
	cs_enable();
	SPI_MasterTxRx(address);
	data2=data>>8;
	aux=data2;
	SPI_MasterTxRx(aux);
	data&=0x00FF;
	aux=data;
	SPI_MasterTxRx(aux);
	cs_disable();
	
	}
	
	
	
void ATM90E26_Init (void)	
	{
	WriteReg90E26(0x30,0x5678);
	WriteReg90E26(0x20,0x5678);
	WriteReg90E26(0x21,0x0FF0);
	WriteReg90E26(0x22,0xC648);
	WriteReg90E26(0x32,0x2158);
	WriteReg90E26(0x31,0x390F);
	WriteReg90E26(0x33,0x2558);
	WriteReg90E26(0x28,0xFA15);
	/* ... */
	}

/*
ATM90E26_GetData
	- Carga datos de V, I y P en variables globales.
	- Verfica si los datos son correctos.
	
*/
void ATM90E26_GetData (void)
	{
	uiFlag1=ReadReg90E26(0x01);
	if(uiFlag1!=0xFFFF)
		{
		uiTension=ReadReg90E26(0x49);
		uiCorriente=ReadReg90E26(0x48);
		if(uiTension==0 && uiCorriente==0) ATM90E26_Init();
		//Redondeo de datos
		if(uiCorriente<=29) uiCorriente=0;
		else uiCorriente-=29;
		if(uiTension<100) uiTension=0;
		}
	else ATM90E26_Init();
		
		
	ucFlagData++;
	}


void ATM90E26_GetVandI (void)
	{
	uiFlag1=ReadReg90E26(0x20);
	if(uiFlag1!=0xFFFF)
		{
		uiTension=ReadReg90E26(0x49);
		uiCorriente=ReadReg90E26(0x48);
		if(uiTension==0 && uiCorriente==0) ATM90E26_Init();
		//Redondeo de datos: los 29mA se sacan xq es lo que consumen los capacitores de proteccion
		if(uiCorriente<=29) uiCorriente=0;
		else uiCorriente-=29;
		if(uiTension<100) uiTension=0;
		}
	else ATM90E26_Init();
	}


/*************************************************************
GrabarMedicion
	- Escribe un conjunto de datos en la memoria
	- Usa 8 bytes por entrada: uiTension (2B), uiCorriente(2B), ucMinuto(1B)
							   ucHora(1B), ucDia(5B), ucMes(4B), ucYear(7B)
	- Usa el puntero ulDirMemoria para guardar datos.
	- A su vez, en ulDirPuntero indica donde se almacena ulDirMemoria en el bloque de punteros.
	
Mapa memoria
	Bloque 64KB - 0x000000 a 0x00FFFF reservado
	Bloque 64KB	- 0x010000 a 0x01FFFF para punteros
	Resto de la memoria - 0x020000 a 0x1FFFFF para guardar mediciones
	
*************************************************************/
void GrabaMedicion (void)
	{
	unsigned char ucDato1,ucDato2;
	
	/***Escritura en memoria***/
	
	//Tension
	ucDato1=uiTension>>8;
	ucDato2=uiTension&0xFF;
	Mem_Program_AAI_Ini(ulDirMemoria,ucDato1,ucDato2);
	//Corriente
	ucDato1=uiCorriente>>8;
	ucDato2=uiCorriente&0xFF;
	Mem_Program_AAI(ucDato1,ucDato2);
	//Minuto y hora
	Mem_Program_AAI(ucMinuto,ucHora);
	//Dia, mes y año
	ucDato1=ucDia<<3;
	ucDato1|=(ucMes>>1);
	ucDato2=ucYear;
	if(ucMes&0x01) ucDato2|=0x80;
	Mem_Program_AAI(ucDato1,ucDato2);
	Mem_Program_AAI_Fin();	
	
	
	/***Manejo de punteros***/
	ulDirMemoria+=8;


	//Borrado de bloque de 4KB para memoria llena
	if(ulDirMemoria+8>=ulPunteroFin)
		{
			
		//Borrar bloque de 4KB indicado por ulPunteroFin
		Mem_Erase_Block(ulPunteroFin,4);
		ulPunteroFin+=4096;
		
		}

	
	if(ulDirPuntero>=0x001FFFB) //Máximo de punteros alcanzado
		{
		Mem_Erase_Block(0x010000,64);
		ulDirPuntero=0x010000;
		}
	Mem_Program(ulDirPuntero,(ulDirMemoria>>16)&0xFF);
	ulDirPuntero++;
	Mem_Program(ulDirPuntero,(ulDirMemoria>>8)&0xFF);
	ulDirPuntero++;
	Mem_Program(ulDirPuntero,(ulDirMemoria)&0xFF);
	ulDirPuntero++;
	Mem_Program(ulDirPuntero,(ulPunteroFin>>16)&0xFF);
	ulDirPuntero++;
	Mem_Program(ulDirPuntero,(ulPunteroFin>>8)&0xFF);
	ulDirPuntero++;
	Mem_Program(ulDirPuntero,(ulPunteroFin)&0xFF);
	ulDirPuntero++;
	
	}
	
	
void InitPunteros (void)
	{
	unsigned long ulAux;
	unsigned char ucDato;
	
    //printf("punteros: busca punteros en memoria\r\n");

	//Si la primera posicion es 0xff, memoria virgen	
	ulAux=DIR_PUNTEROS_INIT;
	ucDato=Mem_Read(ulAux);

	if(ucDato!=0xFF)
		{
		//La memoria no está virgen
		ulAux=DIR_PUNTEROS_INIT;
		do 
			{
			ucDato=Mem_Read(ulAux);
			ulAux+=6;
			delay_us(10);
			} 
		while(ucDato!=0xFF && ulAux<DIR_PUNTEROS_FIN);
		delay_ms(1);
		//printf("punteros1: dirmemoria: 0x%6X dirpuntero: 0x%6X\r\n",ulDirMemoria,ulAux);

		//Con lo que leemos en ulDirPuntero se carga ulDirMemoria
		if(ulAux<DIR_DATA_INIT && ulAux>DIR_PUNTEROS_INIT+6)
			{
			//printf("punteros: Hay informacion previa \r\n");
			ulAux-=12;		
			ulDirPuntero=ulAux;	
			ucDato=Mem_Read(ulDirPuntero);
			ulDirMemoria=ucDato;
			ulDirMemoria<<=8;
			ucDato=Mem_Read(ulDirPuntero+1);
			ulDirMemoria|=ucDato;
			ulDirMemoria<<=8;
			ucDato=Mem_Read(ulDirPuntero+2);
			ulDirMemoria|=ucDato;
			ucDato=Mem_Read(ulDirPuntero+3);
			ulPunteroFin=ucDato;
			ulPunteroFin<<=8;
			ucDato=Mem_Read(ulDirPuntero+4);
			ulPunteroFin|=ucDato;
			ulPunteroFin<<=8;
			ucDato=Mem_Read(ulDirPuntero+5);
			ulDirPuntero+=6;
			ulPunteroFin|=ucDato;
			}
		else
			{
			//printf("punteros: Memoria sin datos 1\r\n");
			//Memoria Vacía
			ulDirPuntero=DIR_PUNTEROS_INIT;
			ulDirMemoria=DIR_DATA_INIT;
			ulPunteroFin=DIR_MEM_FIN;
			}
		//printf("punteros2: dirmemoria: 0x%6X dirpuntero: 0x%6X\r\n",ulDirMemoria,ulAux);
		}
	else
		{
		printf("punteros: Memoria sin datos 2\r\n");
		//Memoria virgen
		ulDirPuntero=DIR_PUNTEROS_INIT;
		ulDirMemoria=DIR_DATA_INIT;
		ulPunteroFin=DIR_MEM_FIN;
		Mem_Program(ulDirPuntero,(ulDirMemoria>>16)&0xFF);
		ulDirPuntero++;
		Mem_Program(ulDirPuntero,(ulDirMemoria>>8)&0xFF);
		ulDirPuntero++;
		Mem_Program(ulDirPuntero,(ulDirMemoria)&0xFF);
		ulDirPuntero++;
		Mem_Program(ulDirPuntero,(ulPunteroFin>>16)&0xFF);
		ulDirPuntero++;
		Mem_Program(ulDirPuntero,(ulPunteroFin>>8)&0xFF);
		ulDirPuntero++;
		Mem_Program(ulDirPuntero,(ulPunteroFin)&0xFF);
		ulDirPuntero++;
		}
	}