/*
 * dth Momentary version 1.1
 *
 * Created: 1/11/2015 7:15:48 PM
 *  Author: Dzung Hoang
 *
 * Firmware for NANJG 105C-based LED drivers using ATTINY13A MCU
 * using momentary switch for mode change.
 *
 * Portions of the code and the comment section below are based upon
 * STAR_momentary version 1.6.
 *
 * Flashlight Usage:
 *
 * From OFF:
 *   Short press turns ON to last mode.
 *   Long press turns ON to moon.
 *   Double press turn ON to turbo.
 *
 * From ON:
 *   Short press turns OFF.
 *   Long press cycles to next mode. Keep pressing to continue cycling.
 *   Double press cycles to previous mode. Keep pressing to continue cycling.
 *
 * Notes:
 *   There is no previous mode from moon. This is to prevent jarring transition from moon to turbo.
 */


/*
 * Changelog
 *
 * 1.0 Initial version
 * 1.1 Modified UI, added blink-on-power option from ToyKeeper, optimizations
 */


/*
 * NANJG 105C Diagram
 *           ---
 *         -|   |- VCC
 *  Star 4 -|   |- Voltage ADC
 *  Star 3 -|   |- PWM
 *         GND -|   |- Star 2
 *           ---
 *
 * FUSES
 *            I use these fuse settings
 *            Low:  0x75        (4.8MHz CPU without 8x divider, 9.4kHz phase-correct PWM or 18.75kHz fast-PWM)
 *            High: 0xff
 *
 *          For more details on these settings, visit http://github.com/JCapSolutions/blf-firmware/wiki/PWM-Frequency
 *
 * STARS
 *            Star 2 = Not used
 *            Star 3 = Not used
 *            Star 4 = Switch input
 *
 * VOLTAGE
 *            Resistor values for voltage divider (reference BLF-VLD README for more info)
 *            Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 *               VCC
 *                |
 *               Vd (~.25 v drop from protection diode)
 *                |
 *              1912 (R1 19,100 ohms)
 *                |
 *                |---- PB2 from MCU
 *                |
 *              4701 (R2 4,700 ohms)
 *                |
 *               GND
 *
 *            ADC = ((V_bat - V_diode) * R2   * 255) / ((R1        + R2  ) * V_ref)
 *            125 = ((3.0   - .25        ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *            121 = ((2.9   - .25        ) * 4700 * 255) / ((19100 + 4700) * 1.1  )
 *
 *            Well 125 and 121 were too close, so it shut off right after lowering to low mode, so I went with
 *            130 and 120
 *
 *            To find out what value to use, plug in the target voltage (V) to this equation
 *                value = (V * 4700 * 255) / (23800 * 1.1)
 *          
 */


/*
 * CPU configuration
 */
// clock frequency used in util/delay.h
#define F_CPU 4800000UL


/*
 * Feature configuration
 */
#define VOLTAGE_MON         1           // set to 0 to disable - ramp down and eventual shutoff when battery is low
#define TURBO_MODE          1           // enable turbo mode with ramp down after timeout
#define DEBOUNCE_BOTH   1           // set to 1 to debounce both press and release, else debounce only press
#define BLINK_ON_POWER  1           // blink once when power is received


/*
 * Pin layout
 */
#define ALT_PWM_PIN PB0
#define PWM_PIN         PB1
#define VOLTAGE_PIN PB2
#define SWITCH_PIN  PB3             // what pin the switch is connected to, which is Star 4
#define STAR3_PIN   PB4             // If not connected, will cycle L-H.  Connected, H-L
#define PWM_LVL         OCR0B           // OCR0B is the output compare register for PB1


/*
 * ADC configuration
 */
#define ADC_CHANNEL 0x01            // MUX 01 corresponds with PB2
#define ADC_DIDR        ADC1D           // Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06            // clk/64
#define ADC_LOW         130             // When do we start ramping
#define ADC_CRIT        120             // When do we shut the light off
#define ADC_DELAY   188             // Delay in ticks between low-bat rampdowns (188 ~= 3s)


/*
 * PWM modes
 */
#define PWM_PHASE   0b00100001
#define PWM_FAST        0b00100011


/*
 * Switch debounce
 */
#define DB_PRES_DUR 0b00000001  // time before we consider the switch pressed (after first realizing it was pressed)
#define DB_REL_DUR  0b00000011  // time before we consider the switch released


