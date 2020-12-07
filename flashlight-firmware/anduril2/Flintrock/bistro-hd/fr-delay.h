#ifndef FR_DELAY_H
#define FR_DELAY_H

 /* Replacement for tk replacement delay functions, now with powersaving sleep.
 *
 * 2017 by Flintrock (C) highly inspired by original from TK.
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
 */ 

#include "fr-tk-attiny.h"

// USE timed sleeps for delay, by FR, only costs a few bytes.
// adding a global time counter (costs a bit more) to allow timeout of fast-presses

#ifndef USE_PWM4 // uses PWM4 instead if active
ISR(_TIMER0_OVF_vect_, ISR_NAKED){	
   __asm__ volatile("reti");
} // timer overflow wake interrupt.
#endif
  
// global clock is an alternative method to alternative method to determine
// timeout of fast_presses (to get to config)
//register uint16_t global_clock asm ("r2");
void _delay_sleep_ms(uint16_t n)
{// 2017 by Flintrock
	set_sleep_mode(SLEEP_MODE_IDLE);
	uint8_t counter;
	#if ! defined(USE_PWM4)  // prefer timer 0 because it's setup twice slower
  	   _TIMSK_ |= (1<<TOIE0); // enable timer overflow interrupt.
  	   TCNT0 = 1; // restart the clock.  Will glitch the PWM, but that's fine.
	   #define cycle_counts 500  // approximate for timing math, to get int result 
    #else  // else an interrupt is already setup in PWM.
	   TCNT1 = 1; // restart the clock.  Will glitch the PWM, but that's fine.
	   #define cycle_counts 250  // will use twice slower prescale when pwm4 is enabled
	                             // but will only ramp up not down and will wake twice per cycle. 
								 // net effect, still 250 counts per cycle (average).
	#endif 
	while (n-- > 0) {
		// prescale set to 1 in bistro, interrupt on "bottom", counts up, then down, so every 512 cycles.
		// so F_CPU/512/1000 ticks per ms, rounding to 500, for attiny 25 8mhz it's 8 ticks per ms.
		// we could maybe make this more exact (ex 0.1ms) using OCR0A/B but they're in use for PWM
		// Is there another way?
		for(counter=0; counter<(F_CPU/cycle_counts/1000); counter++){
  		  sleep_cpu();
		}
	}
//	global_clock+=n>>2;  // Only keep track of delays of 4ms or more, it's good enough and saves space with a 1 byte clock.
	                    // This will be used to reset fast_presses when it rolls over so 255*2 ms or half a second.
						//  Change to n>>1 to reset at 1/2 second instead.
}


void _delay_sleep_s()  
{
	_delay_sleep_ms(1000);
}
#endif