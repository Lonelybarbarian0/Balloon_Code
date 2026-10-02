/*
 * becon.c
 *
 *  Created on: Jun 3, 2026
 *      Author: kailas
 */

/* Includes Begin */
#include <telemetry.h>

#include "main.h"
#include "sx1278.h"
#include "string.h"
#include "image.h"
#include "stm32f4xx_hal_iwdg.h"
#include "neo_6m.h"
#include "ms5611.h"
#include "MPU_6050.h"
/* Includes End */

extern IWDG_HandleTypeDef hiwdg;

uint8_t BECON[BECON_PACKET_SIZE];

int16_t rx_rssi = 0;
uint8_t heater_status = 0x00; // OFF

extern volatile uint8_t TX_DONE;
extern volatile uint8_t RX_DONE;

extern volatile gps_info_t current_gps_data;

extern baro_data_t current_baro_data;

extern Struct_MPU6050 MPU_DATA;

snsr_err_ errr = {0};

volatile uint8_t TX_IMAGE_FLAG = 0x00;
volatile uint8_t TX_BECON_FLAG = 0x00;
volatile uint8_t RETRANSMIT_JPEG_FLAG = 0x00;

volatile uint16_t JPEG_ID = 0;

uint8_t RETRANSMIT_JPEG_ID[60] = {0};
uint8_t RETRANSMIT_JPEG_ID_COUNT = 0;

struct PAYLOAD GDM;

uint16_t CRC16(uint8_t *data , uint16_t len)
{
	uint16_t crc = 0xFFFF;

	for(int i = 0;i < len;i++)
	{
		crc = crc^data[i];

		for(int j=0; j<8;j++)
		{
			if(crc & 0x0001)
				crc = (crc >> 1) ^ POLYNOMIAL;
			else
				crc = crc >> 1;
		}
	}

	crc = crc ^ 0xFFFF;
	return crc;
}


void GDM_Manager(void)
{
	GDM.time = HAL_GetTick();

	GDM.altitude = current_gps_data.altitude;
	GDM.latitude = current_gps_data.latitude;
	GDM.longitude = current_gps_data.longitude;

	GDM.speed = (int8_t)(current_gps_data.speed/100);

	GDM.rssi = rx_rssi;

	if(errr.ms5611_error == 1)
	{
		GDM.temperature = 0xFF;

		GDM.pressure = 0xFFFFFFFF;
	}
	else
	{
		GDM.temperature = (int8_t)(current_baro_data.temperature/100);

		GDM.pressure = current_baro_data.pressure;
	}


	//batt voltage
	//GDM.battery_voltage = batt_vol;

	//heater status
	GDM.heater_status = heater_status;

	if(MPU_DATA.error_flag == 1)
	{
		GDM.pitch = 0xFFFF;
		GDM.roll = 0xFFFF;
		GDM.yawrate = 0xFFFF;
	}

	else
	{
		GDM.pitch   = (int16_t)MPU_DATA.pitch;
		GDM.roll   = (int16_t)MPU_DATA.roll;
		GDM.yawrate = (int16_t)MPU_DATA.gyro_z;
	}

}


