/*
 * neo_6m.h
 *
 * Created  : Jun 9, 2026
 * Modified : Jul 10, 2026
 * Author: evans
 */

#ifndef INC_NEO_6M_H_
#define INC_NEO_6M_H_

/* Includes Begin */
#include "main.h"
#include <stdint.h>
/* Includes End */

/* Macros begin */
#define GPS_RX_BUF_SIZE 128  // configure for GPGGA and GPRMC only!
/* Macros end */

typedef struct {
    int latitude;
    int longitude;
    int altitude;
    int speed; 
    int sats;
    uint8_t fix_qual;
    uint16_t hdop;
    uint16_t course;
    uint32_t time_pos;    // timestamp for pos data
    // uint32_t time_vel; // timestamp for vel data
} gps_info_t;

/* Global Variables */
extern uint8_t GPS_RX_BUF[GPS_RX_BUF_SIZE];
extern uint8_t GPS_PROC_BUF[GPS_RX_BUF_SIZE];
extern volatile uint8_t gps_data_ready;  // Set to true when new GPS data is available for processing
extern volatile gps_info_t current_gps_data;
extern UART_HandleTypeDef *gps_uart;

/* Functions */
void GPS_Init(UART_HandleTypeDef *huart);
void Process_NMEA();

#endif /* INC_NEO_6M_H_ */
