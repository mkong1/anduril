/*
 * This is intended for use on flashlights with a clicky switch and off-time
 * memory capacitor.  Ideally, a triple XP-L powered by a BLF17DD in a Sinner
 * Cypreus tri-EDC host.  It's mostly based on JonnyC's STAR firmware and
 * ToyKeeper's s7.c firmware.
 *
 * Original author: JonnyC
 * Modifications: ToyKeeper / Selene Scriven
 *
 * NANJG 105C Diagram
 *           ---
 *         -|1  8|- VCC
 * mem cap -|2  7|- Voltage ADC
 *  Star 3 -|3  6|- PWM
 *     GND -|4  5|- Star 2
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
#define F_CPU 4800000UL
#define FASTPWM 0x23
#define PHASEPWM 0x21

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define VOLTAGE_MON              // Comment out to disable all battery monitoring
#define OWN_DELAY                // Should we use the built-in delay or our own?
#define USE_PFM                  // Use PFM to make moon mode brighter
// Obsoleted; use moon_ceilings below instead
//#define MOON_PFM_LVL        30   // lower is brighter, 255 is max (same as no PFM)
// NOTE: WDT is required for on-time memory and WDT-based turbo step-down
// NOTE: WDT isn't tested, and probably doesn't work
//#define ENABLE_WDT               // comment out to turn off WDT and save space
#define NON_WDT_TURBO            // enable turbo step-down without WDT
#define TURBO_TIMEOUT       4000 // "ticks" until turbo step-down (~0.005s each)
                                 // (maximum is 65535, or 127 seconds)
                                 // Remove me: (maximum is 254, or 127 seconds)
#define TURBO_MODES         2    // Treat top N modes as "turbo" modes with timed step-down
                                 // (1 for just highest, 2 for two highest, etc)

// Set your PWM levels here (low to high, maximum 255)
// 1,8,39,120,255 for 5-step phase-correct PWM
#define MODE_MOON              0    // can use PFM to fine-tune brightness
#define MODE_LOW               1    // lowest normal mode, ~0.4% / ~12 lm
#define MODE_MED               8    // ~3.1% / ~94 lm
#define MODE_HIGH              42   // ~16.5% / ~494 lm
#define MODE_HIGHER            120  // ~47% / ~1411 lm
#define MODE_MAX               255  // direct drive, ~3000lm
// If you change these, you'll probably want to change the "modes" array below
// Each value must be cumulative, so include the value just above it.
// How many non-blinky modes will we have?
#define SOLID_MODES            6
// How many beacon modes will we have (with background light on)?
#define DUAL_BEACON_MODES      3+SOLID_MODES
// How many beacon modes will we have (without background light on)?
#define SINGLE_BEACON_MODES    1+DUAL_BEACON_MODES
// How many constant-speed strobe modes?
#define FIXED_STROBE_MODES     3+SINGLE_BEACON_MODES
// How many variable-speed strobe modes?
#define VARIABLE_STROBE_MODES  1+FIXED_STROBE_MODES
// battery check mode index
#define BATT_CHECK_MODE        1+VARIABLE_STROBE_MODES
// Note: don't use more than 32 modes,
// or it will interfere with the mechanism used for mode memory
#define TOTAL_MODES            BATT_CHECK_MODE

// NOTE: mode isn't saved this way, this value only applies to on-time memory
#ifdef ENABLE_WDT
#define WDT_TIMEOUT         2    // Number of WTD ticks before mode is saved (.5 sec each)
#endif

// These values were measured using a Ferrero Rocher F6DD driver and a DMM
// Your mileage may vary.  May be off by up to 0.1V or so on different hardware.
#define ADC_42          185 // the ADC value we expect for 4.20 volts
#define VOLTAGE_FULL    171 // 3.9 V, 4 blinks
#define VOLTAGE_GREEN   156 // 3.6 V, 3 blinks
#define VOLTAGE_YELLOW  141 // 3.3 V, 2 blinks
#define VOLTAGE_RED     126 // 3.0 V, 1 blink
#define ADC_LOW         125 // When do we start ramping down
#define ADC_CRIT        115 // When do we shut the light off
// these two are just for testing low-batt behavior w/ a CR123 cell
//#define ADC_LOW         139 // When do we start ramping down
//#define ADC_CRIT        138 // When do we shut the light off
// These were JonnyC's original values
//#define ADC_LOW             130     // When do we start ramping
//#define ADC_CRIT            120     // When do we shut the light off

// Values between 1 and 255 corresponding with cap voltage (0 - 1.1v) where we
// consider it a short press to move to the next mode.
// Not sure the lowest you can go before getting bad readings, but with a value
// of 70 and a 1uF cap, it seemed to switch sometimes even when waiting 10
// seconds between presses.
#define CAP_SHORT       190 // Above this is a "short" press
#define CAP_MED         100  // Above this is a "medium" press
                            // ... and anything below that is a "long" press
#define CAP_PIN         PB3
#define CAP_CHANNEL     0x03   // MUX 03 corresponds with PB3 (Star 4)
#define CAP_DIDR        ADC3D  // Digital input disable bit corresponding with PB3


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
    if (n==0) { _delay_loop_2(400); }
    else {
        while(n-- > 0)
            _delay_loop_2(890);
    }
}
#else
#include <util/delay.h>
#endif

#include <avr/interrupt.h>
#ifdef ENABLE_WDT
#include <avr/wdt.h>
#endif
#include <avr/pgmspace.h>
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
#ifdef USE_PFM
#define CEIL_LVL    OCR0A   // OCR0A is the number of "frames" per PWM loop
#endif

/*
 * global variables
 */

