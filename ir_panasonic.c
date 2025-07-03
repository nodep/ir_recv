#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "hw_setup.h"
#include "keycode.h"
#include "avrdbg.h"
#include "avrutils.h"
#include "ir_panasonic.h"

/*
Panasonic EUR64384 remote controller codes and functions

// power
0x40040538bc81  power
0x400405386954  sleep
0x40040500595c  aux
// tuner
0x400405202500  tuner/band
0x40040520ac89  preset down
0x400405202c09  preset up
// CD
0x400405505104  program
0x40040550c590  cancel
0x40040550e2b7  repeat
0x400405502570  disc
0x400405380835  prev track/disc 1
0x400405505207  next track/disc 2
0x400405505005  play/disc 3
0x400405506035  pause/disc 4
0x400405500055  stop/disc 5
// tape
0x40040510a9bc  deck 1/2
0x400405104055  rewind
0x40040510c0d5  fast fwd
0x40040510d0c5  play backward
0x400405100015  stop
0x400405105045  play
0x40040508838e  v. bass
0x40040508f1fc  on/flat eq
0x40040508c1cc  mode
// volume
0x400405004c49  muting
0x400405008481  vol -
0x400405000401  vol +
*/

#define BETWEEN(val, min, max)		(val > min  &&  val < max)

// power
#define PANA_POWER			0x538bc81
#define PANA_SLEEP			0x5386954
#define PANA_AUX			0x500595c
// tuner
#define PANA_TUNER_BAND		0x5202500
#define PANA_PRESET_DOWN	0x520ac89
#define PANA_PRESET_UP		0x5202c09
// CD
#define PANA_PROGRAM			0x5505104
#define PANA_CANCEL				0x550c590
#define PANA_REPEAT				0x550e2b7
#define PANA_DISC				0x5502570
#define PANA_PREV_TRACK_DISC_1	0x5380835
#define PANA_NEXT_TRACK_DISC_2	0x5505207
#define PANA_PLAY_DISC_3		0x5505005
#define PANA_PAUSE_DISC_4		0x5506035
#define PANA_STOP_DISC_5		0x5500055
// tape
#define PANA_DECK			0x510a9bc
#define PANA_REWIND			0x5104055
#define PANA_FAST_FWD		0x510c0d5
#define PANA_PLAY_BACKWARD	0x510d0c5
#define PANA_STOP			0x5100015
#define PANA_PLAY			0x5105045
// eq
#define PANA_V_BASS			0x508838e
#define PANA_ON_FLAT_EQ		0x508f1fc
#define PANA_MODE			0x508c1cc
// volume
#define PANA_MUTING			0x5004c49
#define PANA_VOL_DOWN		0x5008481
#define PANA_VOL_UP			0x5000401

// decoder states
enum {
	IR_IDLE,
	IR_HEADER,
	IR_DATA,
	IR_ERROR,
};

static bool is_hdr_lo(uint16_t d)		{ return BETWEEN(d, 630, 690); }
static bool is_hdr_hi(uint16_t d)		{ return BETWEEN(d, 270, 370); }
static bool is_dat_short(uint16_t d)	{ return BETWEEN(d, 35, 130 ); }
static bool is_dat_long(uint16_t d)		{ return BETWEEN(d, 200, 300); }

static uint8_t	ir_state = IR_IDLE;
static uint8_t	ir_bits = 0;
static uint32_t	data_hi = 0;
static uint32_t	data_lo = 0;

static bool decode_signal_panasonic(bool is_rising, uint16_t duration)
{
	bool has_new_value = false;
	if (ir_state == IR_IDLE) {
		data_lo = data_hi = ir_bits = 0;
		ir_state = is_rising ? IR_ERROR : IR_HEADER;
	} else if (ir_state == IR_HEADER) {
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

			if (ir_bits < 16) {
				data_hi <<= 1;
				if (is_set)
					data_hi |= 1;
			} else {
				data_lo <<= 1;
				if (is_set)
					data_lo |= 1;
			}
			
			ir_bits++;
			
		} else {
			if (ir_bits == 48) {
				ir_state = IR_IDLE;
				has_new_value = true;
				//dprint("0x%04lx%08lx\n", data_hi, data_lo);
			}
		}
	}
	
	if (ir_state == IR_ERROR) {
		ir_state = IR_IDLE;
	}
		
	return has_new_value;
}

uint8_t get_panasonic_key(bool is_rising, uint16_t duration) {
	uint8_t pressed_key = KC_NO;

	if (decode_signal_panasonic(is_rising, duration)) {
		switch (data_lo) {
			case PANA_MUTING:		pressed_key = KC_AUDIO_MUTE;		break;
			case PANA_VOL_DOWN:		pressed_key = KC_AUDIO_VOL_DOWN;	break;
			case PANA_VOL_UP:		pressed_key = KC_AUDIO_VOL_UP;		break;
			
			case PANA_STOP:
			case PANA_PLAY:			pressed_key = KC_SPACE;				break;

			case PANA_REWIND:		pressed_key = KC_LEFT;				break;
			case PANA_FAST_FWD:		pressed_key = KC_RIGHT;				break;

			case PANA_PRESET_DOWN:	pressed_key = KC_DOWN;				break;
			case PANA_PRESET_UP:	pressed_key = KC_UP;				break;

			// netflix skip
			case PANA_NEXT_TRACK_DISC_2:	pressed_key = KC_S;			break;

			// full screen & such
			case PANA_DECK:				pressed_key = KC_ESC;		break;
			case PANA_PLAY_BACKWARD:	pressed_key = KC_F;			break;
			case PANA_V_BASS:			pressed_key = KC_F11;		break;
		}

		SetBit(PORT(LED_BLU_PORT), LED_BLU_BIT);
	}

	return pressed_key;
}