/*

 * ====================================================================
 *  "Babka" firmware by gChart (Gabriel Hart)
 *  Last updated: 2017-03-20 (v3)
 *
 *  This firmware is based on "Biscotti" by ToyKeeper and is intended
 *  for single-channel attiny13a drivers such as Qlite and nanjc105d.
 *
 *  Change log:
 *    v2: - Added more mode groups
 *        - Went from 2 turbo options to 1 (on or off)
 *        - Code efficiency improvements
 *        - Moved some chunks of code to inline functions for readability
 *        - Changed lowest modes to properly use PHASE
 *        - Changed a couple variables to register variables
 *        - Added a second fast_presses variable for robustness
 *
 *    v3: - Code efficiency improvements (saved like 58 bytes)
 *        - Split toggle function into two separate functions
 *        - More register variables, now possible b/c of the two toggle functions
 *        - Replaced fast_presses variables with an array of size NUM_FP_BYTES
 *        - Variable length mode groups
 *        - More mode groups
 *        - Turbo ramp-down option (disable by commenting TURBO_RAMP_DOWN flag, ~10 bytes)
 *
 * ====================================================================
 *
 * 12 or more fast presses > Configuration mode
 *
 * If hidden modes are enabled, perform a triple-click to enter the hidden modes.
 * Tap once to advance through the hidden modes.  Once you advance past the last
 * hidden mode, you will return to the first non-hidden mode.
 *
 * Configuration Menu
 *   1: Mode Groups:
 *     1: 100
 *     2: 20, 100
 *     3: 5, 35, 100
 *     4: 1, 10, 35, 100
 *     5: 100, 35, 10, 1
 *     6: 100, 35, 5
 *     7: 100, 20
 *     8: 100, Strobe
 *     9: 20, 100, Strobe
 *     10: 5, 35, 100, Strobe
 *     11: 1, 10, 35, 100, Strobe
 *     12: 100, 35, 10, 1, Strobe
 *     13: 100, 35, 5, Strobe
 *     14: 100, 20, Strobe
 *   2: Memory Toggle
 *   3: Hidden Strobe Modes Toggle (Strobe, Beacon, SOS)
 *   4: Hidden Batt Check Toggle (if strobes are enabled, Batt Check appears at the end of them)
 *   5: Turbo Timer Toggle (turbo steps down to 50%)
 *   6: Reset to default configuration
 *   
 * ====================================================================
 *
 * "Biscotti" firmware (attiny13a version of "Bistro")
 * This code runs on a single-channel driver with attiny13a MCU.
 * It is intended specifically for nanjg 105d drivers from Convoy.
 *
 * Copyright (C) 2015 Selene Scriven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * ATTINY13 Diagram
 *		   ----
 *		 -|1  8|- VCC
 *		 -|2  7|- Voltage ADC
 *		 -|3  6|-
 *	 GND -|4  5|- PWM (Nx7135)
 *		   ----
 *
 * FUSES
 *	  I use these fuse settings on attiny13
 *	  Low:  0x75
 *	  High: 0xff
 *
 * CALIBRATION
 *
 *   To find out what values to use, flash the driver with battcheck.hex
 *   and hook the light up to each voltage you need a value for.  This is
 *   much more reliable than attempting to calculate the values from a
 *   theoretical formula.
 *
 */
// Choose your MCU here, or in the build script
#define ATTINY 13
#define NANJG_LAYOUT  // specify an I/O pin layout
// Also, assign I/O pins in this file:
#include "../tk-attiny.h"

//#define DEFAULTS 0b00010010  // 3 modes with memory, no blinkies, no battcheck, no turbo timer
#define DEFAULTS 0b10010010  // 3 modes with memory, no blinkies, no battcheck, with turbo timer

#define VOLTAGE_MON		 // Comment out to disable LVP
#define USE_BATTCHECK	   // Enable battery check mode
//#define BATTCHECK_4bars	 // up to 4 blinks
#define BATTCHECK_8bars	 // up to 8 blinks

#define RAMP_SIZE  6
#define RAMP_PWM   5, 13, 26, 51, 89, 255  // 1, 5, 10, 20, 35, 100%

#define BLINK_BRIGHTNESS	3 // output to use for blinks on battery check (and other modes)
#define BLINK_SPEED		 750 // ms per normal-speed blink

