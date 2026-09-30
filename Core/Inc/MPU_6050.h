
#include "stm32f4xx_hal.h"



#ifndef INC_MPU_6050_H_
#define INC_MPU_6050_H_




#define MPU6050_ADDR (0x68<<1)


#define MPU6050_ACC_SENSE  16384.0f        /*2g and 250deg/s */
#define MPU6050_GYRO_SENSE   131.0f

#define FS_SCALE_GYRO 0x0
#define FS_SCALE_ACC  0x0


#define MPU6050_SMPRT_DIV 0X19
#define MPU6050_WHO_AM_I 0X75
#define MPU6050_CONFIG 0X1A

#define MPU6050_GYRO_CONFIG 0X1B
#define MPU6050_ACCEL_CONFIG 0X1C

#define MPU6050_INT_PIN_CFG 0X37
#define MPU6050_INT_ENABLE 0X38

#define MPU6050_ACCEL_XOUT_H 0X3B

#define MPU6050_PWR_MGMT_1 0X6B




typedef struct _MPU6050{


	float acc_x;
	float acc_y;
	float acc_z;

	float gyro_x;
	float gyro_y;
	float gyro_z;

	float roll;
	float yaw;
	float pitch;

	float acc_x_offset;
	float acc_y_offset;
	float acc_z_offset;
	float gyro_x_offset;
	float gyro_y_offset;
	float gyro_z_offset;
	volatile uint8_t data_ready;
	uint8_t error_flag;

}Struct_MPU6050;


void MPU6050_init(I2C_HandleTypeDef *i2c,Struct_MPU6050 *mpu6050_buf);
void MPU6050_GetData(I2C_HandleTypeDef *i2c, Struct_MPU6050* mpu6050);
void MPU6050_Calibrate(I2C_HandleTypeDef *i2c, Struct_MPU6050 *mpu6050, uint16_t num_samples);

#endif /* INC_MPU_6050_H_ */
