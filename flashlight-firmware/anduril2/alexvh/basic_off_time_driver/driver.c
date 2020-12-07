/*
 * "Off Time Basic Driver" for ATtiny controlled flashlights
 *  Copyright (C) 2014 Alex van Heuvelen (alexvanh)
 *
 * Basic firmware demonstrating a method for using off-time to
 * switch modes on attiny13 drivers such as the nanjg drivers (a feature
 * not supported by the original firmware).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 
 
 
/* This firmware uses off-time memory for mode switching without
 * hardware modifications on nanjg drivers. This is accomplished by
 * storing a flag in an area of memory that does not get initialized.
 * There is enough energy stored in the decoupling capacitor to
 * power the SRAM and keep data during power off for about 500ms.
 *
 * When the firmware starts a byte flag is checked. If the flashlight
 * was off for less than ~500ms all bits will still be 0. If the
 * flashlight was off longer than that some of the bits in SRAM will
 * have decayed to 1. After the flag is checked it is reset to 0.
 * Being off for less than ~500ms means the user half-pressed the
 * switch (using it as a momentary button) and intended to switch modes.
 *
 * This can be used to store any value in memory. However it is not
 * guaranteed that all of the bits will decay to 1. Checking that no
 * bits in the flag have decayed acts as a checksum and seems to be
 * enough to be reasonably certain other SRAM data is still valid.
 *
 * In order for this to work brown out detection must be enabled by
 * setting the correct fuse bits. I'm not sure why this is, maybe
 * reduced current consumption due to the reset being held once the
 * capacitor voltage drops below the threshold?
 */

#define F_CPU 4800000
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h> 

//#define MODE_MEMORY

#ifdef MODE_MEMORY // only using eeprom if mode memory is enabled
uint8_t EEMEM MODE_P;
#endif

// store in uninitialized memory so it will not be overwritten and
// can still be read at startup after short (<500ms) power off
// decay used to tell if user did a short press.
volatile uint8_t noinit_decay __attribute__ ((section (".noinit")));
volatile uint8_t noinit_mode __attribute__ ((section (".noinit")));
// pwm level selected by ramping function
volatile uint8_t noinit_lvl __attribute__ ((section (".noinit")));

// PWM configuration
#define PWM_PIN PB1
#define PWM_LVL OCR0B
#define PWM_TCR 0x21
#define PWM_SCL 0x01

/* Ramping configuration.
 * Configure the LUT used for the ramping function and the delay between
 * steps of the ramp.
 */

// delay in ms between each ramp step
#define RAMP_DELAY 30

#define SINUSOID 4, 4, 5, 6, 8, 10, 13, 16, 20, 24, 28, 33, 39, 44, 50, 57, 63, 70, 77, 85, 92, 100, 108, 116, 124, 131, 139, 147, 155, 163, 171, 178, 185, 192, 199, 206, 212, 218, 223, 228, 233, 237, 241, 244, 247, 250, 252, 253, 254, 255
// natural log of a sinusoid
#define LN_SINUSOID 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 8, 8, 9, 10, 11, 12, 14, 16, 18, 21, 24, 27, 32, 37, 43, 50, 58, 67, 77, 88, 101, 114, 128, 143, 158, 174, 189, 203, 216, 228, 239, 246, 252, 255
// perceived intensity is basically linearly increasing
#define SQUARED 4, 4, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 21, 24, 27, 30, 33, 37, 40, 44, 48, 53, 57, 62, 67, 72, 77, 83, 88, 94, 100, 107, 113, 120, 127, 134, 141, 149, 157, 165, 173, 181, 190, 198, 207, 216, 226, 235, 245, 255
// smooth sinusoidal ramping
#define SIN_SQUARED_4 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 9, 10, 10, 11, 13, 14, 15, 16, 18, 20, 21, 23, 25, 28, 30, 32, 35, 38, 41, 44, 47, 50, 54, 57, 61, 65, 69, 73, 77, 81, 86, 90, 95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 156, 161, 166, 171, 176, 181, 186, 190, 195, 200, 204, 209, 213, 217, 221, 224, 228, 231, 234, 237, 240, 243, 245, 247, 249, 250, 252, 253, 254, 254, 255, 255
// smooth sinusoidal ramping
#define SIN_SQUARED 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 9, 10, 11, 11, 12, 14, 15, 16, 17, 19, 21, 22, 24, 26, 29, 31, 33, 36, 39, 42, 45, 48, 51, 54, 58, 62, 66, 69, 74, 78, 82, 87, 91, 96, 100, 105, 110, 115, 120, 125, 130, 135, 140, 146, 151, 156, 161, 166, 171, 176, 181, 186, 191, 195, 200, 204, 209, 213, 217, 221, 224, 228, 231, 234, 237, 240, 243, 245, 247, 249, 251, 252, 253, 254, 254, 255, 255

