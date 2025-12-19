/*
This file includes source code for the sensor
responsible for detecting tape on the floor in
order for the robot to navigate the warehouse
*/
#define F_CPU 8000000UL
#include <util/delay.h>
#include "TapeSensor.h"
#include "Globals.h"

void ADC_Init() 
{
	ADCSRA = (1 << ADEN);  // Enable ADC
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0); // Prescaler of 64
	ADMUX = (1 << REFS0); // AVcc as reference
}

uint16_t ADC_Read(uint8_t channel) 
{
	// Select ADC channel with safety mask
	channel &= 0x07;
	ADMUX = (ADMUX & 0xF8) | channel;

	// Start conversion
	ADCSRA |= (1 << ADSC);

	// Wait for conversion to complete
	while (ADCSRA & (1 << ADSC));

	// Return the ADC value
	return ADC & 0x3FF;
}

void TapeInit(void)
{
	ADC_Init();
	DDRA |= (1 << PINA0)|(1 << PINA1)|(1 << PINA2)|(1 << PINA3)|(1 << PINA4);
	DDRD |= (1 << PIND0)|(1 << PIND1)|(1 << PIND2)|(1 << PIND3)|(1 << PIND4);

}

// NOTE: This stalls a lot, but I don't think it matters since we only can do one ADC conversion at a time hardware-wise
void TapeReadSensorBack(void)
{
	for (uint8_t i = 0; i < LED_COUNT; ++i)
	{
		// --- SensorValues[0]
		// Select LED
		uint8_t mask = 0x0F;
		PORTA &= ~mask;
		PORTA |= mask & (i+1);
		
		PORTA |= (1<<PINA4); // Enable
		GlobalTapeSensorValues[0][i] = ADC_Read(5); // Read (this one stalls)
		PORTA &= ~(1<<PINA4); // Disable
	}
}

// NOTE: This stalls a lot, but I don't think it matters since we only can do one ADC conversion at a time hardware-wise
void TapeReadSensorFront(void)
{
	for (uint8_t i = 0; i < LED_COUNT; ++i)
	{
		// --- SensorValues[1]
		// Select LED
		uint8_t mask = 0x0F;
		PORTD &= ~mask;
		PORTD |= mask & (i+1);
		
		PORTD |= (1<<PIND4); // Enable
		GlobalTapeSensorValues[1][i] = ADC_Read(6); // Read (this one stalls)
		PORTD &= ~(1<<PIND4); // Disable
	}
}

