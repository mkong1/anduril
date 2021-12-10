/*
 * BLF EE A6 firmware (special-edition group buy light)
 * This light uses a FET+1 style driver, with a FET on the main PWM channel
 * for the brightest high modes and a single 7135 chip on the secondary PWM
 * channel so we can get stable, efficient low / medium modes.  It also
 * includes a capacitor for measuring off time.
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
 * NANJG 105C Diagram
 *           ---
 *         -|   |- VCC
 *     OTC -|   |- Voltage ADC
 *  Star 3 -|   |- PWM (FET)
 *     GND -|   |- PWM (1x7135)
 *           ---
 *
 * FUSES
 *      I use these fuse settings
 *      Low:  0x75  (4.8MHz CPU without 8x divider, 9.4kHz phase-correct PWM or 18.75kHz fast-PWM)
 *      High: 0xfd  (to enable brownout detection)
 *
 *      For more details on these settings, visit http://github.com/JCapSolutions/blf-firmware/wiki/PWM-Frequency
 *
 * STARS
 *      Star 2 = second PWM output channel
 *      Star 3 = mode memory if soldered, no memory by default
 *      Star 4 = Capacitor for off-time
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
 *   To find out what values to use, flash the driver with battcheck.hex
 *   and hook the light up to each voltage you need a value for.  This is
 *   much more reliable than attempting to calculate the values from a
 *   theoretical formula.
 *
 *   Same for off-time capacitor values.  Measure, don't guess.
 */
// Choose your MCU here, or in the build script
//#define ATTINY 13
//#define ATTINY 25


// set some hardware-specific values...
// (while configuring this firmware, skip this section)
#if (ATTINY == 13)
#define F_CPU 4800000UL
#define EEPLEN 64
#elif (ATTINY == 25)
#define F_CPU 8000000UL
#define EEPLEN 128
#else
Hey, you need to define ATTINY.
#endif


/*
 * =========================================================================
 * Settings to modify per driver
 */

//#define FAST 0x23           // fast PWM channel 1 only
//#define PHASE 0x21          // phase-correct PWM channel 1 only
#define FAST 0xA3           // fast PWM both channels
#define PHASE 0xA1          // phase-correct PWM both channels

#define VOLTAGE_MON         // Comment out to disable LVP
#define OWN_DELAY           // Should we use the built-in delay or our own?
// Adjust the timing per-driver, since the hardware has high variance
// Higher values will run slower, lower values run faster.
#if (ATTINY == 13)
#define DELAY_TWEAK         950
#elif (ATTINY == 25)
#define DELAY_TWEAK         2000
#endif

#define OFFTIM3             // Use short/med/long off-time presses
                            // instead of just short/long

// comment out to use extended config mode instead of a solderable star
// (controls whether mode memory is on the star or if it's a setting in config mode)
//#define CONFIG_STARS

// output to use for blinks on battery check mode (primary PWM level, alt PWM level)
// Use 20,0 for a single-channel driver or 0,20 for a two-channel driver
#define BLINK_BRIGHTNESS    0,20

// Mode group 1
#define NUM_MODES1          7
// PWM levels for the big circuit (FET or Nx7135)
#define MODESNx1            0,0,0,7,56,137,255
// PWM levels for the small circuit (1x7135)
#define MODES1x1            3,20,110,255,255,255,0
// My sample:     6=0..6,  7=2..11,  8=8..21(15..32)
// Krono sample:  6=5..21, 7=17..32, 8=33..96(50..78)
// Manker2:       2=21, 3=39, 4=47, ... 6?=68
// PWM speed for each mode
#define MODES_PWM1          PHASE,FAST,FAST,FAST,FAST,FAST,PHASE
// Mode group 2
#define NUM_MODES2          4
#define MODESNx2            0,0,90,255
#define MODES1x2            20,230,255,0
#define MODES_PWM2          FAST,FAST,FAST,PHASE
// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,TURBO .
#define NUM_HIDDEN          4
#define HIDDENMODES         BIKING_STROBE,BATTCHECK,STROBE,TURBO
#define HIDDENMODES_PWM     PHASE,PHASE,PHASE,PHASE
#define HIDDENMODES_ALT     0,0,0,0   // Zeroes, same length as NUM_HIDDEN

#define TURBO     255       // Convenience code for turbo mode
#define BATTCHECK 254       // Convenience code for battery check mode
// Uncomment to enable tactical strobe mode
#define STROBE    253       // Convenience code for strobe mode
// Uncomment to unable a 2-level stutter beacon instead of a tactical strobe
#define BIKING_STROBE 252   // Convenience code for biking strobe mode
// comment out to use minimal version instead (smaller)
#define FULL_BIKING_STROBE

