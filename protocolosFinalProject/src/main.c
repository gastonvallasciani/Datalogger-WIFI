/*
 * Author: Gaston Vallasciani
 * Date: 2018-06-07
 */

/*==================[inclusions]=============================================*/

//#include "rtc.h"   // <= own header (optional)
#include "sapi.h"    // <= sAPI header
#include "ff.h"       // <= Biblioteca FAT FS
#include <string.h>
#include "ESP8266_ESP01.h"


/*==================[macros and definitions]=================================*/
#define FILENAME "newFile.txt"
#define CONNECTOR ", "
#define SAMPLES 20
/*==================[internal data declaration]==============================*/
static FATFS fs;           // <-- FatFs work area needed for each volume
static FIL fp;             // <-- File object needed for each open file

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/
extern int64_t sensorValue;
extern char tcpIpDataToSend[100];
/* Buffer */
static char uartBuff[10];
static char fileBuff[50];

typedef struct{
	uint16_t adcChannel1;
	uint16_t adcChannel2;
	uint16_t adcChannel3;
}adcChannel_t;

adcChannel_t dataLogger;


/*==================[internal functions definition]==========================*/

/*==================[external functions definition]==========================*/
void adcStructConfig(void);
void formatSdBuffer(rtc_t * rtc,adcChannel_t *adcChannel);
void recordStringInSdCard(char *str, uint32_t strLen);
// FUNCION que se ejecuta cada vezque ocurre un Tick
void diskTickHook( void *ptr );

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.

 */
char* itoa(int value, char* result, int base) {
   // check that the base if valid
   if (base < 2 || base > 36) { *result = '\0'; return result; }

   char* ptr = result, *ptr1 = result, tmp_char;
   int tmp_value;

   do {
      tmp_value = value;
      value /= base;
      *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
   } while ( value );

   // Apply negative sign
   if (tmp_value < 0) *ptr++ = '-';
   *ptr-- = '\0';
   while(ptr1 < ptr) {
      tmp_char = *ptr;
      *ptr--= *ptr1;
      *ptr1++ = tmp_char;
   }
   return result;
}

/* Enviar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
void showDateAndTime( rtc_t * rtc ){
   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->mday), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el dia */
   if( (rtc->mday)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, '/' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->month), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el mes */
   if( (rtc->month)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, '/' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->year), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio el año */
   if( (rtc->year)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );


   uartWriteString( UART_USB, ", ");


   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->hour), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio la hora */
   if( (rtc->hour)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, ':' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->min), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio los minutos */
  // uartBuff[2] = 0;    /* NULL */
   if( (rtc->min)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );
   uartWriteByte( UART_USB, ':' );

   /* Conversion de entero a ascii con base decimal */
   itoa( (int) (rtc->sec), (char*)uartBuff, 10 ); /* 10 significa decimal */
   /* Envio los segundos */
   if( (rtc->sec)<10 )
      uartWriteByte( UART_USB, '0' );
   uartWriteString( UART_USB, uartBuff );


   /* Envio un 'enter' */
   uartWriteString( UART_USB, "\r\n");
}


/* FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE RESET. */
int main(void){

   /* ------------- INICIALIZACIONES ------------- */

   /* Inicializar la placa */
   boardConfig();

   // Inicializar el conteo de Ticks con resolucion de 10ms,
   // con tickHook diskTickHook
   tickConfig( 10 );
   tickCallbackSet( diskTickHook, NULL );

   /* Inicializar UART_USB a 115200 baudios */
   uartConfig( UART_USB, 115200 );
   uartConfig( UART_232, 9600 );

   // ADC init
   adcInit(ADC_ENABLE);
   // ADC struct init in 0
   adcStructConfig();

   // SPI configuration
   spiConfig( SPI0 );

   /* Estructura RTC */
     rtc_t rtc;

     rtc.year = 2018;
     rtc.month = 7;
     rtc.mday = 13;
     rtc.wday = 5;
     rtc.hour = 9;
     rtc.min = 18;
     rtc.sec= 0;

     bool_t val = 0;
     uint8_t i = 0;

     /* Inicializar RTC */
     val = rtcConfig( &rtc );

     delay_t delay1s;
     delayConfig( &delay1s, 15000 );

     delay(2000); // El RTC tarda en setear la hora, por eso el delay

     rtc.year = 2018;
     rtc.month = 7;
     rtc.mday = 13;
     rtc.wday = 5;
     rtc.hour = 9;
     rtc.min = 18;
     rtc.sec= 0;

     /* Establecer fecha y hora */
     val = rtcWrite( &rtc );

     // Give a work area to the default drive
     if( f_mount( &fs, "", 0 ) != FR_OK ){
       // If this fails, it means that the function could
       // not register a file system object.
       // Check whether the SD card is correctly connected
     }


     if( !esp01Init( UART_ESP01, UART_DEBUG, UARTS_BAUD_RATE ) ){
        stopProgramError(); // Como dio falso (error) me quedo en un bucle infinito
     }

     if( !esp01ConnectToWifiAP( WIFI_SSID, WIFI_PASSWORD ) ){
        stopProgramError(); // Como dio falso (error) me quedo en un bucle infinito
     }

     gpioToggle( LED1);
   /* ------------- REPETIR POR SIEMPRE ------------- */
   while(1){

	if( delayRead( &delay1s ) ){

		/* Leer fecha y hora */
		gpioToggle( LED1);

		val = rtcRead( &rtc );
		dataLogger.adcChannel1 = adcRead(CH1);
		dataLogger.adcChannel2 = adcRead(CH2);
		dataLogger.adcChannel3 = adcRead(CH3);
		/* Mostrar fecha y hora en formato "DD/MM/YYYY, HH:MM:SS" */
		formatSdBuffer(&rtc,&dataLogger);
		uartWriteString( UART_USB, fileBuff );

		recordStringInSdCard(fileBuff, strlen(fileBuff));

		// Armo el dato a enviar, en este caso para grabar un dato en el canal de Thinspeak
		// Ejemplo: "GET /update?api_key=7E7IOJ276BSDLOBA&field1=66"

		sensorValue = dataLogger.adcChannel1;

		tcpIpDataToSend[0] = 0; // Reseteo la cadena que guarda las otras agregando un caracter NULL al principio

		strcat( tcpIpDataToSend, "GET /update?api_key=" );     // Agrego la peticion de escritura de datos

		strcat( tcpIpDataToSend, THINGSPEAK_WRITE_API_KEY );   // Agrego la clave de escritura del canal

		strcat( tcpIpDataToSend, "&field" );                   // Agrego field del canal
		strcat( tcpIpDataToSend, intToString(THINGSPEAK_FIELD_NUMBER) );

		strcat( tcpIpDataToSend, "=" );                        // Agrego el valor a enviar
		strcat( tcpIpDataToSend, intToString( sensorValue ) );

		// Envio los datos TCP/IP al Servidor de Thingpeak
		// Ver en: https://thingspeak.com/channels/377497/
		esp01SendTPCIPDataToServer( THINGSPEAK_SERVER_URL, THINGSPEAK_SERVER_PORT,tcpIpDataToSend, strlen( tcpIpDataToSend ) );
		}
	}

   /* NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa no es llamado
      por ningun S.O. */
   return 0 ;
}

