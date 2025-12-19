/*
 * i2c.h
 *
 * Created: 2025-11-05 09:36:24
 *  Author: ferpe211
 */ 


#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

extern volatile uint8_t rx_buffer[16];
extern volatile uint8_t rx_index;
extern volatile uint8_t rx_done;

void I2C_init(uint8_t address);
void write_response(uint8_t *data, uint8_t length);

#endif
