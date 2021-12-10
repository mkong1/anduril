/*
 * This firmware simply helps calibrate values for temperature readings.
 * It is not intended to be used as an actual flashlight.
 *
 * It will read the voltage, then read out the raw value as a series of
 * blinks.  It will provide up to three groups of blinks, representing the
 * hundreds digit, the tens digit, then the ones.  So, for a raw value of 183,
 * it would blink once, pause, blink eight times, pause, then blink three times.
 * It will then wait longer and re-read the voltage, then repeat.
 *
 * Attiny25/45/85 Diagram
 *           ----
 *   RESET -|1  8|- VCC
 *  Star 4 -|2  7|- Voltage ADC
 *  Star 3 -|3  6|- PWM
 *     GND -|4  5|- Star 2
 *           ----
 */

//#define ATTINY 13
//#define ATTINY 25
#define FET_7135_LAYOUT  // specify an I/O pin layout
// Also, assign I/O pins in this file:
#include "tk-attiny.h"

/*
 * =========================================================================
 * Settings to modify per driver
 */

#define BLINK_PWM   10

/*
 * =========================================================================
 */

#define OWN_DELAY           // Don't use stock delay functions.
#define USE_DELAY_S         // use _delay_s()
#define USE_DELAY_MS        // Also use _delay_ms()
#include "tk-delay.h"

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define THERMAL_REGULATION
#define TEMP_10bit
#include "tk-voltage.h"

void noblink() {
    PWM_LVL = (BLINK_PWM>>2);
    _delay_ms(5);
    PWM_LVL = 0;
    _delay_ms(200);
}

void blink() {
    PWM_LVL = BLINK_PWM;
    _delay_ms(100);
    PWM_LVL = 0;
    _delay_ms(200);
}

int main(void)
{
    // Set PWM pin to output
    DDRB = (1 << PWM_PIN);

    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0A = 0x21; // phase corrected PWM is 0x21 for PB1, fast-PWM is 0x23
    TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Turn features on or off as needed
    ADC_on_temperature();
    ACSR   |=  (1<<7); //AC off

    // blink once on receiving power
    PWM_LVL = 255;
    _delay_ms(5);
    PWM_LVL = 0;
    _delay_s();

    uint16_t value;
    uint8_t i;
    value = get_temperature();

    while(1) {
        PWM_LVL = 0;

        // get an average of several readings
        value = 0;
        for (i=0; i<8; i++) {
            value += get_temperature();
            PWM_LVL = 2;
            _delay_ms(25);
            PWM_LVL = 0;
            _delay_ms(25);
        }
        value = value >> 3;
        _delay_s();

        // thousands
        while (value >= 1000) {
            value -= 1000;
            blink();
        }
        _delay_s();

        // hundreds
        while (value >= 100) {
            value -= 100;
            blink();
        }
        _delay_s();

        // tens
        if (value < 10) {
            noblink();
        }
        while (value >= 10) {
            value -= 10;
            blink();
        }
        _delay_s();

        // ones
        if (value <= 0) {
            noblink();
        }
        while (value > 0) {
            value -= 1;
            blink();
        }
        _delay_s();

        // ... and wait a bit for next time
        _delay_s();
        _delay_s();

    }
}
