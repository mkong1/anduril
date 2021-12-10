/*
 *
 * TheStar Off-time NANJG105C Firmware by _the_ (originally based on STAR 1.1 & STAR Off-time 1.3 by JonnyC)
 *
 * Main differences:
 * - Heavily optimized for size -> free up enough size for all the options
 * - Star logic changed to match BLF preferences
 * - Hidden modes (Strobe, 1s beacon, 10s alpine distress beacon, turbo that stays on..)
 * - Combination of normal (for basic modes) and short cycle memory (for hidden modes)
 * - High-to-low support rewritten (for size and supporting hidden modes)
 * - Turbo step down to predefined PWM level (allows leaving the high away for simpler UI)
 * - Ramp down smoothly during turbo step down to make it less annoying / noticeable
 * - Voltage monitoring rewritten to consume less power and support blinky modes
 *
 * NANJG 105C Diagram
 *           ---
 *         -|   |- VCC
 *  Star 4 -|   |- Voltage ADC
 *  Star 3 -|   |- PWM
 *     GND -|   |- Star 2
 *           ---
 
 * CPU speed is 4.8Mhz without the 8x divider when low fuse is 0x75
 *
 * define F_CPU 4800000  CPU: 4.8MHz  PWM: 9.4kHz       ####### use low fuse: 0x75  #######
 *                             /8     PWM: 1.176kHz     ####### use low fuse: 0x65  #######
 * define F_CPU 9600000  CPU: 9.6MHz  PWM: 19kHz        ####### use low fuse: 0x7a  #######
 *                             /8     PWM: 2.4kHz       ####### use low fuse: 0x6a  #######
 * 
 * Above PWM speeds are for phase-correct PWM.  This program uses Fast-PWM, which when the CPU is 4.8MHz will be 18.75 kHz
 *
 * FUSES
 *		I use these fuse settings
 *		Low:  0x75
 *		High: 0xff
 *
 * STARS
 *		Star 2 = Memory if NOT connected
 *		Star 3 = H-L if connected, L-H if not
 *		Star 4 = Cap for off-time memory
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
 *		To find out what value to use, plug in the target voltage (V) to this equation
 *			value = (V * 4700 * 255) / (23800 * 1.1)
 *      
 */
#define F_CPU 4800000UL


/*
 * =========================================================================
 * Settings to modify per driver - consumed bytes are only for reference, they may change radically because of some other settings (and global optimization)
 */

// Three basic modes by default, define this for four basic modes (lowlow included in main mode cycle) - consumes about 24 bytes more
//#define FOUR_MODES

// Things to do with star 2 - select one of the following
// Turbo timer is enabled by default, it can be turned off by soldering star 2 if this is enabled - consumes 22 bytes
// #define ENABLE_TURNING_OFF_TURBO_TIMER
// 
// Light can be changed to tactical mode (strobe, turbo, low) + hidden goodies with star 2 - consumes 70 bytes
//#define ENABLE_TACTICAL_MODE
//#define TACTICAL_MODE_ON_BY_DEFAULT
//
// Light can be changed to single mode + hidden goodies with star 2 - consumes 18 bytes
//#define ENABLE_SINGLE_MODE

// Memory is on by default, it can be turned off by soldering star 2 if any of the above star 2 functionality is not enabled

// High-to-low mode order, selectable by star 3, consumes 72 bytes
#define ENABLE_HIGH_TO_LOW
//#define HIGH_TO_LOW_ON_BY_DEFAULT

// Normal strobe by default, define this for alternating - consumes 20 bytes
//#define NORMAL_STROBE
//#define ALTERNATING_STROBE // +18 bytes
#define RANDOM_STROBE // +40 bytes

// SOS mode - consumes 70 bytes
#define ENABLE_SOS

// Set of different beacons - consumes 76 to 84 bytes (4- / 3-modes)
#define ENABLE_BEACONS

