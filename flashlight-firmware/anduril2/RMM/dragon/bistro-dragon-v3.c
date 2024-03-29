/*
 * "Bistro" firmware
 *  Modified for dual color use, as seen on the CWF Dragon Driver by MTN Electronics / RMM 2018
 *  Some other changes:
 *  -Changed configuration options and order of options.
 *  -Small "ramp" size with discrete levels instead of using full ramp. 
 *  -Changed some of the blinking speeds and levels "to taste". 
 *
 * This code runs on a single-channel or dual-channel driver (FET+7135)
 * with an attiny25/45/85 MCU and a capacitor to measure offtime (OTC).
 *
 * Copyright (C) 2015 Selene Scriven
 * 
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
 * ATTINY25/45/85 Diagram
 *           ----
 *         -|1  8|- VCC
 *     OTC -|2  7|- Voltage ADC
 *  Star 3 -|3  6|- PWM (FET, optional)
 *     GND -|4  5|- PWM (1x7135)
 *           ----
 *
 * FUSES
 *      I use these fuse settings on attiny25
 *      Low:  0xd2
 *      High: 0xde
 *      Ext:  0xff
 *
 *      For more details on these settings:
 *      http://www.engbedded.com/cgi-bin/fcx.cgi?P_PREV=ATtiny25&P=ATtiny25&M_LOW_0x3F=0x12&M_HIGH_0x07=0x06&M_HIGH_0x20=0x00&B_SPIEN=P&B_SUT0=P&B_CKSEL3=P&B_CKSEL2=P&B_CKSEL0=P&B_BODLEVEL0=P&V_LOW=E2&V_HIGH=DE&V_EXTENDED=FF
 *
 * STARS
 *      Star 3 = unused
 *
 * CALIBRATION
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
#define ATTINY 25
// FIXME: make 1-channel vs 2-channel power a single #define option
#define FET_7135_LAYOUT  // specify an I/O pin layout
// Also, assign I/O pins in this file:
#include "tk-attiny.h"

/*
 * =========================================================================
 * Settings to modify per driver
 */

// FIXME: make 1-channel vs 2-channel power a single #define option
//#define FAST 0x23           // fast PWM channel 1 only
//#define PHASE 0x21          // phase-correct PWM channel 1 only
#define FAST 0xA3           // fast PWM both channels
#define PHASE 0xA1          // phase-correct PWM both channels

#define VOLTAGE_MON         // Comment out to disable LVP

#define OFFTIM3             // Use short/med/long off-time presses
                            // instead of just short/long

// ../../bin/level_calc.py 64 1 10 1400 y 3 0.25 140
//#define RAMP_SIZE  64
// log curve
//#define RAMP_7135  3,3,3,3,3,3,4,4,4,4,4,5,5,5,6,6,7,7,8,9,10,11,12,13,15,16,18,21,23,27,30,34,39,44,50,57,65,74,85,97,111,127,145,166,190,217,248,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_FET   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,6,11,17,23,30,39,48,59,72,86,103,121,143,168,197,255
// x**2 curve
//#define RAMP_7135  3,5,8,12,17,24,32,41,51,63,75,90,105,121,139,158,178,200,223,247,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_FET   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,4,6,9,12,16,19,22,26,30,33,37,41,45,50,54,59,63,68,73,78,84,89,94,100,106,111,117,123,130,136,142,149,156,162,169,176,184,191,198,206,214,221,255
// x**3 curve

//#define RAMP_7135  6,6,7,7,8,9,10,12,16,20,25,30,36,42,50,59,68,78,90,103,116,131,148,165,184,204,226,249,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_FET   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,4,7,10,13,16,20,24,28,32,36,41,46,51,56,61,67,73,80,86,93,100,107,115,123,131,139,148,157,166,176,186,196,207,218,255

// x**5 curve
//#define RAMP_7135  3,3,3,4,4,5,5,6,7,8,10,11,13,15,18,21,24,28,33,38,44,50,57,66,75,85,96,108,122,137,154,172,192,213,237,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_FET   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,3,6,9,13,17,21,25,30,35,41,47,53,60,67,75,83,91,101,111,121,132,144,156,169,183,198,213,255

