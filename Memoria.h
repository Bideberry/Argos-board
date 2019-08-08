/*
/ * Memoria.h
 *
 * Created: 10/10/2016 03:31:57 p.m.
 *  Author: César
 */ 

//Slave select memoria
#define Mem_Enable() ioport_set_pin_level(IOPORT_CREATE_PIN(PIOB, 0),0);
#define Mem_Disable() ioport_set_pin_level(IOPORT_CREATE_PIN(PIOB, 0),1);

#define DIR_SSID_INIT 0x000000
#define DIR_PWD_INIT 0x000100
#define DIR_DATA_INIT 0x020000
#define DIR_DATA_FIN 0x1FFFF0
#define DIR_PUNTEROS_INIT 0x010000
#define DIR_PUNTEROS_FIN 0x01FFF0
#define DIR_MEM_FIN 0x1FFFF0


//Prototipos funciones memoria
void Mem_Busy(void);
unsigned char Mem_Read(unsigned long);
void Mem_Program(unsigned long, unsigned char);
void Mem_Read_Paq(unsigned long, unsigned char, unsigned char[]);
void Mem_Program_Paq(unsigned long, unsigned char, unsigned char[]);
void Mem_Erase_Block(unsigned long, unsigned char);
void Mem_Erase_Chip(void);
void Mem_Unprotect_All(void);
unsigned char Mem_Read_Status (void);
void Mem_Program_AAI_Ini(unsigned long, unsigned char, unsigned char);
void Mem_Program_AAI(unsigned char, unsigned char);
void Mem_Program_AAI_Fin(void);
void Mem_Read_AAI_Ini(unsigned long);
unsigned char Mem_Read_AAI(void);
void Mem_Read_AAI_Fin(void);

void  MemInit_Test(void);


/*******************************************************/
/* USO DEL TWI (I2C) PARA EL RTC */
/*******************************************************/

/** EEPROM Wait Time */
#define WAIT_TIME   10
/** TWI Bus Clock 400kHz */
#define TWI_CLK     100000
/** Address of AT24C chips */
#define RTC_RAM_ADDRESS         0x6F
#define EEPROM_MEM_ADDR         0
#define EEPROM_MEM_ADDR_LENGTH  2
/** TWI ID for simulated EEPROM application to use */
#define BOARD_ID_TWI_RTC         ID_TWI3
/** TWI Base for simulated TWI EEPROM application to use */
#define BOARD_BASE_TWI_RTC       TWI3

/** Definition of TWI interrupt ID on board. */
#define BOARD_TWI_IRQn          TWI3_IRQn
#define BOARD_TWI_Handler    TWI3_Handler
/** Configure TWI4 pins */
#define CONF_BOARD_TWI4
/** Flexcom application to use */
#define BOARD_FLEXCOM_TWI          FLEXCOM3
/** SDA pin definition */
#define TWI3_SDA_GPIO         (PIO_PA3_IDX)
#define TWI3_SDA_FLAGS       (IOPORT_MODE_MUX_A)
/** SCL pin definition */
#define TWI3_SCL_GPIO         (PIO_PA4_IDX)
#define TWI3_SCL_FLAGS       (IOPORT_MODE_MUX_A)


void RTC_TWI_Init (void);
unsigned char RTC_Read(unsigned char, unsigned char *);
unsigned char RTC_Write(unsigned char, unsigned char);
void LeerHoraRTC(void);
void GrabarHoraRTC(void);


