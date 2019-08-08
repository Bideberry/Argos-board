/*
/ * ATM90E26.h
 *
 * Created: 10/10/2016 03:31:57 p.m.
 *  Author: César
 */ 


/*-----------------------------
PROTOTIPOS DE FUNCIONES
-------------------------------*/
void SPI_MasterInit(void);
unsigned char SPI_MasterTxRx(unsigned char);
unsigned int ReadReg90E26 (unsigned char);
void WriteReg90E26 (unsigned char,unsigned int);
void ATM90E26_Init(void);
void ATM90E26_GetData(void);
void ATM90E26_GetVandI (void);


void GrabaMedicion (void);
void InitPunteros (void);

/*Configuracion SPI*/
#define SPI_MASTER_BASE      SPI4
#define BOARD_FLEXCOM_SPI_4    FLEXCOM4
/* Chip select. */
#define SPI_CHIP_SEL 0
#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)
/* Clock polarity. */
#define SPI_CLK_POLARITY 0
/* Clock phase. */
#define SPI_CLK_PHASE 1
/* Delay before SPCK. */
#define SPI_DLYBS 0x40
/* Delay between consecutive transfers. */
#define SPI_DLYBCT 0x10
//Pines del SPI
/** SPI MISO pin definition. */
#define SPI4_MISO_GPIO         (PIO_PB9_IDX)
#define SPI4_MISO_FLAGS       (IOPORT_MODE_MUX_A)
/** SPI MOSI pin definition. */
#define SPI4_MOSI_GPIO         (PIO_PB8_IDX)
#define SPI4_MOSI_FLAGS       (IOPORT_MODE_MUX_A)
/** SPI SPCK pin definition. */
#define SPI4_SPCK_GPIO         (PIO_PB1_IDX)
#define SPI4_SPCK_FLAGS       (IOPORT_MODE_MUX_A)


//MISO PB9, MOSI PB8, SCK PB1, SS PB0

//para chip enable y disable PIO_PB0_IDX



/**
 * \brief Set peripheral mode for one single IOPORT pin.
 * It will configure port mode and disable pin mode (but enable peripheral).
 * \param pin IOPORT pin to configure
 * \param mode Mode masks to configure for the specified pin (\ref ioport_modes)
 */
#define ioport_set_pin_peripheral_mode(pin, mode) \
	do {\
		ioport_set_pin_mode(pin, mode);\
		ioport_disable_pin(pin);\
	} while (0)
