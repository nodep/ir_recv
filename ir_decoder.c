#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "hw_setup.h"
#include "usbdrv.h"
#include "avrdbg.h"
#include "avrutils.h"
#include "ir_decoder.h"
#include "keycode.h"
#include "ir_panasonic.h"
#include "ir_samsung.h"

void init_decoders(void)
{
	// setup the IR timer
    TCCR1A = 0;
	// prescaler = 1/64 -> 1 count @ 8MHz = 8us, 12MHz = 5.3333us
    TCCR1B = _BV(CS10) | _BV(CS11);

	// setup the I/O pins
	DDRC	= _BV(LED_YLW_BIT)
			| _BV(LED_BLU_BIT)
			| _BV(LED_RED_BIT)
			| _BV(LED_GRN_BIT);
			
	// IR_RECV_BIT pull-up and LED_GRN_BIT on-indicator led
	PORTC	= _BV(IR_RECV_BIT) | _BV(LED_GRN_BIT);
}

uint8_t get_pressed_key(void)
{
	static uint8_t ir_prev_pin_state = _BV(PC1);
	static uint8_t pressed_key = KC_NO;

	// read the IR receiver pin state
	uint8_t ir_pin_state = PINC & _BV(PC1);
	if (ir_pin_state != ir_prev_pin_state) {
		// store counter value
		unsigned int duration = TCNT1;

		// clear the counter
		TCNT1 = 0;

		// have we had an overflow on the IR timer?
		if (TIFR1 & _BV(TOV1)) {
			// store the max value
			duration = 0xffff;

			// clear overflow bit by writing a one
			TIFR1 = _BV(TOV1);
		}
		
		uint8_t new_key = get_panasonic_key(ir_pin_state, duration);
		if (new_key == KC_NO) {
			new_key = get_samsung_key(ir_pin_state, duration);
		}

		if (new_key != KC_NO) {
			SetBit(PORT(LED_BLU_PORT), LED_BLU_BIT);
			pressed_key = new_key;
		}

		ir_prev_pin_state = ir_pin_state;
	}

	// do we have an overflow on the LED timer?
	if (TIFR1 & _BV(TOV1)) {
		ClrBit(PORT(LED_BLU_PORT), LED_BLU_BIT);
		pressed_key = KC_NO;
	}
	
	if (TCNT1 > 20000)
		pressed_key = KC_NO;
	
	return pressed_key;
}
