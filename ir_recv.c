#include <avr/interrupt.h>
#include <avr/pgmspace.h>	// required by usbdrv.h
#include <util/delay.h>     // for _delay_ms()
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "usbdrv.h"
#include "usbconfig.h"
#include "vusb.h"

#include "avrdbg.h"
#include "avrutils.h"
#include "reports.h"
#include "ir_decoder.h"
#include "keycode.h"

int main()
{
    init_dbg();
	vusb_init();

	dprint("I live...\n");
	
	sei();

	init_decoders();

	bool consumer_report_ready = true;
	bool keyboard_report_ready = true;
	bool idle_elapsed = false;
	uint8_t prev_key_pressed = KC_NO;

    for (;;)
	{
		idle_elapsed = vusb_poll();
		
		uint8_t curr_key_pressed = get_pressed_key();
		if (prev_key_pressed != curr_key_pressed) {
			process_new_keycode(curr_key_pressed);

			consumer_report_ready = true;
			keyboard_report_ready = true;
		}

		// send the keyboard report
        if (usbInterruptIsReady()  &&  (keyboard_report_ready  ||  idle_elapsed))
		{
            usbSetInterrupt((void*) &usb_keyboard_report, sizeof usb_keyboard_report);
			keyboard_report_ready = false;
			
			vusb_reset_idle();
		}

		// send the audio and media controls report
        if (usbInterruptIsReady3()  &&  (consumer_report_ready  ||  idle_elapsed))
		{
            usbSetInterrupt3((void*) &usb_consumer_report, sizeof usb_consumer_report);
			consumer_report_ready = false;
		}
		
		prev_key_pressed = curr_key_pressed;
	}
}