// uncomment to ramp up/down to a mode instead of jumping directly
//#define SOFT_START

// THESE VALUES ARE USED FOR EASY LEVEL CHANGING INSTEAD OF USING THE RAMP
#define RAMP_SIZE 8
#define RAMP_7135 25,255,0,0,0,0,0,0
#define RAMP_FET  0,0,1,15,40,90,140,255

// Enable battery indicator mode?
#define USE_BATTCHECK
// Choose a battery indicator style
//#define BATTCHECK_4bars  // up to 4 blinks
//#define BATTCHECK_8bars  // up to 8 blinks
#define BATTCHECK_VpT  // Volts + tenths

// output to use for blinks on battery check (and other modes)
#define BLINK_BRIGHTNESS    RAMP_SIZE/4
// ms per normal-speed blink
//#define BLINK_SPEED         650
#define BLINK_SPEED         980

// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,TURBO .
#define HIDDENMODES         BIKING_STROBE,BATTCHECK,STROBE,TURBO

#define TURBO     RAMP_SIZE       // Convenience code for turbo mode
#define BATTCHECK 254       // Convenience code for battery check mode
#define GROUP_SELECT_MODE 253
#define TEMP_CAL_MODE 252
// Uncomment to enable tactical strobe mode
#define STROBE    251       // Convenience code for strobe mode
// Uncomment to unable a 2-level stutter beacon instead of a tactical strobe
#define BIKING_STROBE 250   // Convenience code for biking strobe mode
// comment out to use minimal version instead (smaller)
#define FULL_BIKING_STROBE
//#define RAMP 249       // ramp test mode for tweaking ramp shape
//#define POLICE_STROBE 248
//#define RANDOM_STROBE 247

// thermal step-down
#define TEMPERATURE_MON

// Calibrate voltage and OTC in this file:
#include "tk-calibration.h"

/*
 * =========================================================================
 */

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#include <avr/pgmspace.h>
//#include <avr/io.h>
//#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>
#include <string.h>

#define OWN_DELAY           // Don't use stock delay functions.
#define USE_DELAY_S         // Also use _delay_s(), not just _delay_ms()
#include "tk-delay.h"

#include "tk-voltage.h"

#ifdef RANDOM_STROBE
#include "tk-random.h"
#endif

/*
 * global variables
 */

// Config option variables
#define FIRSTBOOT 0b01010101
uint8_t firstboot = FIRSTBOOT;  // detect initial boot or factory reset
uint8_t modegroup = 0;     // which mode group (set above in #defines)
uint8_t enable_moon = 0;   // Should we add moon to the set of modes?
uint8_t reverse_modes = 0; // flip the mode order?
uint8_t memory = 0;        // mode memory, or not (set via soldered star)
#ifdef OFFTIM3
uint8_t offtim3 = 1;       // enable medium-press?
#endif
#ifdef TEMPERATURE_MON
uint8_t maxtemp = 85;      // temperature step-down threshold
uint8_t maxtempturbo = 77;
#endif
uint8_t muggle_mode = 0;   // simple mode designed for muggles
// Other state variables
uint8_t mode_override = 0; // do we need to enter a special mode?
uint8_t mode_idx = 0;      // current or last-used mode number
uint8_t eepos = 0;
// counter for entering config mode
// (needs to be remembered while off, but only for up to half a second)
uint8_t fast_presses __attribute__ ((section (".noinit")));

// total length of current mode group's array
uint8_t mode_cnt;
// number of regular non-hidden modes in current mode group
uint8_t solid_modes;
// number of hidden modes in the current mode group
// (hardcoded because both groups have the same hidden modes)
//uint8_t hidden_modes = NUM_HIDDEN;  // this is never used


PROGMEM const uint8_t hiddenmodes[] = { HIDDENMODES };
// default values calculated by group_calc.py
// Each group must be 8 values long, but can be cut short with a zero.
//#define NUM_MODEGROUPS 8  // don't count muggle mode

