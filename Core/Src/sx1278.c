/*
 * sx1278.c
 *
 *  Created on: May 14, 2026
 *      Author: kailas
 */

/* Includes Begin */
#include "sx1278.h"
#include "telemetry.h"

/* Includes End */

volatile uint8_t CURRENT_STATE = 0x00;

volatile uint8_t TX_DONE = 0x01;
volatile uint8_t RX_DONE = 0x00;

volatile uint8_t DIO0_INTR_FLAG;
volatile uint8_t DIO5_INTR_FLAG;

void CS_HIGH(void)
{
	HAL_GPIO_WritePin(SX1278_CS_GPIO_Port, SX1278_CS_Pin, GPIO_PIN_SET);
}

void CS_LOW(void)
{
	HAL_GPIO_WritePin(SX1278_CS_GPIO_Port, SX1278_CS_Pin, GPIO_PIN_RESET);
}

void sx1278_Reset(void)
{
    HAL_GPIO_WritePin(SX1278_RST_GPIO_Port, SX1278_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);    // Hold RESET low (1-10 ms is sufficient)

    HAL_GPIO_WritePin(SX1278_RST_GPIO_Port, SX1278_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(10);   // Wait for the SX1278 to start up
}

HAL_StatusTypeDef SX1278_WriteReg(uint8_t addr, uint8_t data)
{
    uint8_t tx[2];
    HAL_StatusTypeDef HAL_STATUS = HAL_ERROR;

    tx[0] = addr | (0x80);  // Write bit
    tx[1] = data;

    CS_LOW();

    HAL_STATUS = HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);

    CS_HIGH();

    return HAL_STATUS;
}

HAL_StatusTypeDef SX1278_ReadReg(uint8_t addr, uint8_t *data)
{
    uint8_t tx[2];
    uint8_t rx[2];
    HAL_StatusTypeDef HAL_STATUS = HAL_ERROR;

    tx[0] = addr & 0x7F; // Read bit = 0
    tx[1] = 0x00;        // Dummy byte

    CS_LOW();

    HAL_STATUS = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, HAL_MAX_DELAY);

    CS_HIGH();

    *data = rx[1];

    return HAL_STATUS;
}

void sx1278_SetModulation(uint8_t mod)
{
    uint8_t data;

    SX1278_ReadReg(RegOpMode, &data);

    // Go to Sleep if not already
    if ((data & 0x07) != 0x00)
    {
        data &= 0xF8;
        SX1278_WriteReg(RegOpMode, data);
        HAL_Delay(10);
    }

    if (mod == FSK)
    {
        data &= 0x9F;           // ModulationType = 00
    }
    else if (mod == OOK)
    {
        data &= 0x9F;           // Clear ModulationType
        data |= 0x20;           // ModulationType = 01
    }

    SX1278_WriteReg(RegOpMode, data);
    HAL_Delay(10);
}

/*
 *
 * Refer pag 47 Table 19
 */
void sx1278_SetDataRate(uint32_t data_rate)
{
    if(data_rate == 4800)
    {
        SX1278_WriteReg(RegBitrateMsb, 0x1A);

        SX1278_WriteReg(RegBitrateLsb, 0x0B);
    }
    else
    {
        uint16_t DR = 0x0000;

        DR = FXOSC / data_rate;

        SX1278_WriteReg(RegBitrateMsb,(uint8_t)(DR >> 8));

        SX1278_WriteReg(RegBitrateLsb,(uint8_t)(DR));
    }
}

void sx1278_SetFreqDev(uint32_t f_dev, uint32_t data_rate)
{
    if ((f_dev + (data_rate / 2)) <= 250000)
    {
        uint16_t F_DEV;
        uint8_t reg;

        F_DEV = (uint16_t)(((uint64_t)f_dev * 0x80000ULL) / FXOSC);

        /* Preserve reserved bits 7:6 */
        SX1278_ReadReg(RegFdevMsb, &reg);
        reg &= 0xC0;                     // Keep bits 7:6
        reg |= (F_DEV >> 8) & 0x3F;      // Set bits 5:0
        SX1278_WriteReg(RegFdevMsb, reg);

        /* All bits are used in LSB register */
        SX1278_WriteReg(RegFdevLsb, (uint8_t)F_DEV);
    }
}

