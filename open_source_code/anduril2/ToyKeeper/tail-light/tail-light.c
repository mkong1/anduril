/*
 * This is intended for use on bike tail lights with a clicky switch.
 * Ideally, a red XP-E2 running at 700mA.
 * It's mostly based on JonnyC's STAR on-time firmware.
 *
 * Original author: JonnyC
 * Modifications: ToyKeeper / Selene Scriven
 *
 * NANJG 105C Diagram
 *           ---
 *         -|   |- VCC
 *  Star 4 -|   |- Voltage ADC
 *  Star 3 -|   |- PWM
 *     GND -|   |- Star 2
 *           ---
 *
 * FUSES
 *      (check bin/flash*.sh for recommended values)
 *
 * STARS (not used)
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

#define VOLTAGE_MON			// Comment out to disable
#define OWN_DELAY			// Should we use the built-in delay or our own?

#define MODE_MOON			6	// Can comment out to remove mode, but should be set through soldering stars
#define MODE_LOW			14	// Can comment out to remove mode
#define MODE_MED			39	// Can comment out to remove mode
#define MODE_HIGH			120	// Can comment out to remove mode
#define MODE_HIGHER			255	// Can comment out to remove mode
#define SOLID_MODES			5	// How many non-blinky modes will we have?
#define DUAL_BEACON_MODES	5+3	// How many beacon modes will we have (with background light on)?
#define SINGLE_BEACON_MODES	5+3+1	// How many beacon modes will we have (without background light on)?
#define BATT_CHECK_MODE		5+3+1+1	// which level is the battery check?
#define TOTAL_MODES			5+3+1+1	// Total number of modes

#define WDT_TIMEOUT			2	// Number of WTD ticks before mode is saved (.5 sec each)

//#define ADC_LOW				130	// When do we start ramping
//#define ADC_CRIT			120 // When do we shut the light off

#define ADC_42          184 // the ADC value we expect for 4.20 volts
#define ADC_100         184 // the ADC value for 100% full (4.2V resting)
#define ADC_75          174 // the ADC value for 75% full (4.0V resting)
#define ADC_50          164 // the ADC value for 50% full (3.8V resting)
#define ADC_25          154 // the ADC value for 25% full (3.6V resting)
#define ADC_0           139 // the ADC value for 0% full (3.3V resting)
#define ADC_LOW         123 // When do we start ramping down
#define ADC_CRIT        113 // When do we shut the light off

/*
 * =========================================================================
 */

#ifdef OWN_DELAY
#include <util/delay_basic.h>
// Having own _delay_ms() saves some bytes AND adds possibility to use variables as input
static void _delay_ms(uint16_t n)
{
    while(n-- > 0)
        _delay_loop_2(1000);
}
#else
#include <util/delay.h>
#endif

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#define STAR2_PIN   PB0
#define STAR3_PIN   PB4
#define STAR4_PIN   PB3
#define PWM_PIN     PB1
#define VOLTAGE_PIN PB2
#define ADC_CHANNEL 0x01	// MUX 01 corresponds with PB2
#define ADC_DIDR 	ADC1D	// Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06	// clk/64

#define PWM_LVL		OCR0B	// OCR0B is the output compare register for PB1

/*
 * global variables
 */

// Mode storage
uint8_t eepos = 0;
uint8_t eep[32];
#define memory 1
//uint8_t memory = 0;

// Modes (gets set when the light starts up)
static uint8_t modes[TOTAL_MODES] = { // high enough to handle all
	MODE_MOON, MODE_LOW, MODE_MED, MODE_HIGH, MODE_HIGHER, // regular solid modes
	MODE_MOON, MODE_LOW, MODE_MED, // dual beacon modes
	MODE_HIGHER, // single beacon modes
	MODE_MED, // battery check
};
volatile uint8_t mode_idx = 0;
// int     mode_dir = 0; // 1 or -1. Determined when checking stars. Do we increase or decrease the idx when moving up to a higher mode.
#define mode_dir 1
PROGMEM const uint8_t voltage_blinks[] = {
    ADC_0,    // 1 blink  for 0%-25%
    ADC_25,   // 2 blinks for 25%-50%
    ADC_50,   // 3 blinks for 50%-75%
    ADC_75,   // 4 blinks for 75%-100%
    ADC_100,  // 5 blinks for >100%
};

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
	if (eepos < 32) mode_idx = eep[eepos];//&0x40; What the?
	else eepos=0;
}

inline void next_mode() {
	mode_idx += mode_dir;
	if (mode_idx > (TOTAL_MODES - 1)) {
		// Wrap around
		mode_idx = 0;
	}
}

