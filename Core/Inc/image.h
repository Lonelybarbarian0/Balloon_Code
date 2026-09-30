/*
 * image.h
 *
 *  Created on: Jun 8, 2026
 *      Author: kaila
 */

#ifndef INC_IMAGE_H_
#define INC_IMAGE_H_

#endif /* INC_IMAGE_H_ */


#include "main.h"

#define JPEG_PAYLOAD_SIZE 58 //59 bytes

#define ID_SIZE 2
#define CRC_SIZE 2
#define JPEG_IMAGE 0x05

#define RGB_IMAGE  0x10


uint16_t JEPG_SIZE(void);
uint16_t FINAL_JPEG_ID(void);
void Packet_JPEG(uint16_t ID, uint8_t **img, uint16_t *size);
