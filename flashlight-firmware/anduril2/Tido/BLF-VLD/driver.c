/*
 * Versatile driver for ATtiny controlled flashlights
 * Copyright (C) 2010-2011 Tido Klaassen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * driver.c
 *
 *  Created on: 19.10.2010
 *      Author: tido
 */

#include<avr/io.h>
#include<util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/power.h>

/*
 * configure the driver here. For fixed modes, also change the order and/or
 * function variables in the initial EEPROM image down below to your needs.
 */
#define NUM_MODES 3         // how many modes should the flashlight have (1-5)
#define NUM_EXT_CLICKS 6    // how many clicks to enter programming mode
#define EXTENDED_MODES      // enable to make all mode lines available
#define PROGRAMMABLE        // user can re-program the mode slots
#define PROGHELPER          // indicate programming timing by short flashes
//#define NOMEMORY            // #define to disable mode memory
#define FUNC_STROBE         // #define to compile in strobe mode
//#define FUNC_SOS            // #define to compile in SOS signal
//#define FUNC_ALPINE         // #define to compile in alpine distress signal
//#define FUNC_FADE           // #define to compile in fade in-out mode

// Config for battery monitoring
//#define MONITOR_BAT       // enable battery monitoring
#define LOWBAT_TRIG 130     // trigger level for low battery, see README
#define LOWBAT_RAMPDOWN     // decrease output gradually when battery fails
#define LOWBAT_MIN_LVL 0x04 // minimal PWM level to use in low battery situation
#define LOWBAT_MAX_LVL 0x40 // maximum PWM level to start ramping down from
#define ADC_MUX 0x01        // ADC channel to use, see README
#define ADC_DIDR ADC1D      // digital input to disable, see README
#define ADC_PRSCL 0x06      // ADC prescaler of 64

/*
 * PWM specifics can be configured here. You only need to change this if
 * your driver PCB uses different pins for output or you wish to change
 * the PWM frequency via the timer prescaler.
 */
#define PWM_PIN PB1         // look at your PCB to find out which pin the FET
#define PWM_LVL OCR0B       // or 7135 is connected to, then consult the
                            // data-sheet on which pin and OCR to use

#define PWM_TCR 0x21        // phase corrected PWM. Set to 0x81 for PB0,
                            // 0x21 for PB1
#define PWM_SCL 0x01        // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
                            // use this to scale down PWM frequency if needed

/*
 * Configure mode switching based on the state of an I/O pin on start up
 */
//#define PINSWITCH       // enable pin state based mode switching
#define PS_PIN PB4      // pin to query
#define PS_HIGH         // high level indicates mode switch
#define PS_CHARGE       // pull the pin high/low during runtime to (dis)charge
                        // a capacitor connected to the pin


/*
 * override #defines set above if called with build profiles from the IDE
 */

#ifdef BUILD_PROGRAMMABLE
#undef PROGRAMMABLE
#undef EXTENDED_MODES
#undef PROGHELPER
#undef NOMEMORY
#undef MONITOR_BAT
#undef PINSWITCH
#undef FUNC_SOS
#undef FUNC_ALPINE
#undef FUNC_FADE
#define EXTENDED_MODES
#define PROGRAMMABLE
#define PROGHELPER
#define FUNC_STROBE
#endif

#ifdef BUILD_FIXED
#undef PROGRAMMABLE
#undef EXTENDED_MODES
#undef PROGHELPER
#undef NOMEMORY
#undef PINSWITCH
#undef MONITOR_BAT
#undef FUNC_SOS
#undef FUNC_ALPINE
#undef FUNC_FADE
#define EXTENDED_MODES
#define FUNC_STROBE
#endif

#ifdef BUILD_SIMPLE
#undef PROGRAMMABLE
#undef EXTENDED_MODES
#undef PROGHELPER
#undef NOMEMORY
#undef MONITOR_BAT
#undef PINSWITCH
#undef FUNC_SOS
#undef FUNC_ALPINE
#undef FUNC_FADE
#define FUNC_STROBE
#endif