inline void WDT_on() {
	// Setup watchdog timer to only interrupt, not reset, every 500ms.
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = (1<<WDTIE) | (1<<WDP2) | (1<<WDP0); // Enable interrupt every 500ms
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

inline void ADC_on() {
	ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
	DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

inline void ADC_off() {
	ADCSRA &= ~(1<<7); //ADC off
}

#ifdef VOLTAGE_MON
uint8_t get_voltage() {
	// Start conversion
	ADCSRA |= (1 << ADSC);
	// Wait for completion
	while (ADCSRA & (1 << ADSC));
	// See if voltage is lower than what we were looking for
	return ADCH;
}
#endif

ISR(WDT_vect) {
	static uint8_t ticks = 0;
	if (ticks < 255) ticks++;

	if (ticks == WDT_TIMEOUT) {
#if memory
			store_mode_idx(mode_idx);
#else
			// Reset the mode to the start for next time
			store_mode_idx(0);
#endif
	}
}

int main(void)
{
	// All ports default to input, but turn pull-up resistors on for the stars
	// (not the ADC input!  Made that mistake already)
	// (stars not used)
	//PORTB = (1 << STAR2_PIN) | (1 << STAR3_PIN) | (1 << STAR4_PIN);

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

	// memory is always enabled
	//memory = 1;

	// Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
	// Will allow us to go idle between WDT interrupts
	set_sleep_mode(SLEEP_MODE_IDLE);

	// Determine what mode we should fire up
	// Read the last mode that was saved
	read_mode_idx();
	if (mode_idx&0x40) {
		// Indicates we did a short press last time, go to the next mode
		// Remove short press indicator first
		mode_idx &= 0x3f;
		next_mode(); // Will handle wrap arounds
	} else {
		// Didn't have a short press, keep the same mode
	}
	// Store mode with short press indicator
	store_mode_idx(mode_idx|0x40);

	WDT_on();

	// Now just fire up the mode
	PWM_LVL = modes[mode_idx];

	uint8_t i = 0;
#ifdef VOLTAGE_MON
	uint8_t lowbatt_cnt = 0;
	uint8_t voltage;
	voltage = get_voltage();
#endif
	while(1) {
		if(mode_idx < SOLID_MODES) {
			sleep_mode();
		} else if (mode_idx < DUAL_BEACON_MODES) {
			for(i=0; i<4; i++) {
				PWM_LVL = modes[mode_idx-SOLID_MODES+2];
				_delay_ms(5);
				PWM_LVL = modes[mode_idx];
				_delay_ms(65);
			}
			_delay_ms(720);
		} else if (mode_idx < SINGLE_BEACON_MODES) {
			PWM_LVL = modes[SINGLE_BEACON_MODES-1];
			_delay_ms(1);
			PWM_LVL = 0;
			_delay_ms(249);
			PWM_LVL = modes[SINGLE_BEACON_MODES-1];
			_delay_ms(1);
			PWM_LVL = 0;
			_delay_ms(749);
		} else if (mode_idx < BATT_CHECK_MODE) {
			uint8_t blinks = 0;
			// turn off and wait one second before showing the value
			// (also, ensure voltage is measured while not under load)
			PWM_LVL = 0;
			_delay_ms(1000);
			voltage = get_voltage();
			voltage = get_voltage(); // the first one is unreliable
			// division takes too much flash space
			//voltage = (voltage-ADC_LOW) / (((ADC_42 - 15) - ADC_LOW) >> 2);
			// a table uses less space than 5 logic clauses
			for (i=0; i<sizeof(voltage_blinks); i++) {
				if (voltage > pgm_read_byte(voltage_blinks + i)) {
					blinks ++;
				}
			}
			
			// blink up to five times to show voltage
			// (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
			for(i=0; i<blinks; i++) {
				PWM_LVL = MODE_MED;
				_delay_ms(100);
				PWM_LVL = 0;
				_delay_ms(400);
			}
			_delay_ms(1000);  // wait at least 1 second between readouts
		}
#ifdef VOLTAGE_MON
		if (ADCSRA & (1 << ADIF)) {  // if a voltage reading is ready
			voltage = get_voltage();
			// See if voltage is lower than what we were looking for
			if (voltage < ((mode_idx == 0) ? ADC_CRIT : ADC_LOW)) {
				++lowbatt_cnt;
			} else {
				lowbatt_cnt = 0;
			}
			// See if it's been low for a while, and maybe step down
			if (lowbatt_cnt >= 3) {
				if (mode_idx > 0) {
					// Go to a dimmer mode
					if (mode_idx == DUAL_BEACON_MODES) {
						// step down from heartbeat beacon to lowest solid
						mode_idx = 0;
					}
					else if (mode_idx == SOLID_MODES) {
						// step down from lowest flasher to lowest solid
						mode_idx = 0;
					}
					else { // lower the brightness
						mode_idx -= 1;
					}
				} else { // Already at the lowest mode
					// Turn off the light
					PWM_LVL = 0;
					// Disable WDT so it doesn't wake us up
					WDT_off();
					// Power down as many components as possible
					set_sleep_mode(SLEEP_MODE_PWR_DOWN);
					sleep_mode();
				}
				lowbatt_cnt = 0;
				// Wait at least 1 second before lowering the level again
				_delay_ms(1000);  // this will interrupt blinky modes
			}

			// Make sure conversion is running for next time through
			ADCSRA |= (1 << ADSC);
		}
#endif
	}
}
