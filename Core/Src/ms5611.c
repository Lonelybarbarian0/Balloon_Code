/*
 * ms5611.h
 *
 *  Created on: Jun 9, 2026
 *      Author: evans
 */

#include "ms5611.h"
#include "main.h"

I2C_HandleTypeDef *baro_i2c;

baro_data_t current_baro_data = {0};
uint32_t D1;   // digital pressure value
uint32_t D2;   // digital temp value
uint16_t C[6];
int P;         // scaled pressure
int TEMP;      // scaled temp
int T2;
int64_t dT;
int64_t OFF = 0;
int64_t OFF2 = 0;
int64_t SENS = 0;
int64_t SENS2 = 0;

uint8_t temp_buf[3];    // 24 bits for temp and pressure data
uint8_t cmd;

HAL_StatusTypeDef MS5611_Read_Calibration(I2C_HandleTypeDef *hi2c) {

	HAL_StatusTypeDef ret = HAL_OK;

    for (int i = 0; i < 6; i++) {
        cmd = PROM_READ_BASE + (i * 2);
        uint8_t buf[2];
        ret = HAL_I2C_Master_Transmit(hi2c, MS5611_ADDR, &cmd, 1, 5);
        ret = HAL_I2C_Master_Receive(hi2c, MS5611_ADDR, buf, 2, 5);
        C[i] = (buf[0] << 8) | buf[1];
    }

    return ret;

}

HAL_StatusTypeDef MS5611_Init(I2C_HandleTypeDef *hi2c) {

	HAL_StatusTypeDef ret = HAL_OK;
    baro_i2c = hi2c;

    /* Reset the sensor */
    cmd = MS5611_RESET_CMD;
    ret = HAL_I2C_Master_Transmit(baro_i2c, MS5611_ADDR, &cmd, 1, 10);
    HAL_Delay(BARO_DELAY); 

    /* Read calibration data from PROM */
    ret = MS5611_Read_Calibration(baro_i2c);

    return ret;

}

HAL_StatusTypeDef MS5611_Read_Pressure_Temperature(I2C_HandleTypeDef *hi2c) {

	HAL_StatusTypeDef ret = HAL_OK;
    int64_t temp_val;    // temporary variable

    T2 = 0;
    OFF2 = 0;
    SENS2 = 0;

    cmd = MS5611_CONV_D1_CMD;
    ret = HAL_I2C_Master_Transmit(hi2c, MS5611_ADDR, &cmd, 1, 5);
    HAL_Delay(BARO_DELAY);
    cmd = MS5611_ADC_READ_CMD;
    ret =  HAL_I2C_Master_Transmit(hi2c, MS5611_ADDR, &cmd, 1, 5);
    ret =  HAL_I2C_Master_Receive(hi2c, MS5611_ADDR, temp_buf, 3, 5);

    D1 = (temp_buf[0] << 16) | (temp_buf[1] << 8) | temp_buf[2];

    cmd = MS5611_CONV_D2_CMD;
    ret = HAL_I2C_Master_Transmit(hi2c, MS5611_ADDR, &cmd, 1, 5);
    HAL_Delay(BARO_DELAY);
    cmd = MS5611_ADC_READ_CMD;
    ret =  HAL_I2C_Master_Transmit(hi2c, MS5611_ADDR, &cmd, 1, 5);
    ret =  HAL_I2C_Master_Receive(hi2c, MS5611_ADDR, temp_buf, 3, 5);

    D2 = (temp_buf[0] << 16) | (temp_buf[1] << 8) | temp_buf[2];

    dT = (int32_t)D2 - ((int32_t)C[4] << 8);
    TEMP = 2000 + ((dT * C[5]) >> 23);

    OFF = ((int64_t)C[1] << 16) + ((C[3] * dT) >> 7);
    SENS = ((int64_t)C[0] << 15) + ((C[2] * dT) >> 8);

    if (TEMP < 2000) {

        T2 = (dT * dT) >> 31;
        temp_val = TEMP - 2000;
        OFF2 = (5 * temp_val * temp_val) >> 1;
        SENS2 = OFF2 >> 1;

    }

    if (TEMP < -1500) {

        temp_val = (TEMP + 1500);    // temporary variable
        OFF2 = OFF2 + (7 * temp_val * temp_val);
        SENS2 = SENS2 + ((11 * temp_val * temp_val) >> 1);

    }

    TEMP = TEMP - T2;
    OFF = OFF - OFF2;
    SENS = SENS - SENS2;
    P = (((D1 * SENS) >> 21) - OFF) >> 15;

    current_baro_data.pressure = P;
    current_baro_data.temperature = TEMP;

    return ret;
}

// Move this to main
#include <math.h>
int16_t altitude;
float P0 = 101325; // Sea-level pressure
void get_cur_alt_from_pressure() {
    float p = (float)current_baro_data.pressure;

    // 0 - 11 km
    if (p > 22632.1f) {
        altitude = (int16_t)(44330.77f * (1.0f - powf(p / 101325.0f, 0.190295f)));
    }
    // 11 - 20 km
    else if (p > 5474.89f) {
        altitude = (int16_t)(11000.0f - (6341.62f * logf(p / 22632.1f)));
    }
    // 20+ km
    else {
        altitude = (int16_t)(20000.0f + (216650.0f * (powf(p / 5474.89f, -0.029271f) - 1.0f)));
    }
}