// no sense to include programming code if extended modes are disabled
#ifndef EXTENDED_MODES
#undef PROGRAMMABLE
#endif

// no need for the programming helper if programming is disabled
#ifndef PROGRAMMABLE
#undef PROGHELPER
#endif

/*
 * Set up the initial EEPROM image
 *
 * define some mode configurations. Format is
 * "offset in mode_func_arr", "parameter 1", "parameter 2", "parameter 3"
 */
#define MODE_LVL001 0x00, 0x01, 0x00, 0x00  // lowest possible level
#define MODE_LVL002 0x00, 0x02, 0x00, 0x00  //      .
#define MODE_LVL004 0x00, 0x04, 0x00, 0x00  //      .
#define MODE_LVL008 0x00, 0x08, 0x00, 0x00  //      .
#define MODE_LVL016 0x00, 0x10, 0x00, 0x00  //      .
#define MODE_LVL032 0x00, 0x20, 0x00, 0x00  //      .
#define MODE_LVL064 0x00, 0x40, 0x00, 0x00  //      .
#define MODE_LVL128 0x00, 0x80, 0x00, 0x00  //      .
#define MODE_LVL255 0x00, 0xFF, 0x00, 0x00  // highest possible level
#define MODE_EMPTY  0xFF, 0xFF, 0xFF, 0xFF  // empty mode slot

/*
 * define mode lines only if the corresponding mode function has been enabled.
 * This way an EEPROM image containing lines for functions not compiled in will
 * result in a compile time error.
 */
#ifdef FUNC_STROBE
#define MODE_STROBE 0x01, 0x14, 0xFF, 0x00  // simple strobe mode
#define MODE_POLICE 0x01, 0x14, 0x0A, 0x01  // police type strobe
#define MODE_BEACON 0x01, 0x14, 0x01, 0x0A  // beacon, might actually be useful
#endif

#ifdef FUNC_SOS
#define MODE_SOS    0x02, 0x00, 0x00, 0x00  // morse SOS
#endif

#ifdef FUNC_ALPINE
#define MODE_ALPINE 0x03, 0x00, 0x00, 0x00  // alpine distress signal
#endif

#ifdef FUNC_FADE
#define MODE_FADE   0x04, 0xFF, 0x01, 0x01  // fade in/out. Just a gimmick
#endif

/*
 * initialise EEPROM
 * This will be used to build the initial eeprom image.
 */
const uint8_t EEMEM eeprom[64] =
{   0x00, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    // initial mode programming, indices to mode lines in the following array
    0x03, 0x06, 0x08, 0x00, 0x00,
    // mode configuration starts here. Format is:
    // offset in mode_func_arr, func data1, func data2, func data3
    MODE_LVL001,    // 0x00
    MODE_LVL002,    // 0x01
    MODE_LVL004,    // 0x02
    MODE_LVL008,    // 0x03
    MODE_LVL016,    // 0x04
    MODE_LVL032,    // 0x05
    MODE_LVL064,    // 0x06
    MODE_LVL128,    // 0x07
    MODE_LVL255,    // 0x08
    MODE_STROBE,    // 0x09
    MODE_POLICE,    // 0x0A
    MODE_BEACON     // 0x0B
};

/*
 * forward declarations of mode functions
 */
void const_level(uint8_t offset);
void nullmode(uint8_t offset);

#ifdef FUNC_STROBE
void strobe(uint8_t offset);
#endif

#ifdef FUNC_SOS
void sos(uint8_t offset);
#endif

#ifdef FUNC_ALPINE
void alpine(uint8_t offset);
#endif

#ifdef FUNC_FADE
void fade(uint8_t offset);
#endif

// useful macros
#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#define MAX(a, b) ((a) > (b)) ? (a) : (b)

/*
 * prototype for mode function pointers
 */
typedef void(*mode_func)(uint8_t);

/*
 * array holding pointers to the mode functions. Disabled mode functions are
 * replaced with a dummy function, so indices won't change.
 */
