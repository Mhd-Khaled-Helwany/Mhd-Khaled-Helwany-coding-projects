/*
 * i2c.c
 *
 * Created: 2025-11-05 09:34:46
 *  Author: ferpe211
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include "i2c.h"

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
TWBR = ((16 000 000/100 000) -16)/(2*4^2) = 4.5 (rounded to 4)


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

//receive buffer
volatile uint8_t rx_buffer[16]; //Buffer size temporary, subject to change
volatile uint8_t rx_index=0;
volatile uint8_t rx_done=0; //1 when data has been received

//transmit buffer
volatile uint8_t tx_buffer[16]; //Buffer size temporary, subject to change
volatile uint8_t tx_index=0;
volatile uint8_t tx_length=0;


void I2C_init(uint8_t address){	
	// Disable internal pull-ups on TWI pins
	PORTC=0xFF;
	
	// Set slave address
	TWAR = (address << 1);
	
	// Enable TWI with ACK, interrupts, and clear TWINT
	TWCR = (1<<TWIE) | (1<<TWEA) | (1<<TWEN) | (1<<TWINT);
	
	// Enable global interrupts
	sei();
}

void write_response(uint8_t *data, uint8_t length) {
	uint8_t minLength = (length < sizeof(tx_buffer)) ? length : sizeof(tx_buffer);
	for (uint8_t i = 0; i < minLength; i++) {
		tx_buffer[i] = data[i];
	}
	tx_length = minLength;
}


ISR(TWI_vect){
	switch(TWSR & 0xF8){ //masks the TWI Status bits
		//Receiving data from master (SLA+W)
		case TW_SR_SLA_ACK: //Own SLA+W received, ACK sent          -> Ready to receive data byte
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
			rx_index = 0;
			break;
		case TW_SR_DATA_ACK:
			if(rx_index < sizeof(rx_buffer)){ 
				rx_buffer[rx_index++] = TWDR; //writes current data to the rx buffer
			}
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE); 
			break;
		case TW_SR_DATA_NACK:
		case TW_BUS_ERROR:
		case TW_SR_STOP:
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
			rx_done = 1; //finished receiving data
			break;
			
			
		//Sending data to master (SLA+R)
		case TW_ST_SLA_ACK:
			tx_index = 0;
			TWDR = tx_buffer[tx_index++]; //writes first byte from tx_buffer to the TWI data register
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE); 
			break;
		case TW_ST_DATA_ACK:
			if(tx_index < tx_length){
				TWDR = tx_buffer[tx_index++];
			}else{
				TWDR = 0x00; //Default data if index >= buffer length
			}
			TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE);
			break;
		default:
		    TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE); //resets TWI on unexpected
		    break;
		
	}
	
}