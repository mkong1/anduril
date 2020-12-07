//-------------------------------------------------------------------------------------
// eswitch.c - based on STAR_mom_1.0 (STAR firmware) from JonnyC
// =========
//  Modified by Tom E
// 
// Change History
// --------------
// 03/01/2014 - from JohnnyC
// 03/02/2014 - first update: added One-click turn off, shortened the long duration press,
//              add comments, re-format code
// 03/07/2014 - increase long press from 250 to 300 msecs
// 03/08/2014 - new mode sets for FET based Nanjg's added
// 05/04/2014 - add strobe mode (compile option)
// 
//-------------------------------------------------------------------------------------
#define F_CPU 4800000UL

/*
 * =========================================================================
 * Settings to modify per driver
 */

 
 
//-------------------------------------------------------------------------------------
// Used for FET based Phase Corrected PWM:
//-------------------------------------------------------------------------------------
//#define MODES	0,12,64,128,255	// 4 modes %: 5-25-50-100 (for Kenny's Warsun)

#define MODES	0,1,12,102,255		// 4 modes: %: ml-5-40-100 (for Michael G. w/strobe)

//#define MODES	0,1,4,16,100,255	// 5 modes

// RMM - used in a MT-G2 build (5 modes):
//#define MODES	0,1,4,25,120,255

// RMM - also used this set (6 modes):
//#define MODES	0,1,4,8,20,110,255


//-------------------------------------------------------------------------------------
// Used for 7135 based Phase Corrected PWM:
//-------------------------------------------------------------------------------------
//#define MODES	0,5,15,92,255		// 5=moonlight, 15=6%, 92=36%, 255=100%

// Original mode settings, used with Fast PWM:
//#define MODES	0,16,32,125,255	// Must be low to high, and must start with 0

//-------------------------------------------------------------------------------------

#define LONG_PRESS_DUR 21	// How long until a press is a long press (32 is ~0.5 sec), 21 = 0.336 secs

#define PWM_OPTION 0x21		// 0x21 for phase corrected (9.4 kHz) or 0x23 for fast-PWM (18 kHz)

// ----- One-Click Turn OFF option --------------------------------------------
#define IDLE_LOCK			// Comment out to disable
#define IDLE_TIME 75		// make the time-out 1.2 seconds
// ----------------------------------------------------------------------------

// ----- Option Strobe Mode ---------------------------------------------------
#define ENABLE_STROBE
#define XLONG_PRESS_DUR 48	// 38=0.6 secs, 48=0.75 secs

// ----- Turbo Settings -------------------------------------------------------
//#define TURBO			// Comment out to disable - full output, step downs after n number of secs
								// If turbo is enabled, it will be where 255 is listed in the modes above
#define TURBO_TIMEOUT	5625	// time before dropping down (16 msecs per, 5625=90 secs)
// ----------------------------------------------------------------------------
							
// ----- Voltage Monitoring Settings ------------------------------------------
#define VOLTAGE_MON		// Comment out to disable - ramp down and eventual shutoff when battery is low

  #define ADC_LOW		130	// When do we start ramping (3.1v), 5 per 0.1v
  #define ADC_CRIT	115	// When do we shut the light off (2.80v)
  #define ADC_DELAY	188	// Delay in ticks between low-bat rampdowns (188 ~= 3s)
// ----------------------------------------------------------------------------

/*
 * =========================================================================
 */

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>	
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>

#define STAR2_PIN   PB0		// Star 2, MCU pin #5
#define STAR3_PIN   PB4		// Star 3, MCU pin #3
#define SWITCH_PIN  PB3		// Star 4, MCU pin #2, pin the switch is connected to
#define PWM_PIN     PB1		// PWM Output, MCU pin #6
#define VOLTAGE_PIN PB2		// Voltage monitoring input, MCU pin #7

#define ADC_CHANNEL 0x01	// MUX 01 corresponds with PB2
#define ADC_DIDR 	ADC1D		// Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06	// clk/64

#define PWM_LVL OCR0B       // OCR0B is the output compare register for PB1

#define DB_REL_DUR 0b00001111 // time before we consider the switch released after
							  // each bit of 1 from the right equals 16ms, so 0x0f = 64ms


/*
 * The actual program
 * =========================================================================
 */

/*
 * global variables
 */
PROGMEM  const uint8_t modes[] = { MODES };

volatile uint8_t modeIdx = 0;				// current mode index (can be called a mode # (0 = OFF)

volatile uint8_t prevModeIdx = 0;		// used to restore the initial mode when exiting strobe mode

volatile uint8_t press_duration = 0;	// current press duration

volatile uint8_t mypwm=0;					// PWM output value, used in strobe mode

#ifdef IDLE_LOCK
 uint8_t byIdleTicks = 0;
#endif


