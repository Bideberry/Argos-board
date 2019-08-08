/**
 *
 * \file
 *
 * \brief WINC1500 TCP Client Example.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */


#include "asf.h"
#include <string.h>
#include "main.h"
#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "ATM90E26.h"
#include "tc.h"
#include "Memoria.h"




#define STRING_EOL    "\r\n"
#define STRING_HEADER STRING_EOL"-- ARGOS --"STRING_EOL \
								"WIFI_INIT:"STRING_EOL	



/*****************************************/
//Variables y estructuras para modulo Wi-Fi
/*****************************************/
/** Message format definitions. */
typedef struct s_msg_wifi_product {
	uint8_t name[30];
} t_msg_wifi_product;
/** IP address of host. */
uint32_t gu32HostIp = 0;
/** Receive buffer definition. */
unsigned char gau8SocketTestBuffer[MAIN_WIFI_M2M_BUFFER_SIZE];
/** Socket for TCP communication */
static SOCKET tcp_server_socket = -1;
static SOCKET tcp_client_socket = -1;
/** Wi-Fi connection state */
static uint8_t wifi_connected;
//Buffer para usuario
t_msg_wifi_product bufferUser = {
	.name = "CCMMMMMMDDDD",
};
/* SPI clock setting (Hz). */
uint32_t gs_ul_spi_clock = 500000;
/* Configuracion de Wi-Fi manual */
unsigned char WLAN_CUSTOM_SSID[40];
unsigned char WLAN_CUSTOM_PWD[40];
unsigned int uiAuxMem; //para pruebas de lectura/escritura en bloque cero

/*****************************************/
//Variables usuario
/*****************************************/
unsigned int uiDatos=4096,uiAux1,uiAux2;
unsigned int uiTension=22000,uiCorriente=12345,uiPotencia,uiFrecuencia,uiCosPhi;
unsigned long ulAcumTension=0,ulAcumCorriente=0;
unsigned int uiFlag1,uiFlag2;
unsigned char ucFlagData;
unsigned char ucFlagConnected=1;
/*****************************************/
//Variables tiempo y memoria
/*****************************************/
unsigned int uiMiliSeg,uiSeg,uiMin,uiHora;
unsigned char ucSegundo=0,ucMinuto=30,ucHora=0,ucDia=1,ucMes=1,ucYear=16;
unsigned long ulDirMemoria,ulDirPuntero,ulPunteroFin;
unsigned long ulDirLectura;
unsigned char ucWLANreset = 0;


unsigned char ucSendData=0,ucStatus=0;
uint8_t mac_addr[10];
unsigned char ucCont=0;


/*****************************************/
//Estructuras perifericos
/*****************************************/
/* Use TC Peripheral 0. */
#define TC             TC0
#define TC_PERIPHERAL  0
/* Configure TC0 channel 2 as capture input. */
#define TC_CHANNEL_CAPTURE 2
#define ID_TC_CAPTURE ID_TC2
#define PIN_TC_CAPTURE PIN_TC0_TIOA2
#define PIN_TC_CAPTURE_MUX PIN_TC0_TIOA2_MUX
/* Use TC2_Handler for TC capture interrupt. */
#define TC_Handler  TC2_Handler
#define TC_IRQn     TC2_IRQn



/**
 * \brief Configure UART console.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate =		CONF_UART_BAUDRATE,
		.charlength =	CONF_UART_CHAR_LENGTH,
		.paritytype =	CONF_UART_PARITY,
		.stopbits =		CONF_UART_STOP_BITS,
	};

	/* Configure UART console. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}


/**
 * \brief Callback function of TCP client socket.
 * \FROM AP Provision Example
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg Type of Socket notification
 * \param[in] pvMsg A structure contains notification informations.
 *
 * \return None.
 */