// Mode storage
uint8_t eepos = 0;
uint8_t eep[32];
// change to 1 if you want on-time mode memory instead of "short-cycle" memory
// NOTE: Not currently implemented; leave it at 0.
#define memory 0

// Modes (hardcoded to save space, const also to save space)
const uint8_t modes[] = {
    // regular solid modes
    MODE_MOON, MODE_LOW, MODE_MED, MODE_HIGH, MODE_HIGHER, MODE_MAX,
    // dual beacon modes (this level and this level + 2)
    MODE_MOON, MODE_LOW, MODE_MED,
    // heartbeat beacon
    MODE_MAX,
    // constant-speed strobe modes (12.5 Hz, 24 Hz, 60 Hz)
    // these values represent delay in ms between 1ms flashes
    79, 41, 15,
    // variable-speed strobe mode
    MODE_MAX,
    // battery check mode
    MODE_MED,
};
// Semi-hidden modes, only accessible via a "backward" press from first mode.
// Each value is an index into the modes[] array.
const uint8_t neg_modes[] = {
    SOLID_MODES-1,           // Turbo / "impress" mode
    FIXED_STROBE_MODES-2,    // 24Hz strobe
    BATT_CHECK_MODE-1,       // Battery check
};
PROGMEM const uint8_t moon_ceilings[] = {
    (VOLTAGE_FULL+ADC_42)/2, 160,         // > 4.05V
    VOLTAGE_FULL, 60,                     // > 3.9V
    (VOLTAGE_YELLOW+VOLTAGE_FULL)/2, 30,  // > 3.75V
    VOLTAGE_YELLOW, 5,                    // > 3.6V
    0, 2,                                 // < 3.6V
};
PROGMEM const uint8_t voltage_blinks[] = {
    VOLTAGE_RED,    // 1 blink
    VOLTAGE_YELLOW, // 2 blinks
    VOLTAGE_GREEN,  // 3 blinks
    VOLTAGE_FULL,   // 4 blinks
    ADC_42,         // 5 blinks
};
volatile uint8_t mode_idx = 0;
// 1 or -1. Do we increase or decrease the idx when moving up to a higher mode?
// Was set by checking stars in the original STAR firmware, but that's removed
// to save space.
// NOTE: Only '1' is known to work; -1 will probably break and is untested.
#define mode_dir 1

