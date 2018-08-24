/*
 * Author: Gaston Vallasciani
 * Date: 2018-06-07
 * Proyecto Final Protocolos de Comunicacion en sistemas embebidos
 * Logueo de errores en el proyecto final Procesador de audio digital para radios online y radios FM.
 */

/*==================[inclusions]=============================================*/

//#include "rtc.h"   // <= own header (optional)
#include "sapi.h"    // <= sAPI header
#include "ff.h"       // <= Biblioteca FAT FS
#include <string.h>
#include "ESP8266_ESP01.h"
#include "sapi_i2c.h"
#include "ADC_hardwareProxy.h"
#include "ADC_proxyClient.h"
#include "sapi_timer.h"

/*==================[macros and definitions]=================================*/
#define FILENAME "newFile.txt"
#define CONNECTOR ", "
#define SAMPLES 20

#define SD_PRINT_DEBUG 1 // compilacion condicional para habilitar la herramienta de debug de escritura sobre la sd
#define WIFI_PRINT_DEBUG 1 // compilacion condicional para habilitar la herramienta de debug pro wifi

enum{leftChannel = 0, rightChannel};
enum{lowLevelSignal = 0, mediumLevelSignal, highLevelSignal};

typedef struct{
	uint32_t fullBufferCounter;
	uint32_t emptyBufferCounter;
	uint8_t channel;
	uint8_t audioSignalLevelState;
}errorControl_t;

typedef struct{
	uint16_t audioLeftChannel;
	uint16_t audioRightChannel;
}audioChannel_t;

/*==================[internal data declaration]==============================*/
#ifdef SD_PRINT_DEBUG
static FATFS fs;           // <-- FatFs work area needed for each volume
static FIL fp;             // <-- File object needed for each open file
#endif
/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/
errorControl_t leftChannelErrorControlStruct, rightChannelErrorControlStruct;
audioChannel_t audioChannel;
esp01management_t esp01ConnectionManagement;
uint8_t errorRegister = 0x00;
//DEBUG_PRINT_ENABLE
/*==================[external data definition]===============================*/

/* Buffer */
static char uartBuff[10];
#ifdef SD_PRINT_DEBUG
static char fileBuff[250];
#endif
/*==================[internal functions definition]==========================*/
void errorControl(uint16_t acquisitionInput, uint8_t channel);
void thingspeakWrite(char* url, uint32_t port, int64_t wordToWrite);
void alarmLedInit(void);
void errorStructInit(uint8_t channel);
void thingspeakErrorRegister(uint8_t *errorRegister,errorControl_t *leftChannelErrorControlStruct);
/*==================[external functions definition]==========================*/
#ifdef SD_PRINT_DEBUG
void formatSdBuffer(rtc_t * rtc, audioChannel_t *audioChannel, errorControl_t *leftChannelErrorControlStruct, errorControl_t *rightChannelErrorControlStruct);
void recordStringInSdCard(char *str, uint32_t strLen);
#endif
void tickTimerHandler( void *ptr );

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

   // Inicializar la placa
   boardConfig();

   uint32_t nTicks;
   nTicks = Timer_microsecondsToTicks( 10 ); // El timer se interrumpira con una frecuencia de 100kHz

   // Inicializar UART_USB a 115200 baudios
   uartConfig( UART_USB, 115200 );
   uartConfig( UART_232, 9600 );

   // ADC init
   //adcInit(ADC_ENABLE);
   ADCPROXYCLIENT_initialize();	// Se inicializa el ADC con una frecuencia de conversion de 400KHz
   ADCPROXYCLIENT_config();

   // SPI configuration
   spiConfig( SPI0 );

#ifdef WIFI_PRINT_DEBUG
   // Inicializo la maquina de estados que maneja la conexion del modulo esp8266-esp01
   fsmEsp01Init(&esp01ConnectionManagement);