/**************************************************************************************
* is_pressed - debounce the switch release, not the switch press
* ==========
**************************************************************************************/
int is_pressed()
{
	// Keep track of last switch values polled
	static uint8_t buffer = 0x00;

	// Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
	buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);

	// Return "true" if any of the last 4 polls detected a press. All last 4 polls have to
	//  be "not pressed" for this to return "false"
	return (buffer & DB_REL_DUR);
}

/**************************************************************************************
* PCINT_on - Enable pin change interrupts
* ========
**************************************************************************************/
inline void PCINT_on()
{
	GIMSK |= (1 << PCIE);
}

/**************************************************************************************
* PCINT_off - Disable pin change interrupts
* =========
**************************************************************************************/
inline void PCINT_off()
{
	GIMSK &= ~(1 << PCIE);
}

// Need an interrupt for when pin change is enabled to ONLY wake us from sleep.
// All logic of what to do when we wake up will be handled in the main loop.
EMPTY_INTERRUPT(PCINT0_vect);

/**************************************************************************************
* WDT_on - Setup watchdog timer to only interrupt, not reset, every 16ms
* ======
**************************************************************************************/
inline void WDT_on()
{
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = (1<<WDTIE);			// Enable interrupt every 16ms
	sei();							// Enable interrupts
}

/**************************************************************************************
* WDT_off - turn off the WatchDog timer
* =======
**************************************************************************************/
inline void WDT_off()
{
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	MCUSR &= ~(1<<WDRF);			// Clear Watchdog reset flag
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = 0x00;					// Disable WDT
	sei();							// Enable interrupts
}

