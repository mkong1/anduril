/*
 * This is intended for use on flashlights with a clicky switch.
 * Ideally, a Nichia 219B running at 1900mA in a Convoy S-series host.
 * It's mostly based on JonnyC's STAR on-time firmware and ToyKeeper's
 * tail-light firmware.
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
 *      Resistor values for voltage divider (reference BLF-VLD README for more info)
 *      Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
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
 *      ADC = ((V_bat - V_diode) * R2   * 255) / ((R1    + R2  ) * V_ref)
 *      125 = ((3.0   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *      121 = ((2.9   - .25    ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *
 *      Well 125 and 121 were too close, so it shut off right after lowering to low mode, so I went with
 *      130 and 120
 *
 *      To find out what value to use, plug in the target voltage (V) to this equation
 *          value = (V * 4700 * 255) / (23800 * 1.1)
 *
 */
#define NANJG_LAYOUT
#include "tk-attiny.h"
#undef BOGOMIPS  // this particular driver runs slower
#define BOGOMIPS 890

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define VOLTAGE_MON                 // Comment out to disable
#define OWN_DELAY                   // Should we use the built-in delay or our own?
#define USE_DELAY_MS
#define USE_FINE_DELAY

#define FAST_PWM_START      10      // Anything under this will use phase-correct
// Lumen measurements used a Nichia 219B at 1900mA in a Convoy S7 host
#define MODE_MOON           4       // 6: 0.14 lm (6 through 9 may be useful levels)
#define MODE_LOW            14      // 14: 7.3 lm
#define MODE_MED            39      // 39: 42 lm
#define MODE_HIGH           120     // 120: 155 lm
#define MODE_HIGHER         255     // 255: 342 lm
// If you change these, you'll probably want to change the "modes" array below
// How many non-blinky modes will we have?
#define SOLID_MODES           5
// battery check mode index
#define BATT_CHECK_MODE       1+SOLID_MODES
// How many beacon modes will we have (without background light on)?
#define SINGLE_BEACON_MODES   1+BATT_CHECK_MODE
// How many constant-speed strobe modes?
#define FIXED_STROBE_MODES    3+SINGLE_BEACON_MODES
// How many variable-speed strobe modes?
#define VARIABLE_STROBE_MODES 2+FIXED_STROBE_MODES
// How many beacon modes will we have (with background light on)?
#define DUAL_BEACON_MODES     3+VARIABLE_STROBE_MODES

#define USE_BATTCHECK
#define BATTCHECK_VpT  // Use the volts+tenths battcheck style
//#define BATTCHECK_4bars  // Use the volts+tenths battcheck style

#include "tk-calibration.h"

/*
 * =========================================================================
 */

#include "tk-delay.h"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "tk-voltage.h"

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
const uint8_t modes[] = { // high enough to handle all
    MODE_MOON, MODE_LOW, MODE_MED, MODE_HIGH, MODE_HIGHER, // regular solid modes
    MODE_MED, // battery check mode
    MODE_HIGHER, // heartbeat beacon
    82, 41, 15, // constant-speed strobe modes (12 Hz, 24 Hz, 60 Hz)
    MODE_HIGHER, MODE_HIGHER, // variable-speed strobe modes
    MODE_MOON, MODE_LOW, MODE_MED, // dual beacon modes (this level and this level + 2)
};
volatile uint8_t mode_idx = 0;
// 1 or -1. Do we increase or decrease the idx when moving up to a higher mode?
// Is set by checking stars in the original STAR firmware, but that's removed to save space.
#define mode_dir 1

inline void next_mode() {
    mode_idx += mode_dir;
    if (mode_idx > (sizeof(modes) - 1)) {
        // Wrap around
        mode_idx = 0;
    }
}

#define BLINK_BRIGHTNESS MODE_MED
#define BLINK_SPEED 500
#ifdef BATTCHECK_VpT
void blink(uint8_t val, uint16_t speed)
{
    for (; val>0; val--)
    {
        PWM_LVL = BLINK_BRIGHTNESS;
        _delay_ms(speed);
        PWM_LVL = 0;
        _delay_ms(speed<<2);
    }
}
#else
void blink(uint8_t val)
{
    for (; val>0; val--)
    {
        PWM_LVL = BLINK_BRIGHTNESS;
        _delay_ms(100);
        PWM_LVL = 0;
        _delay_ms(400);
    }
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

    // set PWM mode
    if (modes[mode_idx] < FAST_PWM_START) {
        // Set timer to do PWM for correct output pin and set prescaler timing
        TCCR0A = 0x21; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    } else {
        // Set timer to do PWM for correct output pin and set prescaler timing
        TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    }
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Now just fire up the mode
    PWM_LVL = modes[mode_idx];

    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t strobe_len = 0;
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
    uint8_t voltage;
#endif
    while(1) {
        if(mode_idx < SOLID_MODES) { // Just stay on at a given brightness
            sleep_mode();
        } else if (mode_idx < BATT_CHECK_MODE) {
            PWM_LVL = 0;
            get_voltage();  _delay_ms(200);  // the first reading is junk
#ifdef BATTCHECK_VpT
            uint8_t result = battcheck();
            blink(result >> 5, BLINK_SPEED/8);
            _delay_ms(BLINK_SPEED);
            blink(1,5);
            _delay_ms(BLINK_SPEED*3/2);
            blink(result & 0b00011111, BLINK_SPEED/8);
#else
            blink(battcheck());
#endif  // BATTCHECK_VpT
            _delay_ms(2000);  // wait at least 2 seconds between readouts
        } else if (mode_idx < SINGLE_BEACON_MODES) { // heartbeat flasher
            PWM_LVL = modes[SOLID_MODES-1];
            _delay_ms(1);
            PWM_LVL = 0;
            _delay_ms(249);
            PWM_LVL = modes[SOLID_MODES-1];
            _delay_ms(1);
            PWM_LVL = 0;
            _delay_ms(749);
        } else if (mode_idx < FIXED_STROBE_MODES) { // strobe mode, fixed-speed
            PWM_LVL = modes[SOLID_MODES-1];
            if (modes[mode_idx] < 50) { _delay_zero(); }
            else { _delay_ms(1); }
            PWM_LVL = 0;
            _delay_ms(modes[mode_idx]);
        } else if (mode_idx == VARIABLE_STROBE_MODES-2) {
            // strobe mode, smoothly oscillating frequency ~7 Hz to ~18 Hz
            for(j=0; j<66; j++) {
                PWM_LVL = modes[SOLID_MODES-1];
                _delay_ms(1);
                PWM_LVL = 0;
                if (j<33) { strobe_len = j; }
                else { strobe_len = 66-j; }
                _delay_ms(2*(strobe_len+33-6));
            }
        } else if (mode_idx == VARIABLE_STROBE_MODES-1) {
            // strobe mode, smoothly oscillating frequency ~16 Hz to ~100 Hz
            for(j=0; j<100; j++) {
                PWM_LVL = modes[SOLID_MODES-1];
                _delay_zero(); // less than a millisecond
                PWM_LVL = 0;
                if (j<50) { strobe_len = j; }
                else { strobe_len = 100-j; }
                _delay_ms(strobe_len+9);
            }
        } else if (mode_idx < DUAL_BEACON_MODES) { // two-level fast strobe pulse at about 1 Hz
            for(i=0; i<4; i++) {
                PWM_LVL = modes[mode_idx-SOLID_MODES+2];
                _delay_ms(5);
                PWM_LVL = modes[mode_idx];
                _delay_ms(65);
            }
            _delay_ms(720);
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