void store_mode_idx() {  //central method for writing (with wear leveling)
    uint8_t oldpos=eepos;
    eepos=(eepos+1)&31;  //wear leveling, use next cell
    // Write the current mode
    EEARL=eepos;  EEDR=mode_idx; EECR=32+4; EECR=32+4+2;  //WRITE  //32:write only (no erase)  4:enable  2:go
    while(EECR & 2); //wait for completion
    // Erase the last mode
    EEARL=oldpos;           EECR=16+4; EECR=16+4+2;  //ERASE  //16:erase only (no write)  4:enable  2:go
}
inline void read_mode_idx() {
    eeprom_read_block(&eep, 0, 32);
    while((eep[eepos] == 0xff) && (eepos < 32)) eepos++;
    if (eepos < 32) mode_idx = eep[eepos];
    else eepos=0;
}

inline void next_mode() {
    mode_idx += mode_dir;
    if (mode_idx > (TOTAL_MODES - 1)) {
        // Wrap around
        mode_idx = 0;
    }
}

inline void prev_mode() {
    if ((0x40 > mode_idx) && (mode_idx > 0)) {
        // Regular mode: is between 1 and TOTAL_MODES
        mode_idx -= mode_dir;
    } else if ((mode_idx&0x3f) < sizeof(neg_modes)) {
        // "Negative" mode (uses 0x40 bit to indicate "negative")
        mode_idx = (mode_idx|0x40) + mode_dir;
        // FIXME: should maybe change mode group instead?
    } else {
        // Otherwise, always reset to first mode
        // (mode was too negative or otherwise out of range)
        mode_idx = 0;
    }
}