static void socket_cb1(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	switch (u8Msg) {
	/* Socket bind */
	case SOCKET_MSG_BIND:
	{
		tstrSocketBindMsg *pstrBind = (tstrSocketBindMsg *)pvMsg;
		if (pstrBind && pstrBind->status == 0) {
			printf("Trying to listen...\r\n");
			listen(tcp_server_socket, 0);
		} else {
			printf("socket_cb: bind error!\r\n");
			close(tcp_server_socket);
			tcp_server_socket = -1;
		}
	}
	break;

	/* Socket listen */
	case SOCKET_MSG_LISTEN:
	{
		tstrSocketListenMsg *pstrListen = (tstrSocketListenMsg *)pvMsg;
		if (pstrListen && pstrListen->status == 0) {
			printf("socket_cb: Ready to listen.\r\n");
			printf("Trying to accept new connection...\r\n");
			accept(tcp_server_socket, NULL, NULL);
		} else {
			printf("socket_cb: listen error!\r\n");
			close(tcp_server_socket);
			tcp_server_socket = -1;
		}
	}
	break;

	/* Connect accept */
	case SOCKET_MSG_ACCEPT:
	{
		tstrSocketAcceptMsg *pstrAccept = (tstrSocketAcceptMsg *)pvMsg;
		if (pstrAccept) {
			accept(tcp_server_socket, NULL, NULL);
			tcp_client_socket = pstrAccept->sock;
			printf("socket_cb: Client socket is created.\r\n");
			recv(tcp_client_socket, gau8SocketTestBuffer, sizeof(gau8SocketTestBuffer), 0);
		} else {
			printf("socket_cb: accept error!\r\n");
			close(tcp_server_socket);
			tcp_server_socket = -1;
		}
	}
	break;

	/* Message receive */
	case SOCKET_MSG_RECV:
	{
		tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;

		if (pstrRecv && pstrRecv->s16BufferSize > 0) {
			char *p;

			p = strtok((char *)pstrRecv->pu8Buffer, ",");
			
			if (p != NULL && !strncmp(p, "apply", 5)) {
				char str_ssid[M2M_MAX_SSID_LEN], str_pw[M2M_MAX_PSK_LEN];
				uint8 sec_type = 0;

				p = strtok(NULL, ",");
				if (p) {
					strcpy(str_ssid, p);
				}

				p = strtok(NULL, ",");
				if (p) {
					sec_type = atoi(p);
				}

				p = strtok(NULL, ",");
				if (p) {
					strcpy(str_pw, p);
				}

				printf("Disable to AP.\r\n");
				m2m_wifi_disable_ap();
				nm_bsp_sleep(500);
				printf("Connecting to %s.\r\n", (char *)str_ssid);
				m2m_wifi_connect((char *)str_ssid, strlen((char *)str_ssid), sec_type, str_pw, M2M_WIFI_CH_ALL);
				/*Agregado prueba escritura memoria*/
				Mem_Program_Paq(DIR_SSID_INIT, 40, (unsigned char *)str_ssid);
				Mem_Program_Paq(DIR_PWD_INIT, 40, (unsigned char *)str_pw);
				printf("Memoria grabada\r\n");
				break;
			}
		} else {
			printf("socket_cb: recv error!\r\n");
			close(tcp_server_socket);
			tcp_server_socket = -1;
		}

		memset(gau8SocketTestBuffer, 0, sizeof(gau8SocketTestBuffer));
		recv(tcp_client_socket, gau8SocketTestBuffer, sizeof(gau8SocketTestBuffer), 0);
	}
	break;

	default:
		break;
	}
}


