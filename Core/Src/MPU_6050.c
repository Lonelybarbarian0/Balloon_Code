#include "MPU_6050.h"
#include <math.h>

#define RAD_TO_DEG 57.295779f

#define ALPHA      0.96f       // Complementary filter weighting
#define DT         0.01f       // 10 ms = 100 Hz


void MPU6050_init(I2C_HandleTypeDef *i2c,
                  Struct_MPU6050 *mpu6050_buf)
{
    /* Clear all buffers */
    mpu6050_buf->acc_x = 0.0f;
    mpu6050_buf->acc_y = 0.0f;
    mpu6050_buf->acc_z = 0.0f;

    mpu6050_buf->gyro_x = 0.0f;
    mpu6050_buf->gyro_y = 0.0f;
    mpu6050_buf->gyro_z = 0.0f;

    mpu6050_buf->roll = 0.0f;
    mpu6050_buf->pitch = 0.0f;
    mpu6050_buf->yaw = 0.0f;

    mpu6050_buf->data_ready = 0;
    mpu6050_buf->error_flag = 0;

    mpu6050_buf->acc_x_offset = 0.0f;
    mpu6050_buf->acc_y_offset = 0.0f;
    mpu6050_buf->acc_z_offset = 0.0f;

    mpu6050_buf->gyro_x_offset = 0.0f;
    mpu6050_buf->gyro_y_offset = 0.0f;
    mpu6050_buf->gyro_z_offset = 0.0f;


    /* Check I2C communication / WHO_AM_I */
    uint8_t val = 0;

    if (HAL_I2C_Mem_Read(i2c,
                         MPU6050_ADDR,
                         MPU6050_WHO_AM_I,
						 I2C_MEMADD_SIZE_8BIT,
                         &val,
                         1,
                         100) == HAL_OK)
    {
        if (val != 0x68)
        {
            mpu6050_buf->error_flag = 1;
            return;
        }
    }
    else
    {
        mpu6050_buf->error_flag = 1;
        return;
    }


    /* Power management
     *
     * 0x88:
     * - Temperature sensor disabled
     * - Internal 8 MHz oscillator temporarily selected
     */
    val = 0x88;

    if (HAL_I2C_Mem_Write(i2c,
                          MPU6050_ADDR,
                          MPU6050_PWR_MGMT_1,
                          1,
                          &val,
                          1,
                          100) != HAL_OK)
    {
        mpu6050_buf->error_flag = 1;
        return;
    }

    HAL_Delay(10);


    /* Select X-axis gyroscope PLL as clock source */
    val = 0x01;

    if (HAL_I2C_Mem_Write(i2c,
                          MPU6050_ADDR,
                          MPU6050_PWR_MGMT_1,
                          1,
                          &val,
                          1,
                          100) != HAL_OK)
    {
        mpu6050_buf->error_flag = 1;
        return;
    }

    HAL_Delay(10);


    /* Sample rate
     *
     * Gyroscope output rate = 1 kHz
     *
     * Sample Rate = 1000 / (1 + SMPLRT_DIV)
     *
     * SMPLRT_DIV = 9
     *
     * Sample rate = 100 Hz
     */
    val = 9;

    if (HAL_I2C_Mem_Write(i2c,
                          MPU6050_ADDR,
                          MPU6050_SMPRT_DIV,
                          1,
                          &val,
                          1,
                          100) != HAL_OK)
    {
        mpu6050_buf->error_flag = 1;
        return;
    }

    HAL_Delay(10);


    /* Digital Low Pass Filter
     *
     * CONFIG = 0x03
     *
     * Gyroscope bandwidth approximately 42 Hz
     * Accelerometer bandwidth approximately 44 Hz
     */
    val = 0x03;

    if (HAL_I2C_Mem_Write(i2c,
                          MPU6050_ADDR,
                          MPU6050_CONFIG,
                          1,
                          &val,
                          1,
                          100) != HAL_OK)
    {
        mpu6050_buf->error_flag = 1;
        return;
    }

    HAL_Delay(10);


    /* Accelerometer full-scale configuration */
    val = FS_SCALE_ACC;

    if (HAL_I2C_Mem_Write(i2c,
                          MPU6050_ADDR,
                          MPU6050_ACCEL_CONFIG,
                          1,
                          &val,
                          1,
                          100) != HAL_OK)
    {
        mpu6050_buf->error_flag = 1;
        return;
    }

    HAL_Delay(10);


    /* Gyroscope full-scale configuration */
    val = FS_SCALE_GYRO;

    if (HAL_I2C_Mem_Write(i2c,
                          MPU6050_ADDR,
                          MPU6050_GYRO_CONFIG,
                          1,
                          &val,
                          1,
                          100) != HAL_OK)
    {
        mpu6050_buf->error_flag = 1;
        return;
    }

    HAL_Delay(10);


    /* No interrupt configuration required.
     *
     * MPU6050 INT pin can be left unconnected.
     */


    mpu6050_buf->error_flag = 0;
}