/* Current mode levels and groups are as follows:
*  1= Low Red; 2 = High Red
*  3 = ML White 4 = 5% White
*  5 = 15% White 6= 35% White
*  7 = 50% White 8 = 100% White
* Mode Groups:
*  1- low red,high red,ml,5,15,35,50,100
*  2- low red,high red,5,15,35,100
*  3- low red,ml,5,15,35,50,100
*  4- low red,5,15,35,100
*  5- low red,high red,15,100
*  6- low red,15,100
*  7- ML,5 ,15,35,50,100
*  8- ML,5,35,100,low red // MK change
*/

#define NUM_MODEGROUPS 8
PROGMEM const uint8_t modegroups[] = {
    1, 2, 3,  4,  5,  6,  7,  8,
    1, 2, 4,  5,  6,  8,  0,  0,
    1, 3, 4,  5,  6,  7,  8,  0,
    1, 4, 5, 6,  8,  0,  0,  0,
    1, 2, 4, 8, 0, 0,  0,  0,
    1, 5, 8, 0, 0, 0,  0,  0,
    3, 4, 5, 6, 7, 8, 0,  0,
    3, 4, 6, 8, 1, 0, 0, 0,
  //  10, 30, 50,  0,                  // muggle mode, exception to "must be 8 bytes long"
};
uint8_t modes[] = { 1,2,3,4,5,6,7,8,9, HIDDENMODES };  // make sure this is long enough...

// Modes (gets set when the light starts up based on saved config values)
PROGMEM const uint8_t ramp_7135[] = { RAMP_7135 };
PROGMEM const uint8_t ramp_FET[]  = { RAMP_FET };

void save_mode() {  // save the current mode index (with wear leveling)
    uint8_t oldpos=eepos;

    eepos = (eepos+1) & ((EEPSIZE/2)-1);  // wear leveling, use next cell

    eeprom_write_byte((uint8_t *)(eepos), mode_idx);  // save current state
    eeprom_write_byte((uint8_t *)(oldpos), 0xff);     // erase old state
}

#define OPT_firstboot (EEPSIZE-1)
#define OPT_modegroup (EEPSIZE-2)
#define OPT_memory (EEPSIZE-3)
#define OPT_offtim3 (EEPSIZE-4)
#define OPT_maxtemp (EEPSIZE-5)
#define OPT_mode_override (EEPSIZE-6)
#define OPT_moon (EEPSIZE-7)
#define OPT_revmodes (EEPSIZE-8)
#define OPT_muggle (EEPSIZE-9)
void save_state() {  // central method for writing complete state
    save_mode();
    eeprom_write_byte((uint8_t *)OPT_firstboot, firstboot);
    eeprom_write_byte((uint8_t *)OPT_modegroup, modegroup);
    eeprom_write_byte((uint8_t *)OPT_memory, memory);
#ifdef OFFTIM3
    eeprom_write_byte((uint8_t *)OPT_offtim3, offtim3);
#endif
#ifdef TEMPERATURE_MON
    eeprom_write_byte((uint8_t *)OPT_maxtemp, maxtemp);
#endif
    eeprom_write_byte((uint8_t *)OPT_mode_override, mode_override);
    eeprom_write_byte((uint8_t *)OPT_moon, enable_moon);
    eeprom_write_byte((uint8_t *)OPT_revmodes, reverse_modes);
    eeprom_write_byte((uint8_t *)OPT_muggle, muggle_mode);
}

void restore_state() {
    uint8_t eep;

    // check if this is the first time we have powered on
    eep = eeprom_read_byte((uint8_t *)OPT_firstboot);
    if (eep != FIRSTBOOT) {
        // not much to do; the defaults should already be set
        // while defining the variables above
        save_state();
        return;
    }

    // find the mode index data
    for(eepos=0; eepos<(EEPSIZE/2); eepos++) {
        eep = eeprom_read_byte((const uint8_t *)eepos);
        if (eep != 0xff) {
            mode_idx = eep;
            break;
        }
    }

    // load other config values
    modegroup = eeprom_read_byte((uint8_t *)OPT_modegroup);
    memory    = eeprom_read_byte((uint8_t *)OPT_memory);
#ifdef OFFTIM3
    offtim3   = eeprom_read_byte((uint8_t *)OPT_offtim3);
#endif
#ifdef TEMPERATURE_MON
    maxtemp   = eeprom_read_byte((uint8_t *)OPT_maxtemp);
#endif
    mode_override = eeprom_read_byte((uint8_t *)OPT_mode_override);
    enable_moon   = eeprom_read_byte((uint8_t *)OPT_moon);
    reverse_modes = eeprom_read_byte((uint8_t *)OPT_revmodes);
    muggle_mode   = eeprom_read_byte((uint8_t *)OPT_muggle);

    // unnecessary, save_state handles wrap-around
    // (and we don't really care about it skipping cell 0 once in a while)
    //else eepos=0;
}