/**
 * \brief Callback to get the Data from socket.
 * \FROM TCP Client Example
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
static void socket_cb2(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	unsigned char i;
	switch (u8Msg) {
	/* Socket connected */
	case SOCKET_MSG_CONNECT:
		{
		tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
		if (pstrConnect && pstrConnect->s8Error >= 0) 
			{
			printf("socket_cb: connect success!\r\n");
			bufferUser.name[0]=0x00;
			bufferUser.name[1]=0x01;
			for(i=0;i<6;i++) bufferUser.name[i+2]=mac_addr[i];
			bufferUser.name[8]=0x00;
			bufferUser.name[9]=0x00;
			uiDatos=(ulDirMemoria-DIR_DATA_INIT)>>3;
			bufferUser.name[10]=(uiDatos>>8)&0xFF;
			bufferUser.name[11]=uiDatos&0xFF;
			ucStatus=0;
			send(tcp_client_socket, &bufferUser, 12, 0);
			} 
		else 
			{
			printf("socket_cb: connect error!\r\n");
			close(tcp_client_socket);
			tcp_client_socket = -1;
			}
		}
	break;

	/* Message send */
	case SOCKET_MSG_SEND:
		{
		printf("socket_cb: send success!\r\n");
		if(ucStatus==0) 
			{
			ucStatus=1;
			//Acá ya seteo las direcciones de memoria para ir leyendo
			ulDirLectura=DIR_DATA_INIT;
			ucFlagConnected=1;
			printf("socket_send: Mando ID, va a recibir dato\r\n");
			//Voy a recv para recibir la hora y empezar a mandar datos			
			recv(tcp_client_socket, gau8SocketTestBuffer, 8, 0);
			}
		else
			{
			//Para los envíos subsiguientes, después de mandar voy directo a recibir
			recv(tcp_client_socket, gau8SocketTestBuffer, 8, 0);
			}
		ucSendData=1;
		}
	break;

	/* Message receive */
	case SOCKET_MSG_RECV:
		{
		tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
		if (pstrRecv && pstrRecv->s16BufferSize > 0) 
			{
			printf("socket_cb: recv success!\r\n");
			printf("socket_rcv: %d %d %d %d %d %d %d %d\r\n",gau8SocketTestBuffer[0],
				gau8SocketTestBuffer[1],gau8SocketTestBuffer[2],gau8SocketTestBuffer[3],
				gau8SocketTestBuffer[4],gau8SocketTestBuffer[5],gau8SocketTestBuffer[6],
				gau8SocketTestBuffer[7]);

			if(ucStatus==1)
				{
				
				if(gau8SocketTestBuffer[0]==1 && gau8SocketTestBuffer[1]==64)
					{
					//Tengo que responder con 64 bytes de data
					
					Mem_Read_AAI_Ini(ulDirLectura);
					for(uiAux1=0;uiAux1<64;uiAux1++)
						{
						gau8SocketTestBuffer[uiAux1]=Mem_Read_AAI();
						ulDirLectura++;
						if(ulDirLectura>=ulDirMemoria)
							{
							for(uiAux2=uiAux1+1;uiAux2<64;uiAux2++) gau8SocketTestBuffer[uiAux2]=0;
							uiAux1=64;
							}
						}		
					Mem_Read_AAI_Fin();			


					printf("Envio paquete de 8 registros \r\n");
					for(uiAux1=0;uiAux1<8;uiAux1++) printf("socket_send: %d %d %d %d %d %d %d %d\r\n",
					gau8SocketTestBuffer[8*uiAux1+0],gau8SocketTestBuffer[8*uiAux1+1],gau8SocketTestBuffer[8*uiAux1+2],
					gau8SocketTestBuffer[8*uiAux1+3],gau8SocketTestBuffer[8*uiAux1+4],gau8SocketTestBuffer[8*uiAux1+5],
					gau8SocketTestBuffer[8*uiAux1+6],gau8SocketTestBuffer[8*uiAux1+7]);
					
					send(tcp_client_socket, &gau8SocketTestBuffer, 64, 0);
					}
				else
					{
					if(gau8SocketTestBuffer[0]==2 && gau8SocketTestBuffer[1]==0)
						{
						//Aca recibe la fecha y la hora, debe guardarlos en el rtc
						
						ucSegundo=gau8SocketTestBuffer[2];
						ucMinuto=gau8SocketTestBuffer[3];
						ucHora=gau8SocketTestBuffer[4];
						ucDia=gau8SocketTestBuffer[5];
						ucMes=gau8SocketTestBuffer[6];
						ucYear=gau8SocketTestBuffer[7];
						GrabarHoraRTC();
						
						//Reinicio de la memoria
						
						//Mem_Erase_Chip(); //Modificado para borrar todo excepto el primer bloque
						ulDirPuntero=DIR_PUNTEROS_INIT;
						while (ulDirPuntero<DIR_MEM_FIN)
						{
							Mem_Erase_Block(ulDirPuntero, 64);
							delay_ms(100);
							printf("Bloque %x borrado\r\n", ulDirPuntero);
							ulDirPuntero += 0x010000;
						}
						
						delay_ms(100);
						Mem_Unprotect_All();
						delay_ms(1);
						ulDirPuntero=DIR_PUNTEROS_INIT;
						ulDirMemoria=DIR_DATA_INIT;
						ulPunteroFin=DIR_MEM_FIN;
						ulDirLectura=DIR_DATA_INIT;

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
						
						ucStatus=5;
						//Fin de envio de datos, cerrar conexion	
						close(tcp_client_socket);
						tcp_client_socket = -1;
						ucFlagConnected=0;
						}
					else
						{
						if(gau8SocketTestBuffer[0]==11 && gau8SocketTestBuffer[1]==164)
							{
							//Modo de comunicacion donde manda tension y corriente actual 
							gau8SocketTestBuffer[0]=gau8SocketTestBuffer[4]=uiTension>>8;
							gau8SocketTestBuffer[1]=gau8SocketTestBuffer[5]=uiTension&0xFF;
							gau8SocketTestBuffer[2]=gau8SocketTestBuffer[6]=uiCorriente>>8;
							gau8SocketTestBuffer[3]=gau8SocketTestBuffer[7]=uiCorriente&0xFF;
							
							//Como no lee memoria, permite que se escriban nuevos datos
							ucFlagConnected=0;
							//Envia a la PC
							send(tcp_client_socket, &gau8SocketTestBuffer, 8, 0);
							}
						else
							{
							if(gau8SocketTestBuffer[0]==12 && gau8SocketTestBuffer[1]==0)
								{
								ucStatus=5;
								//Fin de envio de datos, cerrar conexion
								close(tcp_client_socket);
								tcp_client_socket = -1;
								ucFlagConnected=0;
								}
							else
								{
								gau8SocketTestBuffer[0]=64;
								send(tcp_client_socket, &gau8SocketTestBuffer, 1, 0);
								}
							}
						}
					}
				}
			}
		else 
			{
			printf("socket_cb: recv error!\r\n");
			close(tcp_client_socket);
			ucFlagConnected=0;
			tcp_client_socket = -1;
			}
		}
	break;

	default:
		break;
	}
}

