// Candle-flicker LED program for ATtiny13A based on analysis at[0]
// and IIR filter proposed and tested at [1].
// 
// Hardware/peripheral usage notes:
// LED control output on pin 5.//changed to fit nanjg layout pin 6 PB1
// Uses hardware PWM and the PWM timer's overflow to
// count out the frames.  Keeps the RNG seed in EEPROM.
// 
// [0] http://inkofpark.wordpress.com/2013/12/15/candle-flame-flicker/
// [1] http://inkofpark.wordpress.com/2013/12/23/arduino-flickering-candle/
#define F_CPU 9600000
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include <stdbool.h>
#include <stdint.h>

// simple 32-bit LFSR PRNG with seed stored in EEPROM
#define POLY 0xA3AC183Cul

#define PWM OCR0B  //PWM-value 




// set up PWM on pin 5 (PORTB bit 0) using TIMER0 (OC0A)
#define TIMER0_OVF_RATE     2343.75 // Hz
static void init_pwm() {
    // COM0A    = 10  ("normal view")
    // COM0B    = 00  (disabled)
    // WGM0     = 001 (phase-correct PWM, TOP = 0xff, OCR update at TOP)
    // CS       = 010 (1:8 with IO clock: 2342 Hz PWM if clock is 9.6Mhz )
    TCCR0A  = 0x81;         // 1000 0001
    TCCR0B  = 0x02;         // 0000 0010
    //DDRB   |= 1 << DDB0;    // pin direction = OUT


}

// set up PWM on pin 6 (PORTB bit 1) using TIMER1 (OC0A)
#define TIMER0_OVF_RATE     2343.75 // Hz
#define pwminit() do{ TCCR0A=0b00100001; TCCR0B=0b00000010; DDRB   |= 1 << DDB1;}while(0)  //changed to nanjg pinlayout..hopefully correct


static uint32_t lfsr = 1;
static void init_rand() {
    static uint32_t EEMEM boot_seed = 1;
    
    lfsr = eeprom_read_dword(&boot_seed);
    // increment at least once, skip 0 if we hit it
    // (0 is the only truly unacceptable seed)
    do {lfsr++;} while (!lfsr);
    eeprom_write_dword(&boot_seed, lfsr);
}
static inline uint8_t rand(uint8_t bits) {
    uint8_t x = 0;
    uint8_t i;
    
    for (i = 0; i < bits; i++) {
        x <<= 1;
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1ul) & POLY);
        x |= lfsr & 1;
    }
    
    return x;
}



// approximate a normal distribution with mean 0 and std 32.
// does so by drawing from a binomial distribution and 'fuzzing' it a bit.
// 
// There's some code at [0] calculating the actual distribution of these
// values.  They fit the intended distribution quite well.  There is a
// grand total of 3.11% of the probability misallocated, and no more than
// 1.6% of the total misallocation affects any single bin.  In absolute
// terms, no bin is more than 0.05% off the intended priority, and most
// are considerably closer.
// 
// [0] https://github.com/mokus0/junkbox/blob/master/Haskell/Math/ApproxNormal.hs
static int8_t normal() {
    // n = binomial(16, 0.5): range = 0..15, mean = 8, sd = 2
    // center = (n - 8) * 16; // shift and expand to range = -128 .. 112, mean = 0, sd = 32
    int8_t center = -128;
    uint8_t i;
    for (i = 0; i < 16; i++) {
        center += rand(1) << 4;
    }
    
    // 'fuzz' follows a symmetric triangular distribution with 
    // center 0 and halfwidth 16, so the result (center + fuzz)
    // is a linear interpolation of the binomial PDF, mod 256.
    // (integer overflow corresponds to wrapping around, blending
    // both tails together).
    int8_t fuzz = (int8_t)(rand(4)) - (int8_t)(rand(4));
    return center + fuzz;
}

#ifdef INKOFPARK

// fixed-point 2nd-order Butterworth low-pass filter.
// The output has mean 0 and std deviation approximately 0.53
// (4342 or so in mantissa).
//
// integer types here are annotated with comments of the form
// "/* x:y */", where x is the number of significant bits in
// the value and y is the base-2 exponent (negated, actually).
// 
// The inspiration for the filter (and identification of basic
// parameters and comparison with some other filters) was done
// by Park Hays and described at [0].  Some variations and
// prototypes of the fixed-point version are implemented at [1].
//
// [0] http://inkofpark.wordpress.com/2013/12/23/arduino-flickering-candle/
// [1] https://github.com/mokus0/junkbox/blob/master/Haskell/Math/BiQuad.hs
#define UPDATE_RATE     60 // Hz
#define FILTER_STDDEV   4.3e3
static int16_t /* 15:13 */ flicker_filter(int16_t /* 7:5 */ x) {
    const int16_t
        /* 3:3  */ a1 = -7,  // round ((-0.87727063) * (1L << 3 ))
        /* 4:5  */ a2 = 10,  // round (  0.31106039  * (1L << 5 ))
        /* 7:10 */ b0 = 111, // round (  0.10844744  * (1L << 10))
        /* 7:9  */ b1 = 111, // round (  0.21689488  * (1L << 9 ))
        /* 7:10 */ b2 = 111; // round (  0.10844744  * (1L << 10))
    static int16_t
        /* 15:13 */ d1 = 0,
        /* 15:14 */ d2 = 0;
    
    int16_t /* 15:13 */ y;
    
    // take advantage of fact that b's are all equal with the 
    // chosen exponents (this is a general feature of 2nd-order 
    // discrete-time butterworth filters, actually)
    int16_t /* 15:14 */ bx = b0 * x >> 1;
    y  = (bx >> 1)                   + (d1 >> 0);
    d1 = (bx >> 0) - (a1 * (y >> 3)) + (d2 >> 1);
    d2 = (bx >> 0) - (a2 * (y >> 4));
    
    return y;
}