void adcStructConfig(void){
	dataLogger.adcChannel1 = 0;
	dataLogger.adcChannel2 = 0;
	dataLogger.adcChannel3 = 0;
}

void formatSdBuffer(rtc_t * rtc,adcChannel_t *adcChannel){
	char auxBuff[10];

	memset( fileBuff, 0, 20 ); // borro el buffer de escritura en el file de la sd

	// Agrego el dia al buffer
	   if( (rtc->mday)<10 )
		   fileBuff[0]='0';

	itoa( (int) (rtc->mday), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));
	strncat(fileBuff,"/",strlen("/"));
	// Agrego el mes al buffer
	if( (rtc->month)<10 ){
		strncat(fileBuff,"0",strlen("0"));
	}

	itoa( (int) (rtc->month), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));
	strncat(fileBuff,"/",strlen("/"));
	// Agrego el ano al buffer
	if( (rtc->year)<10 ){
		strncat(fileBuff,"0",strlen("0"));
	}

	itoa( (int) (rtc->year), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));

	strncat(fileBuff,CONNECTOR,strlen(CONNECTOR));

	// Agrego la hora al buffer
	if( (rtc->hour)<10 ){
		strncat(fileBuff,"0",strlen("0"));
	}

	itoa( (int) (rtc->hour), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));
	strncat(fileBuff,":",strlen(":"));
	// Agrego los minutos al buffer
	if( (rtc->min)<10 ){
		strncat(fileBuff,"0",strlen("0"));
	}

	itoa( (int) (rtc->min), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));
	strncat(fileBuff,":",strlen(":"));
	// Agrego los segundos al buffer
	if( (rtc->sec)<10 ){
		strncat(fileBuff,"0",strlen("0"));
	}

	itoa( (int) (rtc->sec), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));

	strncat(fileBuff,CONNECTOR,strlen(CONNECTOR));

	// Agrego la lectura de los canales 1,2 y 3 del ADC al buffer

	strncat(fileBuff,"CH1:",strlen("CH1:"));

	itoa( (int) (adcChannel->adcChannel1), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));

	strncat(fileBuff,CONNECTOR,strlen(CONNECTOR));

	strncat(fileBuff,"CH2:",strlen("CH2:"));

	itoa( (int) (adcChannel->adcChannel2), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));

	strncat(fileBuff,CONNECTOR,strlen(CONNECTOR));

	strncat(fileBuff,"CH3:",strlen("CH3:"));

	itoa( (int) (adcChannel->adcChannel3), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));

	strncat(fileBuff,"\r\n",strlen("\r\n"));
}

void recordStringInSdCard(char *str, uint32_t strLen){

	UINT nbytes = 0;

	// Create/open a file, then write a string and close it
	if( f_open( &fp, FILENAME, FA_WRITE | FA_OPEN_APPEND ) == FR_OK ){

	  f_write( &fp, str, strLen, &nbytes );
	  f_close(&fp);

	if( strLen == nbytes ){
	     gpioToggle( LEDB );
	} else{
	        gpioToggle( LEDR );
	  }

	} else{
	      // Turn ON LEDR if can't create/open this file
	      gpioWrite( LEDR, ON );
	   }
}

// FUNCION que se ejecuta cada vezque ocurre un Tick
void diskTickHook( void *ptr ){
  // disk_timerproc();   // Disk timer process
}

/*==================[end of file]============================================*/
