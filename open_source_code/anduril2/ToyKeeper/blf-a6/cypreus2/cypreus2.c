/*
 * Generic clicky-switch-with-offtime-cap firmware.
 * Expects a FET+1 style driver, supports two independent power channels.
 * Similar to tk-otc.c but with extra blinkies.
 * (expects to be configured at compile-time, not runtime)
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
 *      (check bin/flash*.sh for recommended values)
 *
 * STARS
 *      Star 2 = second PWM output channel
 *      Star 3 = not used
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
#define F_CPU 4800000UL

/*
 * =========================================================================
 * Settings to modify per driver
 */

//#define FAST 0x23           // fast PWM channel 1 only
//#define PHASE 0x21          // phase-correct PWM channel 1 only
#define FAST 0xA3           // fast PWM both channels
#define PHASE 0xA1          // phase-correct PWM both channels

#define VOLTAGE_MON         // Comment out to disable LVP
// Adjust the timing per-driver, since the hardware has high variance
// Higher values will run slower, lower values run faster.
#define DELAY_TWEAK         950

#define OFFTIM3             // Use short/med/long off-time presses
                            // instead of just short/long

// Mode group 1
// xp-g2 triple: moon (3) + ./level_calc.py 5 1 10 1950 y 20 8 140
// number of regular non-hidden modes
#define SOLID_MODES         6
// PWM levels for the big circuit (FET or Nx7135)
#define MODESNx             0,0,3,33,110,255
// PWM levels for the small circuit (1x7135)
#define MODES1x             3,20,150,255,255,0
// PWM speed for each mode
#define MODES_PWM           PHASE,FAST,FAST,FAST,FAST,PHASE
// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE1,TURBO .
#define NUM_HIDDEN          4
#define HIDDENMODES         BATTCHECK,STROBE2,BIKING_STROBE3,TURBO
#define HIDDENMODES_PWM     PHASE,PHASE,PHASE,PHASE
#define HIDDENMODES_ALT     10,11,6,0   // short-press goes to this mode index
//#define HIDDENMODES_ALT     6,7,12,0   // short-press goes to this mode index

// Secondary hidden modes are technically between regular and hidden modes.
// Specify them in their normal forward order.
#define NUM_HIDDEN2         10
#define HIDDEN2             BIKING_STROBE1,BIKING_STROBE2,BIKING_STROBE3,BATTCHECK,HEART_BEACON,STROBE1,STROBE2,STROBE3,VAR_STROBE1,VAR_STROBE2
//#define HIDDEN2             HEART_BEACON,STROBE1,STROBE2,STROBE3,VAR_STROBE1,VAR_STROBE2,BIKING_STROBE1,BIKING_STROBE2,BIKING_STROBE3,BATTCHECK
// The last one tells it where to go on short press from final hidden2 mode
// (by default, battcheck loops back around to heart beacon)
#define HIDDEN2_NEXT        0,0,0,0,0,0,0,0,0,SOLID_MODES
// You probably want PHASE for each of these
#define HIDDEN2_PWM         PHASE,PHASE,PHASE,PHASE,PHASE,PHASE,PHASE,PHASE,PHASE,PHASE

// Configure options for hidden modes
// output to use for blinks on battery check mode (primary PWM level, alt PWM level)
// Use 20,0 for a single-channel driver or 0,20 for a two-channel driver
#define BLINK_BRIGHTNESS    0,20
// The values here will be used for both FET and 7135 channels
// (the relative brightness of the blink will be the difference between channels)
#define BIKING_STROBE1_LVL   4 // Dim biking strobe
#define BIKING_STROBE2_LVL  25 // Med biking strobe
#define BIKING_STROBE3_LVL 255 // Bright biking strobe
// Strobe speeds are ontime,offtime in ms (or 0 for a third of a ms)
//#define STROBE1_SPEED  50,50 // 10 Hz tactical strobe
#define STROBE1_SPEED  1,79 // 12.5 Hz party strobe
#define STROBE2_SPEED  0,41 // 24 Hz party strobe
#define STROBE3_SPEED  0,15 // 60 Hz party strobe

