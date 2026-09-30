/*
 * ms5611.h
 *
 *  Created on: Jun 9, 2026
 *      Author: evans
 */

#ifndef INC_MS561_H
#define INC_MS561_H

/* Includes Begin */

#include "main.h"

/* Includes End */

/* Macros Begin */
#define BARO_DELAY 3  // ms, delay for sensor conversions (OSR=1024)

#define MS5611_ADDR          0x77 << 1  // I2C address (7-bit shifted for HAL)
#define MS5611_RESET_CMD     0x1E
#define MS5611_CONV_D1_CMD   0x44  // OSR=1024
#define MS5611_CONV_D2_CMD   0x54  // OSR=1024
#define MS5611_ADC_READ_CMD  0x00
#define PROM_READ_BASE       0xA2  // Base address for PROM calibration constant reads 
/* Macros End */

/* Extern Begin */

extern I2C_HandleTypeDef *baro_i2c;

/* Extern End */

volatile typedef struct {
    int temperature;  // deg C
    int pressure;     // mBar
} baro_data_t;

extern uint16_t C[6];  // Calibration constants read from PROM

/*********************** Function Prototypes **************************/
HAL_StatusTypeDef MS5611_Read_Calibration(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MS5611_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MS5611_Read_Pressure_Temperature(I2C_HandleTypeDef *hi2c);

#endif /* INC_MS561_H */
