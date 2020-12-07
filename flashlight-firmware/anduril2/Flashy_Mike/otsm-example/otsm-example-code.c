/* Simple sample firmware for otc less design (single cell).

   Half press less than about 300 ms: next mode
              between 300 and 900 ms: previous mode
              longer:                 blink out voltage reading 
                                      (0 to 255) in 3 decimals

   Suggested capacity of buffer cap (C2): >= 47 uF.
   Vbatt is connected to PB3 (Pin2).
   Voltage divider still connected to Pin7.
   Timing specified for Attiny25 at 10 Mhz.
   Don't fuse BOD.
   Created and built with Atmel Studio.
   Fuses for Attiny25: lfuse: 0xD2, hfuse: 0xDF.
*/

#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define PIN_7135            PB0
#define PIN_FET             PB1
#define PIN_POWER_DETECT    PB3
#define INT_POWER_DETECT    PCINT3
#define VLT_PIN             PB2
#define VLT_CHANNEL         0x01
#define VLT_DIDR            ADC1D
#define ADC_VREF            ((1 << REFS1) | (1 << REFS2))    // 2.56V
#define ADC_PRESCL          0x06

#define PWM_PHASE           0xA1
#define PWM_7135            OCR0A
#define PWM_FET             OCR0B

#define MODE_COUNT          6
#define PWM_LEVELS_7135     30, 80, 255, 255, 255,   0
#define PWM_LEVELS_FET      0,   0,   0,  30,  80, 255

// forward declarations
uint8_t adc_init_and_read();
void blink_once();
void blink_once_long();
void blink_value(uint8_t value);
void delay_250_micro_sec(uint8_t n);
void delay_50_milli_sec(uint8_t n);
void delay_1_sec();
void led_off();
void led_on();

volatile uint8_t s_clicked = 0;
const uint8_t pwm_levels_7135[] = { PWM_LEVELS_7135 };
const uint8_t pwm_levels_fet[] = { PWM_LEVELS_FET };
uint8_t mode_index = 0;

////////////////////////////////////////////////////////////////////////////////

int main(void)
{
    uint8_t blink_voltage = 0;
    uint8_t voltage;

    // set LED ports as output
    DDRB = (1 << PIN_7135) | (1 << PIN_FET);

    // initialize PWM
    TCCR0A = PWM_PHASE;
    TCCR0B = 0x01;

    // enable pin change interrupts
    PCMSK = (1 << INT_POWER_DETECT);
    GIMSK |= (1 << PCIE);
    sei();

    // start with first mode
    PWM_7135 = pwm_levels_7135[mode_index];
    PWM_FET = pwm_levels_fet[mode_index];

    while ( 1 )
    {
        if ( s_clicked )
        {
            if ( s_clicked <= 10 ) // < about 300 ms
            {
                blink_voltage = 0;
                mode_index++;
                if ( mode_index >= MODE_COUNT )
                    mode_index = 0;
            }
            else if ( s_clicked <= 30 ) // < about 900 ms
            {
                blink_voltage = 0;
                mode_index--;
                if  ( mode_index == 255 )
                    mode_index = MODE_COUNT - 1;
            }
            else // > about 900 ms
            {
                mode_index = 0;
                blink_voltage = 1;
            }

            s_clicked = 0;
            PWM_7135 = pwm_levels_7135[mode_index];
            PWM_FET  = pwm_levels_fet[mode_index];
        }

        voltage = adc_init_and_read();

        if ( blink_voltage )
            blink_value(voltage);

        delay_1_sec();
    }
} 

////////////////////////////////////////////////////////////////////////////////

ISR(PCINT0_vect)
{
    // disable output
    PORTB = 0;

    // turn off PWM generation (might not be necessary)
    TCCR0A = 0;
    TCCR0B = 0;

    // ADC off
    ADCSRA &= ~(1 << ADEN);

    // disable pin change interrupt
    GIMSK &= ~(1 << PCIE);
    PCMSK = 0;

    s_clicked = 0;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    do
    {
        s_clicked++;

        wdt_reset();
        WDTCR = (1<<WDIE) | WDTO_30MS;

        sei();
        sleep_mode();
        cli();

    } while ( (PINB & (1 << PIN_POWER_DETECT)) == 0 );

    wdt_disable();

    // debounce
    delay_250_micro_sec(10); 

    // activate pwm generation
    TCCR0A = PWM_PHASE;
    TCCR0B = 0x01;

    // enable pin change interrupts
    PCMSK = (1 << INT_POWER_DETECT);
    GIMSK |= (1 << PCIE);
}

////////////////////////////////////////////////////////////////////////////////

void delay_250_micro_sec(uint8_t n)
{
    while ( n-- )
    {
        int k = 180;
        while ( k-- )
        {
            if ( s_clicked )
                return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void  delay_50_milli_sec(uint8_t n)
{
    do
    {
        delay_250_micro_sec(200);
        if ( s_clicked )
            return;
    } while ( --n );
}

////////////////////////////////////////////////////////////////////////////////

void delay_1_sec()
{
    delay_50_milli_sec(20);
}

////////////////////////////////////////////////////////////////////////////////

void led_on()
{
    PWM_7135 = pwm_levels_7135[mode_index];
    PWM_FET  = pwm_levels_fet[mode_index];
}

////////////////////////////////////////////////////////////////////////////////

void led_off()
{
    PWM_7135 = 0;
    PWM_FET = 0;
}

////////////////////////////////////////////////////////////////////////////////

void blink_once()
{
    led_on();
    delay_50_milli_sec(5);
    led_off();
    delay_50_milli_sec(5);
}

////////////////////////////////////////////////////////////////////////////////

void blink_once_long()
{
    led_on();
    delay_1_sec();
    led_off();
    delay_1_sec();
}

////////////////////////////////////////////////////////////////////////////////

void blink_once_short()
{
    led_on();
    delay_50_milli_sec(1);
    led_off();
    delay_50_milli_sec(5);
}

////////////////////////////////////////////////////////////////////////////////

void blink_value(uint8_t value)
{
    // 100er
    if ( value < 100 )
        blink_once_short();

    while ( value >= 100 )
    {
        value -= 100;
        blink_once();
    }
    delay_1_sec();

    // 10er
    if ( value < 10 )
        blink_once_short();

    while ( value >= 10 )
    {
        value -= 10;
        blink_once();
    }
    delay_1_sec();

    // 1er
    if ( value == 0 )
        blink_once_short();

    while ( value > 0 )
    {
        value--;
        blink_once();
    }
    delay_1_sec();
}

////////////////////////////////////////////////////////////////////////////////

uint8_t adc_read()
{
    uint8_t value;

    while (ADCSRA & (1 << ADSC))
    {
        if ( s_clicked )
            return 0;
    }
    value = ADCH;
    ADCSRA |= (1 << ADSC);
    return value;
}

////////////////////////////////////////////////////////////////////////////////

uint8_t adc_init_and_read()
{
    DIDR0 |= (1 << VLT_DIDR);
    ADMUX  = ADC_VREF | (1 << ADLAR) | VLT_CHANNEL;
    ADCSRA = (1 << ADEN) | (1 << ADSC) | ADC_PRESCL;
    adc_read(); // first run not reliable (see spec)
    return adc_read();
}

////////////////////////////////////////////////////////////////////////////////
// empty interrupt function required otherwise mcu will reset

ISR(WDT_vect)
{
};

////////////////////////////////////////////////////////////////////////////////