#define ENABLE_TURBO       // enable turbo step-down without WDT
// Any primary-channel PWM level above this will be treated as "turbo"
#define TURBO_LEVEL        100
// How many timer ticks before before dropping down.
// Each timer tick is 500ms, so "60" would be a 30-second stepdown.
// Max value of 255 unless you change "ticks"
#define TURBO_TIMEOUT       30

// These values were measured using wight's "A17HYBRID-S" driver built by DBCstm.
// Your mileage may vary.
#define ADC_42          176 // the ADC value we expect for 4.20 volts
#define ADC_100         176 // the ADC value for 100% full (4.2V resting)
#define ADC_75          166 // the ADC value for 75% full (4.0V resting)
#define ADC_50          158 // the ADC value for 50% full (3.8V resting)
#define ADC_25          145 // the ADC value for 25% full (3.5V resting)
#define ADC_0           124 // the ADC value for 0% full (3.0V resting)
#define ADC_LOW         116 // When do we start ramping down (2.8V)
#define ADC_CRIT        112 // When do we shut the light off (2.7V)

// the BLF EE A6 driver may have different offtime cap values than most other drivers
// Values are between 1 and 255, and can be measured with offtime-cap.c
// These #defines are the edge boundaries, not the center of the target.
#ifdef OFFTIM3
#define CAP_SHORT           250  // Anything higher than this is a short press
#define CAP_MED             190  // Between CAP_MED and CAP_SHORT is a medium press
                                 // Below CAP_MED is a long press
#else
#define CAP_SHORT           190  // Anything higher than this is a short press, lower is a long press
#endif

#define TURBO          255  // Convenience code for max turbo mode, must be 255
// Comment out each of the following when not used, to save space
// Also, make sure each enabled item has a different value
#define BATTCHECK      254  // Battery check mode
#define BIKING_STROBE1 253  // Dim biking strobe
#define BIKING_STROBE2 252  // Medium biking strobe
#define BIKING_STROBE3 251  // Full-bright biking strobe
#define STROBE1        250  // Party strobe 1
#define STROBE2        249  // Party strobe 2
#define STROBE3        248  // Party strobe 3
#define HEART_BEACON   247  // Heartbeat beacon
#define VAR_STROBE1    246  // Variable-speed strobe
#define VAR_STROBE2    245  // Variable-speed strobe

/*
 * =========================================================================
 */

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#include <util/delay_basic.h>
// Having own _delay_ms() saves some bytes AND adds possibility to use variables as input
void _delay_ms(uint16_t n)
{
    // TODO: make this take tenths of a ms instead of ms,
    // for more precise timing?
    if (n==0) { _delay_loop_2(400); }
    else {
        while(n-- > 0) _delay_loop_2(DELAY_TWEAK);
    }
}
void _delay_s()  // because it saves a bit of ROM space to do it this way
{
    _delay_ms(1000);
}

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
uint8_t mode_idx = 0;      // current or last-used mode number

// number of regular non-blinky non-hidden modes
#define solid_modes SOLID_MODES
// Number of solid and secondary hidden modes
#define NUM_MODES SOLID_MODES+NUM_HIDDEN2
// total length of current mode group's array
#define mode_cnt NUM_MODES+NUM_HIDDEN

// Modes (hardcoded at compile time)
PROGMEM const uint8_t modesNx[] = { MODESNx, HIDDEN2, HIDDENMODES };
PROGMEM const uint8_t modes1x[] = { MODES1x, HIDDEN2_NEXT, HIDDENMODES_ALT };
PROGMEM const uint8_t modes_pwm[] = { MODES_PWM, HIDDEN2_PWM, HIDDENMODES_PWM };

PROGMEM const uint8_t voltage_blinks[] = {
    ADC_0,    // 1 blink  for 0%-25%
    ADC_25,   // 2 blinks for 25%-50%
    ADC_50,   // 3 blinks for 50%-75%
    ADC_75,   // 4 blinks for 75%-100%
    ADC_100,  // 5 blinks for >100%
    255,      // Ceiling, don't remove
};

