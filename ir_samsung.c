#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "hw_setup.h"
#include "keycode.h"
#include "avrdbg.h"
#include "avrutils.h"
#include "ir_samsung.h"

// Pressing the power button sends these two, in a toggling manner
// If you press once, you get one code, press again you the other
#define SAMSUNG_POWER_1			0xe0e040bf
#define SAMSUNG_POWER_2			0xe0e06798
#define SAMSUNG_COLOR_DOTS_123	0xe0e04bb4
#define SAMSUNG_HAMBURGER		0xe0e058a7
#define SAMSUNG_LEAF_IN_RECT	0xe0e06f90
#define SAMSUNG_UP				0xe0e006f9
#define SAMSUNG_LEFT			0xe0e0a659
#define SAMSUNG_MIDDLE			0xe0e016e9
#define SAMSUNG_RIGHT			0xe0e046b9
#define SAMSUNG_DOWN			0xe0e08679
#define SAMSUNG_BACK			0xe0e01ae5
#define SAMSUNG_HOME			0xe0e09e61
#define SAMSUNG_PLAY_PAUSE		0xe0e09d62
#define SAMSUNG_NETFLIX			0xe0e0cf30
#define SAMSUNG_PRIME_VIDEO		0xe0e02fd0
#define SAMSUNG_RAKUTEN_TV		0xe0e03dc2
#define SAMSUNG_VOLUME_DOWN		0xe0e0d02f
#define SAMSUNG_VOLUME_UP		0xe0e0e01f
#define SAMSUNG_CHANNEL_DOWN	0xe0e008f7
#define SAMSUNG_CHANNEL_UP		0xe0e048b7

#define SAMSUNG_COLOR_DOTS_EXTRA 0xe0e0738c
#define SAMSUNG_WWW				0xe0e0ec13

// This is sent by the original Samsung bluetooth capable,
// smart remote controller on every key except power
#define SAMSUNG_DEFAULT			0xe0e08b74

#define BETWEEN(val, min, max)		(val > min  &&  val < max)

// decoder states
enum {
	IR_IDLE,
	IR_START,
	IR_DATA,
	IR_ERROR,
};

static bool is_hdr_lo(uint16_t d)		{ return BETWEEN(d, 700, 990); }
static bool is_hdr_hi(uint16_t d)		{ return BETWEEN(d, 700, 990); }
static bool is_dat_short(uint16_t d)	{ return BETWEEN(d, 35,  220 ); }
static bool is_dat_long(uint16_t d)		{ return BETWEEN(d, 250, 350); }

static uint8_t	ir_state = IR_IDLE;
static uint8_t	ir_bits = 0;
static uint32_t	data = 0;

static bool decode_signal_samsung(bool is_rising, uint16_t duration)
{
	bool has_new_value = false;
	if (ir_state == IR_IDLE) {
		data = ir_bits = 0;
		ir_state = is_rising ? IR_ERROR : IR_START;
	} else if (ir_state == IR_START) {
		if (is_rising) {
			if (!is_hdr_lo(duration))
				ir_state = IR_ERROR;
		} else {
			if (is_hdr_hi(duration))
				ir_state = IR_DATA;
			else
				ir_state = IR_ERROR;
		}
	} else if (ir_state == IR_DATA) {
		if (!is_rising) {
			bool is_set = false;
			if (is_dat_long(duration))
				is_set = true;
			else if (!is_dat_short(duration))
				ir_state = IR_ERROR;

			if (ir_state == IR_ERROR)
				dprinti(duration);

			data <<= 1;
			if (is_set)
				data |= 1;
			
			ir_bits++;
			
		} else {
			if (ir_bits == 32) {
				ir_state = IR_IDLE;
				has_new_value = true;
				//dprint("0x%08lx\n", data);
			}
		}
	}
	
	if (ir_state == IR_ERROR) {
		ir_state = IR_IDLE;
	}
		
	return has_new_value;
}

uint8_t get_samsung_key(bool is_rising, uint16_t duration) {
	uint8_t pressed_key = KC_NO;

	if (decode_signal_samsung(is_rising, duration)) {
		switch (data) {
			case SAMSUNG_PRIME_VIDEO:	pressed_key = KC_AUDIO_MUTE;		break;
			case SAMSUNG_VOLUME_DOWN:	pressed_key = KC_AUDIO_VOL_DOWN;	break;
			case SAMSUNG_VOLUME_UP:		pressed_key = KC_AUDIO_VOL_UP;		break;
			
			case SAMSUNG_MIDDLE:		pressed_key = KC_SPACE;		break;

			case SAMSUNG_LEFT:			pressed_key = KC_LEFT;		break;
			case SAMSUNG_RIGHT:			pressed_key = KC_RIGHT;		break;

			case SAMSUNG_CHANNEL_DOWN:	pressed_key = KC_DOWN;		break;
			case SAMSUNG_CHANNEL_UP:	pressed_key = KC_UP;		break;

			// netflix skip
			case SAMSUNG_NETFLIX:		pressed_key = KC_S;			break;

			// full screen & such
			case SAMSUNG_BACK:			pressed_key = KC_ESC;		break;
			case SAMSUNG_HOME:			pressed_key = KC_F;			break;
			case SAMSUNG_PLAY_PAUSE:	pressed_key = KC_F11;		break;

			case SAMSUNG_POWER_1:
			case SAMSUNG_POWER_2:
			case SAMSUNG_COLOR_DOTS_123:
			case SAMSUNG_HAMBURGER:
			case SAMSUNG_LEAF_IN_RECT:
			case SAMSUNG_UP:
			case SAMSUNG_DOWN:
			case SAMSUNG_RAKUTEN_TV:
			case SAMSUNG_COLOR_DOTS_EXTRA:
			case SAMSUNG_WWW:
				break;
			default:
				dprint("0x%08lx\n", data);
		}

		SetBit(PORT(LED_BLU_PORT), LED_BLU_BIT);
	}

	return pressed_key;
}