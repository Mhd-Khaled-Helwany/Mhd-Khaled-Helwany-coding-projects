#ifndef GYROSSENSOR_H_
#define GYROSSSENSOR_H_

#include <avr/io.h>
#include <stdint.h>

typedef enum {
	GYRO_IDLE = 0,
	GYRO_ROTATING = 1,
	GYRO_SETTLING = 2,
	GYRO_DONE = 3
} GyroState;

void gyrosSensorInit(void);
GyroState getRotationState(void);
void resetRotationDirection(void);
uint16_t readADC(void);
void gyrosResetAngle(void);
float gyrosGetCurrentAngle(void);
void gyrosUpdateAngle(void);
uint32_t gyrosGetMillis(void);

#endif