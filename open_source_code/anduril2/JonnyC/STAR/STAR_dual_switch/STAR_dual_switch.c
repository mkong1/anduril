/* STAR_dual_switch version 1.0
 *
 * Changelog
 *
 * 1.0 Initial version
 * 1.1 Changed modes to static and no longer access through progmem. Also changed mode levels to more closely match STAR_momentary
 *
 */

/*
 * NANJG 105C Diagram
 *           ---
 *         -|   |- VCC
 *  Star 4 -|   |- Voltage ADC
 *  Star 3 -|   |- PWM
 *     GND -|   |- Star 2
 *           ---
 *
 * FUSES
 *		I use these fuse settings
 *		Low:  0x75	(4.8MHz CPU without 8x divider, 9.4kHz phase-correct PWM or 18.75kHz fast-PWM)
 *		High: 0xff
 *
 *      For more details on these settings, visit http://github.com/JCapSolutions/blf-firmware/wiki/PWM-Frequency
 *
 * STARS
 *		Star 2 = H-L if connected, L-H if not
 *		Star 3 = Memory if not connected
 *		Star 4 = Input for switch
 *
 * VOLTAGE
 *		Resistor values for voltage divider (reference BLF-VLD README for more info)
 *		Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 *           VCC
 *            |
 *           Vd (~.25 v drop from protection diode)
 *            |
 *          1912 (R1 19,100 ohms)
 *            |
 *            |---- PB2 from MCU
 *            |
 *          4701 (R2 4,700 ohms)
 *            |
 *           GND
 *
 *		ADC = ((V_bat - V_diode) * R2   * 255) / ((R1    + R2  ) * V_ref)
 *		125 = ((3.0   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *		121 = ((2.9   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *
 *		Well 125 and 121 were too close, so it shut off right after lowering to low mode, so I went with
 *		130 and 120
 *
 *		To find out what value to use, plug in the target voltage (V) to this equation
 *			value = (V * 4700 * 255) / (23800 * 1.1)
 *      
 */
#define F_CPU 4800000UL

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define VOLTAGE_MON			// Comment out to disable - ramp down and eventual shutoff when battery is low
#define MODES			8,14,39,125,255	// Must be low to high, star determines which way we cycle through them
#define TURBO				// Comment out to disable - full output with a step down after n number of seconds
							// If turbo is enabled, it will be where 255 is listed in the modes above
#define TURBO_TIMEOUT	5625 // How many WTD ticks before before dropping down (.016 sec each)
							// 90  = 5625
							// 120 = 7500
							
#define ADC_LOW			130	// When do we start ramping
#define ADC_CRIT		120 // When do we shut the light off
#define ADC_DELAY		188	// Delay in ticks between low-bat rampdowns (188 ~= 3s)

/*
 * =========================================================================
 */

//#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>	
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>

#define STAR2_PIN   PB0		// If not connected, will cycle L-H.  Connected, H-L
#define STAR3_PIN   PB4		// If not connected, will enable memory
#define SWITCH_PIN  PB3		// what pin the switch is connected to, which is Star 4
#define PWM_PIN     PB1
#define VOLTAGE_PIN PB2
#define ADC_CHANNEL 0x01	// MUX 01 corresponds with PB2
#define ADC_DIDR 	ADC1D	// Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06	// clk/64

#define PWM_LVL OCR0B       // OCR0B is the output compare register for PB1

#define DB_REL_DUR 0b00001111 // time before we consider the switch released after
							  // each bit of 1 from the right equals 16ms, so 0x0f = 64ms

// Switch handling
#define LONG_PRESS_DUR   32 // How many WDT ticks until we consider a press a long press
                            // 32 is roughly .5 s	

/*
 * The actual program
 * =========================================================================
 */

/*
 * global variables
 */
static   uint8_t modes[] = { MODES };
volatile uint8_t mode_idx = 0;
volatile uint8_t press_duration = 0;
volatile uint8_t low_to_high = 0;

// Mode storage
uint8_t eepos = 0;
uint8_t eep[32];
uint8_t memory = 0;

void store_mode_idx(uint8_t lvl) {  //central method for writing (with wear leveling)
	uint8_t oldpos=eepos;
	eepos=(eepos+1)&31;  //wear leveling, use next cell
	// Write the current mode
	EEARL=eepos;  EEDR=lvl; EECR=32+4; EECR=32+4+2;  //WRITE  //32:write only (no erase)  4:enable  2:go
	while(EECR & 2); //wait for completion
	// Erase the last mode
	EEARL=oldpos;           EECR=16+4; EECR=16+4+2;  //ERASE  //16:erase only (no write)  4:enable  2:go
}
inline void read_mode_idx() {
	eeprom_read_block(&eep, 0, 32);
	while((eep[eepos] == 0xff) && (eepos < 32)) eepos++;
	if (eepos < 32) mode_idx = eep[eepos];//&0x10; What the?
	else eepos=0;
}

// Debounced switch press value
int is_pressed()
{
	// Keep track of last switch values polled
	static uint8_t buffer = 0x00;
	// Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
	buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);
	return (buffer & DB_REL_DUR);
}

void next_mode(uint8_t save) {
	if (++mode_idx >= sizeof(modes)) {
		// Wrap around
		mode_idx = 0;
	}
	if (memory && save)	{
		store_mode_idx(mode_idx);
	}
}

void prev_mode(uint8_t save) {
	if (mode_idx == 0) {
		// Wrap around
		mode_idx = sizeof(modes) - 1;
	} else {
		--mode_idx;
	}
	if (memory && save)	{
		store_mode_idx(mode_idx);
	}
}
/*
inline void PCINT_on() {
	// Enable pin change interrupts
	GIMSK |= (1 << PCIE);
}

inline void PCINT_off() {
	// Disable pin change interrupts
	GIMSK &= ~(1 << PCIE);
}
*/

