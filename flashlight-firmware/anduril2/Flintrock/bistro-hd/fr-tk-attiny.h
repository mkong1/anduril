#ifndef FR_ATTINY_H
#define FR_ATTINY_H
/*
 *  Driver layout header. 
 *
 * New layout definition structure 2017, (C) Flintrock (FR).
 * 
 * To work with tk-bistro and other compatible software.
 *
 * Tried to keep it compatible with
 * previous version, but likely some failure there.
 *
 * Inspired by and mostly compatible with Original version by Selene Scriven (C) 2015  
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

//#define ATTINY 13
//#define ATTINY 25

/******************** hardware-specific values **************************/
#if (ATTINY == 13)
    #define F_CPU 4800000UL
	#define _TIMSK_ TIMSK0
	#define _TIMER0_OVF_vect_ TIM0_OVF_vect
    #define EEPSIZE 64
    #define V_REF REFS0
    #define BOGOMIPS 950
// adc_prscl belongs here.  It has nothing to do with layout but does relate some to selected F_CPU above.
   #define ADC_PRSCL   0x06   //  02 is 4, 03, is 8, 04 is 16, 05 is 32, 06 is 64 // adc rate is F_CPU/13/prescale

#elif (ATTINY == 25) || (ATTINY == 45) || (ATTINY ==85)
    // TODO: Use 6.4 MHz instead of 8 MHz?
// FR adds some addresses for use in asm I/O address space
	#define _TIMSK_ TIMSK
	#define _TIMER0_OVF_vect_ TIMER0_OVF_vect
    #define F_CPU 8000000UL
    #define EEPSIZE 128  // not actually correct for 45 and 85 (256) but doesn't matter.
    #define V_REF REFS1    // just defines which bit controls the voltage reference.
	                      // since we don't touch REFS0 2.56V is not available (not very useful anyway).
	#define VCC_REF_PIN 0x00 // ADC ref when using Vcc as a reference really just a 1 bit value. 
	#define _1V_ADC_INPUT 0x0C // ID For one volt reference when used as input (not reference) to ADC 
	// Bogomips appears to be used in tk-delay.h
	// as the number ticks needed for a 1 ms delay where a tick is 4 clock cycles
	// as defined by _delay_loop2:  
	// http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__util__delay__basic_1ga74a94fec42bac9f1ff31fd443d419a6a.html
	// The example there says 65536 corresponds to 261ms for 1Mhz so  250 ticks per ms ie FCPU/4000. 
	// anyway, the POWERSAVE delay re-implements this with a timer clock instead.
	#define BOGOMIPS (F_CPU/4000)
	#define TEMP_CHANNEL 0b00001111
// Mooved adc_prscl here.  It has nothing to do with layout but does relate to selecte F_CPU above.
    #define ADC_PRSCL   0x06   //  02 is 4, 03, is 8, 04 is 16, 05 is 32, 06 is 64 // adc rate is F_CPU/13/prescale
#else
    Hey, you need to define ATTINY.
#endif

// FR's new simplified "auto"-defines, 
// 2017 by Flintrock (C)
// Just define which PWM channels you want, then the extras.
// Also just define everything by PORTB (PB) number.  Lookups work out the other details automatically.

#ifdef CUSTOM_LAYOUT
// A layout for you to customize

/*
 *             ----
 *        PB5               Reset -|1  8|- VCC
 *        PB3    PWM4/OTC/ESWITCH -|2  7|- Voltage/OTSM/ESWITCH    PB2    
 *        PB4                PWM3 -|3  6|- PWM1                    PB1    
 *                            GND -|4  5|- PWM2                    PB0    
 *             ----
 */

// ALL numbers use PB numbers
// First choose which PWM pins to use.  
//#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
//#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
//#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup.  

// ASSIGN PINS FOR EXTRA FEATURES, You can change the pin numbers within reason. They can overlap in some cases.
//#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
//#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
//#define OTSM_PIN       PB2       // PIN to check for voltage shutoff.  Requires #define OTSM in bistro.c to activate.
//#define ESWITCH_PIN    PB3       // Usually instead of CAP_PIN. Requires #define USE_ESWITCH in bistro.c to activate.
                                 // Can double/triple up on the OTSM/Voltage PIN (e: 4 channel driver) by shorting R2 with the e_switch.
								 // Could even pull it to 0.5V and distinguish the two switches with ADC. (not implemented)

