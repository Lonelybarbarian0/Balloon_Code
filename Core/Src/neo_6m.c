/*
 * neo_6m.c
 *
 * Created  : Jun 9, 2026
 * Modified : Jul 10, 2026
 * Author: evans
 */

#include "neo_6m.h"
#include <string.h>
#include <stdlib.h>

/* Global variable instantiations */
volatile gps_info_t current_gps_data = {0};
volatile uint16_t gps_size;
UART_HandleTypeDef *gps_uart;
uint8_t GPS_RX_BUF[GPS_RX_BUF_SIZE];
uint8_t GPS_PROC_BUF[GPS_RX_BUF_SIZE];
volatile uint8_t gps_data_ready = 0;

static char* get_nmea_field(char *sentence, uint8_t field_id) {
    uint8_t comma_cnt = 0;

    while (*sentence && *sentence != '\r' && *sentence != '\n') {
        if (*sentence == ',') {
            comma_cnt++;
            if (comma_cnt == field_id) {
                return sentence + 1; 
            }
        }
        sentence++;
    }
    return NULL;
}

void GPS_Init(UART_HandleTypeDef *huart) {
    gps_uart = huart;
    HAL_UARTEx_ReceiveToIdle_DMA(gps_uart, GPS_RX_BUF, GPS_RX_BUF_SIZE - 1);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {

	gps_size = Size;
    if (huart == gps_uart) { 
        
        if (!gps_data_ready) {
            
            // Limit Size to prevent out-of-bounds memory writes
            if (Size >= GPS_RX_BUF_SIZE) {
                Size = GPS_RX_BUF_SIZE - 1;
            }
            
            gps_size = Size;

            // SAFELY COPY THE DATA FIRST
            memcpy(GPS_PROC_BUF, GPS_RX_BUF, gps_size);

            // Explicitly and safely null-terminate right after the payload
            GPS_PROC_BUF[gps_size] = '\0';

            gps_data_ready = 1;
        }
        
        HAL_UARTEx_ReceiveToIdle_DMA(gps_uart, GPS_RX_BUF, GPS_RX_BUF_SIZE - 1);
    }

}

void Process_NMEA() {

        /* Process NMEA Strings ------------------------------
        $GPGGA: time_pos - timestamp when we got position data
                latitude
                longitude
                altitude
                fix quality
                hdop

        $GPRMC: time_vel - timestamp when we got velocity data
                speed (converted to m/s)
                course - direction of motion in degrees
        --------------------------------------------------*/

    if (gps_data_ready) {

        char *field_ptr;
        
        char *gga_ptr = NULL;
        char *rmc_ptr = NULL;

        // Find where the latest occurence of each sentence actually starts inside the buffer
        char *search_ptr = (char *)GPS_PROC_BUF;
        while ((search_ptr = strstr(search_ptr, "$GPGGA")) != NULL) {
            gga_ptr = search_ptr;
            search_ptr++; // Move forward 1 byte to keep searching rest of buffer
        }

        search_ptr = (char *)GPS_PROC_BUF;
        while ((search_ptr = strstr(search_ptr, "$GPRMC")) != NULL) {
            rmc_ptr = search_ptr;
            search_ptr++;
        }

        gga_ptr = strstr((char *)GPS_PROC_BUF, "$GPGGA");
        rmc_ptr = strstr((char *)GPS_PROC_BUF, "$GPRMC");

        /* Process $GPGGA if present in the buffer */
        if (gga_ptr != NULL) {
            
            field_ptr = get_nmea_field(gga_ptr, 1);
            if (field_ptr) {
                current_gps_data.time_pos = (uint32_t)(atof(field_ptr) * 1e2);
            }

            field_ptr = get_nmea_field(gga_ptr, 2);
            if (field_ptr) {
                double d_mm = atof(field_ptr) * 0.01;
                int degrees = (int)d_mm;
                current_gps_data.latitude = (degrees + ((d_mm - degrees) * 100.0 / 60.0)) * 1e6;
            }
            
            field_ptr = get_nmea_field(gga_ptr, 3);
            if (field_ptr && *field_ptr == 'S') {
                current_gps_data.latitude = -current_gps_data.latitude;
            }
            
            field_ptr = get_nmea_field(gga_ptr, 4);
            if (field_ptr) {
                double d_mm = atof(field_ptr) * 0.01;
                int degrees = (int)d_mm;
                current_gps_data.longitude = (degrees + ((d_mm - degrees) * 100.0 / 60.0)) * 1e6;
            }
            
            field_ptr = get_nmea_field(gga_ptr, 5);
            if (field_ptr && *field_ptr == 'W') {
                current_gps_data.longitude = -current_gps_data.longitude;
            }
            
            field_ptr = get_nmea_field(gga_ptr, 6);
            if (field_ptr) {
                current_gps_data.fix_qual = atoi(field_ptr);
            }
            
            field_ptr = get_nmea_field(gga_ptr, 7);
            if (field_ptr) {
            	current_gps_data.sats = atoi(field_ptr);
            }

            field_ptr = get_nmea_field(gga_ptr, 8);
            if (field_ptr) {
                current_gps_data.hdop = (uint16_t)(atof(field_ptr) * 1e2);
            }
            
            field_ptr = get_nmea_field(gga_ptr, 9);
            if (field_ptr) {
                current_gps_data.altitude = (int)(atof(field_ptr) * 1e1);
            }

        }

        /* Process $GPRMC if present in the buffer */
        if (rmc_ptr != NULL) {
            
            field_ptr = get_nmea_field(rmc_ptr, 7);
            if (field_ptr) {
                current_gps_data.speed = (int)(atof(field_ptr) * 51.44);
            }
            
            field_ptr = get_nmea_field(rmc_ptr, 8);
            if (field_ptr) {
                current_gps_data.course = atoi(field_ptr);
            }

        }

        gps_data_ready = 0;
    }

}