void Parse_Data(void)
{

	BECON[0] = FLAG;

	BECON[1] = 	'C' << 1;
	BECON[2] = 	'Q' << 1;
	BECON[3] = 	' ' << 1;
	BECON[4] = 	' ' << 1;
	BECON[5] = 	' ' << 1;
	BECON[6] = 	' ' << 1;
	BECON[7] =  0x60; //SSID

	BECON[8] = 	'V' << 1;
	BECON[9] = 	'U' << 1;
	BECON[10] = '3' << 1;
	BECON[11] = '3' << 1;
	BECON[12] = 'K' << 1;
	BECON[13] = 'A' << 1;
	BECON[14] = 0x61; //SSID

	BECON[15] = CONTROL;

	BECON[16] = PID;

	BECON[17] = (uint8_t)(GDM.time >> 24);
	BECON[18] = (uint8_t)(GDM.time >> 16);
	BECON[19] = (uint8_t)(GDM.time >> 8);
	BECON[20] = (uint8_t)(GDM.time >> 0);

	BECON[21] = (uint8_t)(GDM.altitude >> 24);
	BECON[22] = (uint8_t)(GDM.altitude >> 16);
	BECON[23] = (uint8_t)(GDM.altitude >> 8);
	BECON[24] = (uint8_t)(GDM.altitude >> 0);

	BECON[25] = (uint8_t)(GDM.latitude >> 24);
	BECON[26] = (uint8_t)(GDM.latitude >> 16);
	BECON[27] = (uint8_t)(GDM.latitude >> 8);
	BECON[28] = (uint8_t)(GDM.latitude >> 0);

	BECON[29] = (uint8_t)(GDM.longitude >> 24);
	BECON[30] = (uint8_t)(GDM.longitude >> 16);
	BECON[31] = (uint8_t)(GDM.longitude >> 8);
	BECON[32] = (uint8_t)(GDM.longitude >> 0);

	BECON[33] = (uint8_t)(GDM.rssi >> 8);
	BECON[34] = (uint8_t)(GDM.rssi >> 0);

	BECON[35] = (uint8_t)(GDM.temperature >> 0);

	BECON[36] = (uint8_t)(GDM.battery_voltage >> 0);

	BECON[37] = (uint8_t)(GDM.heater_status >> 0);

	BECON[38] = (uint8_t)(GDM.pitch >> 8);
	BECON[39] = (uint8_t)(GDM.pitch >> 0);

	BECON[40] = (uint8_t)(GDM.roll >> 8);
	BECON[41] = (uint8_t)(GDM.roll >> 0);

	BECON[42] = (uint8_t)(GDM.yawrate >> 8);
	BECON[43] = (uint8_t)(GDM.yawrate >> 0);

	BECON[44] = (uint8_t)(GDM.speed >> 0);

	BECON[45] = (uint8_t)(GDM.pressure >> 24);
	BECON[46] = (uint8_t)(GDM.pressure >> 16);
	BECON[47] = (uint8_t)(GDM.pressure >> 8);
	BECON[48] = (uint8_t)(GDM.pressure >> 0);

	uint16_t crc = CRC16(BECON + 1, AX_25_WIHTOUT_FLAG);

	BECON[49] = crc & 0xFF;        // LSB first
	BECON[50] = (crc >> 8) & 0xFF; // MSB

	BECON[51] = FLAG;

}

void Heater_Control(void)
{
	if(errr.ms5611_error == 1)
	{
		HAL_GPIO_WritePin(HEATER_GPIO_Port, HEATER_Pin, GPIO_PIN_SET);
	    heater_status = 0x01;
	}
	else
	{
		if(current_baro_data.temperature < 20)
		{
			HAL_GPIO_WritePin(HEATER_GPIO_Port, HEATER_Pin, GPIO_PIN_SET);
			heater_status = 0x01;
		}
		else if (current_baro_data.temperature > 30)
		{
			HAL_GPIO_WritePin(HEATER_GPIO_Port, HEATER_Pin, GPIO_PIN_RESET);
			heater_status = 0x00;
		}
	}
}


void TX_Becon(void)
{
	if((TX_BECON_FLAG == 1) && (TX_DONE == 1)) //this flag is set to 1 on 5 sec timr intr
	{
		//Send BECON[52] over GFSK using sx1278

		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

		uint8_t tx_buff[BECON_PACKET_SIZE + 1];

		tx_buff[0] = BECON_PACKET_SIZE;

		memcpy(&tx_buff[1], BECON, BECON_PACKET_SIZE);

		sx1278_OpMode(STANDBY_MODE);

		sx1278_WaitForModeReady(STANDBY_MODE);

		sx1278_WriteFIFO(tx_buff, BECON_PACKET_SIZE + 1);

		TX_DONE = 0;
		HAL_IWDG_Refresh(&hiwdg);

		sx1278_OpMode(TX);
		sx1278_WaitForModeReady(TX);

		TX_BECON_FLAG = 0;

	}

}