void save_state() {  // central method for writing (with wear leveling)
    // Only save the current mode number
    uint8_t eep;
    uint8_t oldpos=eepos;

    eepos = (eepos+1) & 63;  // wear leveling, use next cell

    eep = mode_idx;
    eeprom_write_byte((uint8_t *)(eepos), eep);      // save current state
    eeprom_write_byte((uint8_t *)(oldpos), 0xff);    // erase old state
}

inline void restore_state() {
    uint8_t eep;
    // find the config data
    for(eepos=0; eepos<64; eepos++) {
        eep = eeprom_read_byte((const uint8_t *)eepos);
        if (eep != 0xff) break;
    }
    // unpack the config data
    if (eepos < 64) {
        mode_idx = eep;
    }
    // unnecessary, save_state handles wrap-around
    // (and we don't really care about it skipping cell 0 once in a while)
    //else eepos=0;
}

inline void next_mode() {
    mode_idx += 1; // Advance by one
    if (mode_idx == SOLID_MODES) { // wrap-around from turbo to moon
        mode_idx = 0;
    }
    else if (mode_idx >= NUM_MODES) {
        // if this mode is a hidden one, the "next" mode is whatever is
        // specified in modes1x at its index
        // (should be 0 for last non-hidden mode,
        //  or whatever other mode makes sense otherwise)
        mode_idx = pgm_read_byte(modes1x + mode_idx-1);
    }
}

#ifdef OFFTIM3
inline void prev_mode() {
    if (mode_idx == NUM_MODES) { // hit the end of primary hidden modes
        mode_idx = 0; // back to moon
    } else if (mode_idx == SOLID_MODES) { // hit the end of secondary hidden modes
        mode_idx = NUM_MODES - 1; // loop secondary hidden modes
    } else if (mode_idx > 0) { // Regular non-moon mode
        mode_idx --; // go dimmer
    } else { // Moon goes back into the primary hidden modes
        mode_idx = mode_cnt - 1;
    }
}
#endif