mode_func mode_func_arr[] =
{
        &const_level
#ifdef FUNC_STROBE
        , &strobe
#else
        , &nullmode
#endif
#ifdef FUNC_SOS
        , &sos
#else
        , &nullmode
#endif
#ifdef FUNC_ALPINE
        , &alpine
#else
        , &nullmode
#endif
#ifdef FUNC_FADE
        , &fade
#else
        , &nullmode
#endif
};


/*
 * enumeration of stages used in programming
 */
typedef enum
{
    prog_undef = 0,
    prog_init = 1,
    prog_1 = 2,
    prog_2 = 3,
    prog_3 = 4,
    prog_4 = 5,
    prog_nak = 6
} Prog_stage;

/*
 * enumeration of different tap types
 */
typedef enum
{
    tap_none = 0,
    tap_short,
    tap_long
} Tap_t;

/*
 * working copy of the first part of the eeprom, which will be loaded on
 * start up. If you change anything here, make sure to bring the EE_* #defines
 * up to date
 */
#define CLICK_CELLS 3       // spread wear of click detection over 3 cells
typedef struct
{
    uint8_t mode;           // index of regular mode slot last used
    uint8_t target_mode;    // index of slot selected for reprogramming
    uint8_t chosen_mode;    // index of chosen mode-line to be stored
    uint8_t prog_stage;     // keep track of programming stages
    uint8_t extended;       // whether to use normal or extended mode list
    uint8_t ext_mode;       // current mode-line in extended mode
    uint8_t clicks;         // number of consecutive taps so far
    uint8_t click_cell;     // index of currently used cell for click detection
    uint8_t last_click[CLICK_CELLS];  // type of last tap (short, long, none)
    uint8_t mode_arr[NUM_MODES]; // array holding offsets to the mode lines for
                                 // the configured modes
} State_t;



/*
 * The serious stuff begins below this line
 * =========================================================================
 */


/*
 * addresses of permanent memory variables in EEPROM. If you change anything
 * here, make sure to update struct State_t
 */
#define EE_MODE 0          	// index of regular mode slot last used
#define EE_TARGETMODE 1     // index of slot selected for reprogramming
#define EE_CHOSENMODE 2     // index of chosen mode-line to be store
#define EE_PROGSTAGE 3      // keep track of programming stages
#define EE_EXTENDED 4       // whether to use normal or extended mode list
#define EE_EXTMODE 5        // current mode-line in extended mode
#define EE_CLICKS 6         // number of consecutive taps so far
#define EE_CLICK_CELL 7     // index of current last click cell
#define EE_LAST_CLICK 8     // type of last tap (short, long, none), base of array
#define EE_MODES_BASE 11    // start of mode slot table (make sure to get the
                            // size of the last_click array right!)
#define EE_MODEDATA_BASE 16 // start of mode data array
                            // space for 4 bytes per mode, 10 lines
#define NUM_EXT_MODES 12    // number of mode data lines in EEPROM

#define EMPTY_MODE 255


/*
 * global variables
 */
uint8_t ticks;  // how many times the watchdog timer has been triggered
State_t state;  // struct holding a partial copy of the EEPROM
                // (should be volatile, but eats too much memory. After start
                // up it's only modified inside the ISR anyway)

volatile uint8_t gl_flags;  // global flags used for signalling certain events
#define GF_LOWBAT 0x01      // flag indicating low battery voltage


