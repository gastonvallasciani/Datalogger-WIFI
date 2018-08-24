#ifndef _ADC_HARWAREPROXY_H_
#define _ADC_HARDWAREPROXY_H_

/*==================[inclusions]=============================================*/
#include "sapi.h"
#include "sapi_datatypes.h"
#include "sapi_peripheral_map.h"
/*==================[cplusplus]==============================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================[macros]=================================================*/
#define CHANNEL0 LPC_ADC0
#define CHANNEL1 LPC_ADC1
/*==================[typedef]================================================*/
typedef enum{BURST_MODE,
			 DMA_MODE,
			 NORMAL_MODE,
//			 IRQ_STATUS
}adcHardwareProxyConfigMode_t;

typedef enum{SET_RESOLUTION = 1,
			 SET_SAMPLE_RATE,
			 SET_LEFT_CHANNEL,
			 SET_RIGHT_CHANNEL,
}adcHardwareProxySet_t;

/*==================[external data declaration]==============================*/

/*==================[external functions declaration]=========================*/
void ADCHARDWAREPROXY_initialize(LPC_ADC_T *channel);
void ADCHARDWAREPROXY_config(LPC_ADC_T *channel,adcHardwareProxyConfigMode_t mode,ADC_CHANNEL_T adcMultiplexedChannel,uint32_t adcSampleRate, uint8_t resolution);
void ADCHARDWAREPROXY_disable(LPC_ADC_T *channel);
void ADCHARDWAREPROXY_marshal(LPC_ADC_T *channel, ADC_CHANNEL_T adcMultiplexedChannel, uint32_t sampleRate, uint8_t resolution, uint8_t action);
uint8_t ADCHARDWAREPROXY_unmarshal(LPC_ADC_T *channel, adcHardwareProxyConfigMode_t mode,ADC_CHANNEL_T adcMultiplexedChannel);
void ADC0_IRQHandler(void);
/*==================[cplusplus]==============================================*/

#ifdef __cplusplus
}
#endif

/*==================[end of file]============================================*/
#endif /* #ifndef _ADC_HARDWAREPROXY_H_ */
