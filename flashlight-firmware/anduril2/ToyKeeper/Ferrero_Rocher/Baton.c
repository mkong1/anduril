/* Firmware for Ferrero Rocher driver
 * and other attiny13a-based e-switch lights.
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
 * ATTINY13A Diagram
 *            ----
 *          -|1  8|- VCC
 * E-switch -|2  7|- Voltage ADC
 *  Red LED -|3  6|- PWM
 *      GND -|4  5|- Green LED
 *            ----
 */

#define F_CPU 4800000UL

// PWM Mode
#define PHASE 0b00000001
#define FAST  0b00000011
#define PWM_MODE   FAST   // PWM mode/speed: PHASE (9 kHz) or FAST (18 kHz)
                          // (FAST has side effects when PWM=0, can't
                          //  shut off light without putting the MCU to sleep)
                          // (PHASE might make audible whining sounds)
// PFM not supported in this firmware, don't uncomment
//#define USE_PFM           // comment out to disable pulse frequency modulation
                          // (makes bottom few modes ramp smoother)

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define BLINK_ON_POWER      // blink once when power is received
                            // (helpful on e-switch lights, annoying on dual-switch lights)
#define VOLTAGE_MON         // Comment out to disable all voltage-related functions:
                            // (including ramp down and eventual shutoff when battery is low)
#define REDGREEN_INDICATORS // comment out to disable red/green power indicator LEDs
#define LOWPASS_VOLTAGE     // Average the last 4 voltage readings for smoother results
                            // (comment out to use only one value, saves space)

// Switch handling
#define LONG_PRESS_DUR   21 // How many WDT ticks until we consider a press a long press
                            // 32 is roughly .5 s, 21 is roughly 1/3rd second
#define TICKS_PER_RAMP   32 // How many WDT ticks per step in the ramp (lower == faster ramp)
#define LOCK_PRESS_DUR  188 // Hold for 3s from off to (un)lock light.
#define RAMP_TIMEOUT     64 // un-reverse ramp dir about 1s after button release


// Must be low to high, starting with 0
// (and the lowest values are highly device-dependent)
#define MODES           0,1,5,23,65,139,255

#define TURBO           // Comment out to disable - full output with a step down after n number of seconds
                        // If turbo is enabled, it will be where 255 is listed in the modes above
#define TURBO_TIMEOUT   5625 // How many WTD ticks before before dropping down (.016 sec each)
                        // 90  = 5625
                        // 120 = 7500

// NOTE: Voltage values are calibrated for the Ferrero Rocher F6-DD driver
// (may need different values for nanjg/qlite drivers, but only a little different)
#define ADC_42          184 // the ADC value we expect for 4.20 volts
#define ADC_100         184 // the ADC value for 100% full (4.2V resting)
#define ADC_75          175 // the ADC value for 75% full (4.0V resting)
#define ADC_50          165 // the ADC value for 50% full (3.8V resting)
#define ADC_25          151 // the ADC value for 25% full (3.5V resting)
#define ADC_0           128 // the ADC value for 0% full (3.0V resting)
#define VOLTAGE_FULL    170 // 3.9 V under load
#define VOLTAGE_GREEN   156 // 3.6 V under load
#define VOLTAGE_YELLOW  142 // 3.3 V under load
#define VOLTAGE_RED     128 // 3.0 V under load
#define ADC_LOW         124 // When do we start ramping down (2.9V)
#define ADC_CRIT        114 // When do we shut the light off (2.7V)

// these two are just for testing low-batt behavior w/ a CR123 cell
//#define ADC_LOW         139 // When do we start ramping down
//#define ADC_CRIT        138 // When do we shut the light off
#define ADC_DELAY       188 // Delay in ticks between low-bat rampdowns (188 ~= 3s)
#define OWN_DELAY       // replace default _delay_ms() with ours, comment to disable


/*
 * =========================================================================
 */

#ifdef OWN_DELAY
#include <util/delay_basic.h>
// Having own _delay_ms() saves some bytes AND adds possibility to use variables as input
static void _delay_ms(uint16_t n)
{
    while(n-- > 0)
        _delay_loop_2(950);
}
#else
#include <util/delay.h>
#endif

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>

#define SWITCH_PIN  PB3     // what pin the switch is connected to, which is Star 4
#define PWM_PIN     PB1
#define VOLTAGE_PIN PB2
#define RED_PIN     PB4     // pin 3
#define GREEN_PIN   PB0     // pin 5
#define ADC_CHANNEL 0x01    // MUX 01 corresponds with PB2
#define ADC_DIDR    ADC1D   // Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06    // clk/64

#define PWM_LVL     OCR0B   // OCR0B is the output compare register for PB1
#ifdef USE_PFM
#define CEIL_LVL    OCR0A   // OCR0A is the number of "frames" per PWM loop
#endif