#define NON_WDT_TURBO            // enable turbo step-down without WDT
// How many timer ticks before before dropping down.
// Each timer tick is 500ms, so "60" would be a 30-second stepdown.
// Max value of 255 unless you change "ticks"
#define TURBO_TIMEOUT       90

// These values were measured using wight's "A17HYBRID-S" driver built by DBCstm.
// Your mileage may vary.
#define ADC_42          195 // the ADC value we expect for 4.20 volts
#define ADC_100         195 // the ADC value for 100% full (4.2V resting)
#define ADC_75          186 // the ADC value for 75% full (4.0V resting)
#define ADC_50          176 // the ADC value for 50% full (3.8V resting)
#define ADC_25          162 // the ADC value for 25% full (3.5V resting)
#define ADC_0           138 // the ADC value for 0% full (3.0V resting)
#define ADC_LOW         129 // When do we start ramping down (2.8V)
#define ADC_CRIT        124 // When do we shut the light off (2.7V)

// the BLF EE A6 driver may have different offtime cap values than most other drivers
// Values are between 1 and 255, and can be measured with offtime-cap.c
// These #defines are the edge boundaries, not the center of the target.
#ifdef OFFTIM3
#define CAP_SHORT           190  // Anything higher than this is a short press
#define CAP_MED             94  // Between CAP_MED and CAP_SHORT is a medium press
                                 // Below CAP_MED is a long press
#else
#define CAP_SHORT           115  // Anything higher than this is a short press, lower is a long press
#endif

/*
 * =========================================================================
 */

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#ifdef OWN_DELAY
#include <util/delay_basic.h>
// Having own _delay_ms() saves some bytes AND adds possibility to use variables as input
void _delay_ms(uint16_t n)
{
    // TODO: make this take tenths of a ms instead of ms,
    // for more precise timing?
    while(n-- > 0) _delay_loop_2(DELAY_TWEAK);
}
void _delay_s()  // because it saves a bit of ROM space to do it this way
{
    _delay_ms(1000);
}
#else
#include <util/delay.h>
#endif

#include <avr/pgmspace.h>
//#include <avr/io.h>
//#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>

#define STAR2_PIN   PB0     // But note that there is no star 2.
#define STAR3_PIN   PB4
#define CAP_PIN     PB3
#define CAP_CHANNEL 0x03    // MUX 03 corresponds with PB3 (Star 4)
#define CAP_DIDR    ADC3D   // Digital input disable bit corresponding with PB3
#define PWM_PIN     PB1
#define ALT_PWM_PIN PB0
#define VOLTAGE_PIN PB2
#define ADC_CHANNEL 0x01    // MUX 01 corresponds with PB2
#define ADC_DIDR    ADC1D   // Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06    // clk/64

#define PWM_LVL     OCR0B   // OCR0B is the output compare register for PB1
#define ALT_PWM_LVL OCR0A   // OCR0A is the output compare register for PB0

/*
 * global variables
 */

// Config / state variables
uint8_t eepos = 0;
uint8_t memory = 0;        // mode memory, or not (set via soldered star)
uint8_t modegroup = 0;     // which mode group (set above in #defines)
uint8_t mode_idx = 0;      // current or last-used mode number
// counter for entering config mode
// (needs to be remembered while off, but only for up to half a second)
uint8_t fast_presses __attribute__ ((section (".noinit")));

// NOTE: Only '1' is known to work; -1 will probably break and is untested.
// In other words, short press goes to the next (higher) mode,
// medium press goes to the previous (lower) mode.
#define mode_dir 1
// total length of current mode group's array
uint8_t mode_cnt;
// number of regular non-hidden modes in current mode group
uint8_t solid_modes;
// number of hidden modes in the current mode group
// (hardcoded because both groups have the same hidden modes)
//uint8_t hidden_modes = NUM_HIDDEN;  // this is never used


// Modes (gets set when the light starts up based on saved config values)
PROGMEM const uint8_t modesNx1[] = { MODESNx1, HIDDENMODES };
PROGMEM const uint8_t modesNx2[] = { MODESNx2, HIDDENMODES };
const uint8_t *modesNx;  // gets pointed at whatever group is current

PROGMEM const uint8_t modes1x1[] = { MODES1x1, HIDDENMODES_ALT };
PROGMEM const uint8_t modes1x2[] = { MODES1x2, HIDDENMODES_ALT };
const uint8_t *modes1x;