/*
 * Switch and mode handling
 */
#define LONG_PRESS_DUR          32          // How many WDT ticks until we consider a press a long press. 32 is roughly 1/2 s
#define DOUBLE_PRESS_DUR        8           // How many WDT ticks below which we consider a press a double press
#define TURBO_TIMEOUT           3750        // How many WTD ticks before before dropping down (.016 sec each)
                                        // 90s  = 5625
                                        // 120s = 7500
#define SLEEP_TIMEOUT           125         // How many WTD ticks after switch released before entering sleep mode (.016 sec each)


/*
 * include header files
 */
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>


/*
 * Type definition
 */


// main state variables
typedef struct
{
        uint8_t on_off_state;                   // 0 = off, 1 = on
        uint8_t mode_idx;                       // brightness level: 0 to num_modes-1
        uint8_t ticks_until_sleep;              // number of ticks left before going to sleep
        uint8_t ticks_since_last_release;   // count number of ticks since last switch released
        uint8_t press_duration;                 // number of 16ms ticks that switch has been pressed
        uint8_t double_press_detected;          // detect double press
        uint8_t long_press_disable;             // disable long press detection
        uint8_t debounce_buffer;                // previous switch status used for debounce
#if TURBO_MODE
        uint16_t turbo_ticks;                   // number of ticks in TURBO mode
#endif
#if VOLTAGE_MON
        uint8_t adc_ticks;                      // count down timer used to check ADC
        uint8_t lowbatt_cnt;                    // counter used for low battery detection
#endif
} state_t;




/*
 * Mode configuration
 */
// put mode tables into EEPROM to save program memory
uint8_t EEMEM mode_level[] = {1, 4, 15, 42, 127, 255};
uint8_t EEMEM mode_phase[] = {PWM_PHASE, PWM_PHASE, PWM_FAST, PWM_FAST, PWM_FAST, PWM_PHASE};
uint8_t EEMEM mode_next[] = {1, 2, 3, 4, 5, 1};
uint8_t EEMEM mode_prev[] = {0, 5, 1, 2, 3, 4};
const uint8_t num_modes_minus1 = sizeof(mode_level) - 1;
#define START_MODE 1


/*
 * global variables
 */
// Use initializer here to reduce size of reset_state().
volatile state_t state =
{
        0,                      // on_off_state
        START_MODE,             // mode_idx
        SLEEP_TIMEOUT,          // ticks_until_sleep
        DOUBLE_PRESS_DUR,   // ticks_since_last_release
        1,                      // press_duration
        0,                      // double_press_detected
        0,                      // long_press_disable
        0xff,                   // debounce_buffer
#if TURBO_MODE
        0,                      // turbo_ticks
#endif
#if VOLTAGE_MON
        ADC_DELAY,              // adc_ticks
        0,                      // lowbatt_cnt
#endif
};


/*
 * HW configuration routines
 */


// Setup and enable watchdog timer to only interrupt, not reset, every 16ms.
inline void WDT_on()
{
        cli();                              // Disable interrupts
        wdt_reset();                        // Reset the WDT
        WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
        WDTCR = (1<<WDTIE);                 // Enable interrupt every 16ms
        sei();                              // Enable interrupts
}


// Disable watchdog timer
inline void WDT_off()
{
        cli();                              // Disable interrupts
        wdt_reset();                        // Reset the WDT
        MCUSR &= ~(1<<WDRF);                // Clear Watchdog reset flag
        WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
        WDTCR = 0x00;                       // Disable WDT
        sei();                              // Enable interrupts
}


#if VOLTAGE_MON
// Setup ADC for voltage monitoring
inline void ADC_on()
{
        ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
        DIDR0 |= (1 << ADC_DIDR);                               // disable digital input on ADC pin to reduce power consumption
        ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}
#endif


// Disable ADC
inline void ADC_off()
{
        ADCSRA &= ~(1<<7); //ADC off
}


// Enable pin change interrupts
inline void PCINT_on() {
        GIMSK |= (1 << PCIE);
}


// Disable pin change interrupts
inline void PCINT_off() {
        GIMSK &= ~(1 << PCIE);
}