#define DB_REL_DUR  0b00001111 // time before we consider the switch released after
                               // each bit of 1 from the right equals 16ms, so 0x0f = 64ms

/*
 * The actual program
 * =========================================================================
 */

/*
 * global variables
 */
PROGMEM const uint8_t modes[]     = { MODES };
volatile int8_t mode_idx = 0;
// FIXME: ramp_dir should be a define
volatile int8_t ramp_dir = 1;
volatile uint8_t locked = 0;
uint8_t press_duration  = 0;
#ifdef LOWPASS_VOLTAGE
uint8_t voltages[] = { VOLTAGE_FULL, VOLTAGE_FULL, VOLTAGE_FULL, VOLTAGE_FULL };
#endif

// Debounced switch press value
int is_pressed()
{
    // Keep track of last switch values polled
    static uint8_t buffer = 0x00;
    // Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
    buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);
    return (buffer & DB_REL_DUR);
}

void ramp() {
    if (locked) { return; }
    mode_idx += ramp_dir;
    // wrap around (can probably remove this)
    if (mode_idx <= 0) {
        mode_idx = sizeof(modes)-1;
    }
    // wrap around, skip moon mode
    if (mode_idx > sizeof(modes)-1) {
        mode_idx = 2;
    }
}

void prev_mode() {
    // only used for turbo step-down and low-voltage step-down...
    // if we were already at the lowest mode, shut off.
    if (mode_idx >= 1) { mode_idx -= 1; }
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
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    WDTCR = (1<<WDTIE);             // Enable interrupt every 16ms
    sei();                          // Enable interrupts
}

inline void WDT_off()
{
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    MCUSR &= ~(1<<WDRF);            // Clear Watchdog reset flag
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    WDTCR = 0x00;                   // Disable WDT
    sei();                          // Enable interrupts
}