void sx1278_SetFrequency(uint32_t freq) //freq can be from 137 to 525Mhz
{
    if ((freq >= 137000000UL) && (freq <= 525000000UL))
    {
        uint32_t F_RF = 0;

        F_RF = ((uint64_t)freq * 0x80000ULL) / FXOSC;

        SX1278_WriteReg(RegFrfMsb, (uint8_t)(F_RF >> 16));

        SX1278_WriteReg(RegFrfMid, (uint8_t)(F_RF >> 8));

        SX1278_WriteReg(RegFrfLsb, (uint8_t)(F_RF));
    }
}

void sx1278_OpMode(uint8_t mode)
{
    uint8_t data;

    SX1278_ReadReg(RegOpMode, &data);

    data &= 0xF8; // Clear mode bits [2:0]

    data |= (mode & 0x07); // Set new mode

    SX1278_WriteReg(RegOpMode, data);
}

/* ************************* TRANSMITTER REGISTERS **************************** */

void sx1278_SetMaxPower(void)
{
    uint8_t reg;

    // PA_BOOST + OutputPower = 15 (17 dBm)
    SX1278_ReadReg(RegPaConfig, &reg);
    reg &= 0x70;          // Preserve MaxPower bits
    reg |= 0x8F;          // PA_BOOST + OutputPower = 15
    SX1278_WriteReg(RegPaConfig, reg);

    // Enable +20 dBm PA DAC
    SX1278_ReadReg(RegPaDac, &reg);
    reg &= 0xF8;
    reg |= 0x07;
    SX1278_WriteReg(RegPaDac, reg);

    // OCP = 240 mA
    SX1278_WriteReg(RegOcp, 0x3B);
}

void sx1278_SetGaussianBT05(void)
{
    uint8_t data;

    SX1278_ReadReg(RegPaRamp, &data);

    data &= 0x9F;      // Clear bits 6:5, preserve others
    data |= 0x40;      // Set ModulationShaping = 10 (BT = 0.5)

    SX1278_WriteReg(RegPaRamp, data);
}

/* ************************ Registers for the receiver *************************** */

void sx1278_RXConfig(uint8_t afc, uint8_t agc)
{
    uint8_t data;

    SX1278_ReadReg(RegRxConfig, &data);

    // Restart RX automatically after saturation/collision
    data |= (1 << 7);

    // Configure AFC
    if (afc)
        data |= (1 << 4);
    else
        data &= ~(1 << 4);

    // Configure AGC
    if (agc)
        data |= (1 << 3);
    else
        data &= ~(1 << 3);

    // Configure RxTrigger
    data &= ~0x07;                       // Clear bits 2:0
    data |= RX_TRIGGER_PREAMBLE;    // 0x07

    SX1278_WriteReg(RegRxConfig, data);
}


// something between -120 for noise to -100 for strong
void sx1278_RSSIThreshold(int8_t tresh)
{
	uint8_t data;

	data = (-tresh) / 2;

	SX1278_WriteReg(RegRssiThresh, data);
}

int16_t sx1278_GetRSSI(void)
{
    uint8_t data;

    if (SX1278_ReadReg(RegRssiValue, &data) != HAL_OK)
        return -127;

    return -(int16_t)data / 2;
}


void sx1278_SetRxBW_50Khz(void) //refer pg 88 in datasheet
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegRxBw, &data);

	data = data & 0xE0;
	data = data | 0x08; //mantissa set to 20
	data = data | 0x03; //exponent set to 3

	SX1278_WriteReg(RegRxBw, data);
}

void sx1278_SetAfcBW_50Khz(void)
{
    uint8_t data = 0x00;

    SX1278_ReadReg(RegAfcBw, &data);

    data &= 0xE0; // preserve reserved bits

    data |= 0x08; // mantissa = 20
    data |= 0x03; // exponent = 3

    SX1278_WriteReg(RegAfcBw, data);
}

void sx1278_AFCAutoClearEnable(void)
{
    uint8_t data;

    SX1278_ReadReg(RegAfcFei, &data);
    data |= (1 << 0);          // AfcAutoClearOn = 1
    SX1278_WriteReg(RegAfcFei, data);
}

void sx1278_PreambleDetect(void)
{
    uint8_t data = 0;

    data |= (1 << 7);      // Enable preamble detector
    data |= (2 << 5);      // Preamble detector size = 3 bytes
    data |= 0x08;          // Allow up to 8 chip errors

    SX1278_WriteReg(RegPreambleDetect, data);
}