inline void next_mode() {
    mode_idx += 1;
    if (mode_idx >= solid_modes) {
        // Wrap around, skipping the hidden modes
        // (note: this also applies when going "forward" from any hidden mode)
        // FIXME? Allow this to cycle through hidden modes?
        mode_idx = 0;
    }
}

#ifdef OFFTIM3
inline void prev_mode() {
    // simple mode has no reverse
    if (muggle_mode) { return next_mode(); }

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

void count_modes() {
    /*
     * Determine how many solid and hidden modes we have.
     *
     * (this matters because we have more than one set of modes to choose
     *  from, so we need to count at runtime)
     */
    // copy config to local vars to avoid accidentally overwriting them in muggle mode
    // (also, it seems to reduce overall program size)
    uint8_t my_modegroup = modegroup;
    uint8_t my_enable_moon = enable_moon;
    uint8_t my_reverse_modes = reverse_modes;

    // override config if we're in simple mode
    if (muggle_mode) {
        my_modegroup = NUM_MODEGROUPS;
        my_enable_moon = 0;
        my_reverse_modes = 0;
    }

    uint8_t *dest;
    const uint8_t *src = modegroups + (my_modegroup<<3);
    dest = modes;

    // Figure out how many modes are in this group
    //solid_modes = modegroup + 1;  // Assume group N has N modes
    // No, how about actually counting the modes instead?
    // (in case anyone changes the mode groups above so they don't form a triangle)
    for(solid_modes=0;
        (solid_modes<8) && pgm_read_byte(src + solid_modes);
        solid_modes++ ) {}

    // add moon mode (or not) if config says to add it
    if (my_enable_moon) {
        modes[0] = 1;
        dest ++;
    }

    // add regular modes
    memcpy_P(dest, src, solid_modes);
    // add hidden modes
    memcpy_P(dest + solid_modes, hiddenmodes, sizeof(hiddenmodes));
    // final count
    mode_cnt = solid_modes + sizeof(hiddenmodes);
    if (my_reverse_modes) {
        // TODO: yuck, isn't there a better way to do this?
        int8_t i;
        src += solid_modes;
        dest = modes;
        for(i=0; i<solid_modes; i++) {
            src --;
            *dest = pgm_read_byte(src);
            dest ++;
        }
        if (my_enable_moon) {
            *dest = 1;
        }
        mode_cnt --;  // get rid of last hidden mode, since it's a duplicate turbo
    }
    if (my_enable_moon) {
        mode_cnt ++;
        solid_modes ++;
    }
}

inline void set_output(uint8_t pwm1, uint8_t pwm2) {
    /* This is no longer needed since we always use PHASE mode.
    // Need PHASE to properly turn off the light
    if ((pwm1==0) && (pwm2==0)) {
        TCCR0A = PHASE;
    }
    */
    PWM_LVL = pwm1;
    ALT_PWM_LVL = pwm2;
}

void set_level(uint8_t level) {
    if (level == 0) {
        set_output(0,0);
    } else {
        level -= 1;
        set_output(pgm_read_byte(ramp_FET  + level),
                   pgm_read_byte(ramp_7135 + level));
    }
}

void set_mode(uint8_t mode) {
#ifdef SOFT_START
    static uint8_t actual_level = 0;
    uint8_t target_level = mode;
    int8_t shift_amount;
    int8_t diff;
    do {
        diff = target_level - actual_level;
        shift_amount = (diff >> 2) | (diff!=0);
        actual_level += shift_amount;
        set_level(actual_level);
    //    _delay_ms(RAMP_SIZE/20);  // slow ramp
		_delay_ms(10);
	    //_delay_ms(RAMP_SIZE/10);  // fast ramp
    } while (target_level != actual_level);
#else
    set_level(mode);
#endif  // SOFT_START
}

void blink(uint8_t val, uint16_t speed)
{
    for (; val>0; val--)
    {
        set_level(BLINK_BRIGHTNESS);
        _delay_ms(speed);
        set_level(0);
        _delay_ms(speed<<2);
    }
}

void strobe(uint8_t ontime, uint8_t offtime) {
    set_level(RAMP_SIZE);
    _delay_ms(ontime);
    set_level(0);
    _delay_ms(offtime);
}

void toggle(uint8_t *var, uint8_t num) {
    // Used for config mode
    // Changes the value of a config option, waits for the user to "save"
    // by turning the light off, then changes the value back in case they
    // didn't save.  Can be used repeatedly on different options, allowing
    // the user to change and save only one at a time.
    blink(num, BLINK_SPEED/8);  // indicate which option number this is
    *var ^= 1;
    save_state();
    // "buzz" for a while to indicate the active toggle window
    for(uint8_t i=0; i<32; i++) {
        set_level(BLINK_BRIGHTNESS * 3 / 4);
        _delay_ms(20);
        set_level(0);
        _delay_ms(20);
    }
    // if the user didn't click, reset the value and return
    *var ^= 1;
    save_state();
    _delay_s();
}

#ifdef TEMPERATURE_MON
uint8_t get_temperature() {
    ADC_on_temperature();
    // average a few values; temperature is noisy
    uint16_t temp = 0;
    uint8_t i;
    get_voltage();
    for(i=0; i<16; i++) {
        temp += get_voltage();
        _delay_ms(5);
    }
    temp >>= 4;
    return temp;
}
#endif  // TEMPERATURE_MON

inline uint8_t read_otc() {
    // Read and return the off-time cap value
    // Start up ADC for capacitor pin
    // disable digital input on ADC pin to reduce power consumption
    DIDR0 |= (1 << CAP_DIDR);
    // 1.1v reference, left-adjust, ADC3/PB3
    ADMUX  = (1 << V_REF) | (1 << ADLAR) | CAP_CHANNEL;
    // enable, start, prescale
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;

    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    // Start again as datasheet says first result is unreliable
    ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));

    // ADCH should have the value we wanted
    return ADCH;
}

