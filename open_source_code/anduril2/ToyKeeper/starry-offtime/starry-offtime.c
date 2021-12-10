/*
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

/*
 * =========================================================================
 * Settings to modify per driver
 */

//#define FAST 0x23           // fast PWM channel 1 only
//#define PHASE 0x21          // phase-correct PWM channel 1 only
#define FAST 0xA3             // fast PWM both channels
#define PHASE 0xA1            // phase-correct PWM both channels

#define VOLTAGE_MON         // Comment out to disable
#define OWN_DELAY           // Should we use the built-in delay or our own?
// Adjust the timing per-driver, since the hardware has high variance
// Higher values will run slower, lower values run faster.
#define DELAY_TWEAK         950

//#define TICKS_250MS       // If enabled, ticks are every 250 ms. If disabled, ticks are every 500 ms
                            // Affects turbo timeout/rampdown timing

#define OFFTIM3             // Use short/med/long off-time presses
                            // instead of just short/long

// PWM levels for the big circuit (FET or Nx7135)
#define MODESNx             0,0,0,70,255
// PWM levels for the small circuit (1x7135)
// (if the big circuit is a FET, use 0 for high modes here instead of 255)
#define MODES1x             3,15,128,255,255
#define MODES_PWM           PHASE,FAST,FAST,FAST,PHASE
// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,255 .
#define HIDDENMODES         BATTCHECK,STROBE,255
#define HIDDENMODES_PWM     PHASE,PHASE,PHASE

#define MODE_TURBO_LOW      140 // Level turbo ramps down to if turbo enabled
#define TURBO                   // comment out to disable turbo step-down
#define TURBO_TIMEOUT       240 // How many WTD ticks before before dropping down.  If ticks set for 500 ms, then 240 x .5 = 120 seconds.  Max value of 255 unless you change "ticks"
                                // variable to uint8_t
//#define TURBO_RAMP_DOWN           // By default we will start to gradually ramp down, once TURBO_TIMEOUT ticks are reached, 1 PWM_LVL each tick until reaching MODE_TURBO_LOW PWM_LVL
                                // If commented out, we will step down to MODE_TURBO_LOW once TURBO_TIMEOUT ticks are reached

// These values were measured using "Moonlight Special" driver from RMM.
// Your mileage may vary.
#define ADC_42          195 // the ADC value we expect for 4.20 volts
#define ADC_100         195 // the ADC value for 100% full (4.2V resting)
#define ADC_75          184 // the ADC value for 75% full (4.0V resting)
#define ADC_50          174 // the ADC value for 50% full (3.8V resting)
#define ADC_25          159 // the ADC value for 25% full (3.5V resting)
#define ADC_0           133 // the ADC value for 0% full (3.0V resting)
#define ADC_LOW         123 // When do we start ramping down (2.8V)
#define ADC_CRIT        118 // When do we shut the light off (2.7V)
// These were JonnyC's original values
//#define ADC_LOW             130     // When do we start ramping
//#define ADC_CRIT            120     // When do we shut the light off

#ifdef OFFTIM3
#define CAP_SHORT           190  // Value between 1 and 255 corresponding with cap voltage (0 - 1.1v) where we consider it a short press to move to the next mode
#define CAP_MED             100  // Value between 1 and 255 corresponding with cap voltage (0 - 1.1v) where we consider it a short press to move to the next mode
#else
#define CAP_SHORT           130  // Value between 1 and 255 corresponding with cap voltage (0 - 1.1v) where we consider it a short press to move to the next mode
                                 // Not sure the lowest you can go before getting bad readings, but with a value of 70 and a 1uF cap, it seemed to switch sometimes
                                 // even when waiting 10 seconds between presses.
#endif

// Comment these out to disable the mode and save space
#define STROBE    254       // Convenience code for strobe mode
#define BATTCHECK 253       // Convenience code for battery check mode

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
    while(n-- > 0) _delay_loop_2(DELAY_TWEAK);
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

#define STAR2_PIN   PB0
#define STAR3_PIN   PB4
#define CAP_PIN     PB3
#define CAP_CHANNEL 0x03    // MUX 03 corresponds with PB3 (Star 4)
#define CAP_DIDR    ADC3D   // Digital input disable bit corresponding with PB3
#define PWM_PIN     PB1
#define VOLTAGE_PIN PB2
#define ADC_CHANNEL 0x01    // MUX 01 corresponds with PB2
#define ADC_DIDR    ADC1D   // Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06    // clk/64

#define PWM_LVL     OCR0B   // OCR0B is the output compare register for PB1
#define ALT_PWM_LVL OCR0A   // OCR0A is the output compare register for PB0

