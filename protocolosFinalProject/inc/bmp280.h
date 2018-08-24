/*
 * bmp280.h
 *
 *  Created on: 16/8/2018
 *      Author: gastonvallasciani
 */

#ifndef MISPROYECTOS_PROTOCOLOSFINALPROJECT_INC_BMP280_H_
#define MISPROYECTOS_PROTOCOLOSFINALPROJECT_INC_BMP280_H_

#define ID_REGISTER 0xD0
#define RESET_REGISTER 0xE0
#define CTRL_HUM_REGISTER 0xF2
#define STATUS_REGISTER 0xF3
#define CTRL_MEAS_REGISTER 0xF4
#define CONFIG_REGISTER 0xF5
#define	PRESS_MSB_REGISTER 0xF7
#define	PRESS_LSB_REGISTER 0xF8
#define	PRESS_XLSB_REGISTER 0xF9
#define	TEMP_MSB_REGISTER 0xFA
#define	TEMP_LSB_REGISTER 0xFB
#define	TEMP_XLSB_REGISTER 0xFC
#define	HUM_MSB_REGISTER 0xFD
#define	HUM_LSB_REGISTER 0xFE

#define SLAVE_ADDRESS 0x77



void bmp280Init(void);
void bmp280Config(void);
void bmp280ReadTemperature(void);
void bmp280ReadHumidity(void);
void bmp280ReadBarometricPressure(void);

void bmp280ReadId(void);



#endif /* MISPROYECTOS_PROTOCOLOSFINALPROJECT_INC_BMP280_H_ */