// select which ramping profile to use.
// store in program memory. It would use too much SRAM.
uint8_t const ramp_LUT[] PROGMEM = { SIN_SQUARED };


/* Rise-Fall Ramping brightness selection /\/\/\/\
 * cycle through PWM values from ramp_LUT (look up table). Traverse LUT 
 * forwards, then backwards. Current PWM value is saved in noinit_lvl so
 * it is available at next startup (after a short press).
*/
void ramp()
{
	uint8_t i = 0;
	while (1){
		for (i = 0; i < sizeof(ramp_LUT); i++){
			PWM_LVL = pgm_read_byte(&(ramp_LUT[i]));
			noinit_lvl = PWM_LVL; // remember after short power off
			_delay_ms(RAMP_DELAY); //gives a period of x seconds
		}
		for (i = sizeof(ramp_LUT) - 1; i > 0; i--){
			PWM_LVL = pgm_read_byte(&(ramp_LUT[i]));
			noinit_lvl = PWM_LVL; // remember after short power off
			_delay_ms(RAMP_DELAY); //gives a period of x seconds
		}
		
	}
}

/* Rising Ramping brightness selection //////
 * Cycle through PWM values from ramp_LUT (look up table). Current PWM 
 * value is saved in noinit_lvl so it is available at next startup 
 * (after a short press)
*/
void ramp2()
{
	uint8_t i = 0;
	while (1){
		for (i = 0; i < sizeof(ramp_LUT); i++){
			PWM_LVL = pgm_read_byte(&(ramp_LUT[i]));
			noinit_lvl = PWM_LVL; // remember after short power off
			_delay_ms(RAMP_DELAY); //gives a period of x seconds
		}
		
		//_delay_ms(1000);
	}
}


int main(void)
{
	// PWM setup 
    // set PWM pin to output
    DDRB |= _BV(PWM_PIN);
    // PORTB = 0x00; // initialised to 0 anyway

    // Initialise PWM on output pin and set level to zero
    TCCR0A = PWM_TCR;
    TCCR0B = PWM_SCL;

    PWM_LVL = 0;
	
	#ifdef 	MODE_MEMORY // get mode from eeprom
	
	noinit_mode =  eeprom_read_byte(&MODE_P);
	
	// skip ramp selected mode (mode 5) if the selected level was lost
	if (noinit_decay && noinit_mode == 5)
	{
		++noinit_mode;
	}
	#else // try to use mode from sram
	
	if (noinit_decay) // not short press, forget mode
	{
		noinit_mode = 0;
	}
	#endif
	
	if (!noinit_decay) // no decay, it was a short press
	{
		++noinit_mode;
	}

    // set noinit data for next boot
    noinit_decay = 0;

    // mode needs to loop back around
    // (or the mode is invalid)
    if (noinit_mode > 5) // there are 6 modes
    {
        noinit_mode = 0;
    }
    
    #ifdef 	MODE_MEMORY // remember mode in eeprom
    eeprom_busy_wait(); //make sure eeprom is ready
	eeprom_write_byte(&MODE_P, noinit_mode); // save mode
	#endif
	
    switch(noinit_mode){
        case 0:
        PWM_LVL = 0xFF;
        break;
        case 1:
        PWM_LVL = 0x40;
        break;
        case 2:
        PWM_LVL = 0x10;
        break;
        case 3:
        PWM_LVL = 0x04;
        break;
        case 4:
        ramp(); // ramping brightness selection
        break;
        case 5:
        PWM_LVL = noinit_lvl; // use value selected by ramping function
        break;
    }
    
    while(1);
    return 0;
}