inline void ADC_on() {
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

inline void ADC_off() {
    ADCSRA &= ~(1<<7); //ADC off
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
    // turn red+green LEDs off
    DDRB = (1 << PWM_PIN); // note the lack of red/green pins here
    // with this commented out, the LEDs dim instead of turning off entirely
    //PORTB &= 0xff ^ ((1 << RED_PIN) | (1 << GREEN_PIN));  // red+green off
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

#ifdef TURBO
    static uint16_t turbo_ticks = 0;
#endif
    static uint8_t  ontime_ticks = 0;
    static uint8_t  doubleclick_ticks = 0;
    static int8_t   saved_mode_idx = 2; // start at first non-moon mode
    uint8_t         i = 0;
#ifdef VOLTAGE_MON
    static uint8_t  lowbatt_cnt = 0;
#ifdef LOWPASS_VOLTAGE
    uint16_t        voltage = 0;
#else
    uint8_t         voltage = 0;
#endif
#endif

    if (mode_idx == 0) {
        ontime_ticks = 0;
        doubleclick_ticks = 0;
    }

    if (is_pressed()) {
        ontime_ticks = 0;  // Reset ontime on button press
#ifdef TURBO
        // Just always reset turbo timer whenever the button is pressed
        turbo_ticks = 0;
#endif
#ifdef VOLTAGE_MON
        // Same with the ramp down delay
        lowbatt_cnt = 0;
#endif

        if (press_duration < 255) {
            press_duration++;
        }

        // Long press  (trigger every TICKS_PER_RAMP time slices)
        if (press_duration == (LONG_PRESS_DUR+1)) {
            // Long press
            // never trigger double-click action after a long press
            doubleclick_ticks = 255;
            // BTW, long-press from off is a shortcut to minimum
            //if (mode_idx == 0) { ramp_dir = 1; }
            // Ensure long-press-from-off won't ramp
            if (mode_idx == 0) { press_duration = LONG_PRESS_DUR+TICKS_PER_RAMP+1; }
            ramp();
        }
        else if (press_duration == LOCK_PRESS_DUR) {
            locked ^= 1;
            if (locked)
            {
                mode_idx = 0;
            }
            else
            {
                mode_idx = saved_mode_idx;
            }
        }
        // let the user keep holding the button to keep cycling through modes
        else if (press_duration == LONG_PRESS_DUR+TICKS_PER_RAMP) {
            press_duration = LONG_PRESS_DUR;
        }
    } else {
        // Not pressed
        if (locked) {  // don't do anything if the lock-out is active
            ontime_ticks = 0;
            press_duration = 0;
            return;
        }
        if (mode_idx != 0) {
            if (ontime_ticks < 255) {
                ontime_ticks ++;
            }
            if (doubleclick_ticks < 255) {
                doubleclick_ticks ++;
            }
        }
        if (press_duration > 0 && press_duration < LONG_PRESS_DUR) {
            // Short press
            if ( mode_idx == 0 ) {
                // short press from off == restore saved mode
                mode_idx = saved_mode_idx;
                ontime_ticks = 255;  // avoid the double-reverse on power-on
                doubleclick_ticks = 0;  // so we can detect double clicks from off
            } else {
                if ((doubleclick_ticks < LONG_PRESS_DUR) && (mode_idx < sizeof(modes)-1)) {
                    // double click from off is a shortcut to maximum
                    // (or...  triple-click while on also works)
                    mode_idx = sizeof(modes) - 1;
                } else {
                    // single click while on will turn the light off
                    // remember the last-used mode
                    saved_mode_idx = mode_idx;
                    mode_idx = 0;
                }
            }
        } else if ( press_duration > 0 ) { // long press was just released
            // FIXME: omit this, no longer used
        } else {
            if (ontime_ticks == RAMP_TIMEOUT) {
                // FIXME: omit this, no longer used
            }
#ifdef TURBO
            // Only do turbo check when switch isn't pressed
            //if (modes[mode_idx] == 255) { // takes more space
            if (mode_idx == sizeof(modes)-1) {
                turbo_ticks++;
                if (turbo_ticks > TURBO_TIMEOUT) {
                    // Go to the previous mode
                    prev_mode();
                }
            }
#endif
            // Only do voltage monitoring when the switch isn't pressed
#ifdef VOLTAGE_MON
            // See if conversion is done
            if (ADCSRA & (1 << ADIF)) {
#ifdef LOWPASS_VOLTAGE
                // Get an average of the past few readings
                for (i=0;i<3;i++) {
                    voltages[i] = voltages[i+1];
                }
                voltages[3] = ADCH;
                voltage = (voltages[0]+voltages[1]+voltages[2]+voltages[3]) >> 2;
#else
                voltage = ADCH;
#endif
                // See if voltage is lower than what we were looking for
                if (voltage < ((mode_idx == 1) ? ADC_CRIT : ADC_LOW)) {
                    ++lowbatt_cnt;
                } else {
                    lowbatt_cnt = 0;
                }
#ifdef REDGREEN_INDICATORS
                if (voltage > VOLTAGE_GREEN) {
                    // turn on green LED
                    DDRB = (1 << PWM_PIN) | (1 << GREEN_PIN);
                    PORTB |= (1 << GREEN_PIN);
                    PORTB &= 0xff ^ (1 << RED_PIN);  // red off
                }
                else if (voltage > VOLTAGE_YELLOW) {
                    // turn on red+green LED (yellow)
                    DDRB = (1 << PWM_PIN) | (1 << GREEN_PIN) | (1 << RED_PIN); // bright green + bright red
                    //DDRB = (1 << PWM_PIN) | (1 << GREEN_PIN); // bright green + dim red
                    PORTB |= (1 << GREEN_PIN) | (1 << RED_PIN);
                }
                else {
                    // turn on red LED
                    DDRB = (1 << PWM_PIN) | (1 << RED_PIN);
                    PORTB |= (1 << RED_PIN);
                    PORTB &= 0xff ^ (1 << GREEN_PIN);  // green off
                }
#endif
            }

            // See if it's been low for a while, and maybe step down
            if (lowbatt_cnt >= ADC_DELAY) {
                prev_mode();
                lowbatt_cnt = 0;
            }

            // Make sure conversion is running for next time through
            ADCSRA |= (1 << ADSC);
#endif
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
    DDRB = (1 << PWM_PIN);

    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0A = 0x20 | PWM_MODE; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    #ifdef USE_PFM
    // 0x08 is for variable-speed PWM
    TCCR0B = 0x08 | 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
    #else
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
    #endif

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    ADC_off();
    #endif
    ACSR   |=  (1<<7); //AC off

    #ifdef BLINK_ON_POWER
    // blink once to let the user know we have power
    //TCCR0A = PHASE | 0b00100000;  // Only use the normal output
    #ifdef USE_PFM
    CEIL_LVL = 255;
    #endif
    PWM_LVL = 255;
    _delay_ms(3);
    PWM_LVL = 0;
    _delay_ms(1);
    #endif

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
            PWM_LVL     = pgm_read_byte(modes + mode_idx);
            last_mode_idx = mode_idx;
            _delay_ms(1);
            if (mode_idx == 0) {
                // Finish executing instructions for PWM level change
                // and/or voltage readout mode before shutdown.
                do {
                    _delay_ms(1);
                } while (0); // FIXME: stay on for a while to catch doubleclicks
                // Go to sleep
                sleep_until_switch_press();
                // FIXME: lock mode can result in fast PWM=0 staying lit
                // (MCU wakes up and stays awake, should go back to sleep)
            }
        }
    }

    return 0; // Standard Return Code
}