/*
 * The watchdog timer is called every 250ms.
 *
 * Function depends on the build configuration. If built with EEPROM base mode
 * switching (default), we keep track of whether the light has been turned on
 * for less than 2 seconds (8 ticks). After this time has elapsed, the
 * last_click cell is written over with tap_none. During mode programming, the
 * function will overwrite the last_click cell after one second with
 * tap_long. If PROGHELPER is defined, we also give hints on when to switch
 * off to follow the acknowledging sequence.
 *
 * If PINSWITCH is defined and the light is in programming state, this function
 * resets prog_stage after two seconds runtime, aborting the acknowledging
 * sequence.
 *
 * If built with battery monitoring, the battery level is checked four times
 * a second, after the light has been running for two seconds. This gives
 * the user a chance to get into a special mode, e.g. SOS or beacon, before
 * the battery protection kicks in and forces the light into the special
 * battery saving mode. The battery low flag is set when four consecutive
 * samplings have found the voltage to be below the given threshold.
 *
 * I know that this is a lot of stuff to put into an ISR, but there is nothing
 * else going on that might suffer from a long running interrupt routine. The
 * PWM runs independently, so constant light modes can not be effected in any
 * way. Strobe functions may be affected, but I guess nobody will notice if
 * a pulse gets delayed by a millisecond or two.
 *
 */
ISR(WDT_vect)
{
#ifdef MONITOR_BAT
    static uint8_t lbat_cnt = 0;
#endif

    if(ticks < 8){

#ifndef PINSWITCH
        uint8_t *click_cell;
        click_cell = (uint8_t *) EE_LAST_CLICK + state.click_cell;
#endif

        ++ticks;

        switch(ticks){
        /* With memory based mode switching, last_click is initialised to
         * tap_short in main() on startup. By the time we reach four ticks,
         * we change it to tap_long (more than a second). After eight ticks
         * (two seconds) we clear last_click.
         *
         * With pin state based switching, we just have to clear prog stage
         * after two seconds.
         */
        case 8:
#ifdef PINSWITCH
            // abort mode programming if light stays on for more than two seconds
            if(state.prog_stage != prog_undef)
                eeprom_write_byte((uint8_t *) EE_PROGSTAGE, prog_undef);
#else
            // two seconds elapsed, reset click type to tap_none
            eeprom_write_byte(click_cell, tap_none);
#endif
            break;

#if defined(PROGRAMMABLE) && !defined(PINSWITCH)
        case 4:
            eeprom_write_byte(click_cell, tap_long);

#ifdef PROGHELPER
            /* give hints on when to switch off in programming mode. Programming
             * stages 1,2 and 4 expect a short tap (0s - 1s), so we signal at
             * 250ms. Stage 3 expects a long tap (1s - 2s), so we signal at
             * 1s. Signalling is done by toggling the PWM-level's MSB for 100ms.
             */
            if(state.prog_stage == prog_3){
                PWM_LVL ^= (uint8_t) 0x80;
                _delay_ms(100);
                PWM_LVL ^= (uint8_t) 0x80;
            }
            break;
        case 1:
            if(state.prog_stage == prog_1
                    || state.prog_stage == prog_2
                    || state.prog_stage == prog_4)
            {
                PWM_LVL ^= (uint8_t) 0x80;
                _delay_ms(100);
                PWM_LVL ^= (uint8_t) 0x80;
            }
#endif      // PROGHELPER
            break;
#endif		// PROGRAMMABLE && !PINSWITCH
        default:
            break;
        }
    }
#ifdef MONITOR_BAT
    else{

        // check if there is a new battery voltage measurement available.
        if(ADCSRA & _BV(ADIF)){
            // if battery voltage is below threshold, increase counter, else
            // reset it.
            if(ADCH < LOWBAT_TRIG)
                ++lbat_cnt;
            else
                lbat_cnt = 0;
        }

        /* set the GF_LOWBAT flag if we have detected a low battery
         * voltage four times in a row. Reset lbat_cnt so we can trigger again.
         * Needed for the ramping down functionality.
         */
        if(lbat_cnt == 4){
            gl_flags |= GF_LOWBAT;
            lbat_cnt = 0;
        }

        // restart ADC
        ADCSRA |= _BV(ADSC);
    }
#endif
}

/*
 * set up the watchdog timer to trigger an interrupt every 250ms
 */