#endif
   // Estructura RTC
     rtc_t rtc;

     rtc.year = 2018;
     rtc.month = 8;
     rtc.mday = 24;
     rtc.wday = 5;
     rtc.hour = 9;
     rtc.min = 15;
     rtc.sec= 0;

     /* Inicializar RTC */
     bool_t val = 0;
     val = rtcConfig( &rtc );

     delay_t delay15s;
     delayConfig( &delay15s, 15000 );

     delay(2000); // El RTC tarda en setear la hora, por eso el delay

     rtc.year = 2018;
     rtc.month = 8;
     rtc.mday = 24;
     rtc.wday = 5;
     rtc.hour = 9;
     rtc.min = 15;
     rtc.sec= 0;

     delay(2000); // El RTC tarda en setear la hora, por eso el delay

     // Se establece fecha y hora
     val = rtcWrite( &rtc );

#ifdef SD_PRINT_DEBUG
     // Give a work area to the default drive
     if( f_mount( &fs, "", 0 ) != FR_OK ){
       // If this fails, it means that the function could
       // not register a file system object.
       // Check whether the SD card is correctly connected
     }
#endif

     errorStructInit(leftChannel);// Inicializo estructura de error canal izquierdo
     errorStructInit(rightChannel);// Inicializo estructura de error canal derecho
     alarmLedInit();// inicializo los led de indicacion de nivel

     Timer_Init( 1 , nTicks, tickTimerHandler ); // Inicializo ultimo el Timer 1 para que no se produzca una interrupcion temprana

   /* ------------- REPETIR POR SIEMPRE ------------- */
   while(1){

#ifdef WIFI_PRINT_DEBUG
	   // Actualizo la maquina de estados que maneja la conexion del esp8266-esp01
	   fsmEsp01Act(&esp01ConnectionManagement);
#endif

	   //Leo datos del buffer circular de adquisicion del ADC
	   if(ADCPROXYCLIENT_access(adcGetValue, &audioChannel.audioLeftChannel) == bufferVacio){
		   leftChannelErrorControlStruct.emptyBufferCounter++;
	   }

	   //Hago el control del nivel de entrada sobre el canal izquierdo (el derecho no esta implementado todavia)
	   errorControl(audioChannel.audioLeftChannel, leftChannel);

	   //Actualizo la informacion de error cada 15 segundos
	   // Se deberia actualizar cada mas tiempo para no degradar el funcionamiento dle programa
	   if( delayRead( &delay15s ) ){
#ifdef SD_PRINT_DEBUG
		   // Se lee la fecha y hora
		   val = rtcRead( &rtc );
		   // Se formatea la estructura de error que se escribe por uart y la sd con fecha y hora en formato "DD/MM/YYYY, HH:MM:SS"
		   formatSdBuffer(&rtc,&audioChannel,&leftChannelErrorControlStruct,&rightChannelErrorControlStruct);
		   //Escribo los datos formateados por la UART
		   uartWriteString( UART_USB, fileBuff );
		   // Guardo la estructura formateada en la tarjeta SD
	   	   recordStringInSdCard(fileBuff, strlen(fileBuff));
#endif
#ifdef WIFI_PRINT_DEBUG
	   	   if(esp01ConnectionManagement == CONNECTED){
	   		   // Escribo el registro de estado de alarmas en el servidor de thingspeak
	   		   thingspeakErrorRegister(&errorRegister,audioChannel.audioLeftChannel);
	   		   //thingspeakWrite(THINGSPEAK_SERVER_URL, THINGSPEAK_SERVER_PORT, audioChannel.audioLeftChannel);
	   		thingspeakWrite(THINGSPEAK_SERVER_URL, THINGSPEAK_SERVER_PORT, errorRegister);
	   	   }
#endif
		}
	}

   /* NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa no es llamado
      por ningun S.O. */
   return 0 ;
}


/*Se inicializan las estructuras de error de canal derecho y canal izquierdo*/
void errorStructInit(uint8_t channel){
	if (channel == leftChannel){
		leftChannelErrorControlStruct.channel = leftChannel;
		leftChannelErrorControlStruct.audioSignalLevelState = lowLevelSignal;
		leftChannelErrorControlStruct.emptyBufferCounter = 0;
		leftChannelErrorControlStruct.fullBufferCounter = 0;
	}
	else if(channel == rightChannel){
		rightChannelErrorControlStruct.channel = rightChannel;
		rightChannelErrorControlStruct.audioSignalLevelState = lowLevelSignal;
		rightChannelErrorControlStruct.emptyBufferCounter = 0;
		rightChannelErrorControlStruct.fullBufferCounter = 0;
	}

}