//#define STAR2_PIN      PB0 // These have no purpose in the bistro software.
//#define STAR3_PIN      PB4
//#define STAR4_PIN      PB3

/* Note: you can define/uncomment multiple functions for a pin even if some are not used for a particular compilation.
*  Which functions are used are defined in bistro.
*  Ex, if you define ESWITCH and Voltage on the same pin (a legitimate combination anyway), but you define  READ_VOLTAGE_FROM_VCC in bistro,
* bistro will enable a pull-up resistor on the pin to define the open-switch state, in theory at least.  In contrast if READ_VOLTAGE_FROM divider
* is defined, it will disable the pull-up on the voltage pin. Pull-up on OTSM_PIN is also disabled if USE_OTSM is defined in bistro.
*/



#endif // CUSTOM_LAYOUT

//FR's new simplified "auto"-defines
// you can't really pick and choose where to put pwms 
// without changing the initialization code.. so define 4 and just choose which ones you want:
// Also just define everything by PB number, use lookups to work it out below the config settings.

#ifdef TRIPLEDOWN_LAYOUT
/*
 *             ----
 *        PB5        Reset -|1  8|- VCC
 *        PB3  ESWITCH/OTC -|2  7|- Voltage ADC/OTSM/INDICATOR   PB2    
 *        PB4   PWM3 (FET) -|3  6|- PWM1 (6x7135)                PB1    
 *                     GND -|4  5|- PWM2 (1x7135)                PB0    
 *             ----
 */

// ALL numbers use PB numbers
// First choose which PWM pins to use.  
#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       //  PB2,PB3, PB4 are only really valid.  If using VCC it's ignored.
#define INDICATOR_PIN  PB2       // This only gets used if enabled with #define USE_INDICATOR in main config
#define OTSM_PIN       PB2       // PIN to check for voltage shutoff (clicky).  Requires #define OTSM in bistro.c to activate.
                                 // This can be the same as the voltage pin, if the voltage is kept > 1.8V while on.
								 // That generally requires using an LDO as a VCC voltage reference.
#define ESWITCH_PIN    PB3       // Usually instead of CAP_PIN. Requires #define USE_ESWITCH in bistro.c to activate.
                                 // Can double/triple up on the OTSM/Voltage PIN (e: 4 channel driver) by shorting R2 with the e_switch.
								 // Could even pull it to 0.5V and distinguish the two switches with ADC. (not implemented)

//#define STAR2_PIN      PB0 // These have no purpose in the bistro software.
//#define STAR3_PIN      PB4
//#define STAR4_PIN      PB3

/* Note: you can define/uncomment multiple functions for a pin even if some are not used for a particular compilation.
*  Which functions are used are defined in bistro.
*  Ex, if you define ESWITCH and Voltage on the same pin (a legitimate combination anyway), but you define  READ_VOLTAGE_FROM_VCC in bistro,
* bistro will enable a pull-up resistor on the pin to define the open-switch state, in theory at least.  In contrast if READ_VOLTAGE_FROM divider
* is defined, it will disable the pull-up on the voltage pin. Pull-up on OTSM_PIN is also disabled if USE_OTSM is defined in bistro.
*/
 


#endif // TRIPLE_DOWN_LAYOUT

/*********************************************************************/

#ifdef QUADRUPLEDOWN_LAYOUT
// An example of a Quad layout, although the real point here is that these can easily be configured "on the fly" as needed.

/*
 *             ----
 *        PB5        Reset -|1  8|- VCC
 *        PB3    PWM4 Blue -|2  7|- Voltage ADC  PB2    
 *        PB4     PWM3 Red -|3  6|- PWM1 White   PB1    
 *                     GND -|4  5|- PWM2 Green   PB0    
 *             ----
 */

// ALL numbers use PB numbers
// First choose which PWM pins to use.  
#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
//#define CAP_PIN        PB3     // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
#define OTSM_PIN       PB2       // PIN to check for voltage shutoff.  Requires #define OTSM in bistro.c to activate.
#define ESWITCH_PIN    PB2       // Usually instead of CAP_PIN. Requires #define USE_ESWITCH in bistro.c to activate.
                                 // Can double/triple up on the OTSM/Voltage PIN (e: 4 channel driver) by shorting R2 with the e_switch.
								 // Could even pull it to 0.5V and distinguish the two switches with ADC. (not implemented)