inline void start_wdt()
{
    uint8_t wdt_mode;

    /*
     * prepare new watchdog config beforehand, as it needs to be set within
     * four clock cycles after unlocking the WDTCR register.
     * Set interrupt mode prescaler bits.
     */
    wdt_mode = ((uint8_t) _BV(WDTIE)
                | (uint8_t) (WDTO_250MS & (uint8_t) 0x07)
                | (uint8_t) ((WDTO_250MS & (uint8_t) 0x08) << 2));

    cli();
    wdt_reset();

    // unlock register
    WDTCR = ((uint8_t) _BV(WDCE) | (uint8_t) _BV(WDE));

    // set new mode and prescaler
    WDTCR = wdt_mode;

    sei();
}

#ifdef MONITOR_BAT
/*
 * Configure the ADC to monitor the battery's voltage
 */
inline void start_adc()
{
    // internal 1.1V, left adjusted, use configured mux
    ADMUX = _BV(REFS0) | _BV(ADLAR) | (uint8_t) ADC_MUX;

    // make sure pin is not set to output
    DDRB &= (uint8_t) ~(_BV(ADC_DIDR));

    // disable digital input on ADC pin
    DIDR0 |= _BV(ADC_DIDR);

    // enable ADC, start conversion, set prescaler
    ADCSRA = _BV(ADEN) | _BV(ADSC) | (uint8_t) ADC_PRSCL;

}


/*
 * special mode to call when battery voltage is low.
 */
static inline void lowbat_mode()
{
    uint8_t pwm;

    // set PWM level to the configured low battery level, unless it is already
    // lower than that
#ifdef LOWBAT_RAMPDOWN
    if(PWM_LVL > 0){
        pwm = MIN(PWM_LVL, LOWBAT_MAX_LVL);
    }else{
        pwm = LOWBAT_MAX_LVL;

        // clear low battery flag, so detection can trigger again
        gl_flags &= ~GF_LOWBAT;
    }
#else
    if(PWM_LVL > 0){
        pwm = MIN(PWM_LVL, LOWBAT_MIN_LVL);
    }else{
        pwm = LOWBAT_MIN_LVL;
    }
#endif // LOWBAT_RAMPDOWN

    while(1){
        // give a short blink every five seconds
        PWM_LVL = pwm;
        _delay_ms(5000);
        PWM_LVL = 0;
        _delay_ms(100);
#ifdef LOWBAT_RAMPDOWN
        // if battery warning is triggered again, halve current PWM level and
        // clear GF_LOWBAT flag.
        // Do this on each triggering, until LOWBAT_MIN_LVL is reached
        if(gl_flags & GF_LOWBAT){
            pwm >>= 1;
            pwm = MAX(pwm, LOWBAT_MIN_LVL);

            gl_flags &= ~GF_LOWBAT;
        }
#endif // LOWBAT_RAMPDOWN
    }
}
#endif

/*
 * Delay for ms milliseconds
 * Calling the avr-libc _delay_ms() with a variable argument will pull in the
 * whole floating point handling code and increase flash image size by about
 * 3.5kB
 */
static void inline sleep_ms(uint16_t ms)
{
    while(ms >= 1){
        _delay_ms(1);
        --ms;
    }
}

/*
 * Delay for sec seconds
 */
static void inline sleep_sec(uint16_t sec)
{
    while(sec >= 1){
        _delay_ms(1000);
        --sec;
    }
}

/*
 * Constant light level
 * Set PWM to the level stored in the mode's first variable.
 */
void const_level(const uint8_t offset)
{

    PWM_LVL = eeprom_read_byte((uint8_t *) EE_MODEDATA_BASE + offset + 1);

    while(1){
#ifdef MONITOR_BAT
        if(gl_flags & GF_LOWBAT)
            break;
#endif
    }

}

#ifdef FUNC_STROBE
/*
 * Variable strobe mode
 * Strobe is defined by three variables:
 * Var 1: pulse width in ms, twice the width between pulses
 * Var 2: number of pulses in pulse group
 * Var 3: pause between pulse groups in seconds
 *
 * With this parameters quite a lot of modes can be realised. Permanent
 * strobing (no pause), police style (strobe group of 1s length, then 1s pause),
 * and beacons (single strobe in group, long pause)
 */