/*
 * global variables
 */

// Mode storage
uint8_t eepos = 0;
uint8_t eep[32];
uint8_t memory = 0;

// Modes (gets set when the light starts up based on stars)
PROGMEM const uint8_t modesNx[] = { MODESNx , HIDDENMODES };
PROGMEM const uint8_t modes1x[] = { MODES1x , HIDDENMODES };
PROGMEM const uint8_t modes_pwm[] = { MODES_PWM , HIDDENMODES_PWM };
volatile uint8_t mode_idx = 0;
// NOTE: Only '1' is known to work; -1 will probably break and is untested.
// In other words, short press goes to the next (higher) mode,
// medium press goes to the previous (lower) mode.
#define mode_dir 1
uint8_t mode_cnt = sizeof(modesNx);

uint8_t lowbatt_cnt = 0;

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
    if (eepos < 32) mode_idx = eep[eepos];//&0x10; What the?
    else eepos=0;
}

inline void next_mode() {
    mode_idx += mode_dir;
    if (mode_idx >= mode_cnt) {
        // Wrap around
        mode_idx = 0;
    }
}

#ifdef OFFTIM3
inline void prev_mode() {
    if (mode_idx > 0) {
        // Regular mode: is between 1 and TOTAL_MODES
        mode_idx -= mode_dir;
    } else {
        // Otherwise, wrap around
        mode_idx = mode_cnt - 1;
    }
    /* For future use:
    // FIXME: use a different mechanism for hidden modes
    if ((0x40 > mode_idx) && (mode_idx > 0)) {
        // Regular mode: is between 1 and TOTAL_MODES
        mode_idx -= mode_dir;
    // FIXME: use a different mechanism for hidden modes
    } else if ((mode_idx&0x3f) < sizeof(neg_modes)) {
        // "Negative" mode (uses 0x40 bit to indicate "negative")
        mode_idx = (mode_idx|0x40) + mode_dir;
    } else {
        // Otherwise, always reset to first mode
        // (mode was too negative or otherwise out of range)
        mode_idx = 0;
    }
    */
}
#endif

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