/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
 *  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
 *  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
 *  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
 *  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
 *  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type. Existing types are:
 *  - tstrM2mWifiStateChanged
 *  - tstrM2MWPSInfo
 *  - tstrM2MP2pResp
 *  - tstrM2MAPResp
 *  - tstrM2mScanDone
 *  - tstrM2mWifiscanResult
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType) {
	case M2M_WIFI_RESP_CON_STATE_CHANGED:
	{
		tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
		if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
			printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\r\n");
			m2m_wifi_request_dhcp_client();
		} else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
			printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED\r\n");
			wifi_connected = 0;
			m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
		}

		break;
	}

	case M2M_WIFI_REQ_DHCP_CONF:
	{
		uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
		wifi_connected = 1;
		printf("wifi_cb: M2M_WIFI_REQ_DHCP_CONF: IP is %u.%u.%u.%u\r\n",
				pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
		break;
	}

	default:
	{
		break;
	}
	}
}

void SysTick_Handler(void)
	{
	uiMiliSeg++;

	if(uiMiliSeg==1000)
		{
		ioport_toggle_pin_level(LED0_GPIO);
		if(ioport_get_pin_level(GPIO_PUSH_BUTTON_1) == 0)
		{
			ucWLANreset ++;
			if (ucWLANreset>=3)
			{
				Mem_Erase_Block(DIR_SSID_INIT, 64);
				printf("Memoria borrada!\r\n");
				ucWLANreset=0;
			}
			
		} else ucWLANreset=0;
		
		uiMiliSeg=0;
		uiSeg++;
		//Acá cada un segundo mide y acumula para promediar al minuto
		if(!ucFlagConnected && uiSeg!=60) ATM90E26_GetVandI();
		ulAcumTension+=uiTension;
		ulAcumCorriente+=uiCorriente;
		if(uiSeg==60)
			{
			uiSeg=0;
			uiMin++;
			if(uiMin==60) uiMin=0;

			//Acá cada un min promedia y guarda en memoria
			uiTension=ulAcumTension/59;
			uiCorriente=ulAcumCorriente/59;
			if(!ucFlagConnected) LeerHoraRTC();
			if(!ucFlagConnected) GrabaMedicion();
			ulAcumTension=0;
			ulAcumCorriente=0;
			}
			
		//Hora del reloj	
		ucSegundo++;
		if(ucSegundo==60)	
			{
			ucSegundo=0;
			ucMinuto++;
			if(ucMinuto==60)
				{
				ucMinuto=0;
				ucHora++;
				if(ucHora==24) ucHora=0;
				}
			}
		}

	}