void strobe(uint8_t offset)
{
    uint8_t pulse, pulse_off, count, pause, i, pwm;

    pulse = eeprom_read_byte((uint8_t *) EE_MODEDATA_BASE + offset + 1);
    count = eeprom_read_byte((uint8_t *) EE_MODEDATA_BASE + offset + 2);
    pause = eeprom_read_byte((uint8_t *) EE_MODEDATA_BASE + offset + 3);

    pulse_off = (uint8_t) pulse << (uint8_t) 2; // pause between pulses,
                                                // 2 * pulse length
    pwm = 255;
    while(1){
        // pulse group
        for(i = 0; i < count; ++i){
            PWM_LVL = pwm;
            sleep_ms(pulse);
            PWM_LVL = 0;
            sleep_ms(pulse_off);
        }

        // pause between groups
        sleep_sec(pause);

#ifdef MONITOR_BAT
        // handle low battery. Keep signalling with reduced PWM level if we are
        // in some sort of beacon mode (pause >= 5 seconds, return otherwise.
        if(gl_flags & GF_LOWBAT){
           if(pause >= 5)
                pwm = 128;
            else
                break;
        }
#endif
    }
}
#endif

#ifdef FUNC_SOS
/*
 * well, morse SOS
 */
void sos(uint8_t offset)
{
    uint8_t pwm, i, j;

    pwm = 255;
    while(1){
        for(i = 0; i < 3; ++i){
            for(j = 0; j < 3; ++j){
                PWM_LVL = pwm;
                if(i == 1)
                    _delay_ms(600);
                else
                    _delay_ms(200);

                PWM_LVL = 0;
                _delay_ms(200);
            }
            _delay_ms(400);
        }

        _delay_ms(5000);

#ifdef MONITOR_BAT
        // reduce brightness when battery is low, but don't stop signalling
        if(gl_flags & GF_LOWBAT)
            pwm = 128;
#endif

    }
}
#endif

#ifdef FUNC_ALPINE
/*
 * Alpine distress signal.
 * Six blinks, ten seconds apart, then one minute pause and start over
 */
void alpine(uint8_t offset)
{
    uint8_t i, pwm;

    pwm = 255;
    while(1){
        for(i = 0; i < 6; ++i){
            PWM_LVL = pwm;
            _delay_ms(200);
            PWM_LVL = 0;
            sleep_sec(10);
        }

        sleep_sec(50);

#ifdef MONITOR_BAT
        // reduce brightness when battery is low, but don't stop signalling
        if(gl_flags & GF_LOWBAT)
            pwm = 128;
#endif

    }
}
#endif

#ifdef FUNC_FADE
/*
 * Fade in and out
 * This is more or less a demonstration with no real use I can see.
 * Mode uses three variables:
 * max: maximum level the light will rise to
 * rise: value the level is increased by during rising cycle
 * fall: value the level is decreased by during falling cycle
 *
 * Can produce triangle or saw tooth curves: /\/\... /|/|... |\|\...
 * As I said, nice toy with no real world use ;)
 */
void fade(uint8_t offset)
{
    uint8_t max, rise, fall, level, oldlevel;

    max = eeprom_read_byte((uint8_t *)EE_MODEDATA_BASE + offset + 1);
    rise = eeprom_read_byte((uint8_t *)EE_MODEDATA_BASE + offset + 2);
    fall = eeprom_read_byte((uint8_t *)EE_MODEDATA_BASE + offset + 3);

    level = 0;
    while(1){
        while(level < max){
            oldlevel = level;
            level += rise;

            if(oldlevel > level) // catch integer overflow
                level = max;

            PWM_LVL = level;
            sleep_ms(10);
        }

        while(level > 0){
            oldlevel = level;
            level -= fall;

            if(oldlevel < level) // catch integer underflow
                level = 0;

            PWM_LVL = level;
            sleep_ms(10);
        }

#ifdef MONITOR_BAT
        if(gl_flags & GF_LOWBAT)
            break;
#endif
    }
}
#endif


