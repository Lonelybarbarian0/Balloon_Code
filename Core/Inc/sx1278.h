/*
 * sx1278.h
 *
 *  Created on: May 14, 2026
 *      Author: kailas
 */

#ifndef INC_SX1278_H_
#define INC_SX1278_H_

#endif /* INC_SX1278_H_ */

/* Includes Begin */
#include "main.h"
/* Includes End */

/* Extern Begin */
extern SPI_HandleTypeDef hspi1;
/* Extern End */


/*
 * @Note: Refer page 93 onwards from the datasheet to get reg description
 */
/* Macros Begin */

//Registers for FSK mode only
#define RegFifo				0x00 //Used to access the rx/tx shared FIFO
#define RegOpMode			0x01 //Used to select the operating mode - LoRa/FSK/OOK (pg 54,55)
#define RegBitrateMsb		0x02 //Bit rate MSB - refer to pg 46,47
#define RegBitrateLsb		0x03 //Bit rate LSB - refer to pg 46,47
#define RegFdevMsb			0x04 //Freq dev setting MSB
#define RegFdevLsb			0x05 //Freq dev setting MSB
#define RegFrfMsb			0x06 //RF Carrier Frequency, Most Significant Bits
#define RegFrfMid			0x07 //RF Carrier Frequency, Intermediate Bits
#define RegFrfLsb			0x08 //RF Carrier Frequency, Least Significant Bits
#define RegPaConfig			0x09 //PA selection and Output Power control
#define RegPaRamp			0x0A //Control of PA ramp time, low phase noise PLL
#define RegOcp				0x0B //Over Current Protection control
#define RegLna				0x0C //LNA settings
#define RegRxConfig			0x0D //AFC, AGC, ctr
#define RegRssiConfig		0x0E //RSSI
#define RegRssiCollision	0x0F //RSSI Collision detector
#define RegRssiThresh		0x10 //RSSI Threshold control
#define RegRssiValue		0x11 //RSSI value in dBm
#define RegRxBw				0x12 //Channel Filter BW Control
#define RegAfcBw			0x13 //AFC Channel Filter BW
#define RegAfcFei			0x1A //AFC and FEI control
#define RegAfcMsb			0x1B //Frequency correction value of the AFC MSB
#define RegAfcLsb			0x1C //Frequency correction value of the AFC LSB
#define RegFeiMsb			0x1D //Value of the calculated frequency error MSB
#define RegFeiLsb			0x1E //Value of the calculated frequency error LSB
#define RegPreambleDetect 	0x1F //Settings of the Preamble Detector
#define RegRxTimeout1		0x20 //Timeout Rx request and RSSI
#define RegRxTimeout2		0x21 //Timeout RSSI and PayloadReady
#define RegRxTimeout3		0x22 //Timeout RSSI and SyncAddress
#define RegRxDelay			0x23 //Delay between Rx cycles
#define RegOsc				0x24 //RC Oscillators Settings, CLKOUT frequency
#define RegPreambleMsb		0x25 //Preamble length, MSB
#define RegPreambleLsb		0x26 //Preamble length, LSB
#define RegSyncConfig		0x27 //Sync Word Recognition control
#define RegSyncValue1 		0x28 //Sync Word bytes 1
#define RegSyncValue2 		0x29 //Sync Word bytes 2
#define RegSyncValue3 		0x2A //Sync Word bytes 3
#define RegSyncValue4		0x2B //Sync Word bytes 4
#define RegSyncValue5		0x2C //Sync Word bytes 5
#define RegSyncValue6		0x2D //Sync Word bytes 6
#define RegSyncValue7		0x2E //Sync Word bytes 7
#define RegSyncValue8		0x2F //Sync Word bytes 8
#define RegPacketConfig1	0x30 //Packet mode settings
#define RegPacketConfig2	0x31 //Packet mode settings
#define RegPayloadLength	0x32 //Payload length setting
#define RegNodeAdrs			0x33 //Node address
#define RegBroadcastAdrs	0x34 //Broadcast address
#define RegFifoThresh		0x35 //Fifo threshold, Tx start condition
#define RegSeqConfig1		0x36 //Top level Sequencer settings
#define RegSeqConfig2		0x37 //Top level Sequencer settings
#define RegTimerResol		0x38 //Timer 1 and 2 resolution control
#define RegTimer1Coef		0x39 //Timer 1 setting
#define RegTimer2Coef		0x3A //Timer 2 setting
#define RegImageCal			0x3B //Image calibration engine control
#define RegTemp				0x3C //Temperature Sensor value
#define RegLowBat			0x3D //Low Battery Indicator Settings
#define RegIrqFlags1		0x3E //Status register: PLL Lock state,Timeout, RSSI
#define RegIrqFlags2		0x3F //Status register: FIFO handling flags, Low Battery
#define RegDioMapping1		0x40 //Mapping of pins DIO0 to DIO3
#define RegDioMapping2		0x41 //Mapping of pins DIO4 and DIO5, ClkOut frequency
#define RegVersion			0x42 //Semtech ID relating the silicon revision
#define RegPllHop			0x44 //Control the fast frequency hopping mode
#define RegTcxo				0x4B //TCXO or XTAL input setting
#define RegPaDac			0x4D //Higher power settings of the PA
#define RegFormerTemp		0x5B //Stored temperature during the former IQ Calibration
#define RegBitRateFrac		0x5D //Fractional part in the Bit Rate division ratio
#define RegAgcRef			0x61 //Adjustment of the AGC thresholds
#define RegAgcThresh1		0x62 //Adjustment of the AGC thresholds
#define RegAgcThresh2		0x63 //Adjustment of the AGC thresholds
#define RegAgcThresh3		0x64 //Adjustment of the AGC thresholds
#define RegPll				0x70 //Control of the PLL bandwidth

