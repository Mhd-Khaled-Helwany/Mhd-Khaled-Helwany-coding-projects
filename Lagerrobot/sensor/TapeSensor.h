#ifndef TAPESENSOR_H_
#define TAPESENSOR_H_

#include <avr/io.h>
#include <stdint.h>

void ADC_Init();
uint16_t ADC_Read(uint8_t channel);

void TapeInit(void);
void TapeReadSensorFront(void);
void TapeReadSensorBack(void);

#endif /* TAPESENSOR_H_ */