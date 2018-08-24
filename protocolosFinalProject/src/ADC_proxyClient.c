/*==================[inclusiones]============================================*/
#include "ADC_hardwareProxy.h"
#include "ADC_proxyClient.h"

#include "sapi.h"
#include "board.h"
#include "sapi_peripheral_map.h"
#include "core_cm4.h"

/*==================[macros and definitions]=================================*/

/*==================[definiciones de datos internos]=========================*/
adcProxyClient_t adcStruct;
extern uint16_t indexReadRx, indexWriteRx, bufferRx[RX_BUFFER_LENGTH];
/*==================[definiciones de funciones externas]=====================*/

/*Funcion publica que inicializa el ADC pro defecto inicializa la frecuencia de meustreo en 400kHz, la resolucion en 10 bits
 *  el canal derecho en el ADC_CH1 y el canal izquierdo en el ADC_CH2*/
void ADCPROXYCLIENT_initialize(void){
	ADCHARDWAREPROXY_initialize(CHANNEL0);
	adcStruct.adcSampleRate = AUDIO_SAMPLE_RATE;
	adcStruct.adcResolution = ADC_10BITS;
	adcStruct.adcRightChannel = ADC_CH1;
	adcStruct.adcLeftChannel = ADC_CH2;
}

/*Funcion publica que configura el ADC*/
void ADCPROXYCLIENT_config(void){
	ADCHARDWAREPROXY_config(CHANNEL0, BURST_MODE, adcStruct.adcRightChannel, adcStruct.adcSampleRate, adcStruct.adcResolution);
}

/*Funcion publica que deshabilita el ADC*/
void ADCPROXYCLIENT_disable(void){
	ADCHARDWAREPROXY_disable(CHANNEL0);
}

/*Funcion publica que permite actualizar buffer circular de adquisicion del ADC y levantar un dato del buffer
 * Parametros:
 * 				action -> adcUpdateVale : permite actualizar el buffer.
 * 				action -> adcGetVale : permite levantar un dato del buffer.
 * 				*dataAcquired -> es utilizado para devolver el valor leido del buffer.
 * Retorno:
 * 				bufferActualizado(0) -> Indica que el buffer ha sido actualizado.
 * 				bufferLleno(1) 		 -> Indica que el buffer de adquisicion esta lleno.
 * 				bufferVacio(2) 		 -> Indica que el buffer de adquisicion esta vacio.
 * 				datoAdquirido(3) 	 -> Indica que se ha levantado un dato del buffer de adquisicion.
 **/
uint8_t ADCPROXYCLIENT_access(accessAction_t  action, uint16_t *dataAcquired){
	uint8_t state=0;
	switch(action){
	case adcUpdateValue:
		if(ADCHARDWAREPROXY_unmarshal(CHANNEL0, BURST_MODE, adcStruct.adcRightChannel)){
			return bufferActualizado; // ADC buffer actualizado
		}else{
			return bufferLleno; // Buffer lleno
		}

		break;
	case adcGetValue:
		if (indexReadRx == indexWriteRx)
				return bufferVacio; // Buffer vacio
			else {
				*dataAcquired = bufferRx[indexReadRx];
				indexReadRx = (indexReadRx+1)%RX_BUFFER_LENGTH;
				return datoAdquirido; // valor del adc adquirido
			}
		break;
	}
}

/*Funcion publica utilizada para realizar modificaciones en la configuracion del ADC (FALTA DESARROLLARLA)*/
void ADCPROXYCLIENT_mutate(uint8_t setAction,ADC_CHANNEL_T adcMultiplexedChannel){

	ADCHARDWAREPROXY_marshal(CHANNEL0, adcMultiplexedChannel, adcStruct.adcSampleRate, adcStruct.adcResolution,setAction);

}

/*==================[fin del archivo]========================================*/