//Paramters
#define FXOSC 32000000 //32Mhz crystal

//Modulation
#define FSK  	0x00
#define OOK 	0x01
#define LORA    0x02

//Op Modes
#define SLEEP_MODE		0x00
#define STANDBY_MODE	0x01
#define FSTX			0x02
#define TX				0x03
#define FSRX			0x04
#define RX				0x05

//LNA Gain
#define G1 0x01 //highest gain
#define G2 0x02 //highest gain – 6 dB
#define G3 0x03 //highest gain – 12 dB
#define G4 0x04 //highest gain – 24 dB
#define G5 0x05 //highest gain – 36 dB
#define G6 0x06 //highest gain – 48 dB

//RX Trigger
#define RX_TRIGGER_RSSI          0x01
#define RX_TRIGGER_PREAMBLE      0x06
#define RX_TRIGGER_RSSI_PREAMBLE 0x07


//void sx1278_RXConfig() to turn AGC and AFC ON and OFF
#define AGC_ON 		0x01
#define AGC_OFF 	0x00
#define AFC_ON		0x01
#define AFC_OFF		0x00

//void sx1278_PacketFormat(uint8_t mode) - to config packet mode
#define FIXED_LENGTH 		0x00
#define VARIABLE_LENGTH		0x01

//void sx1278_DCFreeEnc(uint8_t enc) - to config encodiing
#define NO_ENCODING 		0x00
#define MANCHESTER_ENCODING 0x01
#define WHITENING			0x02

//void sx1278_CRCConfig(uint8_t crc) - CRC ON/OFF
#define CRC_OFF 0x00
#define CRC_ON	0x01

//void sx1278_DataMode(uint8_t mode) - Data Mode
#define CONTINUOUS_MODE 0x00
#define PACKET_MODE		0x01

//void sx1278_StartTXCondition(uint8_t condition, uint8_t threshold) - FIFO level or fifo > 1
#define FIFO_LEVEL 		0x00
#define FIFO_NOT_EMPTY	0x01

//Err codes for sx1278config fn
#define SX1278_TIMEOUT 	0x00
#define SX1278_OK		0x01

/* Macros End */

/* Function Prototypes Begin */
void CS_HIGH();
void CS_LOW();
HAL_StatusTypeDef SX1278_WriteReg(uint8_t addr, uint8_t data);
HAL_StatusTypeDef SX1278_ReadReg(uint8_t addr, uint8_t *data);
void sx1278_Reset(void);
void sx1278_SetFrequency(uint32_t freq);
void sx1278_SetDataRate(uint32_t data_rate);
void sx1278_SetFreqDev(uint32_t f_dev, uint32_t data_rate);
void sx1278_SetModulation(uint8_t mod);
void sx1278_OpMode(uint8_t mode);
void sx1278_SetMaxPower(void);
void sx1278_SetGaussianBT05(void);
void sx1278_MaxLNAGain(void);
void sx1278_RXConfig(uint8_t afc , uint8_t agc);
void sx1278_RSSIThreshold(int8_t tresh);
int16_t sx1278_GetRSSI(void);
void sx1278_SetRxBW_50Khz(void);
void sx1278_SetAfcBW_50Khz(void);
void sx1278_PreambleDetect(void);
void sx1278_SyncTimeoutInr(uint8_t TimeoutSignalSync);
void sx1278_SetPreambleSize(uint16_t size);
void sx1278_SyncOn(void);
void sx1278_SetSyncValue(const uint8_t *value, uint8_t size);
void sx1278_PacketFormat(uint8_t mode);
void sx1278_DCFreeEnc(uint8_t enc);
void sx1278_CRCConfig(uint8_t crc);
void sx1278_DataMode(uint8_t mode);
void sx1278_PayloadLength(uint16_t len);
void sx1278_StartTXCondition(uint8_t condition, uint8_t threshold);
void sx1278_DIO0(void);
void sx1278_DIO1(void);
void sx1278_DIO2(void);
void sx1278_DIO3(void);
void sx1278_DIO4(void);
void sx1278_DIO5(void);
uint8_t sx1278_ReadVersion(void);
void sx1278_WriteFIFO(uint8_t *data, uint8_t len);
void sx1278_ReadFIFO(uint8_t *data, uint8_t len);
void sx1278_DI00IntrHandler(void);
uint8_t sx1278_DIO5IntrHandler(void);
uint8_t sx1278_WaitForModeReady(uint8_t mode);
void sx1278_WaitForTxDone(void);
uint8_t sx1278_Config(void);
/* Function Prototypes End */