void MPU6050_GetData(I2C_HandleTypeDef *i2c,
                     Struct_MPU6050 *mpu6050)
{
    uint8_t temp_data_buffer[14];

    /*
     * Read:
     *
     * ACCEL_XOUT_H
     * ACCEL_XOUT_L
     * ACCEL_YOUT_H
     * ACCEL_YOUT_L
     * ACCEL_ZOUT_H
     * ACCEL_ZOUT_L
     * TEMP_OUT_H
     * TEMP_OUT_L
     * GYRO_XOUT_H
     * GYRO_XOUT_L
     * GYRO_YOUT_H
     * GYRO_YOUT_L
     * GYRO_ZOUT_H
     * GYRO_ZOUT_L
     */

    if (HAL_I2C_Mem_Read(i2c,
                         MPU6050_ADDR,
                         MPU6050_ACCEL_XOUT_H,
                         1,
                         temp_data_buffer,
                         14,
                         100) != HAL_OK)
    {
        mpu6050->error_flag = 1;
        return;
    }


    /* Convert raw accelerometer data */

    int16_t raw_acc_x =
        (int16_t)((temp_data_buffer[0] << 8) |
                  temp_data_buffer[1]);

    int16_t raw_acc_y =
        (int16_t)((temp_data_buffer[2] << 8) |
                  temp_data_buffer[3]);

    int16_t raw_acc_z =
        (int16_t)((temp_data_buffer[4] << 8) |
                  temp_data_buffer[5]);


    /* Convert raw gyroscope data */

    int16_t raw_gyro_x =
        (int16_t)((temp_data_buffer[8] << 8) |
                  temp_data_buffer[9]);

    int16_t raw_gyro_y =
        (int16_t)((temp_data_buffer[10] << 8) |
                  temp_data_buffer[11]);

    int16_t raw_gyro_z =
        (int16_t)((temp_data_buffer[12] << 8) |
                  temp_data_buffer[13]);


    /*
     * Convert to physical units and apply calibration offsets.
     *
     * Accelerometer:
     *     g
     *
     * Gyroscope:
     *     degrees/second
     */

    mpu6050->acc_x =
        ((float)raw_acc_x / MPU6050_ACC_SENSE)
        - mpu6050->acc_x_offset;

    mpu6050->acc_y =
        ((float)raw_acc_y / MPU6050_ACC_SENSE)
        - mpu6050->acc_y_offset;

    mpu6050->acc_z =
        ((float)raw_acc_z / MPU6050_ACC_SENSE)
        - mpu6050->acc_z_offset;


    mpu6050->gyro_x =
        ((float)raw_gyro_x / MPU6050_GYRO_SENSE)
        - mpu6050->gyro_x_offset;

    mpu6050->gyro_y =
        ((float)raw_gyro_y / MPU6050_GYRO_SENSE)
        - mpu6050->gyro_y_offset;

    mpu6050->gyro_z =
        ((float)raw_gyro_z / MPU6050_GYRO_SENSE)
        - mpu6050->gyro_z_offset;


    /* Calculate roll from accelerometer */

    float acc_roll =
        atan2f(mpu6050->acc_y,
               mpu6050->acc_z) * RAD_TO_DEG;


    /* Calculate pitch from accelerometer */

    float acc_pitch =
        atan2f(-mpu6050->acc_x,
               sqrtf((mpu6050->acc_y * mpu6050->acc_y) +
                     (mpu6050->acc_z * mpu6050->acc_z)))
        * RAD_TO_DEG;


    /*
     * Initialize roll/pitch/yaw using the first accelerometer
     * measurement.
     *
     * Accelerometer can determine initial roll and pitch because
     * gravity provides a reference.
     *
     * Accelerometer cannot determine yaw, therefore yaw starts
     * at zero.
     */

    static uint8_t is_initialized = 0;

    if (!is_initialized)
    {
        mpu6050->roll = acc_roll;
        mpu6050->pitch = acc_pitch;
        mpu6050->yaw = 0.0f;

        is_initialized = 1;
    }
    else
    {
        /*
         * Complementary filter
         *
         * Gyroscope provides short-term angle changes.
         * Accelerometer corrects long-term drift in roll/pitch.
         */

        mpu6050->roll =
            ALPHA *
            (mpu6050->roll +
             (mpu6050->gyro_x * DT))
            +
            (1.0f - ALPHA) * acc_roll;


        mpu6050->pitch =
            ALPHA *
            (mpu6050->pitch +
             (mpu6050->gyro_y * DT))
            +
            (1.0f - ALPHA) * acc_pitch;


        /*
         * Yaw
         *
         * MPU6050 has no magnetometer, so yaw is obtained by
         * integrating the Z-axis gyro.
         *
         * This WILL drift over time.
         */

        mpu6050->yaw +=
            mpu6050->gyro_z * DT;
    }


    /* Optional: keep yaw between -180 and +180 degrees */

    if (mpu6050->yaw > 180.0f)
    {
        mpu6050->yaw -= 360.0f;
    }
    else if (mpu6050->yaw < -180.0f)
    {
        mpu6050->yaw += 360.0f;
    }


    mpu6050->error_flag = 0;
}


