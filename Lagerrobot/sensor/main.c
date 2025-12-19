/*
 * Sensor.c
 *
 * Created: 2025-11-05 09:25:02
 * Author : ferpe211
 */

#define F_CPU 8000000UL
#include <avr/interrupt.h> 
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include "i2c.h"
#include "ultrasoundsensor.h"
#include "gyrossensor.h"
#include "TapeSensor.h"

volatile uint8_t GlobalGyroState;
volatile uint16_t GlobalGyroRaw;
volatile int16_t GlobalGyroAngleInt;

volatile uint16_t GlobalUltrasoundDistance;

volatile bool GlobalEnableGyros = true;
volatile bool GlobalEnableUltrasound = true;
volatile bool GlobalEnableTape = true;

#define LED_COUNT 11
volatile uint16_t GlobalTapeSensorValues[2][LED_COUNT];

#define CMD_ULTRASOUND 0x04
#define CMD_TEST 0x05
#define CMD_GYRO_STATE 0x06
#define CMD_GYRO_RESET 0x07
#define CMD_GYRO_RAW 0x08
#define CMD_READ_TAPE_SENSOR_DATA 0x09 // Unused
#define CMD_GYRO_ANGLE_RESET 0x0A
#define CMD_GYRO_GET_ANGLE 0x0B
#define CMD_READ_TAPE_SENSOR_FRONT 0x0C
#define CMD_READ_TAPE_SENSOR_BACK  0x0D

void I2C_FillTxBuffer(uint8_t command);

int main(void)
{
	//uint8_t message_buffer[I2C_BUFFER_SIZE];
	
	i2c_init();
	ultraSoundSensorInit();
	gyrosSensorInit();
	gyrosResetAngle();
	TapeInit();
	
	i2c_restart();
	
	uint32_t timestamp = 0;
	
	while (1)
	{
		if (!i2c_busy)
		{
			if (i2c_last_success)
			{
				if (i2c_incoming_data)
				{
					//i2c_read_response(message_buffer, 1);
					
					//I2C_FillTxBuffer(message_buffer[0]);
				}
				
				if (!i2c_busy)
				{
					i2c_restart();
				}
			}
			else
			{
				i2c_restart();
			}
		}
		
		if (GlobalEnableTape)
		{
			TapeReadSensorBack();
			TapeReadSensorFront();	
		}
		
		if (GlobalEnableGyros)
		{
			gyrosUpdateAngle();
			GlobalGyroRaw = readADC();
			GlobalGyroState = (uint8_t)getRotationState();
			GlobalGyroAngleInt = (int16_t)gyrosGetCurrentAngle();
		}
		
		if (GlobalEnableUltrasound && gyrosGetMillis() > (timestamp + 200))
		{
			PORTB |= (1 << PINB2); //
			_delay_us(10);
			PORTB &= ~(1 << PINB2); // Trigger LOW
			GlobalUltrasoundDistance = getObstacleDistance();
			timestamp = gyrosGetMillis();
		}
	}
}