/**
 * \brief Main application function.
 *
 * Initialize system, UART console, network then test function of TCP client.
 *
 * \return program return value.
 */
int main(void)
{
	tstrWifiInitParam param;
	int8_t ret;
	struct sockaddr_in addr;
	tstrM2MAPConfig strM2MAPConfig;
	
	/* Initialize the board. */
	sysclk_init();
	board_init();

	/* Initialize the UART console. */
	configure_console();
	printf(STRING_HEADER);

	/* Initialize the BSP. */
	nm_bsp_init();

    if (SysTick_Config(sysclk_get_cpu_hz() / 1000)) {
	    printf("main: systick config error!!! \r\n");
	    while (1);
    }
	
	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

	/* Initialize Wi-Fi driver with data and status callbacks. */
	param.pfAppWifiCb = wifi_cb;
	ret = m2m_wifi_init(&param);
	if (M2M_SUCCESS != ret){
		printf("main: m2m_wifi_init call error!(%d)\r\n", ret);
		while (1) {
		}
	}
	printf("main: m2m_wifi_init OK!(%d)\r\n", ret);

	/* Get MAC Address. */
	m2m_wifi_get_mac_address(mac_addr);

	printf("MAC ADDRESS: ");
	printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n",
	mac_addr[0], mac_addr[1], mac_addr[2],
	mac_addr[3], mac_addr[4], mac_addr[5]);
		
	/*Incializar perifericos*/
	printf("main: MEDIDOR INIT...\r\n");
	SPI_MasterInit();
	ATM90E26_Init();
	printf("main: MEDIDOR INIT OK\r\n");
	delay_ms(1);
	printf("main: MEMORIA INIT...\r\n");
	Mem_Unprotect_All();
	delay_ms(1);
	InitPunteros();
	//printf("main: MEMORIA INIT OK dirmemoria: 0x%6X dirpuntero: 0x%6X\r\n",(unsigned int)ulDirMemoria,(unsigned int)ulDirPuntero);
	printf("main: MEMORIA INIT OK\r\n");

	printf("main: RELOJ INIT...\r\n");
	RTC_TWI_Init();
	delay_ms(10);
	RTC_Read(0,&ucCont);
	RTC_Write(0,0x80|ucCont);
	//VBAT ENABLE
	RTC_Write(3,0x19);
	printf("main: RELOJ INIT OK\r\n");

	/*Testeo del RTC*/
	LeerHoraRTC();
	printf("Fecha actual: ");
	printf("%02d:%02d:%02d %02d/%02d/%02d\r\n",ucHora,ucMinuto,ucSegundo,ucDia,ucMes,ucYear);

	ucFlagConnected=0;
	delay_ms(2000);
	printf("main_read: Tension: %d.%02d V    Corriente: %d.%03d A \r\n",
	uiTension/100,uiTension%100,uiCorriente/1000,uiCorriente%1000);
	
	ioport_init();
	/*Configuro boton de placa para reset*/
	ioport_set_pin_dir(GPIO_PUSH_BUTTON_1, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(GPIO_PUSH_BUTTON_1, IOPORT_MODE_PULLUP);


	/* Inicio Argos v2 */
	
	//Leo el primer byte, me alcanza para saber si hay algo escrito
	uiAuxMem = Mem_Read(DIR_SSID_INIT);
	//printf("Leido %d de la memoria\r\n", uiAuxMem);
	
	if (uiAuxMem == 255) {
		/*Inicio modo AP*/
		
		/* Initialize socket address structure. */
		addr.sin_family = AF_INET;
		addr.sin_port = _htons((AP_WIFI_M2M_SERVER_PORT));
		addr.sin_addr.s_addr = 0;
		
		/* Initialize Socket module */
		socketInit();
		registerSocketCallback(socket_cb1, NULL);

		/* Initialize AP mode parameters structure with SSID, channel and OPEN security type. */
		memset(&strM2MAPConfig, 0x00, sizeof(tstrM2MAPConfig));
		strcpy((char *)&strM2MAPConfig.au8SSID, AP_WLAN_SSID);
		strM2MAPConfig.u8ListenChannel = AP_WLAN_CHANNEL;
		strM2MAPConfig.u8SecType = AP_WLAN_AUTH;
		strM2MAPConfig.au8DHCPServerIP[0] = 0xC0; /* 192 */
		strM2MAPConfig.au8DHCPServerIP[1] = 0xA8; /* 168 */
		strM2MAPConfig.au8DHCPServerIP[2] = 0x01; /* 1 */
		strM2MAPConfig.au8DHCPServerIP[3] = 0x01; /* 1 */ 

		/* Bring up AP mode with parameters structure. */
		ret = m2m_wifi_enable_ap(&strM2MAPConfig);
		printf("ret: %d\r\n", ret);
		if (M2M_SUCCESS != ret) {
			printf("main: m2m_wifi_enable_ap call error!\r\n");
			while (1) {
			}
		}

		printf("AP Provision mode started.\r\nOn the PC, connect to %s \r\n", AP_WLAN_SSID);

		while (1) {
			m2m_wifi_handle_events(NULL);
	
			if (tcp_server_socket < 0) {
				/* Open TCP server socket */
				if ((tcp_server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					printf("main: failed to create TCP server socket error!\r\n");
					continue;
				}				
					
			/* Bind service*/
			bind(tcp_server_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
			}
		}		
	} /*Fin modo AP*/
	
	else {
		/*Inicio programa argos v1*/
		Mem_Read_Paq(DIR_SSID_INIT, 40, WLAN_CUSTOM_SSID);
		Mem_Read_Paq(DIR_PWD_INIT, 40, WLAN_CUSTOM_PWD);
		printf("Leido %s de la memoria\r\n", WLAN_CUSTOM_SSID);
		printf("Leido %s de la memoria\r\n", WLAN_CUSTOM_PWD);
		
		/* ARGOS V1 */
		
		/* Initialize socket address structure. */
		addr.sin_family = AF_INET;
		addr.sin_port = _htons(MAIN_WIFI_M2M_SERVER_PORT);
		addr.sin_addr.s_addr = _htonl(MAIN_WIFI_M2M_SERVER_IP);

		/* Initialize socket module */
		socketInit();
		registerSocketCallback(socket_cb2, NULL);

		printf("main: socket OK!(%d)\r\n", ret);
		
		/* Connect to router. */
		//m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
		m2m_wifi_connect((char *)WLAN_CUSTOM_SSID, strlen((char *)WLAN_CUSTOM_SSID), MAIN_WLAN_AUTH, (char *)WLAN_CUSTOM_PWD, M2M_WIFI_CH_ALL);

		printf("main: router connect(%d)\r\n", ret);	

		while (1) {
			/* Handle pending events from network controller. */
			m2m_wifi_handle_events(NULL);

			if (wifi_connected == M2M_WIFI_CONNECTED) {
				/* Open client socket. */
				if (tcp_client_socket < 0) {
					if ((tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						printf("main: failed to create TCP client socket error!\r\n");
						continue;
					}
				
					/* Connect server */
					ret = connect(tcp_client_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

					printf("main_read: Tension: %d.%02d V    Corriente: %d.%03d A \r\n",
							uiTension/100,uiTension%100,uiCorriente/1000,uiCorriente%1000);
					printf("Fecha actual: ");
					printf("%02d:%02d:%02d %02d/%02d/%02d\r\n",ucHora,ucMinuto,ucSegundo,ucDia,ucMes,ucYear);
					delay_ms(1000);

					/* Handle pending events from network controller. */
					m2m_wifi_handle_events(NULL);
					if (ret < 0) {
						printf("Volvio al main con ret<0\r\n");
						close(tcp_client_socket);
						tcp_client_socket = -1;
					}
				}
			}
			else {
				//if(ucStatus==5 && wifi_connected == M2M_WIFI_DISCONNECTED) m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
			}
		}		
	} /*Fin programa argos v1*/
	

	return 0;
} 








