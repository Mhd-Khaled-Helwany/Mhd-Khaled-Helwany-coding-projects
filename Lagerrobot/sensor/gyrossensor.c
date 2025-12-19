/* 
This file includes source code for the sensor
responsible for detecting when the robot is done
rotating


Current evaluation of angle is not accurate, 90 degrees is represented by around uint16_t 500 from gyrosGetCurrentAngle() 
Reset command must be ran before measuring rotation. 
A 90 degree turn could either be for example 504 or 65018 (-504) depending on orientation and direction
*/
#define F_CPU 8000000UL

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "gyrossensor.h"

#define CONSECUTIVE_READINGS 10  // Number of readings with the same value in a row
#define ROTATION_THRESHOLD 50 // Minimum change in gyros value needed before we detect a rotation (used to ignore noise)
// Command codes from the data sheet
#define CMD_ADCC 0x94 // 10010100 Table 9. Format of the ADCC command with ADEN 1
#define CMD_ADCR 0x80 // Table 10. Format of the ADCR command 
#define SCALE_FACTOR 3.2f // 2.59075556f // LSB PER DEGREES PER SECOND for version R2
#define FULL_SCALE_RATE 300.0f  //Full scale range for R2 version

static uint16_t zeroRateOffset = 1008; // This is the value that is returned when no rotation detected
static GyroState currentState = GYRO_IDLE;
static uint8_t stableCount = 0;
static float currentAngle = 0.0f;
static uint32_t lastUpdateTime = 0;
static uint8_t angleTrackingActive = 0;
static volatile uint32_t millisCounter = 0;


// Counter for milliseconds
ISR(TIMER0_COMPA_vect){
	millisCounter++;
}

// Initializes timer0 to count once per millisecond
void gyrosTimerInit(){
	TCCR0A = (1 << WGM01); //Clears timer on compare match
	TCCR0B = (1 << CS01) | (1 << CS00); //Prescaler 64
	OCR0A = 124; //(fCpu*1ms/prescaler)-1 according to datasheet
	TIMSK0 = (1 << OCIE0A); //Output compare match interrupt enable
	sei();
}

// Atomic way of reading the millisecond counter
uint32_t gyrosGetMillis() {
	uint32_t ms;
	cli();
	ms = millisCounter;
	sei();
	return ms;
}

//Executes a SPI transfer
uint8_t spiTransfer(uint8_t data) {
	SPDR = data;
	while(!(SPSR & (1 << SPIF))) {}
	return SPDR;
}

void gyrosSensorInit() {
	DDRB |= (1 << PINB4) | (1 << PINB5) | (1 << PINB7); // Data direction
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); // Initialize SPI communication
    PORTB |=  (1 << PINB4);
	_delay_ms(250); // Delay from docs
	
	//Dummy request, necessary from documentation
	PORTB &= ~(1 << PINB4);
	spiTransfer(CMD_ADCC);
	spiTransfer(0x00);
	PORTB |= (1 << PINB4);
	_delay_us(150);
	
	PORTB &= ~(1 << PINB4);
	spiTransfer(CMD_ADCR);
	spiTransfer(0x00);
	spiTransfer(0x00);
	PORTB |= (1 << PINB4);
	_delay_ms(100);
	
	// Check ADCS for error
	uint16_t reading = readADC();
	if (reading != 0xFFFF){
		zeroRateOffset = reading;
	}
	currentState = GYRO_IDLE;
	stableCount = 0;
	
	currentAngle = 0.0f;
	angleTrackingActive = 0; 
	
	gyrosTimerInit();
	lastUpdateTime = gyrosGetMillis();	
}

uint16_t readADC() {
    uint8_t low, high = 0;
    uint16_t result = 0;
    PORTB &= ~(1 << PINB4);
    spiTransfer(CMD_ADCC); 
    spiTransfer(0x00);
	PORTB |=  (1 << PINB4);
	_delay_us(150);     // A delay here needed > 115 us according to data sheet
	PORTB &= ~(1 << PINB4);
    spiTransfer(CMD_ADCR);
    high = spiTransfer(0x00);
    low  = spiTransfer(0x00);
    PORTB |=  (1 << PINB4);
    result = ((uint16_t)high << 8) | low;
	// Check if conversion is done
	if (!(result & 0x2000)){
		return 0xFFFF;
	}
    result = (result >> 1) & 0x07FF; // Mask the result to 11 bits
    return result;
}

// Method for detecting the current state of the gyro
GyroState getRotationState(){
	uint16_t adcValue = readADC();
	if (adcValue == 0xFFFF){
		return currentState;
	}
	int16_t diff = (int16_t)adcValue - (int16_t)zeroRateOffset;
	if (diff < 0) diff = -diff;
	
	switch(currentState) {
		case GYRO_IDLE:
			if (diff > ROTATION_THRESHOLD){
				currentState = GYRO_ROTATING;
				stableCount = 0;
				gyrosResetAngle();
			}
		break;
		case GYRO_ROTATING:
		if (diff < ROTATION_THRESHOLD){
			currentState = GYRO_SETTLING;
			stableCount = 1;
		}
		break;
		case GYRO_SETTLING:
		if (diff < ROTATION_THRESHOLD){
			stableCount++;
			if (stableCount == CONSECUTIVE_READINGS) { //We need a certain amount of consecutive readings to determine if the GYRO has finished settling
				currentState = GYRO_DONE;
				angleTrackingActive = 0;
			}
			
		}else {
		currentState = GYRO_ROTATING;
		stableCount = 0;
	}
		break;
		case GYRO_DONE:
		break;
	
	}
	return currentState;
}

// Resets the rotation direction
void resetRotationDirection() {
	currentState = GYRO_IDLE;
	stableCount = 0;
	gyrosResetAngle();
}

// Resets the gyro reference angle
void gyrosResetAngle() {
	currentAngle = 0.0f;
	lastUpdateTime = gyrosGetMillis();
	angleTrackingActive = 1; // Sets bool so we know that we are actively tracking the angle
}

// Method for updating the angle
void gyrosUpdateAngle() {
	if (!angleTrackingActive) {
		return;
	}
	uint16_t adcValue = readADC();
	if (adcValue == 0xFFFF){
		return;
	}
	// Formula for converting the angle, Angle = angularRate*deltaTime
	uint32_t currentTime = gyrosGetMillis();
	float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
	lastUpdateTime = currentTime;
	int16_t diff = (int16_t)adcValue - (int16_t)zeroRateOffset;
	if (diff > -10 && diff < 10)
	{
		return;
	}
	float angularRate = (float)diff / SCALE_FACTOR; // Scales the angular difference with constant scale factor
	currentAngle += angularRate * deltaTime;
}

// Getter for angle
float gyrosGetCurrentAngle() {
	return currentAngle;
}