//#define STAR2_PIN      PB0     // These have no purpose in the bistro software.
//#define STAR3_PIN      PB4
//#define STAR4_PIN      PB3



#endif // QUADRUPLE_DOWN_LAYOUT

/*********************************************************************/

/*********************************************************************/

//* Q8  Diagram
/*
 *             ----
 *        PB5        Reset -|1  8|- VCC
 *        PB3     e-switch -|2  7|- Voltage         PB2 (stock: unused)    
 *        PB4  IndicatorLED-|3  6|- PWM (FET)       PB1    
 *                     GND -|4  5|- PWM2 (1x7135)   PB0    
 *             ----
 */

#ifdef Q8_LAYOUT
// First choose which PWM pins to use.
#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
//#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
//#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
//#define OTSM_PIN       PB2       // PIN to check for voltage shutoff
#define ESWITCH_PIN    PB3       // usually instead of CAP_PIN but could double up on OTSM PIN
#define INDICATOR_PIN  PB4         // indicator LED.
//#define STAR3_PIN        PB4

// adc rate is F_CPU/13/prescale

#endif  // BLFA6_LAYOUT


#ifdef FET_7135_LAYOUT

/*
 *             ----
 *        PB5        Reset -|1  8|- VCC
 *        PB3          OTC -|2  7|- PWM4 Blue    PB2    
 *        PB4     PWM3 Red -|3  6|- PWM1 White   PB1    
 *                     GND -|4  5|- PWM2 Green   PB0    
 *             ----
 */

// ALL numbers use PB numbers
// First choose which PWM pins to use.
#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
//#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
//#define OTSM_PIN       PB2       // PIN to check for voltage shutoff
//#define ESWITCH_PIN    PB3       // usually instead of CAP_PIN but could double up on OTSM PIN
#define STAR2_PIN PB0             // These have no purpose in the bistro software.
#define STAR3_PIN PB4


#endif //FET_7135_LAYOUT

/*********************************************************************/

#ifdef FERRERO_ROCHER_LAYOUT
/*
 *            ----
 *    Reset -|1  8|- VCC
 * E-switch -|2  7|- Voltage ADC
 *  Red LED -|3  6|- PWM
 *      GND -|4  5|- Green LED
 *            ----
 */
// ALL numbers use PB numbers
// First choose which PWM pins to use.
#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually               (PWM)
#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple  (GREEN)
#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel            (RED)
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
//#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
//#define OTSM_PIN       PB2       // PIN to check for voltage shutoff
#define ESWITCH_PIN    PB3       // usually instead of CAP_PIN but could double up on OTSM PIN
//#define STAR2_PIN        PB0    // These have no purpose in the bistro software.
//#define STAR3_PIN        PB4
//#define STAR4_PIN        PB3

// adc rate is F_CPU/13/prescale

#endif  // FERRERO_ROCHER_LAYOUT


/*********************************************************************/

//* NANJG 105C Diagram
/*
 *             ----
 *        PB5        Reset -|1  8|- VCC
 *        PB3          OTC -|2  7|- Voltage ADC     PB2    
 *        PB4        Star 3-|3  6|- PWM (FET)       PB1    
 *                     GND -|4  5|- PWM2 (1x7135)   PB0    
 *             ----
 */

#ifdef BLFA6_LAYOUT
// First choose which PWM pins to use.
#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
//#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
//#define OTSM_PIN       PB2       // PIN to check for voltage shutoff
//#define ESWITCH_PIN    PB3       // usually instead of CAP_PIN but could double up on OTSM PIN
#define STAR3_PIN        PB4

// adc rate is F_CPU/13/prescale

#endif  // BLFA6_LAYOUT


//* NANJG 105D Convoy/biscotti
/*
 *             ----
 *        PB5        Reset -|1  8|- VCC
 *        PB3              -|2  7|- Voltage ADC     PB2    
 *        PB4              -|3  6|-                 PB1    
 *                     GND -|4  5|- PWM2 (FET)      PB0    
 *             ----
 */

