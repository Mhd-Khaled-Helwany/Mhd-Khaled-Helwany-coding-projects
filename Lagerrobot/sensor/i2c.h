/*
 * i2c.h
 *
 * Created: 2025-11-05 09:36:24
 *  Author: ferpe211
 */ 


#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

#define I2C_BUFFER_SIZE 32

extern volatile uint8_t i2c_busy;
extern volatile uint8_t i2c_last_success;
extern volatile uint8_t i2c_incoming_data;

void i2c_init(void);
void i2c_restart(void);
void i2c_write_response(uint8_t *data, uint8_t length);
void i2c_read_response(uint8_t *data, uint8_t length);

#endif