#ifdef ENABLE_WDT
inline void WDT_on() {
    // Setup watchdog timer to only interrupt, not reset, every 500ms.
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    WDTCR = (1<<WDTIE) | (1<<WDP2) | (1<<WDP0); // Enable interrupt every 500ms
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
#endif

inline void ADC_on() {
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
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
    // Return the raw value, caller can decide what to do with it
    return ADCH;
}
#endif

#ifdef ENABLE_WDT
ISR(WDT_vect) {
    // Even if this function does nothing,
    // having the WDT enabled will at least wake up the main loop
    // and force it to do voltage monitoring plus optional step-down
    // (then again, doing a _delay_ms() instead of sleep() in the main loop
    //  can achieve the same purpose, and makes fast PWM=0 moon mode work)
    static uint8_t ticks = 0;
    if (ticks < 255) ticks++;

    // do a turbo step-down (NOTE: untested)
    if ((mode_idx == SOLID_MODES-TURBO_MODES) && (ticks > TURBO_TIMEOUT/100)) {
        mode_idx --;
        ticks = 0;
    }
    /*
    // FIXME: this is short-cycle memory, remove it
    // (we use off-time memory instead)
    if (ticks == WDT_TIMEOUT) {
#if memory
        store_mode_idx();
#else
        // Reset the mode to the start for next time
        uint8_t foo = mode_idx;
        mode_idx = 0;
        store_mode_idx();
        mode_idx = foo;
#endif
    }
    */
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
    // defaulting to PHASE allows PWM=0 to mean "off"
    // will override later for solid modes where PWM=0 means "moon"
    TCCR0A = PHASEPWM;
#ifdef USE_PFM
    // 0x08 is for variable-speed PWM
    TCCR0B = 0x08 | 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
    CEIL_LVL = 255; // default
#else
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
#endif

    // Determine what mode we should fire up
    // Read the last mode that was saved
    read_mode_idx();

    // Start up ADC for capacitor pin
    // disable digital input on ADC pin to reduce power consumption
    DIDR0 |= (1 << CAP_DIDR);
    // 1.1v reference, left-adjust, ADC3/PB3
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | CAP_CHANNEL;
    // enable, start, prescale
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;

    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    // Start again as datasheet says first result is unreliable
    ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    if (ADCH > CAP_SHORT) {
        // Indicates they did a short press, go to the next mode
        next_mode(); // Will handle wrap arounds
        store_mode_idx();
    } else if (ADCH > CAP_MED) {
        // User did a medium press, go back one mode
        prev_mode(); // Will handle "negative" modes and wrap-arounds
        store_mode_idx();
    } else {
        // Long press
#if memory
        // Keep the same mode
#else
        // Reset to the first mode
        mode_idx = 0;
        store_mode_idx();
#endif
    }
    // Turn off ADC
    ADC_off();

    // Charge up the capacitor by setting CAP_PIN to output
    DDRB  |= (1 << CAP_PIN);  // Output
    PORTB |= (1 << CAP_PIN);  // High

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    //ADC_off();  // was already off
    #endif
    ACSR  |=  (1<<7); //AC off

#ifdef ENABLE_WDT
    // Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
    // Will allow us to go idle between WDT interrupts
    // (not necessary if we're staying awake during all modes, such as to
    //  enable fast PWM=0 for moon mode)
    set_sleep_mode(SLEEP_MODE_IDLE);

    // enable turbo step-down timer, if there is one
    // also makes voltage monitor work, by interrupting sleep
    WDT_on();
#endif

    // Convert the "negative" mode into its actual (positive) mode
    //if (mode_idx < 0) {
    // The 0x40 bit is used, because I had issues getting eeprom to store
    // and retrieve negative values on a signed integer.  So, I'm setting
    // a "negative" bit manually.
    if (mode_idx & 0x40) {
        mode_idx = neg_modes[(mode_idx&0x3f)-1];
    }

    // Hey look, variable declarations after executable code
    // (we must not be using a strict or ancient C compiler)
    uint8_t i = 0;
    uint8_t j = 0;  // only used for strobes
#ifdef NON_WDT_TURBO
    uint16_t ontime_ticks = 0;
#endif
    uint8_t strobe_len = 0;
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
    uint8_t voltage;
    // Make sure voltage reading is running for later
    //ADCSRA |= (1 << ADSC);
    // ... and prime the battery check for more accurate first reading
    voltage = get_voltage();
#endif

    while(1) {
        if(mode_idx < SOLID_MODES) { // Just stay on at a given brightness
            TCCR0A = FASTPWM;
            PWM_LVL = modes[mode_idx];
#ifdef USE_PFM
            if (mode_idx == 0) {
                //CEIL_LVL = MOON_PFM_LVL;
                voltage = get_voltage();
                for (i=0; i<sizeof(moon_ceilings); i+=2) {
                    if(voltage > pgm_read_byte(moon_ceilings + i)) {
                        //CEIL_LVL = pgm_read_byte(moon_ceilings + i+1);
                        //CEIL_LVL = (CEIL_LVL + pgm_read_byte(moon_ceilings + i+1)) >> 2;
                        j = CEIL_LVL;
                        if (j < pgm_read_byte(moon_ceilings + i+1)) {
                            CEIL_LVL = j + 1;
                        } else {
                            CEIL_LVL = j - 1;
                        }
                        break;
                    }
                }
            } /* else {  // was already set to 255
                CEIL_LVL = 255;
            } */
#endif
            /*
            if (modes[mode_idx] < 3) {
                // use phase-correct for really low modes
                TCCR0A = 0x21; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
            }
            */
#ifdef ENABLE_WDT
            // Saves a little power, but makes fast PWM=0 moon mode not work
            sleep_mode();
#else
            _delay_ms(5); // can't sleep, fast PWM=0 will eat me
#ifdef NON_WDT_TURBO
            if (ontime_ticks < 65535) { ontime_ticks ++; }
            if ((mode_idx >= SOLID_MODES-TURBO_MODES)
                    && (ontime_ticks > TURBO_TIMEOUT)) {
                // step down one level
                mode_idx -= 1;
                // reset in case there's more than one level of turbo
                ontime_ticks = 0;
                // save, so we can short-press to go back up
                store_mode_idx();
            }
#endif
#endif
        } else if (mode_idx < DUAL_BEACON_MODES) { // two-level fast strobe pulse at about 1 Hz
            TCCR0A = FASTPWM;
            for(i=0; i<4; i++) {
                PWM_LVL = modes[mode_idx-SOLID_MODES+2];
                _delay_ms(5);
                PWM_LVL = modes[mode_idx];
                _delay_ms(65);
            }
            _delay_ms(720);
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
#if 0
            // bigger, better-looking version
            j = modes[mode_idx]; // look up only once, saves a few bytes
            strobe_len = 1;
            if (j < 50) { strobe_len = 0; }
            // loop to make timing more consistent
            // (voltage check messes with timing)
            for(i=0; i<30; i++) {
                PWM_LVL = modes[SOLID_MODES-1];
                _delay_ms(strobe_len);
                PWM_LVL = 0;
                _delay_ms(j);
            }
#else
            // minimal version to save space
            PWM_LVL = modes[SOLID_MODES-1];
            _delay_ms(0);
            PWM_LVL = 0;
            _delay_ms(modes[mode_idx]);
#endif
        } else if (mode_idx == VARIABLE_STROBE_MODES-1) {
            // strobe mode, smoothly oscillating frequency ~10 Hz to ~24 Hz
            for(j=0; j<60; j++) {
                PWM_LVL = modes[SOLID_MODES-1];
                _delay_ms(1);
                PWM_LVL = 0;
                if (j<30) { strobe_len = j; }
                else { strobe_len = 60-j; }
                _delay_ms(2*(strobe_len+20));
            }
        } else if (mode_idx < BATT_CHECK_MODE) {
            uint8_t blinks = 0;
            //PWM_LVL = MODE_MED;  // brief flash at start of measurement
            voltage = get_voltage();
            // turn off and wait one second before showing the value
            // (or not, uses extra space)
            //PWM_LVL = 0;
            //_delay_ms(1000);

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
            _delay_ms(2000);  // wait at least 2 seconds between readouts
        }
#ifdef VOLTAGE_MON
        if (ADCSRA & (1 << ADIF)) {  // if a voltage reading is ready
            voltage = ADCH; // get_voltage();
            // See if voltage is lower than what we were looking for
            if (voltage < ((mode_idx <= 1) ? ADC_CRIT : ADC_LOW)) {
                ++lowbatt_cnt;
            } else {
                lowbatt_cnt = 0;
            }
            // See if it's been low for a while, and maybe step down
            if (lowbatt_cnt >= 3) {
                if (mode_idx >= SOLID_MODES) {
                    // step down from blinky modes to medium
                    mode_idx = 2;
                } else if (mode_idx > 1) {
                    // step down from solid modes one at a time
                    // (ignore mode 0 because it's probably invisible anyway
                    //  if the voltage is this low)
                    mode_idx -= 1;
                } else { // Already at the lowest mode
                    mode_idx = 0;
                    // Turn off the light
                    PWM_LVL = 0;
#ifdef ENABLE_WDT
                    // Disable WDT so it doesn't wake us up
                    WDT_off();
#endif
                    // Power down as many components as possible
                    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                    sleep_mode();
                }
                store_mode_idx();
                lowbatt_cnt = 0;
                // Wait at least 2 seconds before lowering the level again
                _delay_ms(2000);  // this will interrupt blinky modes
            }

            // Make sure conversion is running for next time through
            ADCSRA |= (1 << ADSC);
        }
#endif
    }
}