//void sx1278_SyncTimeoutInr(uint8_t TimeoutSignalSync) //refer pg 98
//{
//	SX1278_WriteReg(RegRxTimeout3, TimeoutSignalSync);
//	/* Timeout interrupt is generated TimeoutSignalSync*16*Tbit after
//	the Rx mode is programmed, if SyncAddress doesn’t occur
//	0x00: TimeoutSignalSync is disabled */
//}

void sx1278_SetPreambleSize(uint16_t size) //refer pg 99 (size = no of bytes of pre)
{
	SX1278_WriteReg(RegPreambleMsb, (uint8_t)(size>>8)); //Preamble size MSB

	SX1278_WriteReg(RegPreambleLsb, (uint8_t)size); //preamble size LSB
}


void sx1278_SyncOn(void)
{
    uint8_t data = 0x00;

    SX1278_ReadReg(RegSyncConfig, &data);

    data &= 0x3F; //clear bits 7 and 6
    data |= 0x40; // Auto Restart Rx mode = 01 (On, without PLL re-lock)

    data &= 0xDF; // Preamble polarity = 0xAA

    data |= 0x10; // SyncOn = 1

    // Clear bits [2:0] completely before writing the size
    data &= 0xF8;
    data |= 0x01; // 0x01 means 2 bytes (1 + 1)


    SX1278_WriteReg(RegSyncConfig, data);
}

void sx1278_SetSyncValue(const uint8_t *value, uint8_t size)
{
    if ((size >= 1) && (size <= 8))
    {
        for (uint8_t i = 0; i < size; i++)
        {
            SX1278_WriteReg(RegSyncValue1 + i, value[i]);
        }
    }
}

void sx1278_PacketFormat(uint8_t mode)
{
	uint8_t data;

	SX1278_ReadReg(RegPacketConfig1, &data);

	data &= 0x7F;

	if(mode == FIXED_LENGTH)
	{
		data = data & 0x7F;//sets to Fixed length packet mode
	}
	else if(mode == VARIABLE_LENGTH)
	{
		data = data | 0x80;//sets to variable length packet mode
	}

	if((mode == FIXED_LENGTH) || (mode == VARIABLE_LENGTH))
		SX1278_WriteReg(RegPacketConfig1, data);
}

void sx1278_DCFreeEnc(uint8_t enc)
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegPacketConfig1, &data);
	data = data & 0x9F;

	if(enc == MANCHESTER_ENCODING)
	{
		data = data | 0x20;
	}
	else if(enc == WHITENING)
	{
		data = data | 0x40;
	}

	SX1278_WriteReg(RegPacketConfig1, data);
}

void sx1278_CRCConfig(uint8_t crc) //refer pg 100 in datasheet
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegPacketConfig1, &data);

	if(crc == CRC_OFF)
	{
		data = data & 0xEF; //NO CRC Calculation or Decoding
	}
	else if(crc == CRC_ON)
	{
		data = data & 0xEF;
		data = data | 0x10; //CRC ENABLED
		data = data | 0x08; //After CRC fail, packet is not cleared from FIFO and Intr is triggered
		//data = data & 0xFE; //CCITT CRC implementation with standard whitening
	}

	data = data & 0xF9; //Addr based filtering OFF

	SX1278_WriteReg(RegPacketConfig1, data);
}

void sx1278_DataMode(uint8_t mode)
{
    uint8_t data;

    SX1278_ReadReg(RegPacketConfig2, &data);

    if (mode == CONTINUOUS_MODE)
    {
        data &= 0xBF;      // Clear bit 6
    }
    else if (mode == PACKET_MODE)
    {
        data |= 0x40;      // Set bit 6
    }

    SX1278_WriteReg(RegPacketConfig2, data);
}

void sx1278_PayloadLength(uint16_t len) //refer pg 100 in datasheet
{
	if(len <= 2047)
	{
		uint8_t data = 0x00;

		SX1278_ReadReg(RegPacketConfig2, &data);

		data = data & 0xF8;
		data = data | ((len >> 8) & 0x07);

		SX1278_WriteReg(RegPacketConfig2, data);

		SX1278_WriteReg(RegPayloadLength, (uint8_t)len);
	}
}

void sx1278_StartTXCondition(uint8_t condition, uint8_t threshold)
{
    uint8_t data;

    SX1278_ReadReg(RegFifoThresh, &data);

    // Configure TxStartCondition (bit 7)
    if (condition == FIFO_NOT_EMPTY)
        data |= (1 << 7);
    else
        data &= ~(1 << 7);

    // Configure FIFO threshold (bits 5:0)
    threshold &= 0x3F;
    data &= ~0x3F;          // Clear bits 5:0
    data |= threshold;

    SX1278_WriteReg(RegFifoThresh, data);
}


