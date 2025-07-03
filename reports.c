#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>

#include "reports.h"
#include "keycode.h"
#include "avrutils.h"

// the bits in the consumer report (audio and media controls)
#define FN_MUTE_BIT			0
#define FN_VOL_DOWN_BIT		1
#define FN_VOL_UP_BIT		2
#define FN_PLAY_PAUSE_BIT	3
#define FN_PREV_TRACK_BIT	4
#define FN_NEXT_TRACK_BIT	5

hid_kbd_report_t	usb_keyboard_report;
uint8_t				usb_consumer_report;

// contains the last received LED report
uint8_t usb_led_report;		// bit	LED
							// 0	CAPS
							// 1	NUM
							// 2	SCROLL

void reset_keyboard_report(void)
{
	usb_keyboard_report.modifiers = 0;
	usb_keyboard_report.keys[0] = KC_NO;
	usb_keyboard_report.keys[1] = KC_NO;
	usb_keyboard_report.keys[2] = KC_NO;
	usb_keyboard_report.keys[3] = KC_NO;
	usb_keyboard_report.keys[4] = KC_NO;
	usb_keyboard_report.keys[5] = KC_NO;
}

// updates usb_keyboard_report and usb_consumer_report from the
// data in the key state message contained in the recv_buffer
void process_new_keycode(const uint8_t keycode)
{
	reset_keyboard_report();
	usb_consumer_report = 0;

	if (IS_CONSUMER(keycode)) {
		if (keycode == KC_MUTE)			usb_consumer_report = _BV(FN_MUTE_BIT);
		else if (keycode == KC_VOLU)	usb_consumer_report = _BV(FN_VOL_UP_BIT);
		else if (keycode == KC_VOLD)	usb_consumer_report = _BV(FN_VOL_DOWN_BIT);
	} else {
		usb_keyboard_report.keys[0] = keycode;
	}
}