// Need an interrupt for when pin change is enabled to ONLY wake us from sleep.
// All logic of what to do when we wake up will be handled in the main loop.
EMPTY_INTERRUPT(PCINT0_vect);

inline void WDT_on() {
	// Setup watchdog timer to only interrupt, not reset, every 16ms.
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = (1<<WDTIE);				// Enable interrupt every 16ms
	sei();							// Enable interrupts
}
/*
inline void WDT_off()
{
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	MCUSR &= ~(1<<WDRF);			// Clear Watchdog reset flag
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = 0x00;					// Disable WDT
	sei();							// Enable interrupts
}
*/

inline void ADC_on() {
	ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
    DIDR0 |= (1 << ADC_DIDR);							// disable digital input on ADC pin to reduce power consumption
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

inline void ADC_off() {
	ADCSRA &= ~(1<<7); //ADC off
}

/*
void sleep_until_switch_press()
{
	// This routine takes up a lot of program memory :(
	// Turn the WDT off so it doesn't wake us from sleep
	// Will also ensure interrupts are on or we will never wake up
	WDT_off();
	// Need to reset press duration since a button release wasn't recorded
	press_duration = 0;
	// Enable a pin change interrupt to wake us up
	// However, we have to make sure the switch is released otherwise we will wake when the user releases the switch
	while (is_pressed()) {
		_delay_ms(16);
	}
	PCINT_on();
	// Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
	//set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	// Now go to sleep
	sleep_mode();
	// Hey, someone must have pressed the switch!!
	// Disable pin change interrupt because it's only used to wake us up
	PCINT_off();
	// Turn the WDT back on to check for switch presses
	WDT_on();
	// Go back to main program
}
*/

// The watchdog timer is called every 16ms
ISR(WDT_vect) {

	//static uint8_t  press_duration = 0;  // Pressed or not pressed
	static uint16_t turbo_ticks = 0;
	static uint8_t  adc_ticks = ADC_DELAY;
	static uint8_t  lowbatt_cnt = 0;

	if (is_pressed()) {
		if (press_duration < 255) {
			press_duration++;
		}
		
		if (press_duration == LONG_PRESS_DUR) {
			// Long press
			if (low_to_high) {
				prev_mode(1);
			} else {
				next_mode(1);
			}			
		}
		// Just always reset turbo timer whenever the button is pressed
		turbo_ticks = 0;
		// Same with the ramp down delay
		adc_ticks = ADC_DELAY;
	} else {
		// Not pressed
		if (press_duration > 0 && press_duration < LONG_PRESS_DUR) {
			// Short press
			if (low_to_high) {
				next_mode(1);
			} else {
				prev_mode(1);
			}	
		} else {
			// Only do turbo check when switch isn't pressed
		#ifdef TURBO
			if (modes[mode_idx] == 255) {
				turbo_ticks++;
				if (turbo_ticks > TURBO_TIMEOUT) {
					// Go to the previous mode
					prev_mode(0);
				}
			}
		#endif
			// Only do voltage monitoring when the switch isn't pressed and we aren't at the lowest level
		#ifdef VOLTAGE_MON
			if (mode_idx > 0) {
				if (adc_ticks > 0) {
					--adc_ticks;
				}
				if (adc_ticks == 0) {
					// See if conversion is done
					if (ADCSRA & (1 << ADIF)) {
						// See if voltage is lower than what we were looking for
						if (ADCH < ((mode_idx == 1) ? ADC_CRIT : ADC_LOW)) {
							++lowbatt_cnt;
						} else {
							lowbatt_cnt = 0;
						}
					}
				
					// See if it's been low for a while
					if (lowbatt_cnt >= 4) {
						prev_mode(0);
						lowbatt_cnt = 0;
						// Restart the counter to when we step down again
						adc_ticks = ADC_DELAY;
					}
				
					// Make sure conversion is running for next time through
					ADCSRA |= (1 << ADSC);
				}
			}			
		#endif
		}
		press_duration = 0;
	}
}

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
    TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
	
	// Turn features on or off as needed
	#ifdef VOLTAGE_MON
	ADC_on();
	#else
	ADC_off();
	#endif
	ACSR   |=  (1<<7); //AC off
	
	// Determine if we are going L-H, or H-L based on STAR 2
	if ((PINB & (1 << STAR2_PIN)) == 0) {
		// High to Low
		low_to_high = 0;
	} else {
		low_to_high = 1;
	}
	// Not soldered (1) should enable memory
	memory = ((PINB & (1 << STAR3_PIN)) > 0) ? 1 : 0;
	
	// Don't think we want to ever go to sleep
	// Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
	//set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	//sleep_until_switch_press();
	
	WDT_on();
	
	// Determine what mode we should fire up
	// Read the last mode that was saved
	if (memory) {
		read_mode_idx();
	} else {
		if (low_to_high) {
			mode_idx = 0;
		} else {
			mode_idx = sizeof(modes);
		}		
	}
	
	PWM_LVL = modes[mode_idx];
	
	uint8_t last_mode_idx = mode_idx;
	
	while(1) {
		// We will never leave this loop.  The WDT will interrupt to check for switch presses and 
		// will change the mode if needed.  If this loop detects that the mode has changed, run the
		// logic for that mode while continuing to check for a mode change.
		if (mode_idx != last_mode_idx) {
			// Save the new mode
			last_mode_idx = mode_idx;
			// The WDT changed the mode.
			PWM_LVL = modes[mode_idx];
		}
	}

    return 0; // Standard Return Code
}