//DIO config refer pg 105 and pg 69 table 30 in datasheet
void sx1278_DIO0(void) //Used to know if Payload Received is ready or Packet tx is sent
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegDioMapping1, &data);

	data = data & 0x3F; //Payload Ready in rx and Packet Sent in tx for DIO0

	SX1278_WriteReg(RegDioMapping1, data);
}

void sx1278_DIO1(void) //FIFO empty flag
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegDioMapping1, &data);

	data = data & 0xCF;
	data = data | 0x10;//FIFO empty flag enabled

	SX1278_WriteReg(RegDioMapping1, data);
}

void sx1278_DIO2(void) // SyncAddress interrupt
{
    uint8_t data;

    SX1278_ReadReg(RegDioMapping1, &data);

    data &= 0xF3;      // Clear bits 3:2
    data |= 0x0C;      // Set bits 3:2 = 11 (SyncAddress)

    SX1278_WriteReg(RegDioMapping1, data);
}

void sx1278_DIO3(void) //TX Ready Flag
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegDioMapping1, &data);

	data = data & 0xFC;
	data = data | 0x01;//TX Ready Enabled

	SX1278_WriteReg(RegDioMapping1, data);
}

void sx1278_DIO4(void) //Preambledetect Intr
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegDioMapping2, &data);

	data = data & 0x3F;
	data = data | 0xC0;//Sets Preambledetect Intr

	SX1278_WriteReg(RegDioMapping2, data);
}

void sx1278_DIO5(void) //Mode Ready for RX and TX Flag
{
	uint8_t data = 0x00;

	SX1278_ReadReg(RegDioMapping2, &data);

	data = data & 0xCF;
	data = data | 0x30;//Sets Mode Ready for RX and TX

	SX1278_WriteReg(RegDioMapping2, data);
}

uint8_t sx1278_ReadVersion(void)
{
	uint8_t data = 0x00;
	SX1278_ReadReg(RegVersion, &data);

	return data;
}

void sx1278_WriteFIFO(uint8_t *data, uint8_t len)
{
    uint8_t addr = RegFifo | 0x80;

    CS_LOW();

    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, data, len, HAL_MAX_DELAY);

    CS_HIGH();
}

void sx1278_ReadFIFO(uint8_t *data, uint8_t len)
{
    uint8_t addr = RegFifo & 0x7F;

    CS_LOW();

    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, data, len, HAL_MAX_DELAY);

    CS_HIGH();
}

void sx1278_DI00IntrHandler(void)
{
	if(DIO0_INTR_FLAG == 1)
	{
		uint8_t data = 0x00;

		SX1278_ReadReg(RegIrqFlags2, &data);

		if( (data & 0x08) == 0x08) //Packet Sent Flag
		{
			TX_DONE = 1;
		}

		if( (data & 0x04) == 0x04) //Payload Ready Flag
		{
			RX_DONE = 1;
		}

		DIO0_INTR_FLAG = 0;
	}
}

uint8_t sx1278_DIO5IntrHandler(void)
{
    if (DIO5_INTR_FLAG == 0)
        return 0xFF;

    DIO5_INTR_FLAG = 0;

    uint8_t data = 0;

    SX1278_ReadReg(RegOpMode, &data);

    return (data & 0x07);
}

uint8_t sx1278_WaitForModeReady(uint8_t mode)
{
	uint32_t start = HAL_GetTick();

    while ((HAL_GetTick() - start) < 30)
    {
        uint8_t op;

        SX1278_ReadReg(RegOpMode, &op);

        if ((op & 0x07) == mode)
            return 1;
    }

    return 0;           // Timed out
}

void sx1278_WaitForTxDone(void)
{
	uint32_t time = HAL_GetTick();

	while(HAL_GetTick() - time > 250)
	{
		if(TX_DONE == 1)
			break;
	}
}