PROGMEM const uint8_t modes_pwm1[] = { MODES_PWM1, HIDDENMODES_PWM };
PROGMEM const uint8_t modes_pwm2[] = { MODES_PWM2, HIDDENMODES_PWM };
const uint8_t *modes_pwm;

PROGMEM const uint8_t voltage_blinks[] = {
    ADC_0,    // 1 blink  for 0%-25%
    ADC_25,   // 2 blinks for 25%-50%
    ADC_50,   // 3 blinks for 50%-75%
    ADC_75,   // 4 blinks for 75%-100%
    ADC_100,  // 5 blinks for >100%
    255,      // Ceiling, don't remove
};

void save_state() {  // central method for writing (with wear leveling)
    // a single 16-bit write uses less ROM space than two 8-bit writes
    uint8_t eep;
    uint8_t oldpos=eepos;

    eepos = (eepos+1) & (EEPLEN-1);  // wear leveling, use next cell

#ifdef CONFIG_STARS
    eep = mode_idx | (modegroup << 5);
#else
    eep = mode_idx | (modegroup << 5) | (memory << 6);
#endif
    eeprom_write_byte((uint8_t *)(eepos), eep);      // save current state
    eeprom_write_byte((uint8_t *)(oldpos), 0xff);    // erase old state
}

void restore_state() {
    uint8_t eep;
    // find the config data
    for(eepos=0; eepos<EEPLEN; eepos++) {
        eep = eeprom_read_byte((const uint8_t *)eepos);
        if (eep != 0xff) break;
    }
    // unpack the config data
    if (eepos < EEPLEN) {
        mode_idx = eep & 0x0f;
        modegroup = (eep >> 5) & 1;
#ifndef CONFIG_STARS
        memory = (eep >> 6) & 1;
#endif
    }
    // unnecessary, save_state handles wrap-around
    // (and we don't really care about it skipping cell 0 once in a while)
    //else eepos=0;
}

inline void next_mode() {
    mode_idx += 1;
    if (mode_idx >= solid_modes) {
        // Wrap around, skipping the hidden modes
        // (note: this also applies when going "forward" from any hidden mode)
        mode_idx = 0;
    }
}

#ifdef OFFTIM3
inline void prev_mode() {
    if (mode_idx == solid_modes) {
        // If we hit the end of the hidden modes, go back to moon
        mode_idx = 0;
    } else if (mode_idx > 0) {
        // Regular mode: is between 1 and TOTAL_MODES
        mode_idx -= 1;
    } else {
        // Otherwise, wrap around (this allows entering hidden modes)
        mode_idx = mode_cnt - 1;
    }
}
#endif

#ifdef CONFIG_STARS
inline void check_stars() {
    // Configure options based on stars
    // 0 being low for soldered, 1 for pulled-up for not soldered
#if 0  // not implemented, STAR2_PIN is used for second PWM channel
    // Moon
    // enable moon mode?
    if ((PINB & (1 << STAR2_PIN)) == 0) {
        modes[mode_cnt++] = MODE_MOON;
    }
#endif
#if 0  // Mode order not as important as mem/no-mem
    // Mode order
    if ((PINB & (1 << STAR3_PIN)) == 0) {
        // High to Low
        mode_dir = -1;
    } else {
        mode_dir = 1;
    }
#endif
    // Memory
    if ((PINB & (1 << STAR3_PIN)) == 0) {
        memory = 1;  // solder to enable memory
    } else {
        memory = 0;  // unsolder to disable memory
    }
}
#endif  // ifdef CONFIG_STARS

void count_modes() {
    /*
     * Determine how many solid and hidden modes we have.
     * The modes_pwm array should have several values for regular modes
     * then some values for hidden modes.
     *
     * (this matters because we have more than one set of modes to choose
     *  from, so we need to count at runtime)
     */
    if (modegroup == 0) {
        solid_modes = NUM_MODES1;
        modesNx = modesNx1;
        modes1x = modes1x1;
        modes_pwm = modes_pwm1;
    } else {
        solid_modes = NUM_MODES2;
        modesNx = modesNx2;
        modes1x = modes1x2;
        modes_pwm = modes_pwm2;
    }
    mode_cnt = solid_modes + NUM_HIDDEN;
}

#ifdef VOLTAGE_MON
inline void ADC_on() {
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
#if (ATTINY == 13)
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
#elif (ATTINY == 25)
    ADMUX  = (1 << REFS1) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
#endif
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}
#else
inline void ADC_off() {
    ADCSRA &= ~(1<<7); //ADC off
}
#endif

