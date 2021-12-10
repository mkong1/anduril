/*

 * ====================================================================
 *  Ramping UI for nanjg105c and a clicky switch by gChart (Gabriel Hart)
 *
 *  This firmware has its roots loosely based  on "Biscotti" by ToyKeeper 
 *
 * ====================================================================
 *
 * 12 or more fast presses > Configuration mode
 *
 * Configuration Menu
 *   1: Memory toggle
 *   2: Stop-at-the-top toggle (without any user interaction, stops itself at turbo)
 *   3: Turbo Timer Toggle (turbo steps down to 50%)
 *   4: Reset to default configuration
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
 *   Same for off-time capacitor values.  Measure, don't guess.
 */
// Choose your MCU here, or in the build script
#define ATTINY 13
#define NANJG_LAYOUT  // specify an I/O pin layout
// Also, assign I/O pins in this file:
#include "../tk-attiny.h"

#define DEFAULTS 0b00000100 // No memory, doesn't stop at the top, turbo timer is enabled

#define FAST  0xA3		  // fast PWM both channels
#define PHASE 0xA1		  // phase-correct PWM both channels
#define VOLTAGE_MON		 // Comment out to disable LVP

#define BLINK_BRIGHTNESS	40 // output to use for blinks in config mode
#define BLINK_SPEED		 750 // ms per normal-speed blink

#define USE_BATTCHECK	   // Enable battery check mode
#define BATTCHECK_8bars	 // up to 8 blinks

#define TURBO_MINUTES 5 // when turbo timer is enabled, how long before stepping down
#define TICKS_PER_MINUTE 120 // used for Turbo Timer timing
#define TURBO_LOWER 128  // the PWM level to use when stepping down
#define TURBO_THRESHOLD sizeof(ramp_values)-5 // Min output level that we consider to be turbo?

#define RAMP_TIME  5  // number of seconds to go from min brightness to max brightness

#define RAMP_SIZE sizeof(ramp_values)
//#define RAMP_VALUES 1,1,1,1,1,2,2,2,2,3,3,4,5,5,6,7,8,9,10,11,13,14,16,18,20,22,24,26,29,32,34,38,41,44,48,51,55,60,64,68,73,78,84,89,95,101,107,113,120,127,134,142,150,158,166,175,184,193,202,212,222,233,244,255
#define RAMP_VALUES 5,5,5,5,5,6,6,6,6,7,7,8,8,9,10,11,12,13,14,15,17,18,20,22,23,25,28,30,32,35,38,41,44,47,51,55,59,63,67,71,76,81,86,92,97,103,109,116,122,129,136,144,151,159,167,176,185,194,203,213,223,233,244,255

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

register uint8_t options asm("r6");
register uint8_t eepos asm("r7");

#define NUM_FP_BYTES 3
uint8_t fast_presses[NUM_FP_BYTES] __attribute__ ((section (".noinit")));

#define RAMPING_UP 0
#define RAMPING_DOWN 1
uint8_t ramp_direction __attribute__ ((section (".noinit")));
uint8_t ramp_stopped __attribute__ ((section (".noinit")));
uint8_t output __attribute__ ((section (".noinit")));
uint8_t output_in_eeprom;

PROGMEM const uint8_t ramp_values[]  = { RAMP_VALUES };

inline uint8_t memory_is_enabled()   { return (options     ) & 0b00000001; }
inline uint8_t stop_at_the_top()     { return (options >> 1) & 0b00000001; }
inline uint8_t ttimer_is_enabled()   { return (options >> 2) & 0b00000001; }

void save_output() {  // save the current output level (with wear leveling)
	uint8_t oldpos=eepos;
	eepos = (eepos+1) & ((EEPSIZE/2)-1);  // wear leveling, use next cell
	eeprom_write_byte((uint8_t *)(eepos), output);  // save current state
	eeprom_write_byte((uint8_t *)(oldpos), 0xff);     // erase old state
}

#define OPT_options (EEPSIZE-1)
void save_state() {  // central method for writing complete state
	save_output();
	eeprom_write_byte((uint8_t *)OPT_options, options);
}

inline void reset_state() {
	output_in_eeprom = 1;
	options = DEFAULTS;  // 3 brightness levels with memory
	save_state();
}

inline void restore_state() {
	uint8_t eep;
	uint8_t first = 1;
	uint8_t i;
	// find the output level data
	for(i=0; i<(EEPSIZE-6); i++) {
		eep = eeprom_read_byte((const uint8_t *)i);
		if (eep != 0xff) {
			eepos = i;
			output_in_eeprom = eep;
			first = 0;
			break;
		}
	}
	// if no output level was found, assume this is the first boot
	if (first) {
		reset_state();
		return;
	}

	// load other config values
	options = eeprom_read_byte((uint8_t *)OPT_options);
}

