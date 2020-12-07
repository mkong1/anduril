/* MTN_momentary_temp version 1.0
 *
 * Changelog
 *
 * 1.0 First stab at this based on STAR_momentary v1.6
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
 *		Star 2 = Alt PWM output
 *		Star 3 = Temp sensor input
 *		Star 4 = Switch input
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

// PWM Mode
#define PHASE 0b00000001
#define FAST  0b00000011

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define VOLTAGE_MON			// Comment out to disable - ramp down and eventual shutoff when battery is low
#define TEMP_MON            // Comment out to disable - ramp down/up to stabilize temp (read as voltage)
#define MODES			0,0,32,125,255		// Must be low to high, and must start with 0
#define ALT_MODES		0,2,32,125,255		// Must be low to high, and must start with 0, the defines the level for the secondary output. Comment out if no secondary output
#define MODE_PWM		0,PHASE,FAST,FAST,FAST		// Define one per mode above. 0 tells the light to go to sleep
#define TURBO				// Comment out to disable - full output with a step down after n number of seconds
							// If turbo is enabled, it will be where 255 is listed in the modes above
#define TURBO_TIMEOUT	5625 // How many WTD ticks before before dropping down (.016 sec each)
							// 90  = 5625
							// 120 = 7500
							
#define BATT_LOW		130	// When do we start ramping
#define BATT_CRIT		120 // When do we shut the light off
#define TEMP_HIGH       120 // When we should lower the output
#define TEMP_LOW        100 // When we should raise the output
#define BATT_ADC_DELAY	188	// Delay in ticks between low-bat rampdowns (188 ~= 3s)
#define TEMP_ADC_DELAY	188	// Delay in ticks before checking temp (188 ~= 3s)
#define TEMP_ADC_DELAY_AFTER_RAMP 2000 // Delay in ticks before checking temp again after a ramp

#define MOM_ENTER_DUR   128 // 16ms each.  Comment out to disable this feature
#define MOM_EXIT_DUR    128 // 16ms each

#define MOM_MODE_IDX    4   // The index of the mode to use in MODES above, starting at index of 0

#define LOW_TO_HIGH			// comment out to go high to low for the mode order instead

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

#define SWITCH_PIN  PB3		// what pin the switch is connected to, which is Star 4
#define PWM_PIN     PB1
#define ALT_PWM_PIN PB0
#define ADC_DIDR 	ADC1D	// Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06	// clk/64

#define PWM_LVL     OCR0B   // OCR0B is the output compare register for PB1
#define ALT_PWM_LVL OCR0A   // OCR0A is the output compare register for PB0

//#define DEBOUNCE_BOTH          // Comment out if you don't want to debounce the PRESS along with the RELEASE
                               // PRESS debounce is only needed in special cases where the switch can experience errant signals
#define DB_PRES_DUR 0b00000001 // time before we consider the switch pressed (after first realizing it was pressed)
#define DB_REL_DUR  0b00001111 // time before we consider the switch released
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
const uint8_t modes[]     = { MODES    };
#ifdef ALT_MODES
const uint8_t alt_modes[] = { ALT_MODES };
#endif
const uint8_t mode_pwm[] = { MODE_PWM };
volatile uint8_t mode_idx = 0;
volatile uint8_t press_duration = 0;
volatile uint8_t in_momentary = 0;
#ifdef VOLTAGE_MON
volatile uint8_t adc_channel = 1;	// MUX 01 corresponds with PB2, 02 for PB4. Will switch back and forth
#else
volatile uint8_t adc_channel = 2;	// MUX 01 corresponds with PB2, 02 for PB4. Will switch back and forth
#endif

// Debounce switch press value
#ifdef DEBOUNCE_BOTH
int is_pressed()
{
	static uint8_t pressed = 0;
	// Keep track of last switch values polled
	static uint8_t buffer = 0x00;
	// Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
	buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);
	
	if (pressed) {
		// Need to look for a release indicator by seeing if the last switch status has been 0 for n number of polls
		pressed = (buffer & DB_REL_DUR);
	} else {
		// Need to look for pressed indicator by seeing if the last switch status was 1 for n number of polls
		pressed = ((buffer & DB_PRES_DUR) == DB_PRES_DUR);
	}

	return pressed;
}
#else
int is_pressed()
{
	// Keep track of last switch values polled
	static uint8_t buffer = 0x00;
	// Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
	buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);
	
	return (buffer & DB_REL_DUR);
}
#endif

void next_mode() {
	if (++mode_idx >= sizeof(modes)) {
		// Wrap around
		mode_idx = 0;
	}	
}

void prev_mode() {
	if (mode_idx == 0) {
		// Wrap around
		mode_idx = sizeof(modes) - 1;
	} else {
		--mode_idx;
	}
}

inline void PCINT_on() {
	// Enable pin change interrupts
	GIMSK |= (1 << PCIE);
}

inline void PCINT_off() {
	// Disable pin change interrupts
	GIMSK &= ~(1 << PCIE);
}

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

inline void WDT_off()
{
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	MCUSR &= ~(1<<WDRF);			// Clear Watchdog reset flag
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = 0x00;					// Disable WDT
	sei();							// Enable interrupts
}

void ADC_on() {
	ADMUX  = (1 << REFS0) | (1 << ADLAR) | adc_channel; // 1.1v reference, left-adjust, ADC1/PB2 or ADC2/PB3
    DIDR0 |= (1 << ADC_DIDR);							// disable digital input on ADC pin to reduce power consumption
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale, Single Conversion mode
}

void ADC_off() {
	ADCSRA &= ~(1 << ADSC); //ADC off
}

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

// The watchdog timer is called every 16ms
ISR(WDT_vect) {

	//static uint8_t  press_duration = 0;  // Pressed or not pressed
	static uint16_t turbo_ticks = 0;
	static uint16_t batt_adc_ticks = BATT_ADC_DELAY;
	static uint16_t temp_adc_ticks = TEMP_ADC_DELAY;
	static uint8_t  lowbatt_cnt = 0;
	static uint8_t  high_temp_cnt = 0;
	static uint8_t  low_temp_cnt = 0;
	static uint8_t  highest_mode_idx = 255;

	if (is_pressed()) {
		if (press_duration < 255) {
			press_duration++;
		}
		
		#ifdef MOM_ENTER_DUR
		if (in_momentary) {
			// Turn on full output
			mode_idx = MOM_MODE_IDX;
			if (press_duration == MOM_EXIT_DUR) {
				// Turn light off and disable momentary
				mode_idx = 0;
				in_momentary = 0;
			}
			return;
		}
		#endif	

		if (press_duration == LONG_PRESS_DUR) {
			// Long press
			#ifdef LOW_TO_HIGH
				prev_mode();
			#else
				next_mode();
			#endif
			highest_mode_idx = mode_idx;
		}
		#ifdef MOM_ENTER_DUR
		if (press_duration == MOM_ENTER_DUR) {
			in_momentary = 1;
			press_duration = 0;
		}
		#endif
		// Just always reset turbo timer whenever the button is pressed
		turbo_ticks = 0;
		// Same with the ramp down delay
		batt_adc_ticks = BATT_ADC_DELAY;
		temp_adc_ticks = TEMP_ADC_DELAY;
	
	} else {
		
		#ifdef MOM_ENTER_DUR
		if (in_momentary) {
			// Turn off the light
			mode_idx = 0;
			return;
		}
		#endif
		
		// Not pressed
		if (press_duration > 0 && press_duration < LONG_PRESS_DUR) {
			// Short press
			#ifdef LOW_TO_HIGH
				next_mode();
			#else
				prev_mode();
			#endif
			highest_mode_idx = mode_idx;
		} else {
			// Only do turbo check when switch isn't pressed
		#ifdef TURBO
			if (modes[mode_idx] == 255) {
				turbo_ticks++;
				if (turbo_ticks > TURBO_TIMEOUT) {
					// Go to the previous mode
					prev_mode();
				}
			}
		#endif
			// Only do voltage monitoring when the switch isn't pressed
			// See if conversion is done. We moved this up here because we want to stay on
			// the current ADC input until the conversion is done, and then switch to the new
			// input, start the monitoring
			if (batt_adc_ticks > 0) {
				--batt_adc_ticks;
			}
			if (temp_adc_ticks > 0) {
				--temp_adc_ticks;
			}
			if (ADCSRA & (1 << ADIF)) {
				if (adc_channel == 0x01) {
					
					if (batt_adc_ticks == 0) {
						// See if voltage is lower than what we were looking for
						if (ADCH < ((mode_idx == 1) ? BATT_CRIT : BATT_LOW)) {
							++lowbatt_cnt;
						} else {
							lowbatt_cnt = 0;
						}
					
						// See if it's been low for a while
						if (lowbatt_cnt >= 4) {
							prev_mode();
							highest_mode_idx = mode_idx;
							lowbatt_cnt = 0;
							// If we reach 0 here, main loop will go into sleep mode
							// Restart the counter to when we step down again
							batt_adc_ticks = BATT_ADC_DELAY;
							temp_adc_ticks = TEMP_ADC_DELAY;
						}
					}
					// Switch ADC to temp monitoring
					adc_channel = 0x02;
					ADMUX = ((ADMUX & 0b11111100) | adc_channel);
				} else if (adc_channel == 0x02) {
					
					if (temp_adc_ticks == 0) {
						// See if temp is higher than the high threshold
						if (ADCH > ((mode_idx == 1) ? 255 : TEMP_HIGH)) {
							++high_temp_cnt;
						// See if it's lower that the low threshold, but not at or above the batt-adjusted mode
						} else if (ADCH < (modes[mode_idx] >= modes[highest_mode_idx] ? 0 : TEMP_LOW)) {
							++low_temp_cnt;						
						} else {
							high_temp_cnt = 0;
							low_temp_cnt = 0;
						}
				
						// See if it's been low for a while
						if (high_temp_cnt >= 4) {
							// TODO - step down half
							prev_mode();
							high_temp_cnt = 0;
							low_temp_cnt = 0;
							// If we reach 0 here, main loop will go into sleep mode
							// Restart the counter to when we step down again
							temp_adc_ticks = TEMP_ADC_DELAY_AFTER_RAMP;
						} else if (low_temp_cnt >= 4) {
							// TODO - step up half
							next_mode();
							high_temp_cnt = 0;
							low_temp_cnt = 0;
							// TODO - TEMP_ADC_DELAY_AFTER_RAMP?? Might jump up and cause it to overheat waiting too long
							temp_adc_ticks = TEMP_ADC_DELAY;
						}
					}
					#ifdef VOLTAGE_MON
					// Switch ADC to battery monitoring
					adc_channel = 0x01;
					ADMUX = ((ADMUX & 0b11111100) | adc_channel);
					#endif;
				}
			}
			// Start conversion for next time through
			ADCSRA |= (1 << ADSC);
		}
		press_duration = 0;
	}
}

int main(void)
{	
	// Set all ports to input, and turn pull-up resistors on for the inputs we are using
	DDRB = 0x00;
	PORTB = (1 << SWITCH_PIN);

	// Set the switch as an interrupt for when we turn pin change interrupts on
	PCMSK = (1 << SWITCH_PIN);
	
    // Set PWM pin to output
	#ifdef ALT_MODES
    DDRB = (1 << PWM_PIN) | (1 << ALT_PWM_PIN);
	#else
	DDRB = (1 << PWM_PIN);
	#endif

    // Set timer to do PWM for correct output pin and set prescaler timing
    //TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
	
	// Turn features on or off as needed
	ADC_on();
	ACSR   |=  (1<<7); //AC off
	
	// Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_until_switch_press();
	
	uint8_t last_mode_idx = 0;
	
	while(1) {
		// We will never leave this loop.  The WDT will interrupt to check for switch presses and 
		// will change the mode if needed.  If this loop detects that the mode has changed, run the
		// logic for that mode while continuing to check for a mode change.
		if (mode_idx != last_mode_idx) {
			// The WDT changed the mode.
			if (mode_idx > 0) {
				// No need to change the mode if we are just turning the light off
				// Check if the PWM mode is different
				if (mode_pwm[last_mode_idx] != mode_pwm[mode_idx]) {
					#ifdef ALT_MODES
					TCCR0A = mode_pwm[mode_idx] | 0b10100000;  // Use both outputs
					#else
					TCCR0A = mode_pwm[mode_idx] | 0b00100000;  // Only use the normal output
					#endif
				}
			}
			PWM_LVL     = modes[mode_idx];
			#ifdef ALT_MODES
			ALT_PWM_LVL = alt_modes[mode_idx];
			#endif
			last_mode_idx = mode_idx;
			if (mode_pwm[mode_idx] == 0) {
				_delay_ms(1); // Need this here, maybe instructions for PWM output not getting executed before shutdown?
				// Go to sleep
				sleep_until_switch_press();
			}
		}
	}

    return 0; // Standard Return Code
}