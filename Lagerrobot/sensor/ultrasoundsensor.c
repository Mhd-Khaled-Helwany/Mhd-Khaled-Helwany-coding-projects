#include <stdio.h>
#include <avr/io.h>
#define F_CPU 8000000UL
#include <util/delay.h>

#define ECHO_PIN PINB3
#define TRIG_PIN PINB2

void ultraSoundSensorInit(){
    DDRB &= ~(1 << ECHO_PIN);  // Echo as input
    DDRB |= (1 << TRIG_PIN);   // Trigger as output
}

uint16_t getPulseTime(){
    // Wait for echo to go HIGH (with timeout)
    uint16_t timeout = 0;
    while(!(PINB & (1 << ECHO_PIN))){ 
		PORTB |= 1;
        _delay_us(1);
        timeout++;
        if(timeout > 30000) { // ~30ms timeout
            return 0;
		}
    }
    
    // Start timer when echo goes HIGH
    TCNT1 = 0;
    TCCR1B = (1 << CS11); // Prescaler 8 for 8MHz CPU = 1 tick per 1µs
    
	// Wait for echo to go LOW (with timeout)
    while(PINB & (1 << ECHO_PIN)) { 
         if(TCNT1 >= 40000) {  // ~40ms timeout (for max range)
    		break;
		 }
    }
    
    // Stop timer
    TCCR1B = 0x00;
    uint16_t ticks = TCNT1;
    
	// With prescaler 8 on 8MHz: 1 tick = 1µs
	return ticks; // Returns time in microseconds
}

uint16_t getObstacleDistance()
{
    // Get echo pulse width in microseconds
    uint16_t pulse_time_us = getPulseTime();
    
    if(pulse_time_us == 0)
	{
        return 0; // Error/timeout
	}
	
    // Distance = (pulse_time × speed_of_sound) / 2
    // Speed of sound = 340 m/s = 0.034 cm/µs
    // Distance (cm) = pulse_time_us * 0.034 / 2 = pulse_time_us / 58
    uint16_t distance = pulse_time_us / 58;
    
    return distance;
}