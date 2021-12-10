/*
 * This is intended for use on UV flashlights with a clicky switch.
 * It uses brownout detection to provide an offtime-based UI.
 * UV 365nm LED on 16mm Noctigon from RMM
 * Nanjg 101-AK-A1 7135*4 1.4A LED DRIVER with only 2x7135
 * UF-602C host
 *
 * Original author: JonnyC
 * Modifications: ToyKeeper / Selene Scriven
 * Derived from ToyKeeper/s7.c
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
 *      Use ToyKeeper/battcheck.hex to measure your driver's ADC values.
 *
 */
#define F_CPU 4800000UL

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define VOLTAGE_MON                 // Comment out to disable
#define OWN_DELAY                   // Should we use the built-in delay or our own?

// Calibrated for a 2x7135 AK-101 driver with unknown-model UV emitter from RMM
#define MODE_MOON           6
#define MODE_LOW            30
#define MODE_MED            120
#define MODE_HIGHER         255
// If you change these, you'll probably want to change the "modes" array below
#define SOLID_MODES         4       // How many non-blinky modes will we have?
#define SINGLE_BEACON_MODES 4+1     // How many beacon modes will we have (without background light on)?
#define FIXED_STROBE_MODES  4+1+3   // How many constant-speed strobe modes?
#define BATT_CHECK_MODE     4+1+3+1 // battery check mode index
// Note: don't use more than 32 modes, or it will interfere with the mechanism used for mode memory
#define TOTAL_MODES         BATT_CHECK_MODE

#define ADC_42          184 // the ADC value we expect for 4.20 volts
#define ADC_100         184 // the ADC value for 100% full (4.2V resting)
#define ADC_75          175 // the ADC value for 75% full (4.0V resting)
#define ADC_50          166 // the ADC value for 50% full (3.8V resting)
#define ADC_25          152 // the ADC value for 25% full (3.5V resting)
#define ADC_0           129 // the ADC value for 0% full (3.0V resting)
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
    // TODO: make this take tenths of a ms instead of ms,
    // for more precise timing?
    // (would probably be better than the if/else here for a special-case
    // sub-millisecond delay)
    if (n==0) { _delay_loop_2(300); }
    else {
        while(n-- > 0)
            _delay_loop_2(890);
    }
}
#else
#include <util/delay.h>
#endif

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#define STAR2_PIN   PB0
#define STAR3_PIN   PB4
#define STAR4_PIN   PB3
#define PWM_PIN     PB1
#define VOLTAGE_PIN PB2
#define ADC_CHANNEL 0x01    // MUX 01 corresponds with PB2
#define ADC_DIDR    ADC1D   // Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06    // clk/64

#define PWM_LVL     OCR0B   // OCR0B is the output compare register for PB1

/*
 * global variables
 */

// Mode storage
// store in uninitialized memory so it will not be overwritten and
// can still be read at startup after short (<500ms) power off
// decay used to tell if user did a short press.
volatile uint8_t noinit_decay __attribute__ ((section (".noinit")));
volatile uint8_t noinit_mode __attribute__ ((section (".noinit")));

// change to 1 if you want on-time mode memory instead of "short-cycle" memory
// (actually, don't.  It's not supported any more, and doesn't work)
#define memory 0

// Modes (hardcoded to save space)
static uint8_t modes[TOTAL_MODES] = { // high enough to handle all
    MODE_MOON, MODE_LOW, MODE_MED, MODE_HIGHER, // regular solid modes
    MODE_HIGHER, // heartbeat beacon
    240, 90, 31, // constant-speed strobe modes (4 Hz, 10 Hz, 24 Hz)
    MODE_MED, // battery check mode
};
volatile uint8_t mode_idx = 0;
// 1 or -1. Do we increase or decrease the idx when moving up to a higher mode?
// Is set by checking stars in the original STAR firmware, but that's removed to save space.
#define mode_dir 1
PROGMEM const uint8_t voltage_blinks[] = {
    ADC_0,    // 1 blink  for 0%-25%
    ADC_25,   // 2 blinks for 25%-50%
    ADC_50,   // 3 blinks for 50%-75%
    ADC_75,   // 4 blinks for 75%-100%
    ADC_100,  // 5 blinks for >100%
};

inline void next_mode() {
    mode_idx += mode_dir;
    if (mode_idx > (TOTAL_MODES - 1)) {
        // Wrap around
        mode_idx = 0;
    }
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

    // Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
    // Will allow us to go idle between WDT interrupts (which we're not using anyway)
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Determine what mode we should fire up
    // Read the last mode that was saved
    if (noinit_decay) // not short press, forget mode
    {
        noinit_mode = 0;
        mode_idx = 0;
    } else { // short press, advance to next mode
        mode_idx = noinit_mode;
        next_mode();
        noinit_mode = mode_idx;
    }
    // set noinit data for next boot
    noinit_decay = 0;

    // Now just fire up the mode
    PWM_LVL = modes[mode_idx];

    uint8_t i = 0;
    uint8_t strobe_len = 0;
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
    uint8_t voltage;
#endif
    while(1) {
        if(mode_idx < SOLID_MODES) { // Just stay on at a given brightness
            sleep_mode();
        } else if (mode_idx < SINGLE_BEACON_MODES) { // heartbeat flasher
            PWM_LVL = modes[SOLID_MODES-1];
            _delay_ms(10);
            PWM_LVL = 0;
            _delay_ms(240);
            PWM_LVL = modes[SOLID_MODES-1];
            _delay_ms(10);
            PWM_LVL = 0;
            _delay_ms(740);
        } else if (mode_idx < FIXED_STROBE_MODES) { // strobe mode, fixed-speed
            strobe_len = 10;
            PWM_LVL = modes[SOLID_MODES-1];
            _delay_ms(strobe_len);
            PWM_LVL = 0;
            _delay_ms(modes[mode_idx]);
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
                    mode_idx = 0;
                } else { // Already at the lowest mode
                    // Turn off the light
                    PWM_LVL = 0;
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