int main(void)
{
    // check the OTC immediately before it has a chance to charge or discharge
    uint8_t cap_val = read_otc();  // save it for later

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

    // Enable the current mode group
    count_modes();


    // TODO: Enable this?  (might prevent some corner cases, but requires extra room)
    // memory decayed, reset it
    // (should happen on med/long press instead
    //  because mem decay is *much* slower when the OTC is charged
    //  so let's not wait until it decays to reset it)
    //if (fast_presses > 0x20) { fast_presses = 0; }

    // check button press time, unless the mode is overridden
    if (! mode_override) {
        if (cap_val > CAP_SHORT) {
            // Indicates they did a short press, go to the next mode
            // We don't care what the fast_presses value is as long as it's over 15
            fast_presses = (fast_presses+1) & 0x1f;
            next_mode(); // Will handle wrap arounds
#ifdef OFFTIM3
        } else if (cap_val > CAP_MED) {
            // User did a medium press, go back one mode
            fast_presses = 0;
            if (offtim3) {
                prev_mode();  // Will handle "negative" modes and wrap-arounds
            } else {
                next_mode();  // disabled-med-press acts like short-press
                              // (except that fast_presses isn't reliable then)
            }
#endif
        } else {
            // Long press, keep the same mode
            // ... or reset to the first mode
            fast_presses = 0;
            if (muggle_mode  || (! memory)) {
                // Reset to the first mode
                mode_idx = 0;
            }
        }
    }
    save_mode();

    // Charge up the capacitor by setting CAP_PIN to output
    DDRB  |= (1 << CAP_PIN);    // Output
    PORTB |= (1 << CAP_PIN);    // High

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    ADC_off();
    #endif

    uint8_t output;
    uint8_t actual_level;
#ifdef TEMPERATURE_MON
    uint8_t overheat_count = 0;
#endif
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
    uint8_t i = 0;
    uint8_t voltage;
    // Make sure voltage reading is running for later
    ADCSRA |= (1 << ADSC);
#endif
    //output = pgm_read_byte(modes + mode_idx);
    output = modes[mode_idx];
    actual_level = output;
    // handle mode overrides, like mode group selection and temperature calibration
    if (mode_override) {
        // do nothing; mode is already set
        //mode_idx = mode_override;
        fast_presses = 0;
        output = mode_idx;
    }
    while(1) {
        if (fast_presses > 0x0f) {  // Config mode
            _delay_s();       // wait for user to stop fast-pressing button
            fast_presses = 0; // exit this mode after one use
            mode_idx = 0;

// ***DISABLED OPTIONS & CHANGED ORDER FROM ORIGINAL BISTRO***
            // Enter or leave "muggle mode"?
  //          toggle(&muggle_mode, 1);
   //         if (muggle_mode) { continue; };  // don't offer other options in muggle mode

   

      //      toggle(&enable_moon, 3);

  

            // Enter the mode group selection mode?
            mode_idx = GROUP_SELECT_MODE;
  //          toggle(&mode_override, 5);
			toggle(&mode_override, 1);
            mode_idx = 0;
	
	        toggle(&memory, 2);
	
	        toggle(&reverse_modes, 3);



#ifdef TEMPERATURE_MON
            // Enter temperature calibration mode?
            mode_idx = TEMP_CAL_MODE;
            toggle(&mode_override, 4);
            mode_idx = 0;
#endif

#ifdef OFFTIM3
            toggle(&offtim3, 5);
#endif
// *** DISABLED OPTION ***
   //         toggle(&firstboot, 8);

            //output = pgm_read_byte(modes + mode_idx);
            output = modes[mode_idx];
            actual_level = output;
        }
#ifdef STROBE
        else if (output == STROBE) {
            // 10Hz tactical strobe
            strobe(20,35);
        }
#endif // ifdef STROBE
#ifdef POLICE_STROBE
        else if (output == POLICE_STROBE) {
            // police-like strobe
            for(i=0;i<8;i++) {
                strobe(20,40);
            }
            for(i=0;i<8;i++) {
                strobe(40,80);
            }
        }
#endif // ifdef POLICE_STROBE
#ifdef RANDOM_STROBE
        else if (output == RANDOM_STROBE) {
            // pseudo-random strobe
            uint8_t ms = 34 + (pgm_rand() & 0x3f);
            strobe(ms, ms);
            strobe(ms, ms);
        }
#endif // ifdef RANDOM_STROBE
#ifdef BIKING_STROBE
        else if (output == BIKING_STROBE) {
            // 2-level stutter beacon for biking and such
#ifdef FULL_BIKING_STROBE
            // normal version
			// CHANGED OUTPUT LEVELS
            for(i=0;i<4;i++) {
         //       set_output(255,0);
				set_output(255,0);	
		        _delay_ms(5);
                set_output(0,20);
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
#ifdef RAMP
        else if (output == RAMP) {
            int8_t r;
            // simple ramping test
            for(r=1; r<=RAMP_SIZE; r++) {
                set_level(r);
                _delay_ms(25);
            }
            for(r=RAMP_SIZE; r>0; r--) {
                set_level(r);
                _delay_ms(25);
            }
        }
#endif  // ifdef RAMP
#ifdef BATTCHECK
        else if (output == BATTCHECK) {
#ifdef BATTCHECK_VpT
            // blink out volts and tenths
            uint8_t result = battcheck();
            blink(result >> 5, BLINK_SPEED/8);
            _delay_ms(BLINK_SPEED);
            blink(1,5);
            _delay_ms(BLINK_SPEED*3/2);
            blink(result & 0b00011111, BLINK_SPEED/8);
#else  // ifdef BATTCHECK_VpT
            // blink zero to five times to show voltage
            // (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
            blink(battcheck(), BLINK_SPEED/8);
#endif  // ifdef BATTCHECK_VpT
            // wait between readouts
            _delay_s(); _delay_s();
        }
#endif // ifdef BATTCHECK
        else if (output == GROUP_SELECT_MODE) {
            // exit this mode after one use
            mode_idx = 0;
            mode_override = 0;

            for(i=0; i<NUM_MODEGROUPS; i++) {
                modegroup = i;
                save_state();

 //               blink(1, BLINK_SPEED/3);
				blink(1, BLINK_SPEED/5);
            }
            _delay_s(); _delay_s();
        }
#ifdef TEMP_CAL_MODE
        else if (output == TEMP_CAL_MODE) {
            // make sure we don't stay in this mode after button press
            mode_idx = 0;
            mode_override = 0;

            // Allow the user to turn off thermal regulation if they want
            maxtemp = 255;
            save_state();
            set_mode(RAMP_SIZE/4);  // start somewhat dim during turn-off-regulation mode
            _delay_s(); _delay_s();

            // run at highest output level, to generate heat
            set_mode(RAMP_SIZE);

            // measure, save, wait...  repeat
            while(1) {
                maxtemp = get_temperature();
                save_state();
                _delay_s(); _delay_s();
            }
        }
#endif  // TEMP_CAL_MODE
        else {  // Regular non-hidden solid mode
            set_mode(actual_level);
#ifdef TEMPERATURE_MON
            uint8_t temp = get_temperature();

            // step down? (or step back up?)
            if (temp >= maxtemp) {
                overheat_count ++;
                // reduce noise, and limit the lowest step-down level
                if ((overheat_count > 20) && (actual_level > 4)) {
					actual_level --; //bigger steps
				if (actual_level > 6) {		 
			_delay_s();
			_delay_s();
			_delay_s();
				//don't ramp down too fast!
				}
				else {
				_delay_ms(5000);
				_delay_ms(5000);
				_delay_ms(5000);
				}					
					overheat_count = 0;  // don't ramp down too fast
                }
            } else {
                // if we're not overheated, ramp up to the user-requested level
                overheat_count = 0;
                if ((temp < maxtemp - 1) && (actual_level < output)) {
                    actual_level ++;
					_delay_s(); _delay_s(); _delay_s(); _delay_s(); _delay_s(); _delay_s(); 
					_delay_s(); _delay_s(); _delay_s(); _delay_s(); _delay_s(); _delay_s(); 
					_delay_s(); _delay_s(); _delay_s(); _delay_s(); _delay_s(); _delay_s();  // don't step back up too fast and overshoot.  
				
				}
            }
            set_mode(actual_level);

            ADC_on();  // return to voltage mode
#endif
            // Otherwise, just sleep.
            _delay_ms(500);

            // If we got this far, the user has stopped fast-pressing.
            // So, don't enter config mode.
            fast_presses = 0;
        }
#ifdef VOLTAGE_MON
        if (ADCSRA & (1 << ADIF)) {  // if a voltage reading is ready
            voltage = ADCH;  // get the waiting value
            // See if voltage is lower than what we were looking for
            if (voltage < ADC_LOW) {
                lowbatt_cnt ++;
            } else {
                lowbatt_cnt = 0;
            }
            // See if it's been low for a while, and maybe step down
            if (lowbatt_cnt >= 8) {
                // DEBUG: blink on step-down:
                //set_level(0);  _delay_ms(100);

                if (actual_level > RAMP_SIZE) {  // hidden / blinky modes
                    // step down from blinky modes to medium
                    actual_level = RAMP_SIZE / 2;
                } else if (actual_level > 1) {  // regular solid mode
                    // step down from solid modes somewhat gradually
					actual_level--;
                    //actual_level = (actual_level >> 1);
                } else { 
                    // Turn off the light
                    set_level(0);
                    // Power down as many components as possible
                    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                    sleep_mode();
                }
                set_mode(actual_level);
                output = actual_level;
                //save_mode();  // we didn't actually change the mode
                lowbatt_cnt = 0;
                // Wait at least 2 seconds before lowering the level again
                _delay_ms(250);  // this will interrupt blinky modes
            }

            // Make sure conversion is running for next time through
            ADCSRA |= (1 << ADSC);
        }
#endif  // ifdef VOLTAGE_MON
    }

    //return 0; // Standard Return Code
}
	