void JPEG_Image_Transmit(void)
{
    if(TX_IMAGE_FLAG && TX_DONE)
    {

    	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

        uint8_t PAYLOAD[63] = {0};
        uint8_t *image_packet;
        uint16_t size;
        uint8_t tx_buff[64];

        uint16_t final_id = FINAL_JPEG_ID();

        if(JPEG_ID >= final_id)
        {
            JPEG_ID = 0;
            TX_IMAGE_FLAG = 0;

            return;
        }

        PAYLOAD[0] = JPEG_ID >> 8;
        PAYLOAD[1] = JPEG_ID;
        PAYLOAD[2] = JPEG_IMAGE;

        Packet_JPEG(JPEG_ID, &image_packet, &size);

        memcpy(&PAYLOAD[3], image_packet, size);

        uint16_t crc = CRC16(PAYLOAD, 61);

        PAYLOAD[61] = crc;
        PAYLOAD[62] = crc >> 8;

        tx_buff[0] = sizeof(PAYLOAD);
        memcpy(&tx_buff[1], PAYLOAD, sizeof(PAYLOAD));

        sx1278_OpMode(STANDBY_MODE);
        sx1278_WaitForModeReady(STANDBY_MODE);

        sx1278_WriteFIFO(tx_buff, sizeof(PAYLOAD) + 1);

        TX_DONE = 0;

        sx1278_OpMode(TX);
        sx1278_WaitForModeReady(TX);

        JPEG_ID++;      // Advance to the next packet ONLY after queueing this one

    }

    if(!TX_IMAGE_FLAG && TX_DONE && RETRANSMIT_JPEG_FLAG)
    {
    	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

        uint8_t PAYLOAD[63] = {0};
        uint8_t *image_packet;
        uint16_t size;
        uint8_t tx_buff[64];

        static uint8_t index = 0;

        if(index >= RETRANSMIT_JPEG_ID_COUNT*2)
        {
        	RETRANSMIT_JPEG_ID_COUNT = 0;
        	RETRANSMIT_JPEG_FLAG = 0;
        	index = 0;

        	return;
        }

        uint16_t final_id = FINAL_JPEG_ID();

        PAYLOAD[0] = RETRANSMIT_JPEG_ID[index];
        PAYLOAD[1] = RETRANSMIT_JPEG_ID[index+1];
        PAYLOAD[2] = JPEG_IMAGE;

        uint16_t id = ((uint16_t)RETRANSMIT_JPEG_ID[index] << 8) | RETRANSMIT_JPEG_ID[index+1];

        if(id >= final_id)
        {
            index += 2;
            return;
        }

        Packet_JPEG(id, &image_packet, &size);
        memcpy(&PAYLOAD[3], image_packet, size);

        uint16_t crc = CRC16(PAYLOAD, 61);

        PAYLOAD[61] = crc;
        PAYLOAD[62] = crc >> 8;

        tx_buff[0] = sizeof(PAYLOAD);
        memcpy(&tx_buff[1], PAYLOAD, sizeof(PAYLOAD));

        sx1278_OpMode(STANDBY_MODE);
        sx1278_WaitForModeReady(STANDBY_MODE);

        sx1278_WriteFIFO(tx_buff, sizeof(PAYLOAD) + 1);

        TX_DONE = 0;

        sx1278_OpMode(TX);
        sx1278_WaitForModeReady(TX);

        index += 2;
    }
}


void RX_Handler(void)
{
	if((TX_BECON_FLAG == 0) && (TX_IMAGE_FLAG == 0))
	{
		if(RX_DONE == 1)
		{

			rx_rssi = sx1278_GetRSSI();

			uint8_t data[66] = {0};
			uint8_t len = 0;

			sx1278_ReadFIFO(&len, 1);

			RX_DONE = 0;

			if((len <= 255) && (len > 0))
			{
				sx1278_ReadFIFO(data, len);

				if(data[0] == 0x55)
				{
					if(data[1] == 0x49) //Transmit JPEG Command
					{
						TX_IMAGE_FLAG = 1;
						HAL_Delay(250);
					}
					if(data[1] == 0x45) //Retransmit JPEG Packets Command
					{

						RETRANSMIT_JPEG_ID_COUNT = data[2];

						if ((RETRANSMIT_JPEG_ID_COUNT <= 30) &&
						    (len == (3 + RETRANSMIT_JPEG_ID_COUNT * 2)))
						{
							memcpy(RETRANSMIT_JPEG_ID, &data[3], (RETRANSMIT_JPEG_ID_COUNT*2));

							RETRANSMIT_JPEG_FLAG = 1;
							HAL_Delay(250);
						}

					}
					if(data[1] == 0x37)
					{
						TX_BECON_FLAG = 1;
						HAL_Delay(250);
					}
					if(data[1] == 0x27)
					{
						HAL_NVIC_SystemReset();
					}
				}
			}
		}

		if(TX_DONE == 1)
		{
			uint8_t current_mode = 0;

			SX1278_ReadReg(RegOpMode, &current_mode);

			if((current_mode & 0x07) != 0x05) // if not in rx mode set to rx mode
			{
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

				sx1278_OpMode(RX);
				sx1278_WaitForModeReady(RX);
			}
		}

	}
}