// set PWM mode
inline void set_pwm_mode()
{
        if (state.on_off_state)
        {
            // read mode phase and level from EEPROM
            TCCR0A = eeprom_read_byte(&mode_phase[state.mode_idx]);
            PWM_LVL = eeprom_read_byte(&mode_level[state.mode_idx]);
        }
        else
        {
            // use phase-accurate PWM mode with level 0 to get LED fully off
            TCCR0A = PWM_PHASE;
            PWM_LVL = 0;
        }
}


// Need an interrupt for when pin change is enabled to ONLY wake us from sleep.
// All logic of what to do when we wake up will be handled in the main loop.
EMPTY_INTERRUPT(PCINT0_vect);


// initialize MCU functions
inline void initialize_mcu()
{
        // turn off watchdog timer as a precaution
        WDT_off();
    
        // Set all ports to input, and turn pull-up resistors on for the inputs we are using
        DDRB = 0x00;
        PORTB = (1 << SWITCH_PIN);


        // Set the switch as an interrupt for when we turn pin change interrupts on
        PCMSK = (1 << SWITCH_PIN);
    
        // Set PWM pin to output
        DDRB = (1 << PWM_PIN);


        // Set timer to do PWM for correct output pin and set prescaler timing
        TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)


        // Turn features on or off as needed
#if VOLTAGE_MON
        ADC_on();
#else
        ADC_off();
#endif
        ACSR   |=  (1<<7); //AC off


        // Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}


/*
 * Read state of switch with debounce
 */
uint8_t is_pressed()
{
        uint8_t debounce_pressed;


#if DEBOUNCE_BOTH
        // retrieve debounce_pressed from MSB of state.debounce_buffer
        debounce_pressed = state.debounce_buffer & 128;
#endif


        // Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
        state.debounce_buffer = ((state.debounce_buffer << 1) & 0x7f) | ((~(PINB >> SWITCH_PIN)) & 1);


#if DEBOUNCE_BOTH
        debounce_pressed = (debounce_pressed ? ((state.debounce_buffer & DB_REL_DUR) != 0) : ((state.debounce_buffer & DB_PRES_DUR) == DB_PRES_DUR)) ? 128 : 0;
        // store debounce_pressed in MSB of state.debounce_buffer
        state.debounce_buffer |= debounce_pressed;
#else
        debounce_pressed = (state.debounce_buffer & DB_REL_DUR);
#endif


        return debounce_pressed;
}


inline void next_mode()
{
        state.mode_idx = eeprom_read_byte(&mode_next[state.mode_idx]);
}


inline void prev_mode()
{
        state.mode_idx = eeprom_read_byte(&mode_prev[state.mode_idx]);
}


inline void reset_state()
{
        state.ticks_until_sleep = SLEEP_TIMEOUT;


        // start by simulating a key press
        state.debounce_buffer = 0xff;
        state.press_duration = 1;
        state.ticks_since_last_release = DOUBLE_PRESS_DUR;
}


void sleep_until_switch_press()
{
        // Turn the WDT off so it doesn't wake us from sleep
        // Will also ensure interrupts are on or we will never wake up
        WDT_off();
        // Enable a pin change interrupt to wake us up
        PCINT_on();
        // Now go to sleep
        sleep_mode();
        // Hey, someone must have pressed the switch!!
        // Disable pin change interrupt because it's only used to wake us up
        PCINT_off();
        // reset state and simulate a key press
        reset_state();
        // Turn the WDT back on to check for switch presses
        WDT_on();
        // Go back to main program
}


/*
 * Watchdog routine
 * The watchdog timer is called every 16ms.
 */