/**************************************************************************************
* ADC_on - Turn the AtoD Converter ON
* ======
**************************************************************************************/
inline void ADC_on()
{
	ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
    DIDR0 |= (1 << ADC_DIDR);							// disable digital input on ADC pin to reduce power consumption
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

/**************************************************************************************
* ADC_off - Turn the AtoD Converter OFF
* =======
**************************************************************************************/
inline void ADC_off()
{
	ADCSRA &= ~(1<<7); //ADC off
}

/**************************************************************************************
* sleep_until_switch_press - only called with the light OFF
* ========================
**************************************************************************************/
void sleep_until_switch_press()
{
	// Turn the WDT off so it doesn't wake us from sleep
	// Will also ensure interrupts are on or we will never wake up
	WDT_off();
	
	// Make sure the switch is released otherwise we will wake when the user releases the switch
	while (is_pressed())
	{
		_delay_ms(16);

	  #ifdef ENABLE_STROBE
		// If a long press got us here to go into OFF mode, check button is held for strobe mode 
		if (press_duration < 255)
		{
			if (++press_duration == XLONG_PRESS_DUR)
			{
				modeIdx = 99;			// Set strobe mode
				WDT_on();
				return;
			}
		}
	  #endif
	}
	
	press_duration = 0;	// Need to reset press duration since a button release wasn't recorded
	
	
	PCINT_on();		// Enable a pin change interrupt to wake us up
	
	// Put the MCU in a low power state
	sleep_mode();	// Now go to sleep
	
	// Hey, someone must have pressed the switch!!
	
	PCINT_off();	// Disable pin change interrupt because it's only used to wake us up
	
	WDT_on();		// Turn the WDT back on to check for switch presses
	// Go back to main program
}

/**************************************************************************************
* next_mode - switch's to next mode, higher output mode
* =========
**************************************************************************************/
inline void next_mode()
{
	prevModeIdx	 = modeIdx;
	if (++modeIdx >= sizeof(modes))
	{
		modeIdx = 0;	// Wrap around
	}
}

/**************************************************************************************
* prev_mode - switch's to previous mode, lower output mode
* =========
**************************************************************************************/
inline void prev_mode()
{
	prevModeIdx	 = modeIdx;
	if ((modeIdx == 0) || (modeIdx > sizeof(modes)))
	{
		modeIdx = sizeof(modes) - 1;	// Wrap around
	}
	else
	{
		--modeIdx;
	}
}

/**************************************************************************************
* WDT_vect - The watchdog timer - this is invoked every 16ms
* ========
**************************************************************************************/
ISR(WDT_vect)
{
  #ifdef TURBO
	static uint16_t turbo_ticks = 0;
  #endif

  #ifdef VOLTAGE_MON
	static uint8_t  adc_ticks = ADC_DELAY;
	static uint8_t  lowbatt_cnt = 0;
  #endif

	if (is_pressed())
	{
		if (press_duration < 255)
		{
			press_duration++;
		}
		
		if (press_duration == LONG_PRESS_DUR)
		{
			// Long press, go to previous mode
			prev_mode();

		  #ifdef IDLE_LOCK
			byIdleTicks = 0;	// reset idle time
		  #endif
		}

	  #ifdef ENABLE_STROBE
		if (press_duration == XLONG_PRESS_DUR)
		{
			modeIdx = 99;		// make it the "special" strobe value
		}
	  #endif
		

	  #ifdef TURBO
		turbo_ticks = 0;			// Always reset turbo timer whenever the button is pressed
	  #endif

	  #ifdef VOLTAGE_MON
		adc_ticks = ADC_DELAY;	// Same with the ramp down delay
	  #endif
	}
	else		// Not pressed (debounced qualified)
	{
		if (press_duration > 0 && press_duration < LONG_PRESS_DUR)
		{
			if (modeIdx == 99)
			{
				modeIdx = prevModeIdx;
			}
			else
			{
				next_mode();	// Short press, go to next mode
				
			  #ifdef IDLE_LOCK
				if (byIdleTicks >= IDLE_TIME)
				modeIdx = 0;	// Turn OFF the light
				byIdleTicks = 0;	// reset idle time
			  #endif
			}

		}
		else
		{
		  #ifdef IDLE_LOCK
			if (++byIdleTicks == 0)
				byIdleTicks = 255;
		  #endif

			// Only do turbo check when switch isn't pressed
		  #ifdef TURBO
			if (pgm_read_byte(&modes[modeIdx]) == 255)
			{
				turbo_ticks++;
				if (turbo_ticks > TURBO_TIMEOUT)
				{
					prev_mode();	// Go to the previous mode
				}
			}
		  #endif

			// Only do voltage monitoring when the switch isn't pressed
		  #ifdef VOLTAGE_MON
			if (adc_ticks > 0)
			{
				--adc_ticks;
			}
			if (adc_ticks == 0)
			{
				// See if conversion is done
				if (ADCSRA & (1 << ADIF))
				{
					// See if voltage is lower than what we were looking for
					if (ADCH < ((modeIdx == 1) ? ADC_CRIT : ADC_LOW))
					{
						++lowbatt_cnt;
					}
					else
					{
						lowbatt_cnt = 0;
					}
				}
				
				// See if it's been low for a while
				if (lowbatt_cnt >= 4)
				{
					prev_mode();
					lowbatt_cnt = 0;
					// If we reach 0 here, main loop will go into sleep mode
					// Restart the counter to when we step down again
					adc_ticks = ADC_DELAY;
				}
				
				// Make sure conversion is running for next time through
				ADCSRA |= (1 << ADSC);
			}
		  #endif
		}
		press_duration = 0;
	} // button released (not pressed)
}


/**************************************************************************************
* main - main program loop. This is where it all happens...
* ====
**************************************************************************************/
int main(void)
{	
	// Set all ports to input, and turn pull-up resistors on for the inputs we are using
	DDRB = 0x00;
	PORTB = (1 << SWITCH_PIN) | (1 << STAR2_PIN) | (1 << STAR3_PIN);

	// Set the switch as an interrupt for when we turn pin change interrupts on
	PCMSK = (1 << SWITCH_PIN);
	
	// Set PWM pin to output
	DDRB = (1 << PWM_PIN);

	// Set timer to do PWM for correct output pin and set prescaler timing
	TCCR0A = PWM_OPTION; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
	TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
	
	// Turn features on or off as needed
  #ifdef VOLTAGE_MON
	ADC_on();
  #else
	ADC_off();
  #endif

	ACSR   |=  (1<<7); //AC off
	
	// Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_until_switch_press();
	
	uint8_t last_mode_idx = 0;
	
	while (1)	// run forever
	{
		// We will never leave this loop.  The WDT will interrupt to check for switch presses and 
		// will change the mode if needed.  If this loop detects that the mode has changed, run the
		// logic for that mode while continuing to check for a mode change.
		if (modeIdx != last_mode_idx)
		{
			last_mode_idx = modeIdx;	// The WDT changed the mode.

			//---------------------------------------------------
			// Mode Handling
			//---------------------------------------------------

		  #ifdef ENABLE_STROBE
			if (modeIdx == 99)	// strobe at 12.5Hz
			{
				mypwm = 255;
				while (1)
				{
					PWM_LVL = mypwm; _delay_ms(20);
					
					PWM_LVL = 0; _delay_ms(60);
					
					// If the mode got changed, break out of here to process the mode change
					if (modeIdx != last_mode_idx)
						break;
				} // run forever
			}
			else
		  #endif
			{
				PWM_LVL = pgm_read_byte(&modes[modeIdx]);
				if (PWM_LVL == 0)
				{
					_delay_ms(1); // Need this here, maybe instructions for PWM output not getting executed before shutdown?
					
					sleep_until_switch_press();	// Go to sleep
				}
			}

		} // mode change detected

	} // while

	return 0; // Standard Return Code (will never get here)
}