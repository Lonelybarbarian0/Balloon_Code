/*
 * becon.h
 *
 *  Created on: Jun 3, 2026
 *      Author: kailas
 */

#ifndef INC_TELEMETRY_H_
#define INC_TELEMETRY_H_

#endif /* INC_TELEMETRY_H_ */

/* Includes Begin */
#include "main.h"
/* Includes End */

/* Macros Begin */

/* AX.25 Packet Structure
 * [FLAG] [DEST_ADDR + SSID] [SOURCE_ADDR + SSID] [CONTROL] [PID] [PAYLOAD] [CRC16] [FLAG]
 * NOTE: CRC must be calculated without including the FLAGS
 */
#define FLAG 				0x7E //1 byte
#define CONTROL				0x03 //1 byte
#define PID					0xF0 //1 byte

#define BECON_PAYLOAD_SIZE 	32 //32 bytes
#define AX_25_WIHTOUT_FLAG	48 //48 bytes
#define BECON_PACKET_SIZE	52 //52 bytes

#define POLYNOMIAL 			0x8408 //x^16+x^12+x^5+1

#define RX_STATE 	0x00
#define TX_STATE 	0x01
#define STBY_STATE	0x02

#define JPEG_PACKET_SIZE_SX1278	 65 //keeping every data packet 65 bytes or less to tx
// DESTINATION_ADDR		"CQ    " //6 bytes
// SOURCE_ADDR			"VU33KA" //6 bytes

#define NO_TASK			0x00
#define JPEG_TX_TASK 	0x01
#define BECON_TX_TASK 	0x02


/* Macros End */

struct PAYLOAD{
	uint32_t time;
	uint32_t altitude;
	uint32_t latitude;
	uint32_t longitude;
	int16_t rssi;
	int8_t temperature;
	uint8_t battery_voltage;
	uint8_t heater_status;
	int16_t pitch;
	int16_t roll;
	int16_t yawrate;
	int8_t speed;
	uint32_t pressure;
};

typedef struct snsr_err{
	uint8_t sx1278_error;
	uint8_t mpu6050_error;
	uint8_t ms5611_error;
}snsr_err_;

uint16_t CRC16(uint8_t *data , uint16_t len);
void GDM_Manager(void);
void Parse_Data(void);
void Heater_Control(void);
void TX_Becon(void);
void RX_Handler(void);
void JPEG_Image_Transmit(void);
void Pet_Reset(void);
