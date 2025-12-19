/*
 * Globals.h
 *
 * Created: 2025-11-25 08:20:40
 *  Author: micka297
 */ 

#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <stdint.h>
#include <stdbool.h>

extern volatile uint8_t GlobalGyroState;
extern volatile uint16_t GlobalGyroRaw;
extern volatile int16_t GlobalGyroAngleInt;

extern volatile uint16_t GlobalUltrasoundDistance;

#define LED_COUNT 11
extern volatile uint16_t GlobalTapeSensorValues[2][LED_COUNT];

extern volatile bool GlobalEnableGyros;
extern volatile bool GlobalEnableUltrasound;
extern volatile bool GlobalEnableTape;



#endif /* GLOBALS_H_ */