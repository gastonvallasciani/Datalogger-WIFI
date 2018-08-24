/*
 * ESP8266_ESP01.h
 *
 *  Created on: 3/8/2018
 *      Author: gastonvallasciani
 */

#ifndef MISPROYECTOS_PROTOCOLOSFINALPROJECT_INC_ESP8266_ESP01_H_
#define MISPROYECTOS_PROTOCOLOSFINALPROJECT_INC_ESP8266_ESP01_H_

#define UART_DEBUG                 UART_USB
#define UART_ESP01                 UART_232
#define UARTS_BAUD_RATE            115200

#define ESP01_RX_BUFF_SIZE         1024

//#define WIFI_SSID                  "INVITADOS-CADIEEL"     // Setear Red Wi-Fi
//#define WIFI_PASSWORD              "CORDOBA9504" // Setear password
#define WIFI_SSID                  "Telecentro-c420"     // Setear Red Wi-Fi
#define WIFI_PASSWORD              "QXFRUY6EWEVD" // Setear password

#define THINGSPEAK_SERVER_URL      "api.thingspeak.com"
#define THINGSPEAK_SERVER_PORT     80
#define THINGSPEAK_WRITE_API_KEY   "920ND9HQO0PQ8CT8"
#define THINGSPEAK_FIELD_NUMBER    1

typedef enum{ESP01_INIT,CONNECTION_ERROR,CONECT_TO_WIFI_AP,CONNECTED,NOT_CONNECTED,INIT_ERROR}esp01management_t;

void fsmEsp01Init(uint8_t *actualState);
void fsmEsp01Act(uint8_t *actualState);

bool_t esp01Init( uartMap_t uartForEsp, uartMap_t uartForDebug, uint32_t baudRate );

void esp01CleanRxBuffer( void );

bool_t esp01ShowWiFiNetworks( void );

bool_t esp01ConnectToWifiAP( char* wiFiSSID, char* wiFiPassword );

bool_t esp01ConnectToServer( char* url, uint32_t port );

bool_t esp01ConnectToServer( char* url, uint32_t port );

bool_t esp01SendTCPIPData( char* strData, uint32_t strDataLen );

bool_t esp01SendTPCIPDataToServer( char* url, uint32_t port, char* strData, uint32_t strDataLen );

void stopProgramError( void );



#endif /* MISPROYECTOS_PROTOCOLOSFINALPROJECT_INC_ESP8266_ESP01_H_ */