#define TURBO_MINUTES 5 // when turbo timer is enabled, how long before stepping down
#define TICKS_PER_MINUTE 120 // used for Turbo Timer timing
#define TURBO_LOWER 128  // the PWM level to use when stepping down
#define TURBO	 RAMP_SIZE	// Convenience code for turbo mode
#define TURBO_RAMP_DOWN  // disable this to jump right from turbo to the stepdown, no ramping

#define GROUP_SELECT_MODE 252

// These need to be in sequential order, no gaps.
// Make sure to update FIRST_BLINKY and LAST_BLINKY as needed.
// BATT_CHECK should be the last blinky, otherwise the non-blinky
// mode groups will pick up any blinkies after BATT_CHECK
#define FIRST_BLINKY 240
#define STROBE	240
#define BEACON 241
#define SOS 242
#define BATT_CHECK 243
#define LAST_BLINKY 243

// Calibrate voltage and OTC in this file:
#include "../tk-calibration.h"

/*
 * =========================================================================
 */

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <string.h>

#define OWN_DELAY		   // Don't use stock delay functions.
#define USE_DELAY_MS	 // use _delay_ms()
#define USE_DELAY_S		 // Also use _delay_s(), not just _delay_ms()
#include "../tk-delay.h"

#include "../tk-voltage.h"

// borrowed some register variable settings / ideas from Flintrock
register uint8_t options asm("r6");   //uint8_t options = 0;
register uint8_t mode_idx asm("r7");  //uint8_t mode_idx = 0;	 
register uint8_t eepos asm("r8");     //uint8_t eepos = 0;
uint8_t num_modes;

#define NUM_FP_BYTES 3
uint8_t fast_presses[NUM_FP_BYTES] __attribute__ ((section (".noinit")));

#define NUM_MODEGROUPS 14 // Can define up to 16 groups
PROGMEM const uint8_t modegroups[] = {
	6, 0,
	4, 6, 0,
	2, 5, 6, 0,
	1, 3, 5, 6, 0,
	6, 5, 3, 1, 0,
	6, 5, 2, 0,
	6, 4, 0,
	6, STROBE, 0,
	4, 6, STROBE, 0,
  2, 5, 6, STROBE, 0,
	1, 3, 5, 6, STROBE, 0,
  6, 5, 3, 1, STROBE, 0,
	6, 5, 2, STROBE, 0,
	6, 4, STROBE, 0,
};
uint8_t modes[] = { 0,0,0,0,0 };  // make sure this is long enough...

// Modes (gets set when the light starts up based on saved config values)
PROGMEM const uint8_t ramp_PWM[]  = { RAMP_PWM };

inline uint8_t modegroup_number()     { return (options     ) & 0b00001111; }
inline uint8_t memory_is_enabled()    { return (options >> 4) & 0b00000001; }
inline uint8_t blinkies_are_enabled() { return (options >> 5) & 0b00000001; }
inline uint8_t battcheck_is_enabled() { return (options >> 6) & 0b00000001; }
inline uint8_t ttimer_is_enabled()    { return (options >> 7) & 0b00000001; }

void save_mode() {  // save the current mode index (with wear leveling)
	uint8_t oldpos=eepos;
	eepos = (eepos+1) & ((EEPSIZE/2)-1);  // wear leveling, use next cell
	eeprom_write_byte((uint8_t *)(eepos), mode_idx);  // save current state
	eeprom_write_byte((uint8_t *)(oldpos), 0xff);     // erase old state
}

#define OPT_options (EEPSIZE-1)
void save_state() {  // central method for writing complete state
	save_mode();
	eeprom_write_byte((uint8_t *)OPT_options, options);
}

inline void reset_state() {
	mode_idx = 0;
	options = DEFAULTS;  // 3 brightness levels with memory
	save_state();
}

inline void restore_state() {
	uint8_t eep;
	uint8_t first = 1;
	uint8_t i;
	// find the mode index data
	for(i=0; i<(EEPSIZE-6); i++) {
		eep = eeprom_read_byte((const uint8_t *)i);
		if (eep != 0xff) {
			eepos = i;
			mode_idx = eep;
			first = 0;
			break;
		}
	}
	// if no mode_idx was found, assume this is the first boot
	if (first) {
		reset_state();
		return;
	}

	// load other config values
	options = eeprom_read_byte((uint8_t *)OPT_options);
}

