#ifndef TK_VOLTAGE_H
#define TK_VOLTAGE_H
/*
 * Voltage / battcheck functions.
 *
 * Copyright (C) 2015 Selene Scriven
 * Modified version for use with fr-calibration.h
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
 */

//#include "tk-attiny.h"
#include "fr-calibration.h"

/* Probably should not have un-inlined functions and data initialization in a header :P, but
 *  the idea here anyway is this only gets included in one translation unit. */

// Added by FR:  Setup stuff defines and functions for inverted voltage reads

#ifdef DETECT_ONE_S  //  This conflicts with precompiling the voltage config
#undef READ_VOLTAGE_FROM_VCC
#undef READ_VOLTAGE_FROM_DIVIDER
#endif


uint8_t inv_mask=0;

#ifdef TEMPERATURE_MON
inline void ADC_on_temperature() {
    // TODO: (?) enable ADC Noise Reduction Mode, Section 17.7 on page 128
    //       (apparently can only read while the CPU is in idle mode though)
    // select ADC4 by writing 0b00001111 to ADMUX
    // 1.1v reference, left-adjust, ADC4
    ADMUX  = (1 << V_REF) | (1 << ADLAR) | TEMP_CHANNEL;
    // disable digital input on ADC pin to reduce power consumption
    //DIDR0 |= (1 << TEMP_DIDR);
    // enable, start, prescale
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;
//    _delay_ms(50); // is this really helpful?
	// Or Wait for completion
	// while (ADCSRA & (1 << ADSC));
#ifndef READ_VOLTAGE_FROM_DIVIDER	
	inv_mask=0; // don't invert ADC
#endif	
}
#endif  // TEMPERATURE_MON



#ifdef READ_VOLTAGE_FROM_VCC
//setup ADC parameters to read VCC using inverted read
#define v_ref VCC_REF_PIN //  Use Vcc as voltage referrence.
#define adc_channel _1V_ADC_INPUT // This actually measures the internal ref as the ADC input.
#define voltage_read_inv_mask 0xFF //invert voltage reading
#endif

#ifdef READ_VOLTAGE_FROM_DIVIDER
//setup ADC parameters to read classic way
  #ifdef REFERENCE_DIVIDER_READS_TO_VCC
     #define v_ref VCC_REF_PIN
  #else  
    #define v_ref 1
  #endif
#define adc_channel ADC_CHANNEL
#define voltage_read_inv_mask 0x00 // don't invert voltage reading
#endif



#ifndef READ_VOLTAGE_FROM_VCC 
#ifndef READ_VOLTAGE_FROM_DIVIDER
// If not set globally we can use inline functions to switch between ADC methods
// all three of below variable must be unset or things will break
// however this still need an implementation to swap the calibration table
// and that will require some memmory.  Left the code here anyway for now. -FR
uint8_t adc_channel=ADC_CHANNEL;
uint8_t v_ref=1; // Use internal reference.  
uint8_t voltage_read_inv_mask=0x00;
#define ONES_MAXV ADC_46

inline void set_adc_vcc_mode(){ // -by Flintrock
	// preconfigure adc variables for inverted Vcc reading
	// should be used before ADC_on
	adc_channel=0x0C; // read internal 1.1v reference
	v_ref=0x00; // Use Vcc as reference
	voltage_read_inv_mask=0xff;
}

inline void set_adc_normal_mode(){ //-by Flintrock
	// pre-congifure adc variables for normal operation
	// This is defualt and only needs to be called if set_adc_vcc_mode was already used.
	// ADC_on must be called again after this.
	adc_channel=ADC_CHANNEL; // read the configured ADC voltage pin
	v_ref=1; // use internal 1.1v as reference
    voltage_read_inv_mask=0;
}
#endif
#endif

inline void ADC_on() {
	// modified by FR for variable configuration.
    // disable digital input on ADC pin to reduce power consumption
//    DIDR0 |= (1 << ADC_DIDR); // moved to initialization
    // 1.1v reference, left-adjust, ADC1/PB2
    ADMUX  = (v_ref << V_REF) | (1 << ADLAR) | adc_channel;
////    ADMUX  = (v_ref << V_REF) | (1 << ADLAR) | adc_channel;
    // enable, start, prescale
//if we define it, we don't need to |= it in get_voltage and elsewhere.  Saves a read and an or.
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;

//	ADCSRA &= ~(1<<7); //ADC off
//	ADCSRA |= (1 << ADSC);
	    // Wait for completion
   if (!v_ref)	{ // if not 1.0V reference, need stabilization time, for switching back from temperature ADC.
	  while (ADCSRA & (1 << ADSC));
    } // this normally optimizes out if unused.
#ifndef READ_VOLTAGE_FROM_DIVIDER  
	inv_mask=voltage_read_inv_mask;
#endif
}

uint8_t get_voltage() {
    // Start conversion
    //	_delay_ms(50); // wait for adc to stablize.
    ADCSRA |= (1 << ADSC); // this is an SBIC, not an RMW
    // Wait for completion
//	_delay_ms(1);
    while (ADCSRA & (1 << ADSC));
    // Send back the result
#ifdef READ_VOLTAGE_FROM_DIVIDER
    return ADCH;
#else
//  Control if to invert the reading
// ^ means XOR. 0xFF XOR Y  produces 255-Y.  0x00 XOR Y produces Y 
    return inv_mask^ADCH;
#endif
}

void ADC_off() {
//    ADCSRA &= ~(1<<7); //ADC off
// &= takes 6 bytes instead of 2. no need to save the value,
// If we did need it, it's defined in ADC_INIT macro
    ADCSRA = ~(1<<7)&0xff; //ADC off
}


