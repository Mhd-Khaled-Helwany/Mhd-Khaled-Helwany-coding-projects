/*
This file contains methods for the sensor module avr to send and recive data via i2c 
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <string.h>
#include "i2c.h"
#include "../sensor/Globals.h"
#include "../sensor/gyrossensor.h"


#define ADDR 0x10 //address from master ??

// Implementation inspired by https://github.com/kelvinlawson/avr311-twi-slave-gcc/blob/master/standard/TWI_slave.c

/*
TWINT � Set when the TWI operation is complete; cleared by writing a 1 to it.

TWEA � Enables sending an ACK after receiving a byte.

TWSTA � Sends a START condition on the bus when set.

TWSTO � Sends a STOP condition on the bus when set.

TWWC � Indicates that the TWI data register (TWDR) cannot be written because transmission is in progress.

TWEN � Enables the TWI interface and activates the SDA/SCL pins.

TWIE � Enables the TWI interrupt when set.


TWSR � TWI Status Register
TWDR � TWI Data Register

standard i2c clck speed 100 kHz


SLA_W : Slave Address + Write bit


*/

/**
 * TWI Slave Mode Status Codes (TWSR bits 7:3; mask prescaler bits with 0xF8)
 *
 * 0x60  Own SLA+W received, ACK sent          -> Ready to receive data byte
 * 0x68  Arbitration lost as Master; SLA+W received -> Ready to receive data
 * 0x70  General call received, ACK sent       -> Ready to receive data
 * 0x78  Arbitration lost as Master; GCA received -> Ready to receive data
 * 0x80  Previously addressed (SLA+W); data received, ACK sent -> Receive next data byte
 * 0x88  Previously addressed (SLA+W); data received, NACK sent -> Switch to not-addressed slave mode
 * 0x90  General call; data received, ACK sent -> Receive next data byte
 * 0x98  General call; data received, NACK sent -> Switch to not-addressed slave mode
 * 0xA0  STOP or repeated START received while addressed -> Switch to not-addressed slave mode
 *
 * 0xA8  Own SLA+R received, ACK sent          -> Load data byte to transmit
 * 0xB0  Arbitration lost as Master; SLA+R received -> Load data byte to transmit
 * 0xB8  Data byte transmitted, ACK received  -> Load next data byte
 * 0xC0  Data byte transmitted, NACK received -> Switch to not-addressed slave mode
 * 0xC8  Last data byte transmitted (TWEA=0), ACK received -> Switch to not-addressed slave mode
 *
 * Notes:
 * - TWINT must be cleared to continue operations.
 * - TWEA enables ACK when receiving data.
 * - TWSTO triggers STOP condition.
 * - TWDR contains data to be transmitted/received.
 * - Always mask TWSR with 0xF8 to check status independent of prescaler.
 */

// buffer
volatile uint8_t tx_buffer[I2C_BUFFER_SIZE]; 
volatile uint8_t rx_buffer[I2C_BUFFER_SIZE];


volatile uint8_t i2c_busy=0;

volatile uint8_t i2c_message_length = 0;

volatile uint8_t i2c_last_success = 0;

volatile uint8_t i2c_incoming_data = 0;


void i2c_init(void){
	// Disable internal pull-ups on TWI pins
	PORTC=0xFF;
	
	// Set slave address
	TWAR = (ADDR << 1);
	
	// Enable TWI with ACK, interrupts, and clear TWINT
	TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWEN) | (1<<TWINT);
	
	// Enable global interrupts
	sei();
}

void i2c_restart(void)
{
	while (i2c_busy) {}
	TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
	i2c_busy = 0;
}

void i2c_write_response(uint8_t *data, uint8_t length)
{
	uint8_t minLength = (length < sizeof(tx_buffer)) ? length : sizeof(tx_buffer);
	for (uint8_t i = 0; i < minLength; i++) {
		tx_buffer[i] = data[i];
	}
	i2c_message_length = minLength;
	
	TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
	i2c_busy = 1;
}

void i2c_read_response(uint8_t *data, uint8_t length)
{
	while (i2c_busy) {}
	
	if(i2c_last_success)
	{
		for (uint8_t i = 0; i < length; i++)
		{
			data[i] = rx_buffer[i];
		}
		i2c_incoming_data = 0;
	}
}

void I2C_FillTxBuffer(uint8_t command);