void set_pwm(uint8_t pwm) {
	TCCR0A = PHASE;
	TCCR0B = 0x01; 
	PWM_LVL = pwm;
}

void set_level(uint8_t level) {
	if (level == 0) { set_pwm(0); }
	else { set_pwm( pgm_read_byte(ramp_values + level - 1) ); }
}

void blink(uint8_t val, uint16_t speed)
{
	for (; val>0; val--)
	{
		set_pwm(BLINK_BRIGHTNESS);
		_delay_ms(speed);
		set_pwm(0);
		_delay_ms(speed);
		_delay_ms(speed);
	}
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

inline uint8_t we_did_a_fast_press() {
	uint8_t i = NUM_FP_BYTES-1;
	while (i && (fast_presses[i] == fast_presses[i-1] )){ --i; }
	return !i;
}
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
	
	restore_state(); // Read config values and saved state

  if ( we_did_a_fast_press() ) {
    increment_fast_presses();
		ramp_stopped = !ramp_stopped;
		
		if(fast_presses[0] == 2) {  // jump to turbo on a double-tap from anywhere
			output = RAMP_SIZE;
			ramp_stopped = 1;
		}
		
    if(ramp_stopped) { save_output(); }

  } else { // Long press
    reset_fast_presses();
		ramp_direction = RAMPING_UP;
    if( memory_is_enabled() ) {
      output = output_in_eeprom;
			ramp_stopped = 1;
    }
    else {
      output = 1;
			ramp_stopped = 0;
    }
  }

	#ifdef VOLTAGE_MON
	ADC_on();
	#else
	ADC_off();
	#endif

	uint16_t ticks = 0;
	
#ifdef VOLTAGE_MON
	uint8_t lowbatt_cnt = 0;
	uint8_t voltage;
	ADCSRA |= (1 << ADSC); // Make sure voltage reading is running for later
#endif
	
	while(1) {
		if (fast_presses[0] >= 12) {  // Config mode if 12 or more fast presses
			_delay_s();	   // wait for user to stop fast-pressing button
			reset_fast_presses(); // exit this mode after one use

			toggle_options((options ^ 0b00000001), 1); // memory
			toggle_options((options ^ 0b00000010), 2); // stop at the top
			toggle_options((options ^ 0b00000100), 3); // turbo timer
			toggle_options(DEFAULTS, 4); // reset to defaults
		}
		else if (fast_presses[0] == 3) {
		//else if (output == BATT_CHECK) {
			 blink(battcheck(), BLINK_SPEED/4);
			 _delay_s(); _delay_s();
			// shouldn't need to worry about resetting fast_presses here, but may need to later
		}
    else if ( !ramp_stopped ) {
			set_level(output);
			if(output == 1 || output == RAMP_SIZE) { _delay_s(); }  // pause for a second at the lowest & highest levels
			
			if( (ramp_direction == RAMPING_DOWN && output == 1) || (ramp_direction == RAMPING_UP && output == RAMP_SIZE) ) {
				ramp_direction = !ramp_direction;
			}
			
			if( output == RAMP_SIZE && stop_at_the_top() ) {
				ramp_stopped = 1;
			}
			
			if(ramp_direction == RAMPING_UP) { output++; }
			else { output--; }
			
			_delay_ms(RAMP_TIME*1000/RAMP_SIZE);
			
			// if we've been ramping around for more than half a second, reset the fast_presses
			ticks = ticks + (RAMP_TIME*1000/RAMP_SIZE);
			if(ticks > 500) {
				ticks = 0;
				reset_fast_presses();
			}
			
    }
		else {
			if ((output >= TURBO_THRESHOLD) && ( ttimer_is_enabled() ) && (ticks > (TURBO_MINUTES * TICKS_PER_MINUTE))) {
				set_pwm(TURBO_LOWER);
			}
      else {
        ticks ++; // count ticks for turbo timer
				set_level(output);
      }
      
			_delay_ms(500);  // Otherwise, just sleep.
      reset_fast_presses();
		}
#ifdef VOLTAGE_MON
		if (ADCSRA & (1 << ADIF)) {  // if a voltage reading is ready
			voltage = ADCH;  // get the waiting value
	
			if (voltage < ADC_LOW) { // See if voltage is lower than what we were looking for
				lowbatt_cnt ++;
			} else {
				lowbatt_cnt = 0;
			}
			
			if (lowbatt_cnt >= 8) {  // See if it's been low for a while, and maybe step down
				if (output > 1) {  // not yet at the lowest level
					output = (output>>1); // divide output in half using bitwise operators
				} else { // Can't go any lower
					set_pwm(0); // Turn off the light
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