inline void WDT_on() {
    // Setup watchdog timer to only interrupt, not reset
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    #ifdef TICKS_250MS
    WDTCR = (1<<WDTIE) | (1<<WDP2); // Enable interrupt every 250ms
    #else
    WDTCR = (1<<WDTIE) | (1<<WDP2) | (1<<WDP0); // Enable interrupt every 500ms
    #endif
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
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

inline void ADC_off() {
    ADCSRA &= ~(1<<7); //ADC off
}

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
    // Only set output for solid modes
    uint8_t out = pgm_read_byte(modesNx + mode);
    if ((out < 250) || (out == 255)) {
        set_output(pgm_read_byte(modesNx + mode), pgm_read_byte(modes1x + mode));
    }
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

uint8_t low_voltage(uint8_t voltage_val) {
    uint8_t voltage = get_voltage();
    // See if voltage is lower than what we were looking for
    if (voltage < voltage_val) {
        // See if it's been low for a while
        if (++lowbatt_cnt > 8) {
            lowbatt_cnt = 0;
            return 1;
        }
        _delay_ms(100);  // don't take a reading *too* often
    } else {
        lowbatt_cnt = 0;
    }
    return 0;
}
#endif

ISR(WDT_vect) {
    static uint8_t ticks = 0;
    if (ticks < 255) ticks++;
    // If you want more than 255 for longer turbo timeouts
    //static uint16_t ticks = 0;
    //if (ticks < 60000) ticks++;

#ifdef TURBO
    //if (ticks == TURBO_TIMEOUT && modes[mode_idx] == MODE_TURBO) { // Doesn't make any sense why this doesn't work
    if (ticks >= TURBO_TIMEOUT && mode_idx == (mode_cnt - 1) && PWM_LVL > MODE_TURBO_LOW) {
        #ifdef TURBO_RAMP_DOWN
        set_output(PWM_LVL - 1, PWM_LVL - 1);
        #else
        // Turbo mode is always at end
        set_output(MODE_TURBO_LOW, MODE_TURBO_LOW);
        /*
        if (MODE_TURBO_LOW <= modes[mode_idx-1]) {
            // Dropped at or below the previous mode, so set it to the stored mode
            // Kept this as it was the same functionality as before.  For the TURBO_RAMP_DOWN feature
            // it doesn't do this logic because I don't know what makes the most sense
            store_mode_idx(--mode_idx);
        }
        */
        #endif
    }
#endif

}

int main(void)
{
    // All ports default to input, but turn pull-up resistors on for the stars (not the ADC input!  Made that mistake already)
    // only one star, because one is used for PWM channel 2
    // and the other is used for the off-time capacitor
    PORTB = (1 << STAR3_PIN);

    // Determine what mode we should fire up
    // Read the last mode that was saved
    read_mode_idx();

    check_stars(); // Moving down here as it might take a bit for the pull-up to turn on?

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
    if (ADCH > CAP_SHORT) {
        // Indicates they did a short press, go to the next mode
        next_mode(); // Will handle wrap arounds
        store_mode_idx(mode_idx);
#ifdef OFFTIM3
    } else if (ADCH > CAP_MED) {
        // User did a medium press, go back one mode
        prev_mode(); // Will handle "negative" modes and wrap-arounds
        store_mode_idx(mode_idx);
#endif
    } else {
        // Didn't have a short press, keep the same mode
        // ... or reset to the first mode
        if (! memory) {
            // Reset to the first mode
            mode_idx = 0;
            store_mode_idx(mode_idx);
        }
    }
    // Turn off ADC
    ADC_off();

    // Charge up the capacitor by setting CAP_PIN to output
    DDRB  |= (1 << CAP_PIN);    // Output
    PORTB |= (1 << CAP_PIN);    // High

    // Set PWM pin to output
    DDRB |= (1 << PWM_PIN);     // enable main channel
    DDRB |= (1 << STAR2_PIN);   // enable second channel

    // Set timer to do PWM for correct output pin and set prescaler timing
    //TCCR0A = 0x23; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    //TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    ADC_off();
    #endif
    ACSR   |=  (1<<7); //AC off

    // Enable sleep mode set to Idle that will be triggered by the sleep_mode() command.
    // Will allow us to go idle between WDT interrupts
    //set_sleep_mode(SLEEP_MODE_IDLE);  // not used due to blinky modes

    WDT_on();

    // Now just fire up the mode
    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    set_mode(mode_idx);

    uint8_t output;
#ifdef VOLTAGE_MON
    uint8_t i = 0;
    uint8_t hold_pwm;
#ifdef BATTCHECK
    uint8_t voltage;
#endif
#endif
    while(1) {
        output = pgm_read_byte(modesNx + mode_idx);
        if (0) {}  // placeholder in case STROBE isn't defined
#ifdef STROBE
        else if (output == STROBE) {
            set_output(255,255);
            _delay_ms(50);
            set_output(0,0);
            _delay_ms(50);
        }
#endif
#ifdef BATTCHECK
        else if (output == BATTCHECK) {
            uint8_t blinks = 0;
            // turn off and wait one second before showing the value
            // (also, ensure voltage is measured while not under load)
            set_output(0,0);
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
                set_output(0,40);
                _delay_ms(100);
                set_output(0,0);
                _delay_ms(400);
            }

            _delay_ms(1000);  // wait at least 1 second between readouts
        }
#endif
    #ifdef VOLTAGE_MON
        if (low_voltage(ADC_LOW)) {
            // We need to go to a lower level
            if (mode_idx == 0 && ALT_PWM_LVL <= modes1x[mode_idx]) {
                // Can't go any lower than the lowest mode
                // Wait until we hit the critical level before flashing 10 times and turning off
                while (!low_voltage(ADC_CRIT));
                i = 0;
                while (i++<10) {
                    set_output(0,0);
                    _delay_ms(250);
                    set_mode(0);
                    _delay_ms(500);
                }
                // Turn off the light
                set_output(0,0);
                // Disable WDT so it doesn't wake us up
                WDT_off();
                // Power down as many components as possible
                set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                sleep_mode();
            } else {
                // Flash 3 times before lowering
                hold_pwm = ALT_PWM_LVL;
                i = 0;
                while (i++<3) {
                    set_output(0,0);
                    _delay_ms(250);
                    set_output(hold_pwm,hold_pwm);
                    _delay_ms(500);
                }
                // Lower the mode by half, but don't go below lowest level
                if ((ALT_PWM_LVL >> 1) < modes1x[0]) {
                    set_mode(0);
                    mode_idx = 0;
                } else {
                    set_output(0,ALT_PWM_LVL >> 1);
                }
                // See if we should change the current mode level if we've gone under the current mode.
                if (ALT_PWM_LVL < modes1x[mode_idx]) {
                    // Lower our recorded mode
                    mode_idx--;
                }
            }
            // Wait 3 seconds before lowering the level again
            _delay_ms(3000);
        }
    #endif
        //sleep_mode();  // incompatible with blinky modes
    }

    return 0; // Standard Return Code
}