void thingspeakErrorRegister(uint8_t *errorRegister,errorControl_t *leftChannelErrorControlStruct){
	*errorRegister = 0x00;
	if(leftChannelErrorControlStruct->channel == leftChannel){
		*errorRegister += 0x01;
	}
	switch(leftChannelErrorControlStruct->audioSignalLevelState){
	case lowLevelSignal:
		*errorRegister += 0x02;
		break;
	case mediumLevelSignal:
		*errorRegister += 0x04;
		break;
	case highLevelSignal:
		*errorRegister += 0x08;
		break;
	}
	if(leftChannelErrorControlStruct->fullBufferCounter>1000000){
		*errorRegister += 0x10;
	}
	if(leftChannelErrorControlStruct->emptyBufferCounter>1000000){
		*errorRegister += 0x20;
	}
}


/*Funcion utilizada para formatear el mensaje que se manda por uart al modulo wifi para escribir en el servidor de thinkspeak*/
void thingspeakWrite(char* url, uint32_t port, int64_t wordToWrite){
	// Armo el dato a enviar, en este caso para grabar un dato en el canal de Thinspeak
	// Ejemplo: "GET /update?api_key=7E7IOJ276BSDLOBA&field1=66"
	char tcpIpDataToSendLocal[100];

	tcpIpDataToSendLocal[0] = 0; // Reseteo la cadena que guarda las otras agregando un caracter NULL al principio

	strcat( tcpIpDataToSendLocal, "GET /update?api_key=" );     // Agrego la peticion de escritura de datos

	strcat( tcpIpDataToSendLocal, THINGSPEAK_WRITE_API_KEY );   // Agrego la clave de escritura del canal

	strcat( tcpIpDataToSendLocal, "&field" );                   // Agrego field del canal
	strcat( tcpIpDataToSendLocal, intToString(THINGSPEAK_FIELD_NUMBER) );

	strcat( tcpIpDataToSendLocal, "=" );                        // Agrego el valor a enviar
	strcat( tcpIpDataToSendLocal, intToString( wordToWrite ) );

	// Envio los datos por TCP/IP al Servidor de Thingpeak
	esp01SendTPCIPDataToServer( url, port,tcpIpDataToSendLocal, strlen( tcpIpDataToSendLocal ) );
}


/*Se actualiza la indicacion de nivel de entrada en la estructura de control de errores*/
void errorControl(uint16_t acquisitionInput, uint8_t channel){
	if(acquisitionInput > 900){
		if(channel == leftChannel){
			leftChannelErrorControlStruct.audioSignalLevelState = highLevelSignal;
		}
		else if(channel == rightChannel){
			rightChannelErrorControlStruct.audioSignalLevelState = highLevelSignal;
		}
		gpioWrite( LED1, ON );
		gpioWrite( LED2, OFF );
		gpioWrite( LED3, OFF );
	}
	else if(acquisitionInput > 776){
		if(channel == leftChannel){
			leftChannelErrorControlStruct.audioSignalLevelState = mediumLevelSignal;
		}
		else if(channel == rightChannel){
			rightChannelErrorControlStruct.audioSignalLevelState = mediumLevelSignal;
		}
		gpioWrite( LED1, OFF );
		gpioWrite( LED2, ON );
		gpioWrite( LED3, OFF );
	}
	else{
		if(channel == leftChannel){
			leftChannelErrorControlStruct.audioSignalLevelState = lowLevelSignal;
		}
		else if(channel == rightChannel){
			rightChannelErrorControlStruct.audioSignalLevelState = lowLevelSignal;
		}
		gpioWrite( LED1, OFF );
		gpioWrite( LED2, OFF );
		gpioWrite( LED3, ON );
	}
}


/*Se inicializan los led indicadores de nivel de entrada*/
void alarmLedInit(void){
	gpioWrite( LED1, OFF );
	gpioWrite( LED2, OFF );
	gpioWrite( LED3, OFF );
}