void nullmode(uint8_t offset)
{
    return;
}

#ifdef PROGRAMMABLE
static inline void do_program(const uint8_t last_click)
{
    /*
     * mode programming. We have the mode slot to be programmed saved in
     * state.target_mode, the mode to store in state.chosen_mode. User needs
     * to acknowledge by following a tapping pattern of short-short-long-short.
     */
    switch(state.prog_stage){
    case prog_init:
        state.prog_stage = prog_1;
        break;
    case prog_1:
    case prog_2:
        if(last_click == tap_short)
            ++state.prog_stage;
        else
            state.prog_stage = prog_nak;
        break;
    case prog_3:
        if(last_click == tap_long)
            ++state.prog_stage;
        else
            state.prog_stage = prog_nak;
        break;
    case prog_4:
        if(last_click == tap_short){
            // sequence completed, update mode_arr and eeprom
            state.mode_arr[state.target_mode] = state.chosen_mode;
            eeprom_write_byte((uint8_t *)EE_MODES_BASE + state.target_mode,
                    state.chosen_mode);
        }
        // fall through
    case prog_nak:
    default:
        // clean up when leaving programming mode
        state.prog_stage = prog_undef;
        state.target_mode = EMPTY_MODE;
        state.chosen_mode = EMPTY_MODE;
        break;
    }

    state.clicks = 0;
}
#endif

#ifdef PINSWITCH
/*
 * Mode switching / programming based on the state of an I/O pin.
 */
static inline uint8_t get_last_click()
{
    uint8_t last_click;

    // if the pin is still high (or low), the light has only been turned off
    // for a short time. We register this as a tap_short
#ifdef PS_HIGH
    if(PINB & _BV(PS_PIN))
#else
    if(!(PINB & _BV(PS_PIN)))
#endif
    {
        last_click = tap_short;
    }else{
        // if the light has been turned off for a longer time, we register this
        // as either a tap_long or a tap_none, depending on whether we are
        // in programming mode or not
        if(state.prog_stage != prog_undef)
            last_click = tap_long;
        else
            last_click = tap_none;
    }


#ifdef PS_CHARGE
    // charge or discharge a capacitor connected to the I/O pin by setting
    // it to high or low.
    DDRB |= _BV(PS_PIN);
#ifdef PS_HIGH
    PORTB |= _BV(PS_PIN);
#else
    PORTB &= (uint8_t) ~_BV(PS_PIN);
#endif // PS_HIGH
#endif // PS_CHARGE

    return last_click;
}
#else

/*
 * Mode switching based on previous run time.
 * On start up we write tap_short into a memory cell. After one second this cell
 * will be overwritten with tap_long by the timer ISR. After two seconds the
 * cell will be overwritten with tap_none.
 */
static inline uint8_t get_last_click()
{
    uint8_t last_click, next_cell;

    // make sure we cycle through the memory cells in the last_click array
    // to spread the wear on the eeprom a bit
    last_click = state.last_click[state.click_cell];

    // initialise click type for next start up
    next_cell = state.click_cell;
    ++next_cell;
    if(next_cell >= CLICK_CELLS)
        next_cell = 0;

    state.last_click[next_cell] = tap_short;
    state.click_cell = next_cell;

    return last_click;
}
#endif