ISR(WDT_vect)
{
        if (state.ticks_until_sleep == 0)
        {
            // do nothing if we are in sleep mode
            return;
        }


        uint8_t change_pwm_mode = 0;            // signal when to change PWM mode
    
        // Process switch input and UI mode selection
        if (is_pressed())
        {
            // detect double press when switch is first pressed
            if ((state.press_duration++ == 0) && (state.ticks_since_last_release < DOUBLE_PRESS_DUR))
            {
                state.double_press_detected = 1;
                if (state.on_off_state)
                {
                    // turbo
                    state.mode_idx = num_modes_minus1;
                    // disable further long press actions
                    state.long_press_disable = 1;
                }
                else
                {
                    prev_mode();
                    state.on_off_state = 1;
                }
                change_pwm_mode = 1;
            }


            if ((state.press_duration == LONG_PRESS_DUR) && (state.double_press_detected == 0) && (state.on_off_state == 0))
            {
                // long press and previously off so turn on in lowest mode
                state.mode_idx = 0;
                state.on_off_state = 1;
                change_pwm_mode = 1;
                // disable further long press actions
                state.long_press_disable = 1;
            }


            if (!state.long_press_disable)
            {
                if ((state.press_duration == LONG_PRESS_DUR) || (state.press_duration == 2*LONG_PRESS_DUR))
                {
                    if (state.double_press_detected == 0)
                    {
                        next_mode();
                    }
                    else
                    {
                        prev_mode();
                    }
                    change_pwm_mode = 1;
                }
            }


            // detect really long key press and treat as a sequence of long key presses
            if (state.press_duration >= 2*LONG_PRESS_DUR)
            {
                state.press_duration = LONG_PRESS_DUR;
            }


            // reset relevant counters here
#if VOLTAGE_MON
            state.adc_ticks = ADC_DELAY;
#endif
            state.ticks_since_last_release = 0;
#if TURBO_MODE
            state.turbo_ticks = 0;
#endif
        }
        else
        {
            if (state.press_duration > 0 && state.press_duration < LONG_PRESS_DUR)
            {
                // short press
                if (!state.double_press_detected)
                {
                    // toggle on_off_state
                    state.on_off_state = state.on_off_state ^ 1;
                    change_pwm_mode = state.on_off_state;
                }
            }
            
            // we delay turning off until DOUBLE_PRESS_DUR has passed to avoid a blink when double press from ON
            if (state.on_off_state == 0 && state.ticks_since_last_release == DOUBLE_PRESS_DUR)
            {
                change_pwm_mode = 1;
            }


#if TURBO_MODE
            // Only do turbo check when switch isn't pressed and light is on
            if (state.on_off_state && state.mode_idx == num_modes_minus1)
            {
                if (++state.turbo_ticks > TURBO_TIMEOUT)
                {
                    --state.mode_idx;
                    change_pwm_mode = 1;
                }
            }
            else
            {
                state.turbo_ticks = 0;
            }
#endif


#if VOLTAGE_MON
            // Only do voltage monitoring when the switch isn't pressed and light is on
            if (state.on_off_state)
            {
                if (state.adc_ticks > 0)
                {
                    --state.adc_ticks;
                }
                else
                {
                    // See if conversion is done
                    if (ADCSRA & (1 << ADIF))
                    {
                        // See if voltage is lower than what we were looking for
                        state.lowbatt_cnt = ADCH < ((state.mode_idx == 0) ? ADC_CRIT : ADC_LOW) ? state.lowbatt_cnt+1 : 0;
                    }
                    // See if it's been low for a while
                    if (state.lowbatt_cnt >= 4)
                    {
                        if (state.mode_idx == 0)
                        {
                            state.on_off_state = 0;
                        }
                        else
                        {
                            --state.mode_idx;
                        }
                        state.lowbatt_cnt = 0;
                        state.adc_ticks = ADC_DELAY;
                        change_pwm_mode = 1;
                    }
                    // Make sure conversion is running for next time through
                    ADCSRA |= (1 << ADSC);
                }
            }
            else
            {
                state.lowbatt_cnt = 0;
                state.adc_ticks = ADC_DELAY;
            }
#endif


            // reset relevant counters here
            state.press_duration = 0;
            state.double_press_detected = 0;
            state.long_press_disable = 0;
            if (state.ticks_since_last_release < 255)
            {
                ++state.ticks_since_last_release;
            }
        }
    
        if (change_pwm_mode)
        {
            set_pwm_mode();
        }
    
        if (state.on_off_state)
        {
            state.ticks_until_sleep = SLEEP_TIMEOUT;
        }
        else if (state.ticks_until_sleep > 0)
        {
            --state.ticks_until_sleep;
        }
}


int main(void)
{
        initialize_mcu();
    
#if BLINK_ON_POWER
        TCCR0A = PWM_PHASE;
        PWM_LVL = eeprom_read_byte(&mode_level[num_modes_minus1]);
        _delay_ms(3);
        PWM_LVL = 0;
        _delay_ms(3);
#endif


        sleep_until_switch_press();


        while(1)
        {
            if (!state.ticks_until_sleep)
            {
                // Go to sleep
                sleep_until_switch_press();
            }
        }
}