uint8_t sx1278_Config(void)
{
	/*
	 * freq: 437Mhz
	 * Modulation:GFSK
	 * data rate: 4800bps
	 * Freq Dev: 3845Hz
	 * Set to Max Power
	 * Gaussian Filter: BT is 0.5
	 * preamble size: 8 bytes
	 * Sync: ON
	 * sync word size: 2 bytes
	 * Custom Sync Word
	 * CRC: OFF
	 * RX auto restart: ON
	 * RX Sync Timeout Inr: OFF
	 * DCFree: Whitening Enbaled
	 * DataMode: Packet Mode, not continuos mode
	 * Format: Variable Length Format
	 * Tx Condition: FIFO has atleast one byte and FIFO threshold inr at 32bytes in FIFO
	 * AGC and AFC: Enabled
	 * RX BW: 50kHz
	 * AFC BW: 50kHz
	 * Must specify Payload Length
	 * Use DIO to identify - Payload/Packet Sent/Ready | FIFO EMPTY | Sync Timeout Intr | Tx Ready | Preamble Detect | Mode Ready
	 *
	 *
	 * Change OpMode
	 * Write and Read FIFO
	 * use DIO + some register read to know current state of transiever
	 * Make a state machine to handle Becon + Image data + Retransmission
	 * Make a MCU based timr after preambleDetect to restart rx to prevent it from waiting forever
	 * Implement SW CRC then do RS
	 * Need a state machine to handle tx becon -> rx -> image send command -> image tx -> retransmit command/image send command -> cycle
	 */

	uint8_t Sync_value[2] = {0x67,0x5E};

	uint8_t mode = 0xFF;

	sx1278_Reset();

	sx1278_OpMode(SLEEP_MODE);

	uint32_t start = HAL_GetTick();

	HAL_Delay(10);

	SX1278_ReadReg(RegOpMode, &mode);

	while ((mode & 0x07) != SLEEP_MODE)
	{
	    SX1278_ReadReg(RegOpMode, &mode);

	    if ((HAL_GetTick() - start) >= 200)
	        return SX1278_TIMEOUT;

	    HAL_Delay(10);
	}

	sx1278_SetModulation(FSK); //FSK Modulation

	sx1278_SetDataRate(4800); //4800bps

	sx1278_SetFreqDev(3845,4800); //3845Hz Fdev for 4800bps air rate

	sx1278_SetFrequency(437000000); //437Mhz Carrier Freq

	sx1278_SetMaxPower(); //Enabled 20dbm output power

	sx1278_SetGaussianBT05(); //Sets BT = 0.5 Gaussian Filter Enabled with PA rise time 40us default

	sx1278_RXConfig(AFC_ON, AGC_ON); //Turns OFF AFC and ON AGC

	sx1278_RSSIThreshold(-110);

	sx1278_SetRxBW_50Khz(); //Sets rx BW 50kHz

	sx1278_SetAfcBW_50Khz(); //Sets AFC BW 50kHz

	sx1278_AFCAutoClearEnable();

	sx1278_PreambleDetect(); //Turns on Preamble Detect Intr of 4 bytes and 3 bit err tolerance

	sx1278_SetPreambleSize(8); //8 bytes Preamble size set

	sx1278_SyncOn(); //Turns on sync byte , sync set to 2 bytes , and auto rx restart on

    sx1278_SetSyncValue(Sync_value, sizeof(Sync_value));

	//sx1278_SyncTimeoutInr(20); //Intr occurs when no sync detected after the preamble till 67 ms

    sx1278_PacketFormat(VARIABLE_LENGTH); //Variable length format

    sx1278_DCFreeEnc(NO_ENCODING); //Data Whitening Enabled

	sx1278_CRCConfig(CRC_OFF); //Turned off hardware CRC & addr based filtering off

	sx1278_DataMode(PACKET_MODE); //Packet Mode Enabled

	sx1278_PayloadLength(200); //max length of the Payload in bytes we will be using

	sx1278_StartTXCondition(FIFO_NOT_EMPTY, 1);//Enabled FIFO not empty tx condition and FIFO threshold Intr at 32bytes of data

	sx1278_DIO0(); //Payload Ready Or Packet Sent Intr

	sx1278_DIO1(); //FIFO Empty

	sx1278_DIO2(); //Sync Timeout Interrupt

	sx1278_DIO3(); //TXFS Done

	sx1278_DIO4(); //Preamble detect Intr

	sx1278_DIO5(); //Mode Ready for RX and TX

	sx1278_OpMode(STANDBY_MODE);

	start = HAL_GetTick();

	HAL_Delay(10);

	SX1278_ReadReg(RegOpMode, &mode);

	while ((mode & 0x07) != STANDBY_MODE)
	{
	    SX1278_ReadReg(RegOpMode, &mode);

	    if ((HAL_GetTick() - start) >= 200)
	        return SX1278_TIMEOUT;

	    HAL_Delay(10);
	    sx1278_OpMode(STANDBY_MODE);
	    HAL_Delay(10);
	}

	return SX1278_OK;
}