#ifdef NANJG105D_LAYOUT
// First choose which PWM pins to use.
//#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
//#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
//#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
//#define OTSM_PIN       PB2       // PIN to check for voltage shutoff
//#define ESWITCH_PIN    PB3       // usually instead of CAP_PIN but could double up on OTSM PIN
//#define STAR3_PIN        PB4

// adc rate is F_CPU/13/prescale

#endif  // BLFA6_LAYOUT


#ifdef NANJG_LAYOUT // is this even right for anything?
// First choose which PWM pins to use.
#define ENABLE_PWM1           // PWM on pb1  (pin 5)  Single 7135 usually
//#define ENABLE_PWM2           // PWM on pb0  (pin 6)  FET in FET+1 or 7135s in triple
//#define ENABLE_PWM3           // PWM on pb4  (pin 3)  FET in triple channel
//#define ENABLE_PWM4         // PWM on pb3  (pin 2)  Can be used for RGBW setup

// SELECT PINS FOR EXTRA FEATURES, these may overlap in some cases.
//#define CAP_PIN        PB3       // OTC CAP or secondary OTSM CAP
#define VOLTAGE_PIN    PB2       // set to pb number or VCC Only PB2,PB3, PB4 and VCC are really valid
//#define OTSM_PIN       PB2       // PIN to check for voltage shutoff
//#define ESWITCH_PIN    PB3       // usually instead of CAP_PIN but could double up on OTSM PIN
#define STAR2_PIN        PB0        // These have no purpose in the bistro software.
#define STAR3_PIN        PB4
#define STAR4_PIN        PB3

// adc rate is F_CPU/13/prescale

#endif  // NANJG_LAYOUT


/*************************************************END OF LAYOUT DEFINITIONS*********************************************/
/*******************You should not need to edit below here**************************************************************/
// Below are universal associations, mostly to define the PWM OCR associations and adc_mux 
// corresponding to selected pins.
// the idea is these would eventually go after all defines.
// 

#ifdef ENABLE_PWM1
  #define PWM_PIN     PB1     // pin 6, FET PWM
  #define PWM1_LVL     OCR0B   // OCR0B is the output compare register for PB1
#endif

#ifdef ENABLE_PWM2
  #define PWM2_PIN    PB0     // pin 5
  #define PWM2_LVL    OCR0A   // output compare register for PB4
  // for backwards compatibility, not used in bistro-HD
  #define ALT_PWM_PIN PWM2_PIN
  #define ALT_PWM1_LVL PWM2_LVL
#endif

#ifdef ENABLE_PWM3
  #define PWM3_PIN    PB4     // pin 3
  #define PWM3_LVL    OCR1B   // output compare register for PB4
  // for backwards compatibility, not used in bistro-HD
  #define FET_PWM_PIN      
  #define FET_PWM1_LVL PWM3_LVL   
#endif

#ifdef ENABLE_PWM4
#define PWM4_PIN    PB3     // pin 3
#define PWM4_LVL    OCR1A   // output compare register to be used with PB3
                            // This one does NOT attach to PB3, An interrupt will be used instead.
#endif


#if defined(VOLTAGE_PIN) 
//&& VOLTAGE_PIN != VCC_REF_PIN
// Should use_vcc just be determined by this config?
  #define ADC_PIN VOLTAGE_PIN        // SYNONYMS
  #define ADC_DIDR VOLTAGE_PIN       // Digital input disable bit corresponding with PB2, redundant
#endif

#if defined(ADC_PIN) && !defined(VOLTAGE_PIN)
  #define VOLTAGE_PIN ADC_PIN // for backwards compatibility
#endif


#if defined(VOLTAGE_PIN)
// Lookup the adc channel
  #if VOLTAGE_PIN == PB2   // is there a more elegant way?
      #define ADC_CHANNEL 1
  #elif VOLTAGE_PIN == PB4
	  #define ADC_CHANNEL 2
  #elif VOLTAGE_PIN == PB3
      #define ADC_CHANNEL 3
  #elif VOLTAGE_PIN == PB5  // reset pin, probably going to be... interesting to try.
      #define ADC_CHANNEL 0  
  #else
     No ADC channel exists on the selected VOLTAGE_PIN.  Try PB2, PB3, PB4, or PB5 (PB3 on most drivers).
  #endif
#endif


//#ifndef PWM1_LVL

//#endif




#endif  // FR_ATTINY_H
