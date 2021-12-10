/*
 * This firmware simply helps calibrate values for voltage readings.
 * It is not intended to be used as an actual flashlight.
 *
 * It will read the voltage, then read out the raw value as a series of
 * blinks.  It will provide up to three groups of blinks, representing the
 * hundreds digit, the tens digit, then the ones.  So, for a raw value of 183,
 * it would blink once, pause, blink eight times, pause, then blink three times.
 * It will then wait longer and re-read the voltage, then repeat.
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
 */
// set some hardware-specific values...
// (while configuring this firmware, skip this section)
#if (ATTINY == 13)
#define F_CPU 4800000UL
#define EEPLEN 64
#define DELAY_TWEAK 950
#elif (ATTINY == 25)
#define F_CPU 8000000UL
#define EEPLEN 128
#define DELAY_TWEAK 2000
#else
Hey, you need to define ATTINY.
#endif


/*
 * =========================================================================
 * Settings to modify per driver
 */

#define OWN_DELAY   // Should we use the built-in delay or our own?

#define BLINK_PWM   20

/*
 * =========================================================================
 */

#ifdef OWN_DELAY
#include <util/delay_basic.h>
// Having own _delay_ms() saves some bytes AND adds possibility to use variables as input
static void _delay_ms(uint16_t n)
{
    while(n-- > 0)
        _delay_loop_2(DELAY_TWEAK);
}
#else
#include <util/delay.h>
#endif

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#define STAR2_PIN   PB0
#define STAR3_PIN   PB4
#define STAR4_PIN   PB3
#define CAP_PIN     PB3
#define CAP_CHANNEL 0x03    // MUX 03 corresponds with PB3 (Star 4)
#define CAP_DIDR    ADC3D   // Digital input disable bit corresponding with PB3
#define PWM_PIN     PB1
#define VOLTAGE_PIN PB2
#define ADC_CHANNEL 0x01    // MUX 01 corresponds with PB2
#define ADC_DIDR    ADC1D   // Digital input disable bit corresponding with PB2
#define ADC_PRSCL   0x06    // clk/64

#define PWM_LVL     OCR0B   // OCR0B is the output compare register for PB1

inline void ADC_on() {
#if (ATTINY == 13)
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
#elif (ATTINY == 25)
    ADMUX  = (1 << REFS1) | (1 << ADLAR) | ADC_CHANNEL; // 1.1v reference, left-adjust, ADC1/PB2
#endif
    DIDR0 |= (1 << ADC_DIDR);                           // disable digital input on ADC pin to reduce power consumption
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
}

inline void ADC_off() {
    ADCSRA &= ~(1<<7); //ADC off
}

uint8_t get_reading() {
    // Start up ADC for capacitor pin
    DIDR0 |= (1 << CAP_DIDR);                           // disable digital input on ADC pin to reduce power consumption
#if (ATTINY == 13)
    ADMUX  = (1 << REFS0) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC3/PB3
#elif (ATTINY == 25)
    ADMUX  = (1 << REFS1) | (1 << ADLAR) | CAP_CHANNEL; // 1.1v reference, left-adjust, ADC3/PB3
#endif
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;   // enable, start, prescale
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
#if 1
    // Start again as datasheet says first result is unreliable
    ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
#endif
    // just return the value
    return ADCH;
}

void noblink() {
    PWM_LVL = (BLINK_PWM>>2);
    _delay_ms(5);
    PWM_LVL = 0;
    _delay_ms(200);
}

void blink() {
    PWM_LVL = BLINK_PWM;
    _delay_ms(150);
    PWM_LVL = 0;
    _delay_ms(200);
}

int main(void)
{
    uint16_t value;

    // Set PWM pin to output
    DDRB = (1 << PWM_PIN);

    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0A = 0x21; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Turn features on or off as needed
    ADC_on();
    ACSR   |=  (1<<7); //AC off

    // get an average of several readings
    /*
    uint8_t i;
    value = 0;
    for (i=0; i<8; i++) {
        value += get_reading();
        _delay_ms(50);
    }
    value = value >> 3;
    */
    value = get_reading();

    // Turn off ADC
    ADC_off();

    // Charge up the capacitor by setting CAP_PIN to output
    DDRB  |= (1 << CAP_PIN);    // Output
    PORTB |= (1 << CAP_PIN);    // High


    // blink once on receiving power
    //PWM_LVL = 255;
    //_delay_ms(5);
    //PWM_LVL = 0;

    // hundreds
    while (value >= 100) {
        value -= 100;
        blink();
    }
    _delay_ms(1000);

    // tens
    if (value < 10) {
        noblink();
    }
    while (value >= 10) {
        value -= 10;
        blink();
    }
    _delay_ms(1000);

    // ones
    if (value <= 0) {
        noblink();
    }
    while (value > 0) {
        value -= 1;
        blink();
    }
    _delay_ms(1000);

    while(1) {
        _delay_ms(1000);
    }
}