inline void next_mode() {
	mode_idx++;
	
	if(fast_presses[0] == 3 && mode_idx <= num_modes) {  // triple-tap from a solid mode
		if( blinkies_are_enabled() ) mode_idx = FIRST_BLINKY; // if blinkies enabled, go to first one
		else if( battcheck_is_enabled() ) mode_idx = BATT_CHECK; // else if battcheck enabled, go to it
	}
	
	// if we hit the end of the solid modes or the blinkies (or battcheck if disabled), go to first solid mode
	if ( (mode_idx == num_modes) || (mode_idx > LAST_BLINKY) || (mode_idx == BATT_CHECK && !battcheck_is_enabled() )) {
		mode_idx = 0;
	}
	
}

inline void count_modes() {
	uint8_t group = 0, mode, i, mc=0;
	uint8_t *dest = modes;
	const uint8_t *src = modegroups;
	for(i=0; i<sizeof(modegroups); i++) {
		mode = pgm_read_byte(src+i);
		// if we hit a 0, that means we're moving on to the next group
		if (mode==0) { group++; } 
		// else if we're in the right group, store the mode and increase the mode count
		else if (group == modegroup_number()) { *dest++ = mode; mc++; } 
	}
	num_modes = mc;
}

inline void set_output(uint8_t pwm1) {
	PWM_LVL = pwm1;
}

void set_level(uint8_t level) {
	if (level == 1) { TCCR0A = PHASE; } 
	set_output(pgm_read_byte(ramp_PWM  + level - 1));
}

void blink(uint8_t val, uint16_t speed)
{
	for (; val>0; val--)
	{
		set_level(BLINK_BRIGHTNESS);
		_delay_ms(speed);
		set_output(0);
		_delay_ms(speed);
		_delay_ms(speed);
	}
}

inline void toggle_mode(uint8_t value, uint8_t num) {
	blink(num, BLINK_SPEED/4);  // indicate which option number this is
	uint8_t temp = mode_idx;
	mode_idx = value;
	save_state();
	blink(32, 500/32); // "buzz" for a while to indicate the active toggle window
	
	// if the user didn't click, reset the value and return
	mode_idx = temp;
	save_state();
	_delay_s();
}

void toggle_options(uint8_t value, uint8_t num) {
	blink(num, BLINK_SPEED/4);  // indicate which option number this is
	uint8_t temp = options;
	options = value;
	save_state();
	blink(32, 500/32); // "buzz" for a while to indicate the active toggle window
	
	// if the user didn't click, reset the value and return
	options = temp;
	save_state();
	_delay_s();
}

#if 0
inline uint8_t we_did_a_fast_press() {
	uint8_t i = NUM_FP_BYTES-1;
	while (i && (fast_presses[i] == fast_presses[i-1] )){ --i; }
	return !i;
}
#else
inline uint8_t we_did_a_fast_press() {
	uint8_t i;
	for(i=0; i<NUM_FP_BYTES-1; i++) { if(fast_presses[i] != fast_presses[i+1]) return 0;}
	return 1;
}
#endif
inline void increment_fast_presses() {
	uint8_t i;
	for(i=0; i<NUM_FP_BYTES; i++) { fast_presses[i]++; }
}

void reset_fast_presses() {
	uint8_t i;
	for(i=0; i<NUM_FP_BYTES; i++) { fast_presses[i] = 0; }
}