#ifdef SD_PRINT_DEBUG
/*Funcion utilizada para formatear el mensaje de error que se envia por UART y se escribe en la SD*/
void formatSdBuffer(rtc_t * rtc, audioChannel_t *audioChannel, errorControl_t *leftChannelErrorControlStruct, errorControl_t *rightChannelErrorControlStruct){
	char auxBuff[10];

	memset( fileBuff, 0, 250 ); // borro el buffer de escritura en el file de la sd

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

	strncat(fileBuff,"LCh:",strlen("LCh:"));

	itoa( (int) (audioChannel->audioLeftChannel), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));

	strncat(fileBuff,CONNECTOR,strlen(CONNECTOR));

	strncat(fileBuff,"RCh:",strlen("RCh:"));

	itoa( (int) (audioChannel->audioRightChannel), (char*)auxBuff, 10 );
	strncat(fileBuff,auxBuff,strlen(auxBuff));

	strncat(fileBuff,"\r\n",strlen("\r\n"));

	if(leftChannelErrorControlStruct->channel == leftChannel){
		strncat(fileBuff,"LCh errors status,",strlen("LCh errors status,"));
		strncat(fileBuff,"Signal Level:",strlen("Signal Level:"));
		if(leftChannelErrorControlStruct->audioSignalLevelState == lowLevelSignal){
			strncat(fileBuff,"Low,",strlen("Low,"));
		}
		else if(leftChannelErrorControlStruct->audioSignalLevelState == mediumLevelSignal){
			strncat(fileBuff,"Medium,",strlen("Medium,"));
		}
		else if(leftChannelErrorControlStruct->audioSignalLevelState == highLevelSignal){
			strncat(fileBuff,"High,",strlen("High,"));
		}
		strncat(fileBuff,"Full Buffer Counter:",strlen("Full Buffer Counter:"));
		itoa( (int) (leftChannelErrorControlStruct->fullBufferCounter), (char*)auxBuff, 10 );
		strncat(fileBuff,auxBuff,strlen(auxBuff));

		strncat(fileBuff,CONNECTOR,strlen(CONNECTOR));

		strncat(fileBuff,"Empty Buffer Counter:",strlen("Empty Buffer Counter:"));
		itoa( (int) (leftChannelErrorControlStruct->emptyBufferCounter), (char*)auxBuff, 10 );
		strncat(fileBuff,auxBuff,strlen(auxBuff));
	}

	strncat(fileBuff,"\r\n",strlen("\r\n"));

	if(rightChannelErrorControlStruct->channel == rightChannel){
		strncat(fileBuff,"RCh errors status,",strlen("RCh errors status,"));
		strncat(fileBuff,"Signal Level:",strlen("Signal Level:"));
		if(rightChannelErrorControlStruct->audioSignalLevelState == lowLevelSignal){
			strncat(fileBuff,"Low,",strlen("Low,"));
		}
		else if(rightChannelErrorControlStruct->audioSignalLevelState == mediumLevelSignal){
			strncat(fileBuff,"Medium,",strlen("Medium,"));
		}
		else if(rightChannelErrorControlStruct->audioSignalLevelState == highLevelSignal){
			strncat(fileBuff,"High,",strlen("High,"));
		}
		strncat(fileBuff,"Full Buffer Counter:",strlen("Full Buffer Counter:"));
		itoa( (int) (rightChannelErrorControlStruct->fullBufferCounter), (char*)auxBuff, 10 );
		strncat(fileBuff,auxBuff,strlen(auxBuff));

		strncat(fileBuff,CONNECTOR,strlen(CONNECTOR));

		strncat(fileBuff,"Empty Buffer Counter:",strlen("Empty Buffer Counter:"));
		itoa( (int) (rightChannelErrorControlStruct->emptyBufferCounter), (char*)auxBuff, 10 );
		strncat(fileBuff,auxBuff,strlen(auxBuff));
	}

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
#endif

// FUNCION que se ejecuta cada vez que se desborda el timer1
void tickTimerHandler( void *ptr ){
	if(ADCPROXYCLIENT_access(adcUpdateValue, &audioChannel.audioLeftChannel) == bufferLleno){
			leftChannelErrorControlStruct.fullBufferCounter++;
		}

}

/*==================[end of file]============================================*/