#else

// scipy-designed filter with 4Hz cutoff and 100Hz sample rate.
// parameters set using the following little snippet:
// >>> rate=156.25
// >>> cutoff = 2.2/(rate/2.)
// >>> b, a = signal.butter(2, cutoff, 'low')
//
// which yields:
//
//  b = 0.00184036,  0.00368073,  0.00184036
//  a = 1.        , -1.87503716,  0.88239861
//
// B was then rescaled to 1, 2, 1 in order to improve the
// numerical accuracy by both reducing opportunities for 
// round-off error and increasing the amount of precision
// that can be allocated in other places.  (it also
// conveniently replaces 3 multiplies with shifts)
//
// This function implements the transposed direct-form 2, using
// 16-bit fixed-point math.  Integer types here are annotated
// with comments of the form "/* x:y */", where x is the number
// of significant bits in the value and y is the (negated)
// base-2 exponent.
// 
// The inspiration for the filter (and identification of basic
// parameters and comparison with some other filters) was done
// by Park Hays and described at [0].  Some variations and
// prototypes of the fixed-point version are implemented at [1].
// The specific parameters of this filter, though, are changed
// as follows:
// 
// The higher sampling rate puts the nyquist frequency at 50 Hz
// (digital butterworth filters appear to roll off  to -inf at
// the nyquist rate, though the fixed-point implementation has
// a hard noise floor due to the fixed quantum).  This probably
// doesn't make that much difference visually but it seems to
// greatly improve the numerical properties of the fixed-point
// version.
// 
// SciPy's butterworth parameter designer also seems to put the
// cutoff frequency higher than advertised (or maybe that's an
// effect of the limited sampling rate, but PSD plots seem to
// show the cutoff much higher than expected, and an 8Hz cutoff
// was just far too fast-moving for my taste), so I pulled it
// down to something that looked pleasant when running, yielding
// the following filter.
//
// [0] http://inkofpark.wordpress.com/2013/12/23/arduino-flickering-candle/
// [1] https://github.com/mokus0/junkbox/blob/master/Haskell/Math/BiQuad.hs
#define UPDATE_RATE     450 // Hz
#define FILTER_STDDEV   6.2e3
static int16_t /* 15:13 */ flicker_filter(int8_t /* 7:5 */ x) {
    const int16_t
        /* 9:8 */ a1 = -480, // round ((-1.87503716) * (1L << 8 ))
        /* 8:8 */ a2 =  226; // round (  0.88239861  * (1L << 8 ))
        // b0 = 1, b1 = 2, b2 = 1; multiplies as shifts below
    static int16_t
        /* 15:6 */ d1 = 0,
        /* 15:6 */ d2 = 0;
    
    int16_t /* 15:6 */ y;
    
    y  = ((int16_t) x << 1)                           + (d1 >> 0);
    d1 = ((int16_t) x << 2) - (a1 * (int32_t) y >> 8) + (d2 >> 0);
    d2 = ((int16_t) x << 1) - (a2 * (int32_t) y >> 8);
    
    return y;
}

#endif

static uint8_t next_intensity() {
    const uint8_t m = 171, s = 2;
    const int16_t scale = (s * FILTER_STDDEV) / (255 - m);
    
    int16_t x = m + flicker_filter(normal()) / scale;
    return x < 0 ? 0 : x > 255 ? 255 : x;
}

// use timer so update rate isn't
// affected by calculation time
static volatile bool tick = false;
ISR(TIM0_OVF_vect) {
    static uint8_t cycles = 0;
    if (!cycles--) {
        tick = true;
        cycles = TIMER0_OVF_RATE / UPDATE_RATE;
    }
}

int main(void)
{
    init_rand();
    //init_pwm(); //old standard init
	pwminit(); //changed to nanjg standard PB1 leg 6
    
    
 //enable timer overflow interrupt timer
    TIMSK0 = 1 << TOIE0;
    sei();
    
    while(1)
    {
        // sleep till next update, then perform it.
        while (!tick) sleep_mode();
        tick = false;
        
        PWM = next_intensity();
		//PWM= 222;
    }
}