int main(void)
{
    uint8_t offset, mode_idx = 0, func_idx, last_click;
#ifdef EXTENDED_MODES
    uint8_t signal = 0;
#endif

    // read the state data at the start of the eeprom into a struct
    eeprom_read_block(&state, 0, sizeof(State_t));

    last_click = get_last_click();

#ifdef EXTENDED_MODES
    // if we are in standard mode and got NUM_EXT_CLICKS in a row, change to
    // extended mode
    if(!state.extended && state.clicks >= NUM_EXT_CLICKS){
        state.extended = 1;
        state.ext_mode = EMPTY_MODE;
        state.prog_stage = prog_undef;
        state.clicks = 0;

        // delay signal until state is saved in eeprom
        signal = 1;
    }

    // handling of extended mode
    if(state.extended){

        // in extended mode, we can cycle through modes indefinitely until
        // one mode is held for more than two seconds
        if(last_click != tap_none){
            ++state.ext_mode;

            if(state.ext_mode >= NUM_EXT_MODES)
                state.ext_mode = 0;

            mode_idx = state.ext_mode;
        } else{
            // leave extended mode if previous mode was on longer than 2 seconds
            state.extended = 0;
            signal = 1; // wait with signal until eeprom is written

#ifdef PROGRAMMABLE
            // remember last mode and init programming
            state.chosen_mode = state.ext_mode;
            state.prog_stage = prog_init;
#endif
        }
    }

#ifdef PROGRAMMABLE
    if(state.prog_stage >= prog_init){
        do_program(last_click);
    }
#endif	// PROGRAMMABLE
#else   // EXTENDED_MODES
    state.extended = 0; // make sure we don't get stuck in extended modes
                        // if the EEPROM gets corrupted. (found by sixty545)
#endif	// EXTENDED_MODES

    // standard mode operation
    if(!state.extended){
        if(last_click != tap_none){
            // we're coming back from a tap, increment mode
            ++state.mode;

#ifdef EXTENDED_MODES
            // ...and count consecutive clicks
            ++state.clicks;
#endif

        }else{
            // start up from regular previous use (longer than 2 seconds)
#ifdef EXTENDED_MODES
#ifdef PROGRAMMABLE
            // remember current mode slot in case it is to be programmed
            if(state.prog_stage == prog_undef)
                state.target_mode = state.mode;
#endif
            // reset click counter
            state.clicks = 1;
#endif

#ifdef NOMEMORY
            // reset mode slot
            state.mode = 0;
#endif
        }

        if(state.mode >= NUM_MODES)
            state.mode = 0;

        // get index of mode stored in the current slot
        mode_idx = state.mode_arr[state.mode];
    }

    // write back state to eeprom but omit the mode configuration.
    // Minimises risk of corruption. Everything else will right itself
    // eventually, but modes will stay broken until reprogrammed.
    eeprom_write_block(&state, 0, sizeof(State_t) - sizeof(state.mode_arr));

    // set up PWM
    // set PWM pin to output
    DDRB |= _BV(PWM_PIN);
    // PORTB = 0x00; // initialised to 0 anyway

    // Initialise PWM on output pin and set level to zero
    TCCR0A = PWM_TCR;
    TCCR0B = PWM_SCL;
    // PWM_LVL = 0; // initialised to 0 anyway


#ifdef EXTENDED_MODES
    //give a short blink to indicate entering or leaving extended mode
    if(signal){
        // some people (including me) get twitchy during mode programming and
        // switch off at the first flash. Small delay to make sure the EEPROM
        // has settled down
        _delay_ms(50);

        // blink one time
        for(uint8_t i = 0; i < 2; ++i){
            PWM_LVL = ~PWM_LVL;
            _delay_ms(50);
        }
    }
#endif

    // sanity check in case of corrupted eeprom
    if(mode_idx >= NUM_EXT_MODES)
        mode_idx = 0;

    // fetch pointer to selected mode func from array
    offset = mode_idx << 2;
    func_idx = eeprom_read_byte((uint8_t *) EE_MODEDATA_BASE + offset);

#ifdef MONITOR_BAT
    // set up and start ADC
    start_adc();
#endif

    // start watchdog timer
    start_wdt();

    // call mode func
    (*mode_func_arr[func_idx])(offset);

    // if we reach this, the mode function has returned. This only happens if
    // the battery alarm has been triggered
#ifdef MONITOR_BAT
    lowbat_mode();
#else
    while(1)
        ;
#endif

    return 0; // Standard Return Code
}