// Voltage monitoring - consumes 180 bytes
#define ENABLE_VOLTAGE_MONITORING

// Some standard PWM values
#define PWM_OFF					0				// Used for signaling
#define PWM_MIN					7				// Less than this won't light up reliably :(
#define PWM_MAX					255				// Maximum brightness

// PWM values for modes
#define MODE_LOWLOW					PWM_MIN		// 2mA@2.8A 3mA@4.2A
//#define MODE_LOW					11			// 28mA@2.8A 38mA@4.2A
#define MODE_LOW					16			// 67mA@2.8A 100mA@4.2A
#define MODE_MED					70			// 0.66A@2.8A 1.0A@4.2A
#define MODE_HIGH					120			// 1.2A@2.8A 1.8A@4.2A
#define MODE_TURBO					252
#define MODE_TURBO_STAY_ON			PWM_MAX

#define MODE_INDEX_TURBO_RAMPDOWN	0xf0

// Low light during the pauses in some beacons
#define PWM_BEACON_BACKGROUND		MODE_MED	// ~0.66A (@2.8A)
#define PWM_SLOW_BEACON_BACKGROUND	MODE_LOW	// ~0.3A (@2.8A)

// Turbo timeout - How many WTD ticks before before ramping down (0.5s each)
//#define TURBO_TIMEOUT			60			// 30s for hot rods
//#define TURBO_TIMEOUT			180			// 90s for normal usage
#define TURBO_TIMEOUT			120			// 60s for normal usage (@4.2A)

// Turbo stepdown PWM value (select one suitable for your light)
//#define PWM_TURBO_STEPDOWN		140			// ~1.4A (@2.8A)
#define PWM_TURBO_STEPDOWN		200			// ~2.1A (@2.8A)
//#define PWM_TURBO_STEPDOWN		140			// ~2.1A (@4.2A)
//#define PWM_TURBO_STEPDOWN		100			// ~1.4A (@4.2A)

// Special mode PWM values - Must be under PWM_MIN
#define MODE_STROBE							1
#define MODE_BEACON							2
#define MODE_ALPINE_DISTRESS_BEACON			3
#define MODE_SOS							4
#define MODE_BEACON_WITH_BACKGROUND			5
#define MODE_SLOW_BEACON_WITH_BACKGROUND	6
#define MODE_MOTION_STOPPING_STROBE         254
//#define MODE_MOTION_STOPPING_STROBE_SLOW	253

// Basic modes
#ifdef FOUR_MODES
#define N_BASIC_MODES 4
#define BASIC_MODES MODE_LOWLOW, MODE_LOW, MODE_MED, MODE_TURBO
#else
#define N_BASIC_MODES 3
#define BASIC_MODES MODE_LOW, MODE_MED, MODE_TURBO
#endif

// Hidden modes
#ifdef FOUR_MODES
#  define N_HIDDEN_MODES 7
#  define HIDDEN_MODES MODE_STROBE, MODE_BEACON_WITH_BACKGROUND, MODE_SLOW_BEACON_WITH_BACKGROUND, MODE_BEACON, MODE_ALPINE_DISTRESS_BEACON, MODE_SOS, MODE_TURBO_STAY_ON
#else
#  define N_HIDDEN_MODES 9
#  define HIDDEN_MODES PWM_MIN, MODE_STROBE, MODE_MOTION_STOPPING_STROBE, MODE_BEACON_WITH_BACKGROUND, MODE_SLOW_BEACON_WITH_BACKGROUND, MODE_BEACON, MODE_ALPINE_DISTRESS_BEACON, MODE_SOS, MODE_TURBO_STAY_ON
#endif

// When to enter hidden modes?
#ifdef FOUR_MODES
#  define HIDDEN_MODE_THRESHOLD 0x70
#else
#  define HIDDEN_MODE_THRESHOLD 0x50
#endif

// Ramping down from turbo consumes additional 18 bytes (currently used only if MODE_TURBO is on)
#define RAMP_DOWN