ISR(TWI_vect)
{
	static uint8_t i2c_index;

	switch(TWSR & 0xF8) // Masks the TWI Status bits
	{
		// Receiving data from master (SLA+W)
		case TW_SR_SLA_ACK: // Own SLA+W received, ACK sent -> Ready to receive data byte			
			i2c_incoming_data = 1;
			i2c_index = 0;
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
			i2c_busy = 1;
			break;
		case TW_SR_DATA_ACK:
			rx_buffer[i2c_index++] = TWDR; // writes current data to the rx buffer
			i2c_last_success = 1;
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE); 
			i2c_busy = 1;
			break;
		case TW_SR_STOP:
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
			i2c_busy = 0;
			I2C_FillTxBuffer(rx_buffer[0]);
			break;
		case TW_SR_DATA_NACK:
		case TW_ST_LAST_DATA:
		case TW_BUS_ERROR:
			TWCR = (1<<TWSTO)|(1<<TWINT); 
			break;
		// Sending data to master (SLA+R)
		case TW_ST_SLA_ACK:
			i2c_index = 0;  // Fall through
		case TW_ST_DATA_ACK:
			TWDR = tx_buffer[i2c_index++];
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
			i2c_busy = 1;
			break;
		case TW_ST_DATA_NACK:
			if (i2c_index == i2c_message_length)
			{
				i2c_last_success = 1;
			}
			else
			{
				// 
			}
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
			i2c_busy = 0;
			break;
		
		default:
		    TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE); // Resets TWI on unexpected
			i2c_busy = 0;
		    break;
		
	}
	
}

// Op codes for what type of data we are sending
#define CMD_ULTRASOUND 0x04
#define CMD_TEST 0x05
#define CMD_GYRO_STATE 0x06
#define CMD_GYRO_RESET 0x07
#define CMD_GYRO_RAW 0x08
#define CMD_READ_TAPE_SENSOR_DATA 0x09 // Currently unused
#define CMD_GYRO_ANGLE_RESET 0x0A
#define CMD_GYRO_GET_ANGLE 0x0B
#define CMD_READ_TAPE_SENSOR_FRONT 0x0C
#define CMD_READ_TAPE_SENSOR_BACK  0x0D
#define CMD_ENABLE_TAPE_SENSOR 0x0E
#define CMD_ENABLE_GYROS 0x0F
#define CMD_ENABLE_ULTRASOUND 0x10
#define CMD_DISABLE_TAPE_SENSOR 0x11
#define CMD_DISABLE_GYROS 0x12
#define CMD_DISABLE_ULTRASOUND 0x13


// Fills buffer with data making it ready to be sent
void I2C_FillTxBuffer(uint8_t command)
{
	switch (command)
	{
		case CMD_TEST:
		{
			uint8_t dummy_data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
			i2c_write_response(dummy_data, 4);
		}
		break;

		case CMD_GYRO_STATE:
		{
			i2c_write_response((uint8_t *)&GlobalGyroState, 1);
		}
		break;

		case CMD_GYRO_RESET:
		{
			resetRotationDirection(); // TODO: move this?
			uint8_t success = 1; // TODO: Is this really necessary?
			i2c_write_response(&success, 1);
		}
		break;

		case CMD_GYRO_RAW: // Read raw ADC data from the gyro
		{
			i2c_write_response((uint8_t *)&GlobalGyroRaw, sizeof(GlobalGyroRaw));
		}
		break;

		case CMD_GYRO_ANGLE_RESET: // Resets angle and sets a reference point as angle 0
		{
			gyrosResetAngle();
			uint8_t success = 1;
			i2c_write_response(&success, 1);
		}
		break;

		case CMD_GYRO_GET_ANGLE:
		{
			i2c_write_response((uint8_t *)&GlobalGyroAngleInt, sizeof(GlobalGyroAngleInt));
		}
		break;

		case CMD_ULTRASOUND:
		{
			i2c_write_response((uint8_t *)&GlobalUltrasoundDistance, sizeof(GlobalUltrasoundDistance));
		}
		break;

		case CMD_READ_TAPE_SENSOR_FRONT:
		{
			i2c_write_response((uint8_t *)&GlobalTapeSensorValues[1], sizeof(GlobalTapeSensorValues[1]));
		}
		break;

		case CMD_READ_TAPE_SENSOR_BACK:
		{
			i2c_write_response((uint8_t *)&GlobalTapeSensorValues[0], sizeof(GlobalTapeSensorValues[0]));
		}
		break;
		
		case CMD_ENABLE_GYROS:
		{
			GlobalEnableGyros = true;
		}
		break;
		
		case CMD_ENABLE_ULTRASOUND:
		{
			GlobalEnableUltrasound = true;
		}
		break;
		
		case CMD_ENABLE_TAPE_SENSOR:
		{
			GlobalEnableTape = true;
		}
		break;
		
		case CMD_DISABLE_GYROS:
		{
			GlobalEnableGyros = false;
		}
		break;
		
		case CMD_DISABLE_ULTRASOUND:
		{
			GlobalEnableUltrasound = false;
		}
		break;
		
		case CMD_DISABLE_TAPE_SENSOR:
		{
			GlobalEnableTape = false;
		}
		break;

		default:
		{
			// Optional: handle unknown command
		}
		break;
	}
}