#ifdef VOLTAGE_MON
inline void ADC_on() {
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
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

void set_mode(mode) {
    TCCR0A = pgm_read_byte(modes_pwm + mode);
    set_output(pgm_read_byte(modesNx + mode), pgm_read_byte(modes1x + mode));
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

inline void blink(uint8_t val)
{
    for (; val>0; val--)
    {
        set_output(BLINK_BRIGHTNESS);
        _delay_ms(100);
        set_output(0,0);
        _delay_ms(400);
    }
}

void strobe(uint8_t ontime, uint8_t offtime)
{
    set_output(255,0);
    _delay_ms(ontime);
    set_output(0,0);
    _delay_ms(offtime);
}

void bike_flash(uint8_t lvl)
{
    uint8_t i;
    // 2-level stutter beacon for biking and such
    for(i=0;i<4;i++) {
        set_output(lvl, 0);
        _delay_ms(5);
        set_output(0, lvl);
        _delay_ms(65);
    }
    _delay_ms(720);
}

int main(void)
{
    uint8_t cap_val;

    // Read the off-time cap *first* to get the most accurate reading
    // Start up ADC for capacitor pin
    DIDR0 |= (1 << CAP_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC3/PB3
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale

    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    // Start again as datasheet says first result is unreliable
    ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    cap_val = ADCH; // save this for later

    // All ports default to input, but turn pull-up resistors on for the stars (not the ADC input!  Made that mistake already)
    // only one star, because one is used for PWM channel 2
    // and the other is used for the off-time capacitor
    PORTB = (1 << STAR3_PIN);

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
    restore_state();


    // idea for later: use memory decay instead of OTC for short / med press threshold?
    if (cap_val > CAP_SHORT) {
        // Indicates they did a short press, go to the next mode
        next_mode(); // Will handle wrap arounds
#ifdef OFFTIM3
    } else if (cap_val > CAP_MED) {
        // User did a medium press, go back one mode
        prev_mode(); // Will handle "negative" modes and wrap-arounds
#endif
    } else {
        // Long press, reset to the first mode
        // (this UI is incompatible with mode memory)
        mode_idx = 0;
    }
    save_state();

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

    uint8_t output;
#ifdef ENABLE_TURBO
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
        switch(output) {
#ifdef STROBE1
            case STROBE1: // 12.5 Hz party strobe
                strobe(STROBE1_SPEED);
                break;
#endif
#ifdef STROBE2
            case STROBE2: // 24 Hz party strobe
                strobe(STROBE2_SPEED);
                break;
#endif
#ifdef STROBE3
            case STROBE3: // 60 Hz party strobe
                strobe(STROBE3_SPEED);
                break;
#endif
#ifdef BIKING_STROBE1
            case BIKING_STROBE1: // dim biking flasher
                bike_flash(BIKING_STROBE1_LVL);
                break;
#endif
#ifdef BIKING_STROBE2
            case BIKING_STROBE2: // med biking flasher
                bike_flash(BIKING_STROBE2_LVL);
                break;
#endif
#ifdef BIKING_STROBE3
            case BIKING_STROBE3: // max biking flasher
                bike_flash(BIKING_STROBE3_LVL);
                break;
#endif
#ifdef HEART_BEACON
            case HEART_BEACON: // Heartbeat beacon
                strobe(1,249);
                strobe(1,249);
                _delay_ms(500);
                break;
#endif
#ifdef VAR_STROBE1
            case VAR_STROBE1: // slow variable-speed strobe
                // smoothly oscillating frequency ~7 Hz to ~18 Hz
                {
                    uint8_t j, speed;
                    for(j=0; j<66; j++) {
                        if (j<33) { speed = j; }
                        else { speed = 66-j; }
                        strobe(1,(speed+33-6)<<1);
                    }
                }
                break;
#endif
#ifdef VAR_STROBE2
            case VAR_STROBE2: // fast variable-speed strobe
                // smoothly oscillating frequency ~16 Hz to ~100 Hz
                {
                    uint8_t j, speed;
                    for(j=0; j<100; j++) {
                        if (j<50) { speed = j; }
                        else { speed = 100-j; }
                        strobe(0, speed+9);
                    }
                }
                break;
#endif
#ifdef BATTCHECK
            case BATTCHECK: // battery check mode
                {
                    // turn off and wait one second before showing the value
                    // (also, ensure voltage is measured while not under load)
                    set_output(0,0);
                    _delay_s();
                    voltage = get_voltage();
                    //voltage = get_voltage(); // the first one is unreliable
                    // one blink per voltage range
                    for (i=0;
                            voltage > pgm_read_byte(voltage_blinks + i);
                            i ++) {}


                    // blink zero to five times to show voltage
                    // (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
                    blink(i);
                    //_delay_s();  // wait at least 1 second between readouts
                }
                break;
#endif
            default: // Regular non-hidden solid mode
                set_mode(mode_idx);
                // This part of the code will mostly replace the WDT tick code.
#ifdef ENABLE_TURBO
                // Do some magic here to handle turbo step-down
                //if (ticks < 255) ticks++;  // don't roll over
                ticks ++;  // actually, we don't care about roll-over prevention
                if ((ticks > TURBO_TIMEOUT) 
                        && (output >= TURBO_LEVEL)) {
                    if (mode_idx >= SOLID_MODES) { // handle step-down from hidden turbo
                        mode_idx = SOLID_MODES - 2;
                    } else { // step-down from non-hidden turbo(s)
                        mode_idx = mode_idx - 1; // step down one level
                    }
                    ticks = 0; // in case we need to step down again
                    set_mode(mode_idx);
                    save_state();
                }
#endif
                // Otherwise, just sleep.
                _delay_ms(500);
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
    }

    //return 0; // Standard Return Code
}