void set_output(uint8_t pwm1, uint8_t pwm2) {
    // Need PHASE to properly turn off the light
    if ((pwm1==0) && (pwm2==0)) {
        TCCR0A = PHASE;
    }
    PWM_LVL = pwm1;
    ALT_PWM_LVL = pwm2;
}

void set_mode(uint8_t mode) {
    TCCR0A = pgm_read_byte(modes_pwm + mode);
    set_output(pgm_read_byte(modesNx + mode), pgm_read_byte(modes1x + mode));
    /*
    // Only set output for solid modes
    uint8_t out = pgm_read_byte(modesNx + mode);
    if ((out < 250) || (out == 255)) {
        set_output(pgm_read_byte(modesNx + mode), pgm_read_byte(modes1x + mode));
    }
    */
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

void blink(uint8_t val)
{
    for (; val>0; val--)
    {
        set_output(BLINK_BRIGHTNESS);
        _delay_ms(100);
        set_output(0,0);
        _delay_ms(400);
    }
}

#ifndef CONFIG_STARS
void toggle(uint8_t *var) {
    // Used for extended config mode
    // Changes the value of a config option, waits for the user to "save"
    // by turning the light off, then changes the value back in case they
    // didn't save.  Can be used repeatedly on different options, allowing
    // the user to change and save only one at a time.
    *var ^= 1;
    save_state();
    blink(2);
    *var ^= 1;
    save_state();
    _delay_s();
}
#endif // ifndef CONFIG_STARS

int main(void)
{
    uint8_t cap_val;

    // Read the off-time cap *first* to get the most accurate reading
    // Start up ADC for capacitor pin
    DIDR0 |= (1 << CAP_DIDR);                           // disable digital input on ADC pin to reduce power consumption
#if (ATTINY == 13)
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC3/PB3
#elif (ATTINY == 25)
    ADMUX  = (1 << REFS1) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
#endif
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale

    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    // Start again as datasheet says first result is unreliable
    ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    cap_val = ADCH; // save this for later

#ifdef CONFIG_STARS
    // All ports default to input, but turn pull-up resistors on for the stars (not the ADC input!  Made that mistake already)
    // only one star, because one is used for PWM channel 2
    // and the other is used for the off-time capacitor
    PORTB = (1 << STAR3_PIN);
#endif

    // Set PWM pin to output
    DDRB |= (1 << PWM_PIN);     // enable main channel
    DDRB |= (1 << ALT_PWM_PIN); // enable second channel

    // Set timer to do PWM for correct output pin and set prescaler timing
    //TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    //TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
    TCCR0A = PHASE;
    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Read config values and saved state
#ifdef CONFIG_STARS
    check_stars();
#endif
    restore_state();
    // Enable the current mode group
    count_modes();


    // memory decayed, reset it
    // (should happen on med/long press instead
    //  because mem decay is *much* slower when the OTC is charged
    //  so let's not wait until it decays to reset it)
    //if (fast_presses > 0x20) { fast_presses = 0; }

    if (cap_val > CAP_SHORT) {
        // We don't care what the value is as long as it's over 15
        fast_presses = (fast_presses+1) & 0x1f;
        // Indicates they did a short press, go to the next mode
        next_mode(); // Will handle wrap arounds
#ifdef OFFTIM3
    } else if (cap_val > CAP_MED) {
        fast_presses = 0;
        // User did a medium press, go back one mode
        prev_mode(); // Will handle "negative" modes and wrap-arounds
#endif
    } else {
        // Long press, keep the same mode
        // ... or reset to the first mode
        fast_presses = 0;
        if (! memory) {
            // Reset to the first mode
            mode_idx = 0;
        }
    }
    save_state();

    // Turn off ADC
    //ADC_off();

    // Charge up the capacitor by setting CAP_PIN to output
    DDRB  |= (1 << CAP_PIN);    // Output
    PORTB |= (1 << CAP_PIN);    // High

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    ADC_off();
    #endif
    //ACSR   |=  (1<<7); //AC off

    // Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
    // Will allow us to go idle between WDT interrupts
    //set_sleep_mode(SLEEP_MODE_IDLE);  // not used due to blinky modes

    uint8_t output;
#ifdef NON_WDT_TURBO
    uint8_t ticks = 0;
#endif
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
    uint8_t i = 0;
    uint8_t voltage;
    // Make sure voltage reading is running for later
    ADCSRA |= (1 << ADSC);
#endif
    while(1) {
        output = pgm_read_byte(modesNx + mode_idx);
        if (fast_presses > 0x0f) {  // Config mode
            _delay_s();       // wait for user to stop fast-pressing button
            fast_presses = 0; // exit this mode after one use
            mode_idx = 0;

#ifdef CONFIG_STARS
            // Short/small version of the config mode
            // Toggle the mode group, blink, then exit
            modegroup ^= 1;
            save_state();
            count_modes();  // reconfigure without a power cycle
            blink(1);
#else
            // Longer/larger version of the config mode
            // Toggle the mode group, blink, un-toggle, continue
            toggle(&modegroup);

            // Toggle memory, blink, untoggle, exit
            toggle(&memory);
#endif  // ifdef CONFIG_STARS
        }
#ifdef STROBE
        else if (output == STROBE) {
            // 10Hz tactical strobe
            set_output(255,0);
            _delay_ms(50);
            set_output(0,0);
            _delay_ms(50);
        }
#endif // ifdef STROBE
#ifdef BIKING_STROBE
        else if (output == BIKING_STROBE) {
            // 2-level stutter beacon for biking and such
#ifdef FULL_BIKING_STROBE
            // normal version
            for(i=0;i<4;i++) {
                set_output(255,0);
                _delay_ms(5);
                set_output(0,255);
                _delay_ms(65);
            }
            _delay_ms(720);
#else
            // small/minimal version
            set_output(255,0);
            _delay_ms(10);
            set_output(0,255);
            _delay_s();
#endif
        }
#endif  // ifdef BIKING_STROBE
#ifdef BATTCHECK
        else if (output == BATTCHECK) {
            voltage = get_voltage();
            // figure out how many times to blink
            for (i=0;
                    voltage > pgm_read_byte(voltage_blinks + i);
                    i ++) {}

            // blink zero to five times to show voltage
            // (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
            blink(i);
            // wait between readouts
            _delay_s(); _delay_s();
        }
#endif // ifdef BATTCHECK
        else {  // Regular non-hidden solid mode
            set_mode(mode_idx);
            // This part of the code will mostly replace the WDT tick code.
#ifdef NON_WDT_TURBO
            // Do some magic here to handle turbo step-down
            //if (ticks < 255) ticks++;  // don't roll over
            ticks ++;  // actually, we don't care about roll-over prevention
            if ((ticks > TURBO_TIMEOUT) 
                    && (output == TURBO)) {
                mode_idx = solid_modes - 2; // step down to second-highest mode
                set_mode(mode_idx);
                save_state();
            }
#endif
            // Otherwise, just sleep.
            _delay_ms(500);

            // If we got this far, the user has stopped fast-pressing.
            // So, don't enter config mode.
            fast_presses = 0;
        }
#ifdef VOLTAGE_MON
#if 1
        if (ADCSRA & (1 << ADIF)) {  // if a voltage reading is ready
            voltage = ADCH; // get_voltage();
            // See if voltage is lower than what we were looking for
            //if (voltage < ((mode_idx <= 1) ? ADC_CRIT : ADC_LOW)) {
            if (voltage < ADC_LOW) {
                lowbatt_cnt ++;
            } else {
                lowbatt_cnt = 0;
            }
            // See if it's been low for a while, and maybe step down
            if (lowbatt_cnt >= 8) {
                // DEBUG: blink on step-down:
                //set_output(0,0);  _delay_ms(100);
                i = mode_idx; // save space by not accessing mode_idx more than necessary
                // properly track hidden vs normal modes
                if (i >= solid_modes) {
                    // step down from blinky modes to medium
                    i = 2;
                } else if (i > 0) {
                    // step down from solid modes one at a time
                    i -= 1;
                } else { // Already at the lowest mode
                    i = 0;
                    // Turn off the light
                    set_output(0,0);
                    // Power down as many components as possible
                    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                    sleep_mode();
                }
                set_mode(i);
                mode_idx = i;
                save_state();
                lowbatt_cnt = 0;
                // Wait at least 2 seconds before lowering the level again
                _delay_ms(250);  // this will interrupt blinky modes
            }

            // Make sure conversion is running for next time through
            ADCSRA |= (1 << ADSC);
        }
#endif
#endif  // ifdef VOLTAGE_MON
        //sleep_mode();  // incompatible with blinky modes

        // If we got this far, the user has stopped fast-pressing.
        // So, don't enter config mode.
        //fast_presses = 0;  // doesn't interact well with strobe, too fast
    }

    //return 0; // Standard Return Code
}