int main(void)
{

	DDRB |= (1 << PWM_PIN);	 // Set PWM pin to output, enable main channel
	TCCR0A = FAST; // Set timer to do PWM for correct output pin and set prescaler timing
	TCCR0B = 0x01; // Set timer to do PWM for correct output pin and set prescaler timing

	restore_state(); // Read config values and saved state

	count_modes(); // Enable the current mode group
	 // check button press time, unless we're in group selection mode
	if (mode_idx != GROUP_SELECT_MODE) {
		if ( we_did_a_fast_press() ) { // sram hasn't decayed yet, must have been a short press
			increment_fast_presses();
			next_mode(); // Will handle wrap arounds
		} else { // Long press, keep the same mode
			reset_fast_presses();
			if( !memory_is_enabled() ) {mode_idx = 0;}  // if memory is turned off, set mode_idx to 0
		}
	}
	save_mode();

	// Turn features on or off as needed
	#ifdef VOLTAGE_MON
	ADC_on();
	#else
	ADC_off();
	#endif

	uint8_t output;
	uint8_t i = 0;
	uint16_t ticks = 0;
#ifdef TURBO_RAMP_DOWN
	uint8_t adj_output = 255;
#endif
	
#ifdef VOLTAGE_MON
	uint8_t lowbatt_cnt = 0;
	uint8_t voltage;
	ADCSRA |= (1 << ADSC); // Make sure voltage reading is running for later
#endif

	if(mode_idx > num_modes) { output = mode_idx; }  // special modes, override output
	else { output = modes[mode_idx]; }
	
	while(1) {
		if (fast_presses[0] >= 12) {  // Config mode if 12 or more fast presses
			_delay_s();	   // wait for user to stop fast-pressing button
			reset_fast_presses(); // exit this mode after one use

			toggle_mode(GROUP_SELECT_MODE, 1); // Enter the mode group selection mode?
			toggle_options((options ^ 0b00010000), 2); // memory
			toggle_options((options ^ 0b00100000), 3); // hidden blinkies
			toggle_options((options ^ 0b01000000), 4); // hidden battcheck
			toggle_options((options ^ 0b10000000), 5); // turbo timer
			toggle_options(DEFAULTS, 6); // reset to defaults
		}
		else if (output == STROBE) { // 10Hz tactical strobe
			for(i=0;i<8;i++) {
				set_level(RAMP_SIZE);
				_delay_ms(33);
				set_output(0);
				_delay_ms(67);
			}
		}
		else if (output == BEACON) {
			set_level(RAMP_SIZE);
			_delay_ms(10);
			set_output(0);
			_delay_s(); _delay_s();
		}
		else if (output == SOS) {  // dot = 1 unit, dash = 3 units, space betwen parts of a letter = 1 unit, space between letters = 3 units
			#define SOS_SPEED 200  // these speeds aren't perfect, but they'll work for the [never] times they'll actually get used
			blink(3, SOS_SPEED); // 200 on, 400 off, 200 on, 400 off, 200 on, 400 off
			_delay_ms(SOS_SPEED);  // wait for 200
			blink(3, SOS_SPEED*5/2);  // 500 on, 1000 off, 500 on, 1000 off, 500 on, 1000 off (techically too long on that last off beat, but oh well)
			blink(3, SOS_SPEED);  // 200 on, 400 off, 200 on, 400 off, 200 on, 400 off
			_delay_s(); _delay_s();
		}
		else if (output == BATT_CHECK) {
			 blink(battcheck(), BLINK_SPEED/4);
			 _delay_s(); _delay_s();
		}
		else if (output == GROUP_SELECT_MODE) {
			mode_idx = 0; // exit this mode after one use

			for(i=0; i<NUM_MODEGROUPS; i++) {
				toggle_options(((options & 0b11110000) | i), i+1);
			}
			_delay_s();
		}
		else {
			if ((output == TURBO) && ( ttimer_is_enabled() ) && (ticks > (TURBO_MINUTES * TICKS_PER_MINUTE))) {
	#ifdef TURBO_RAMP_DOWN
				if (adj_output > TURBO_LOWER) { adj_output = adj_output - 2; }
				set_output(adj_output);
	#else
				set_output(TURBO_LOWER);
	#endif
			}
			else {
				ticks ++; // count ticks for turbo timer
				set_level(output);
			}

			_delay_ms(500);  // Otherwise, just sleep.

		}
		reset_fast_presses();
#ifdef VOLTAGE_MON
		if (ADCSRA & (1 << ADIF)) {  // if a voltage reading is ready
			voltage = ADCH;  // get the waiting value
	
			if (voltage < ADC_LOW) { // See if voltage is lower than what we were looking for
				lowbatt_cnt ++;
			} else {
				lowbatt_cnt = 0;
			}
			
			if (lowbatt_cnt >= 8) {  // See if it's been low for a while, and maybe step down
				//set_output(0);  _delay_ms(100); // blink on step-down:

				if (output > RAMP_SIZE) {  // blinky modes 
					output = RAMP_SIZE / 2; // step down from blinky modes to medium
				} else if (output > 1) {  // regular solid mode
					output = output - 1; // step down from solid modes somewhat gradually
				} else { // Already at the lowest mode
					set_output(0); // Turn off the light
					set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Power down as many components as possible
					sleep_mode();
				}
				set_level(output);
				lowbatt_cnt = 0;
				_delay_s(); // Wait before lowering the level again
			}

			ADCSRA |= (1 << ADSC); // Make sure conversion is running for next time through
		}
#endif  // ifdef VOLTAGE_MON
	}
}