#define WDT_TIMEOUT			6	// Number of WTD ticks light needs to be turned on before resetting short click counter (.5 sec each)

#define ADC_LOW				125	// When do we warn the user and start ramping down
#define ADC_CRIT			120 // When do we shut the light off

#define CAP_THRESHOLD		130  // Value between 1 and 255 corresponding with cap voltage (0 - 1.1v) where we consider it a short press to move to the next mode
								 // Not sure the lowest you can go before getting bad readings, but with a value of 70 and a 1uF cap, it seemed to switch sometimes
								 // even when waiting 10 seconds between presses.

/*
 * =========================================================================
 */

//#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay_basic.h>
// Having own _delay_ms() saves some bytes
static void _delay_ms(uint16_t n)
{	
	while(n-- > 0)
	{
		_delay_loop_2(992);
	}
}


#include <avr/interrupt.h>
#include <avr/wdt.h>	
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>

#define STAR2_PIN   PB0
#define STAR3_PIN   PB4

#define CAP_PIN     PB3
#define CAP_CHANNEL 0x03	// MUX 03 corresponds with PB3 (Star 4)
#define CAP_DIDR    ADC3D	// Digital input disable bit corresponding with PB3

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

#ifdef ENABLE_TURNING_OFF_TURBO_TIMER
uint8_t turbo_timer_enabled =  0;
#endif

// Modes (not const, as this gets adjusted when the light starts up based on stars)
static uint8_t modes[] = { BASIC_MODES, HIDDEN_MODES };

uint8_t mode_idx = 0;
uint8_t selected_pwm = 0;
volatile uint8_t adjusted_pwm = 0; // Volatile, because can be altered in WDT

void store_mode_idx(uint8_t lvl)
{  
	// Central method for writing (with wear leveling)
	uint8_t oldpos=eepos;
	eepos=(eepos+1)&31;  //wear leveling, use next cell

	// Write the current mode
	EEARL=eepos;  EEDR=lvl; EECR=32+4; EECR=32+4+2;  //WRITE  //32:write only (no erase)  4:enable  2:go

	while(EECR & 2) //wait for completion
		; // Empty loop, do nothing, just wait..

	// Erase the last mode
	EEARL=oldpos;           EECR=16+4; EECR=16+4+2;  //ERASE  //16:erase only (no write)  4:enable  2:go
}

inline uint8_t read_stored_idx()
{
	uint8_t *peep = eep;
	eeprom_read_block(eep, 0, 32);

	while((*peep == 0xff) && (eepos < 32)) 
		eepos++, peep++;

	if(eepos < 32) 
		return *peep;
	else 
		eepos=0;

	return 0;
}

inline void next_mode(uint8_t short_clicks) 
{
	if(++mode_idx >= N_BASIC_MODES) 
	{
		if(short_clicks < HIDDEN_MODE_THRESHOLD ||			// too little short clicks -> wrap around, else go to hidden
		   mode_idx >= (N_BASIC_MODES + N_HIDDEN_MODES))	// or all hidden modes used -> wrap around
			mode_idx = 0; // Wrap around
	}
}