#ifdef USE_BATTCHECK
#ifdef BATTCHECK_4bars
PROGMEM const uint8_t voltage_blinks[] = {
               // 0 blinks for less than 1%
    ADC_0p,    // 1 blink  for 1%-25%
    ADC_25p,   // 2 blinks for 25%-50%
    ADC_50p,   // 3 blinks for 50%-75%
    ADC_75p,   // 4 blinks for 75%-100%
    ADC_100p,  // 5 blinks for >100%
    255,       // Ceiling, don't remove  (6 blinks means "error")
};
#endif  // BATTCHECK_4bars
#ifdef BATTCHECK_8bars
PROGMEM const uint8_t voltage_blinks[] = {
               // 0 blinks for less than 1%
    ADC_val(30),    // 1 blink  for 1%-12.5%
    ADC_val(33),    // 2 blinks for 12.5%-25%
    ADC_val(35),    // 3 blinks for 25%-37.5%
    ADC_val(37),    // 4 blinks for 37.5%-50%
    ADC_val(38),    // 5 blinks for 50%-62.5%
    ADC_val(39),    // 6 blinks for 62.5%-75%
    ADC_val(40),    // 7 blinks for 75%-87.5%
    ADC_val(41),    // 8 blinks for 87.5%-100%
    ADC_val(42),    // 9 blinks for >100%
    255,       // Ceiling, don't remove  (10 blinks means "error")
};
#endif  // BATTCHECK_8bars

#ifdef BATTCHECK_VpT
/*
PROGMEM const uint8_t v_whole_blinks[] = {
               // 0 blinks for (shouldn't happen)
    0,         // 1 blink for (shouldn't happen)
    ADC_20,    // 2 blinks for 2V
    ADC_30,    // 3 blinks for 3V
    ADC_40,    // 4 blinks for 4V
    255,       // Ceiling, don't remove
};
PROGMEM const uint8_t v_tenth_blinks[] = {
               // 0 blinks for less than 1%
    ADC_30,
    ADC_33,
    ADC_35,
    ADC_37,
    ADC_38,
    ADC_39,
    ADC_40,
    ADC_41,
    ADC_42,
    255,       // Ceiling, don't remove
};
*/

//PROGMEM const uint8_t voltage_blinks[] = {
    //// 0 blinks for (shouldn't happen)
    //ADC_25,(2<<5)+5,
    //ADC_26,(2<<5)+6,
    //ADC_27,(2<<5)+7,
    //ADC_28,(2<<5)+8,
    //ADC_29,(2<<5)+9,
    //ADC_30,(3<<5)+0,
    //ADC_31,(3<<5)+1,
    //ADC_32,(3<<5)+2,
    //ADC_33,(3<<5)+3,
    //ADC_34,(3<<5)+4,
    //ADC_35,(3<<5)+5,
    //ADC_36,(3<<5)+6,
    //ADC_37,(3<<5)+7,
    //ADC_38,(3<<5)+8,
    //ADC_39,(3<<5)+9,
    //ADC_40,(4<<5)+0,
    //ADC_41,(4<<5)+1,
    //ADC_42,(4<<5)+2,
    //ADC_43,(4<<5)+3,
    //ADC_44,(4<<5)+4,
    //255,   (1<<5)+1,  // Ceiling, don't remove
//};

////Now using compile time macro to calculate table
PROGMEM const uint8_t voltage_blinks[] = {
	// 0 blinks for (shouldn't happen)
	ADC_val(25),(2<<5)+5,
	ADC_val(26),(2<<5)+6,
	ADC_val(27),(2<<5)+7,
	ADC_val(28),(2<<5)+8,
	ADC_val(29),(2<<5)+9,
	ADC_val(30),(3<<5)+0,
	ADC_val(31),(3<<5)+1,
	ADC_val(32),(3<<5)+2,
	ADC_val(33),(3<<5)+3,
	ADC_val(34),(3<<5)+4,
	ADC_val(35),(3<<5)+5,
	ADC_val(36),(3<<5)+6,
	ADC_val(37),(3<<5)+7,
	ADC_val(38),(3<<5)+8,
	ADC_val(39),(3<<5)+9,
	ADC_val(40),(4<<5)+0,
	ADC_val(41),(4<<5)+1,
	ADC_val(42),(4<<5)+2,
	ADC_val(43),(4<<5)+3,
	ADC_val(44),(4<<5)+4,
	ADC_val(45),(4<<5)+4,
	ADC_val(46),(4<<5)+4,
	255,   (1<<5)+1,  // Ceiling, don't remove
};
//battcheck_Vpt
inline uint8_t battcheck() {
    // Return an composite int, number of "blinks", for approximate battery charge
    // Uses the table above for return values
    // Return value is 3 bits of whole volts and 5 bits of tenths-of-a-volt
    uint8_t i, voltage;
//	ADC_on();
    voltage = get_voltage();
    // figure out how many times to blink
    for (i=0;
         voltage > pgm_read_byte(voltage_blinks + i);
         i += 2) {}
    return pgm_read_byte(voltage_blinks + i + 1);
}

#else  // #ifdef BATTCHECK_VpT
inline uint8_t battcheck() {
    // Return an int, number of "blinks", for approximate battery charge
    // Uses the table above for return values
    uint8_t i, voltage;
//	ADC_on();
    voltage = get_voltage();
    // figure out how many times to blink
    for (i=0;
         voltage > pgm_read_byte(voltage_blinks + i);
         i ++) {}
    return i;
}
#endif  // BATTCHECK_VpT
#endif  // USE_BATTCHECK
#endif  // TK_VOLTAGE_H  