void MPU6050_Calibrate(I2C_HandleTypeDef *i2c,
                       Struct_MPU6050 *mpu6050,
                       uint16_t num_samples)
{
    uint8_t temp_buf[14];

    int32_t sum_ax = 0;
    int32_t sum_ay = 0;
    int32_t sum_az = 0;

    int32_t sum_gx = 0;
    int32_t sum_gy = 0;
    int32_t sum_gz = 0;

    uint16_t valid_samples = 0;


    /* Clear existing offsets */

    mpu6050->acc_x_offset = 0.0f;
    mpu6050->acc_y_offset = 0.0f;
    mpu6050->acc_z_offset = 0.0f;

    mpu6050->gyro_x_offset = 0.0f;
    mpu6050->gyro_y_offset = 0.0f;
    mpu6050->gyro_z_offset = 0.0f;


    /*
     * Sensor must remain completely stationary during
     * calibration.
     *
     * 2 ms between samples gives approximately 500 Hz
     * sampling during calibration.
     */

    for (uint16_t i = 0; i < num_samples; i++)
    {
        if (HAL_I2C_Mem_Read(i2c,
                             MPU6050_ADDR,
                             MPU6050_ACCEL_XOUT_H,
                             1,
                             temp_buf,
                             14,
                             100) == HAL_OK)
        {
            sum_ax +=
                (int16_t)((temp_buf[0] << 8) |
                          temp_buf[1]);

            sum_ay +=
                (int16_t)((temp_buf[2] << 8) |
                          temp_buf[3]);

            sum_az +=
                (int16_t)((temp_buf[4] << 8) |
                          temp_buf[5]);


            sum_gx +=
                (int16_t)((temp_buf[8] << 8) |
                          temp_buf[9]);

            sum_gy +=
                (int16_t)((temp_buf[10] << 8) |
                          temp_buf[11]);

            sum_gz +=
                (int16_t)((temp_buf[12] << 8) |
                          temp_buf[13]);


            valid_samples++;
        }

        HAL_Delay(2);
    }


    /* Check whether any samples were successfully obtained */

    if (valid_samples == 0)
    {
        mpu6050->error_flag = 1;
        return;
    }


    /*
     * Calculate average accelerometer values.
     *
     * These are converted to g.
     */

    float mean_ax =
        ((float)sum_ax / (float)valid_samples)
        / MPU6050_ACC_SENSE;

    float mean_ay =
        ((float)sum_ay / (float)valid_samples)
        / MPU6050_ACC_SENSE;

    float mean_az =
        ((float)sum_az / (float)valid_samples)
        / MPU6050_ACC_SENSE;


    /*
     * Assuming the sensor is calibrated while:
     *
     * X = 0 g
     * Y = 0 g
     * Z = +1 g
     */

    mpu6050->acc_x_offset = mean_ax;

    mpu6050->acc_y_offset = mean_ay;

    mpu6050->acc_z_offset =
        mean_az - 1.0f;


    /*
     * Calculate average gyro offsets.
     *
     * When stationary:
     *
     * X = 0 deg/s
     * Y = 0 deg/s
     * Z = 0 deg/s
     */

    mpu6050->gyro_x_offset =
        ((float)sum_gx / (float)valid_samples)
        / MPU6050_GYRO_SENSE;

    mpu6050->gyro_y_offset =
        ((float)sum_gy / (float)valid_samples)
        / MPU6050_GYRO_SENSE;

    mpu6050->gyro_z_offset =
        ((float)sum_gz / (float)valid_samples)
        / MPU6050_GYRO_SENSE;


    mpu6050->error_flag = 0;
}