inline void WDT_on() 
{
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

inline void ADC_on() 
{
	ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
    DIDR0 |= (1 << ADC_DIDR);							// disable digital input on ADC pin to reduce power consumption
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

inline void ADC_off() 
{
	ADCSRA &= ~(1<<7); //ADC off
}

#ifdef ENABLE_VOLTAGE_MONITORING
uint8_t low_voltage(uint8_t voltage_val) 
{
    static uint8_t lowbatt_cnt = 0;

	// Start conversion
	ADCSRA |= (1 << ADSC);

	// Wait for completion
	while(ADCSRA & (1 << ADSC))
		; // Empty loop, do nothing, just wait..

	// See if voltage is lower than what we were looking for	
	if(ADCH < voltage_val) 
	{
		// See if it's been low for a while
		if(++lowbatt_cnt > 8) 
		{
			lowbatt_cnt = 0;
			return 1;
		}
	} 
	else 
	{
		lowbatt_cnt = 0;
	}
	return 0;
}
#endif


ISR(WDT_vect) 
{
	static uint8_t ticks = 0;

	if(ticks < 255) 
		ticks++;

	if(ticks == WDT_TIMEOUT) 
	{
		store_mode_idx(mode_idx); // Reset short click counter
	} 
#ifdef MODE_TURBO
#ifdef ENABLE_TURNING_OFF_TURBO_TIMER
	else if(turbo_timer_enabled && ticks == TURBO_TIMEOUT && selected_pwm == MODE_TURBO) // Turbo mode is *not* always at end
#else
	else if(ticks == TURBO_TIMEOUT && selected_pwm == MODE_TURBO)
#endif
	{
		store_mode_idx(mode_idx | MODE_INDEX_TURBO_RAMPDOWN); // Store the knowledge of ramp down, so that next mode can be turbo again - do it *before* ramping down

#ifdef RAMP_DOWN
		while(adjusted_pwm > PWM_TURBO_STEPDOWN) // might be already less if low voltage stepdown was performed before this!
		{
			PWM_LVL = --adjusted_pwm;
			_delay_ms(25);
		}
#else
		if(adjusted_pwm > PWM_TURBO_STEPDOWN) // might be already less if low voltage stepdown was performed before this!
			adjusted_pwm = PWM_TURBO_STEPDOWN;
#endif // RAMP_DOWN
	}
#endif // MODE_TURBO

#ifdef ENABLE_VOLTAGE_MONITORING
	// Block for voltage monitoring
	{
		static uint8_t adjusted_to_min = 0;

		if(adjusted_to_min) // Already in lowest mode, check only against critically low voltage
		{
			if(low_voltage(ADC_CRIT))
				adjusted_pwm = PWM_OFF; // Signal main loop to turn off the light
		}
		else if(low_voltage(ADC_LOW)) 
		{
			// We need to go to a lower PWM - blink to notify the user
			// One blink is enough to save program space
			PWM_LVL = 0;
			_delay_ms(250);
			//PWM_LVL = adjusted_pwm; // Turned back on in the main loop, unnecessary to waste program space here

			adjusted_pwm /= 2;

			if(adjusted_pwm < PWM_MIN) // Can't go any lower -> Change to lowest possible PWM level and stay there until critical voltage
			{
				adjusted_pwm = PWM_MIN;
				adjusted_to_min = 1;				
			}			
		}
	}	
#endif
}

#ifdef ENABLE_HIGH_TO_LOW
void revert_modes(void) // Revert modes in place
{
	uint8_t spare, *plow = modes, *phigh = modes+N_BASIC_MODES-1;
	
	while(plow < phigh)
	{
		spare = *plow;
		*plow = *phigh;
		*phigh = spare;
		plow++, phigh--;
	}
}
#endif

int main(void)
{	
	uint8_t short_click = 0;
	
	// All ports default to input, but turn pull-up resistors on for the stars (not the ADC input!  Made that mistake already)
	PORTB = (1 << STAR2_PIN) | (1 << STAR3_PIN);
	
	// Start up ADC for capacitor pin
	DIDR0 |= (1 << CAP_DIDR);							// disable digital input on ADC pin to reduce power consumption
	ADMUX  = (1 << REFS0) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC3/PB3
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
	
	// Wait for completion
	while (ADCSRA & (1 << ADSC))
		;
	// Start again as datasheet says first result is unreliable
	ADCSRA |= (1 << ADSC);
	// Wait for completion
	while (ADCSRA & (1 << ADSC))
		;

	if(ADCH > CAP_THRESHOLD) 
		short_click = 1;

	// Turn off ADC
	ADC_off();

	// Charge up the capacitor by setting CAP_PIN to output
	DDRB  |= (1 << CAP_PIN);	// Output
    PORTB |= (1 << CAP_PIN);	// High

    // Set PWM pin to output
    DDRB = (1 << PWM_PIN);

    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
	
	// Turn features on or off as needed
#ifdef ENABLE_VOLTAGE_MONITORING
	ADC_on();
#else
	ADC_off();
#endif
	ACSR   |=  (1<<7); //AC off
	
#ifdef ENABLE_TURNING_OFF_TURBO_TIMER
	turbo_timer_enabled = ((PINB & (1 << STAR2_PIN)) > 0) ? 1 : 0;
#endif

#ifdef ENABLE_SINGLE_MODE
	if((PINB & (1 << STAR2_PIN)) == 0)
	{
			modes[0] = modes[1] = 
#ifdef FOUR_MODES
			modes[2] = 
#endif
			MODE_TURBO;

			// Last basic mode is already MODE_TURBO - no need to waste bytes changing that
	}		
#endif

#ifdef ENABLE_HIGH_TO_LOW
#ifdef HIGH_TO_LOW_ON_BY_DEFAULT
	if((PINB & (1 << STAR3_PIN)) > 0)
#else
	if((PINB & (1 << STAR3_PIN)) == 0)
#endif
		revert_modes();
#endif
	
#ifdef ENABLE_TACTICAL_MODE
#ifdef TACTICAL_MODE_ON_BY_DEFAULT
	if((PINB & (1 << STAR2_PIN)) > 0)
#else
	if((PINB & (1 << STAR2_PIN)) == 0)
#endif
	{
		modes[1] = MODE_TURBO;

		if(modes[0] == MODE_TURBO) // Single mode
		{
			modes[2] = 
#ifdef FOUR_MODES
			modes[3] = 
#endif
			MODE_TURBO;
		}
		else
		{
			modes[0] = MODE_STROBE;
			modes[2] = MODE_LOW;
#ifdef FOUR_MODES
			modes[3] = MODE_LOWLOW;
#endif
		}
	}
#endif


	// Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
	// Will allow us to go idle between WDT interrupts
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	// Mode memory handling block
	{
		uint8_t n_short_clicks = 0;

		// Determine what mode we should fire up
		// Read the last mode that was saved
		mode_idx = read_stored_idx();

		// Handle short press counter
		n_short_clicks = (mode_idx & 0xf0);
		mode_idx &= 0x0f;

		if(short_click) // This click was short
		{
			if(n_short_clicks == MODE_INDEX_TURBO_RAMPDOWN) // TODO: Test if this logic works in practice, or do we need to use always double tap from turbo?
				n_short_clicks = 0; // Keep turbo, reset counter
			else
				next_mode(n_short_clicks); // Will handle wrap arounds

			store_mode_idx(mode_idx | ((n_short_clicks < HIDDEN_MODE_THRESHOLD) ? n_short_clicks+0x10 : n_short_clicks));
		} 
		else // Didn't have a short press, keep the same mode, stored without short click counter
		{
			if((PINB & (1 << STAR2_PIN)) == 0) // Tactical or Single or No memory, reset to 1st mode
				mode_idx = 0;

			store_mode_idx(mode_idx);
		}
	}		
	
	// Start watchdog timer (used for storing the mode after delay, turbo timer, and voltage monitoring)
	WDT_on();
	
	// Now just fire up the mode
	selected_pwm = modes[mode_idx]; // Note: Actual PWM can be less than selected PWM (e.g. in case of low voltage)
	
	if(selected_pwm < PWM_MIN) // Hidden blinky modes
		adjusted_pwm = PWM_MAX; // All blinky modes initially with full power
	else
		adjusted_pwm = selected_pwm;

	// block for main loop
	{
		uint8_t ii = 0; // Loop counter, used by multiple branches
#ifdef ENABLE_BEACONS
		uint8_t beacon_background = PWM_OFF;
#endif
	
		while(1)
		{
#ifdef ENABLE_VOLTAGE_MONITORING
			if(adjusted_pwm == PWM_OFF) // Voltage monitoring signaled us to turn off the light -> break out of the main loop
				break;
#endif

			PWM_LVL = adjusted_pwm; // must be set inside loop, is volatile & might have changed because of voltage monitoring

			switch(selected_pwm)
			{
			case MODE_STROBE: // Disorienting alternating strobe
#ifdef NORMAL_STROBE
				// 51ms = ~19.5Hz, ~60% DC
				_delay_ms(25);
				PWM_LVL = 0;
				_delay_ms(26);
#endif
#ifdef ALTERNATING_STROBE
				_delay_ms(31);
				PWM_LVL = 0;
				if(ii < 19) // 51ms = ~19.5Hz, ~60% DC
					_delay_ms(20);
				else if(ii < 32) // 77ms = ~13Hz, ~40% DC
					_delay_ms(46);
				else
					ii = 255;
#endif
#ifdef RANDOM_STROBE 
				{   // 77ms = 13Hz, 51ms = 19.5Hz / 40-60% DC
					ii = (5 * ii) + 128;
					_delay_ms(31);
					PWM_LVL = 0;
					_delay_ms(ii > 127 ? 46 : 20);
				}					
#endif
				break;
			case MODE_MOTION_STOPPING_STROBE: // 8Hz, 1.6% DC
				_delay_ms(2);
				PWM_LVL = 0;
				_delay_ms(123);
				break;
#ifdef ENABLE_SOS
			case MODE_SOS:
				if(ii / 3 == 1)
					_delay_ms(600);  // Dash for 'O' (3xDot)
				else
					_delay_ms(200);  // Dot for 'S'

				PWM_LVL = 0;

				switch(ii)
				{
				default:
					_delay_ms(200);  // Pause inside a letter (1xDot)
					break;
				case 2:
				case 5:
					_delay_ms(600); // Pause between letters (3xDot)
					break;
				case 8:
					_delay_ms(2500); // Pause between "words" (should be 7xDot, but I like it longer)
					ii = 255;
					break;
				}
				break;
#endif
#ifdef ENABLE_BEACONS
			case MODE_BEACON_WITH_BACKGROUND:
				beacon_background = PWM_BEACON_BACKGROUND;
				goto beacon_common;
			case MODE_SLOW_BEACON_WITH_BACKGROUND:
				beacon_background = PWM_SLOW_BEACON_BACKGROUND;
				// no break - fall through to beacon code
			case MODE_BEACON:
			case MODE_ALPINE_DISTRESS_BEACON:
			beacon_common:
				_delay_ms(50);
				PWM_LVL = beacon_background;
				_delay_ms(950);
				
				if(selected_pwm == MODE_ALPINE_DISTRESS_BEACON)
				{
					if(ii > 5)
					{
						_delay_ms(59000);
						ii = 255;
					}
					else
						_delay_ms(9000);
				}
				else if(selected_pwm == MODE_SLOW_BEACON_WITH_BACKGROUND)
					_delay_ms(1500);
#endif
				break;
			default:
				sleep_mode();
				break;
			}

			ii++; // Loop counter, used by multiple branches
		}
	}

#ifdef ENABLE_VOLTAGE_MONITORING
	//
	// Critically low voltage -> Turn off the light
	// 
		
	// Disable WDT so it doesn't wake us up from sleep
	WDT_off();

	// Would be nice to blink a couple of times with lowest brightness to notify the user, but not implemented due space restrictions

	// Turn the light off
	PWM_LVL = 0;

	// Disable ADC so it doesn't consume power
	ADC_off();

	// Power down as many components as possible
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	// Deep sleep until powered off - consumes ~110uA during the deep sleep
	while(1)
		sleep_mode();
#endif

    return 0; // Standard Return Code -> would return to idle loop with interrupts disabled.
}
