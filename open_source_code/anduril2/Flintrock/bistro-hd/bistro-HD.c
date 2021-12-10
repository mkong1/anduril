/*
 * "Bistro-HD" firmware  
 * This code runs on a single-channel or dual-channel (or triple or quad) driver (FET+7135)
 * with an attiny13/25/45/85 MCU and several options for measuring off time.
 *
 * Original version Copyright (C) 2015 Selene Scriven, 
 * Modified significantly by Texas Ace (triple mode groups) and 
 * Flintrock (code size, Vcc, OTSM, eswitch, 4-channel, delay-sleep,.. more, see manual) 
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
 *  TA adds many great custom mode groups and nice build scripts
 *  Flintrock Updates:
 *    Implements OTSM "off-time sleep mode" to determine click length based on watchdog timer with mcu powered by CAP.
 *    Adds Vcc inverted voltage read option, with calibration table, allows voltage read of 1S lights without R1/R2 divider hardware.
 *    Implemented 4th PWM channel. (untested, it could even work!)
 *    Now supports E-switches.
 *    Algorithm-generated calibration tables now allow voltage adjustment with one or two configs in fr-calibration.h
 *    Modified tk-attiny.h to simplify layout definitions and make way for more easily-customizable layouts.     
 *    Added BODS to voltage shutdown (and E-switch poweroff) to protect low batteries longer (~4uA for supported chips).
 *    Implements space savings (required for other features):
 *    ... See Changes.txt for details.
 *
 *    see fr-tk-attiny.h for mcu layout.
 *
 * FUSES
 *
 * #fuses (for attin25/45/85): high: 
 *            DD => BOD disabled early revision chips may not work with BOD and OTSM, but disabling it risks corruption.
 *			  DE => BOD set to 1.8V  Non V-version chips are still not specced for this and may corrupt.  late model V chips with this setting is best.
 *            DF => BOD set to 2.7V  Safe on all chips, but may not work well with OTSM, without huge caps.
 *
 *       low  C2, 0ms startup time.  Probably won't work with BODS capable late model chips, but should improve OTSM a little if not using one anyway. 
 *            D2, 4ms  Testing finds this seems to work well on attiny25 BODS capable chip, datasheet maybe implies 64 is safer (not clear and hardware dependent). 
              E2  64ms startup.  Will probably consume more power during off clicks and might break click timing.
 *      
 *        Ext: 0xff
 *       Tested with high:DE low:D2, Ext 0xff on non-V attiny25, but it's probably not spec compliant with that chip.
 *
 *      For more details on these settings
 *      http://www.engbedded.com/cgi-bin/fcx.cgi?P_PREV=ATtiny25&P=ATtiny25&M_LOW_0x3F=0x12&M_HIGH_0x07=0x06&M_HIGH_0x20=0x00&B_SPIEN=P&B_SUT0=P&B_CKSEL3=P&B_CKSEL2=P&B_CKSEL0=P&B_BODLEVEL0=P&V_LOW=E2&V_HIGH=DE&V_EXTENDED=FF
 *
 *
 * CALIBRATION
 *   FR's new method Just adjust parameters in fr-calibration.h
 *   Now uses something between fully calculated and fully measured method.
 *   You can probably get close using calculations in the comments, and 
 *   you can make simple adjustments from there to get a good result.
 *   Or, use the battcheck builds with a full baterry (4.2V) to get a 1-point calibration.
 *
 * 
 */



#ifndef ATTINY
/************ Choose your MCU here, or override in Makefile or build script*********/
// The build script will override choices here

//  choices are now 13, 25, 45, and 85.  Yes, 45 and 85 are now different

#define ATTINY 85
//#define ATTINY 25
//#define ATTINY 45  // yes these are different now, hopefully only in ways we already know.
//#define ATTINY 85  //   bust specifically they have a 2 byte stack pointer that OTSM accesses.
#endif

#ifndef CONFIG_FILE_H
/**************Select config file here or in build script*****************/


///// Use the default CONFIG FILE? /////
// This one will be maintained with all the latest configs available even if commented out.
// It should be the template for creating new custom configs.
//#define CONFIG_FILE_H "configs/config_default.h"

///Or select alternative configuration file, last one wins.///
//#define CONFIG_FILE_H "configs/config_custom-HD.h"
//#define CONFIG_FILE_H "configs/config_BLFA6_EMU-HD.h"
//#define CONFIG_FILE_H "configs/config_biscotti-HD.h"
//#define CONFIG_FILE_H "configs/config_tripledown-HD.h"
//#define CONFIG_FILE_H "configs/config_classic-HD.h"
//#define CONFIG_FILE_H "configs/config_TAv1-OTC-HD.h"
//#define CONFIG_FILE_H "configs/config_TAv1-OTSM-HD.h"
//#define CONFIG_FILE_H "configs/config_TAv1-OTSM-LDO-HD.h"
//#define CONFIG_FILE_H "configs/config_eswitch-TA-HD.h"
//#define CONFIG_FILE_H "configs/config_dual-switch-TA-HD.h"
//#define CONFIG_FILE_H "configs/config_dual-switch-noinit-TA-HD.h"
//#define CONFIG_FILE_H "configs/config_dual-switch-dumbclick-TA-HD.h"
//#define CONFIG_FILE_H "configs/config_dual-switch-turboclick-TA-HD.h"
//#define CONFIG_FILE_H "configs/config_4channel-dual-switch-HD.h"
#define CONFIG_FILE_H "configs/config_eswitch-Q8-fetplusone-HD.h"

//Make it a battcheck build? 
//#define VOLTAGE_CAL

//Note: the modegroups file is specified in the config file, since they must be compatible.
#endif


/**********************************************************************************
**********************END OF CONFIGURATION*****************************************
***********************************************************************************/

// Now a bunch of stuff to process configs, setup macros, defaults, etc:

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" // this is here for actual_level.
                                               // it's not uninitialized, gcc just can't figure it out.
#include <avr/pgmspace.h>
//#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay_basic.h>
//#include <string.h>

#include CONFIG_FILE_H // this is primary configuration file.


// This is used to simplify the toggle function
// eliminating mode_override, using this threshold instead:
#define MINIMUM_OVERRIDE_MODE 245  // DO NOT EDIT.  DO NOT DEFINE ANY STROBES HIGER OR EQUAL TO THIS.

#define GROUP_SELECT_MODE 254
#define TEMP_CAL_MODE 253

/******* Define strobe magic numbers**** *********************/
//Now separate from actually selecting which ones are used.

#define BATTCHECK        244      // Convenience code for battery check mode
//mode codes for strobes, must be less than MINIMUM_OVERRIDE_MODE
#define BIKING_STROBE    243     // Single flash biking strobe mode
#define FULL_BIKING_STROBE     // Stutter bike strobe, uncomment to enable
#define POLICE_STROBE    242     // Dual mode alternating strobe
#define RANDOM_STROBE    241
#define SOS              240
#define STROBE_8HZ       239
#define STROBE_10HZ      238
#define STROBE_16HZ      237
#define STROBE_OLD_MOVIE  236
#define STROBE_CREEPY    235     // Creepy strobe mode, or really cool if you have a bunch of friends around
#define RAMP             234     //Ramping "strobe"


//*************** Defaults needed before modegroups.h, but NOT yet one's overring modegroups.h ***********/


// FULL_BIKING_STROBE modifies BIKING_STROBE so BIKING_STROBE must be defined.
#if defined(FULL_BIKING_STROBE) && !defined(BIKING_STROBE)
 #define BIKING_STROBE
#endif

// If medium clicks are disabled, don't define hidden modes:
// must come before modegroups.h
#if !defined(OFFTIM3)
  #define HIDDENMODES       // No hiddenmodes, defined as empty.
#endif

#include "fr-tk-attiny.h" // should be compatible with old ones, but it might not be, so renamed it.

// Read the modegroups configuration file:
#include MODEGROUPS_H

//*********Now can define defaults including ones to override things in MODEGROUPS_H ***********

// There's really no reason not to use LOOP_TOGGLES NOW, probably will remove the old way in the future:
#ifdef NO_LOOP_TOGGLES
  #undef LOOP_TOGGLES
#else
  #define LOOP_TOGGLES  
#endif

// Inlining the strobe function saves a bunch of bytes
// on builds that use only one strobe.
#ifndef INLINE_STROBE
  #define INLINE_STROBE  //inline keyword for strobe function.
#endif

// Enable the e-switch lockout menu option 
#if defined(USE_ESWITCH) && (OTSM_PIN != ESWITCH_PIN) && defined(USE_ESWITCH_LOCKOUT_TOGGLE)
 #define USE_LOCKSWITCH
#endif


// Apply defaults for what temp regulation modes to use
// Some historical baggage here probably:
#if defined(TEMPERATURE_MON)
    #ifdef TURBO_TIMEOUT
       #error "You cannot define TURBO_TIMEOUT and TEMPERATURE_MON together"
       #error "Consider usining TEMP_STEP_DOWN instead"
    #endif
  #ifdef TEMP_STEP_DOWN // TEMP method already set, do nothing.
    #ifndef MINIMUM_TURBO_TIME
      #define MINIMUM_TURBO_TIME 10 // default minimum turbo time for TEMP_STEP_DOWN.
    #endif
  #else	                         // else set the default
    #define CLASSIC_TEMPERATURE_MON
  #endif 
#endif

// click parsing is more reliable with faster sleep cycles.
// if a pin is changing during read, it will be read again on next sleep cycle
// before the click time threshold changes.
//
//But what about dual OTSM/ESWITCH? Well for now that also disables debounce 
// It must unless they are on distinguishable pins at least since debounce will break OTSM.
// And without debounce delays there won't be missed clicks.
// If OTSM+e-switch still needs debounce, it just won't work on same pin
// and will need to apply both debounce and sleep time based on pin detection in that case.
// will know after testing.
#if defined(USE_ESWITCH) && (!defined USE_OTSM)
  #undef SLEEP_TIME_EXPONENT 
  #define SLEEP_TIME_EXPONENT 1 // 32 ms
#endif

#define OWN_DELAY           // Don't use stock delay functions.
// This is actually not optional anymore. since built in delay requires a compile time constant and strobe()
// cannot provide that.

#define USE_DELAY_S         // Also use _delay_s(), not just _delay_ms()

// set a default value for the mode to step down to
// after timeout or overheat, if it's not already configured.
#ifndef TURBO_STEPDOWN
 #define TURBO_STEPDOWN RAMP_SIZE/2
#endif


// how many ms to sleep between voltage/temp/stepdown checks.
#define LOOP_SLEEP 500 

// Won't compile with -O0 (sometimes useful for debugging) without following:
#ifndef __OPTIMIZE__
#define inline
#endif


#define Q(x) #x
#define Quote(x) Q(x)  // for placing expanded defines into inline assembly with quotes around them.
// the double expansion is a weird but necessary quirk. Found this on SO I think.

//#define IDENT(x) x
//#define PATH(x,y) Quote(IDENT(x)IDENT(y))
//#include PATH(CONFIG_DIR,CONFIG_FILE_H)


// ATTINY13 has no temperature sensor:
#if (ATTINY==13)  && defined (TEMPERATURE_MON)
  #undef TEMPERATURE_MON
  #warning disabling TEMPERATURE_MON for attiny13
#endif

// Holds the present modegroup including moon and hidden modes:
uint8_t modes[MAX_MODES + sizeof(hiddenmodes)];  // make sure this is long enough

//Initialize the level ramps
//If both the ramp is defined and the harware channel is enabled, we're using the level:
#if defined(PWM1_LVL)&&defined(RAMP_PWM1)
  PROGMEM const uint8_t ramp_pwm1[] = { RAMP_PWM1 };
  #define USE_PWM1
#endif
#if defined(PWM2_LVL)&&defined(RAMP_PWM2)
  PROGMEM const uint8_t ramp_pwm2[] = { RAMP_PWM2 };
  #define USE_PWM2
#endif
#if defined(PWM3_LVL)&&defined(RAMP_PWM3)
  PROGMEM const uint8_t ramp_pwm3[] = { RAMP_PWM3 };
  #define USE_PWM3
#endif
#if defined(PWM4_LVL)&&defined(RAMP_PWM4)
  PROGMEM const uint8_t ramp_pwm4[] = { RAMP_PWM4 };
  #define USE_PWM4
#endif


// Calibrate voltage and OTC in this file:
#include "fr-calibration.h"

//USE timed sleeps to save power?  Improves moon mode battery life and OTSM performance:
#ifdef POWERSAVE
#include "fr-delay.h" // must include them in this order.
#define _delay_ms _delay_sleep_ms
#define _delay_s _delay_sleep_s
#else
#include "tk-delay.h"
#endif


#ifdef USE_RANDOM_STROBE
#include "tk-random.h"
#endif

// The voltage and teperature ADC functions:
#include "tk-voltage.h"

void blink_value(uint8_t value); // declare early so we can use it anywhere

///////////////////////REGISTER VARIABLES///////////////////////////////////
// make it a register variable.  Saves many i/o instructions.
// but could change as code evolves.  Worth testing what works best again later.

// ************NOTE global_counter is now reserved as r2 in tk-delay.h. (commented out now actually)**********
// safe registers for avr-gcc are r2 through r7 (always restored by callee and never passed).
//  r8 through r15 MIGHT be safe (not clear) but are certainly not safe for use in interrupts.

register uint8_t  mode_idx asm("r6");           // current or last-used mode number
// stragely, redeclaring this saves 10 bytes, should be illegal no?  And why? -FR
// Doing only the second declaration however loses about 20~30 bytes, so is not identical to re-declaring.
//uint8_t mode_idx;
register uint8_t eepos asm("r7");  // eeprom read/write position for wear-leveling of mode_idx save.
//uint8_t eepos;

// a counter to see how long the light was off during a click
// actually counts the number of watchdog wakes while off.
// Now translate OTC readings to this variable as well.
#if defined(USE_OTSM) || defined(USE_ESWITCH) || defined(USE_OTC)
//Reserving a register for this allows checking it in re-entrant interrupt without needing register maintenance
// as a side effect it saves several in/out operations that save more space.
register uint8_t wake_count asm  ("r5");
#endif
/////////////////////END REGISTER VARIABLE/////////////////////////////////

// fast_presses
// counter for entering config mode, remembers click count with tk noinit trick.
// If No OTC or OTSM is used it also detects off time by RAM decay
// However that's very unreliable with one byte.  
// FR adds USE_SAFE_PRESSES options that compares 4 bytes to detect RAM decay
// if 0 is equal probability to 1 (maybe not quite true) this gives a safety factor of 2^24 so about 16 million.
// (needs to be remembered while off, but only for up to half a second)

// Redundancy bytes to verify RAM integrety for fast_presses variable kept while off:
//  3 is about 1 in 3000 safety.  4 is about 1 in 70000.  2 is 1 in a hundred maybe. (0's and 1's aren't equally probable)
#if !(defined(USE_OTC)||defined(USE_OTSM))&& defined(USE_SAFE_PRESSES)
 #define SAFE_PRESSES
#endif
#ifndef N_SAFE_BYTES
 #define N_SAFE_BYTES 4
#endif

#ifdef SAFE_PRESSES
  uint8_t fast_presses_array[N_SAFE_BYTES] __attribute__ ((section (".noinit")));
  #define fast_presses fast_presses_array[0]
#else // using fast presses for click timing, use redundancy to check RAM decay:
  uint8_t fast_presses __attribute__ ((section (".noinit")));
#endif


// total length of current mode group's array
uint8_t mode_cnt __attribute__ ((section (".noinit")));
// number of regular non-hidden modes in current mode group
uint8_t solid_modes __attribute__ ((section (".noinit")));
// number of hidden modes in the current mode
// (hardcoded because both groups have the same hidden modes)
//uint8_t hidden_modes = NUM_HIDDEN;  // this is never used

// macros for configuring the timer:
#define FAST 0x03           // fast PWM
#define PHASE 0x01         // phase-correct PWM

/******** FR makes these all aray variables now ************************/
// can now loop to save, loop to load, and loop to toggle
// saves about 30 subtroutine calls, so a bunch of flash space.
/******Be careful here, this is messy and easy to break there may be a better way**********/

// define the order of save variables
// Yes, they could just be put in the array aliases below
// This a messy way to allow not allocating a couple of bytes later.
// During initialization we can check by name if each toggle is above or below 
//  the highest one used, and skip the initialization.
// This same issue is responsible for all the element counting mess below.
// If you change these, you must change the ntoggles defines below too
#define muggle_mode_IDX     1 // start at 1 to match EEPSIZE math
#define memory_IDX          2 // these all get saved at EEPSIZE-N
#define enable_moon_IDX     3
#define reverse_modes_IDX   4
#define MODEGROUP_IDX       5  //GROUP_SELECT don't need to define mod_override entries here, just leave a space.
#define offtim3_IDX         6
#define TEMP_CAL_IDX        7 // TEMPERATURE_MON
#define firstboot_IDX       8
#define lockswitch_IDX      9  // don't forget to update n_saves above.
#define TOTAL_TOGGLES    9         // all saves above are toggle variables

// assign aliases for array entries for toggle variables.
#define OPT_firstboot (EEPSIZE-1-firstboot_IDX) // only one that uses individual reads.
#define muggle_mode   OPT_array[muggle_mode_IDX] // start at 1 to match EEPSIZE math
#define memory        OPT_array[memory_IDX] // these all get saved at EEPSIZE-N
#define enable_moon   OPT_array[enable_moon_IDX]
#define reverse_modes OPT_array[reverse_modes_IDX]
// next memory element will serve as a dummy toggle variable for all mode-override toggles.
#define mode_override OPT_array[MODEGROUP_IDX] // 
#define offtim3       OPT_array[offtim3_IDX]
//#define TEMPERATURE_MON_IDX OPT_array[7] // TEMPERATURE_MON, doesn't need to be defined,
                                           // re-uses mode_override toggle in old method
                                           // and has no toggle at all in new method.
#define firstboot     OPT_array[firstboot_IDX]  
#define lockswitch    OPT_array[lockswitch_IDX]  // don't forget to update n_saves above.

// this allows to save a few bytes by not initializing unneeded toggles at end of array:
// hmm, heck of a bunch of mess just to save a few bytes though.
#if defined(USE_ESWITCH_LOCKOUT_TOGGLE)
  #define n_toggles (TOTAL_TOGGLES)   // number  of options to save/restore
#elif defined(USE_FIRSTBOOT)
  #define n_toggles (TOTAL_TOGGLES - 1)  // number  of options to save/restore
#elif defined(TEMPERATURE_MON)
  #define n_toggles (TOTAL_TOGGLES - 2)  // number  of options to save/restore
#elif defined(OFFTIM3)
  #define n_toggles (TOTAL_TOGGLES - 3)  // number  of options to save/restore
#else
  #define n_toggles (TOTAL_TOGGLES - 4)  
#endif  // we must have the MODEGROUP menu, so can't go below here.

// defined non-toggles saves:
#define modegroup     OPT_array[n_toggles+1]
#define maxtemp       OPT_array[n_toggles+2]
  #define n_extra_saves 2   //  non-toggle saves

// NEXT_LOCKOUT enables check to disable next mode on next press under certain conditions.
// It's used by turbo step-down functions.


#define n_saves (n_toggles + n_extra_saves)
  uint8_t OPT_array[n_saves+1] __attribute__ ((section (".noinit")));
#ifndef LOOP_TOGGLES
  #define final_toggles  (-1) //disable loop toggles.
                       // redefining n_toggles breaks modegroup macro and fixing that situation seems ugly.
#endif
//#define _configs      OPT_array[12]

uint8_t overrides[n_toggles+1]; // will hold values override mode_idx if any for each toggle.

/*********End of save variable definitions***************/

//#if defined(USE_TURBO_TIMEOUT) || (defined(TEMPERATURE_MON) && defined(TEMP_STEP_DOWN))
//#define NEXT_LOCKOUT  1
//#else
//#define NEXT_LOCKOUT  0
//#endif
#if defined(OFFTIM3)&&defined(USE_ESWITCH)
  #if defined(DUMB_CLICK)||defined(TURBO_CLICK)
       #define NEXT_LOCKOUT 1 // compile in support to lockout next mode
  #endif
#endif
#if ( defined(TEMPERATURE_MON)&&defined(TEMP_STEP_DOWN) )\
                  || defined(USE_TURBO_TIMEOUT) 
       #define NEXT_LOCKOUT 1 // compile in support to lockout next mode
#endif
#ifndef NEXT_LOCKOUT
   #define NEXT_LOCKOUT 0
// clear_presses always sets to 0.
   #define clear_presses_value 0
#else
// use clear_presses value as a controllable lockout toggle:
// needs to be reset on re-entry to main from OTSM sleep, so no need to initialize it.
   uint8_t clear_presses_value __attribute__ ((section (".noinit")));
#endif

// improves transparency


#define DISABLE_NEXT 255 // Magic value for fast_presses that disables next mode on next boot
// this define does NOT disable next mode.  It just defines a signaling value
// for certain functions to use when those functions need to disable it for one click.

//#define POWER_LOCKOUT_CHECK 254 // Magic value for fast_presses that locks out power on next boot.

// Magic number stored in eeprom on first boot. 
// If it's NOT set, it's a first boot, so we don't read configs.
#define FIRSTBOOT 0b01010101

//define PORTB IO address for asm use:
#define PORTB_IO  _SFR_IO_ADDR(PORTB)

// define a bit field in an I/O register with sbi/cbi support, for efficient boolean manipulation:

typedef struct
{
    unsigned char bit0:1;
    unsigned char bit1:1;
    unsigned char bit2:1;
    unsigned char bit3:1;
    unsigned char bit4:1;
    unsigned char bit5:1;
    unsigned char bit6:1;
    unsigned char bit7:1;
}io_reg;

/***************LETS RESERVE GPIOR0 FOR LOCAL VARIABLE USE ********************/
// This seemed more useful at one point than it maybe actually is, but it works.
// Still useful though because checking a gpio bit can be done in an interrupt without clobbering working registers.
//   .. saving bytes and speeding up the interrupt.
//define a global bitfield in lower 32 IO space with in/out sbi/cbi access
// unfortunately, attiny13 has no GPIORs, but can't handle OTSM or pwm4 anyway.

        #define gbit             *((volatile uint8_t*)_SFR_MEM_ADDR(GPIOR2)) // pointer to whole status byte.
        #define gbit_pwm4zero			((volatile io_reg*)_SFR_MEM_ADDR(GPIOR2))->bit0 // pwm4 disable toggle.
        #define gbit_pinchange	     	((volatile io_reg*)_SFR_MEM_ADDR(GPIOR2))->bit1  // flags interrupt type without 
                                                                                         // need to clobber a register

//The bitfield variables work in C, but raw addresses and bit shifts can help for asm:
        #define gbit_pwm4zero_bit 0 // This gets used for inline asm in PWM ISR.


// Convert wake_time configurations to number of wake_counts
// This depends on the wake frequency defined by SLEEP_TIME_EXPONENT
#if defined(USE_OTSM) || defined(USE_ESWITCH) || defined(USE_OTC)
// math done by compiler, not runtime:
// plus 1 because there's always an extra un-timed pin-change wake
//   ex:With 1/4 second wakes anything over 0.5s is 3 or more wakes, not two.
//   The comparison is done as >=.  If wake_count_short was two, an 0.26 s press
//   would register as medium, not short.

#if defined(USE_OTC) && !defined(USE_ESWITCH)
  #define wake_count_med (255-CAP_MED)
  #define wake_count_short (255-CAP_SHORT)
#else
// updated with two components, first 4 wakecounts are double speed.
//#define wake_interval ( (uint16_t)1<<(6-SLEEP_TIME_EXPONENT) )
//#define wake_count_short (uint8_t)(1+((double)wake_time_short)*((uint16_t)1<<(6-SLEEP_TIME_EXPONENT) ) 

#define wake_count_short (uint8_t)(1+((double)wake_time_short)*((uint16_t)1<<(6-SLEEP_TIME_EXPONENT)))
#define wake_count_med (uint8_t)(1+((double)wake_time_med)*((uint16_t)1<<(6-SLEEP_TIME_EXPONENT)))

//#define wake_count_short (uint8_t)(1+((double)wake_time_short)*((uint16_t)1<<(6-SLEEP_TIME_EXPONENT)))
//#define wake_count_med (uint8_t)(1+((double)wake_time_med)*((uint16_t)1<<(6-SLEEP_TIME_EXPONENT)))

#endif
#endif


/**************WARN ON SOME BROKEN COMBINATIONS******************/
#ifndef ATTINY
 #error You must define ATTINY inbistro.c or in the build script/Makefile.
#endif

#ifndef CONFIG_FILE_H
 #error You must define CONFIG_FILE_H inbistro.c or in the build script/Makefile.
#endif

#ifndef MODEGROUPS_H
  #error You must define MODEGROUPS_H in the configuration file.
#endif

#ifdef USE_ESWITCH
   #ifndef ESWITCH_PIN
    #error You must define an ESWITCH_PIN in fr-tk-attiny.h or undefine USE_ESWITCH
   #endif
#endif

#ifdef USE_OTC
   #ifndef CAP_PIN
    #error You must define a CAP_PIN in fr-tk-attiny.h or undefine USE_OTC
   #endif
   #if defined(READ_VOLTAGE_FROM_DIVIDER)&& (CAP_PIN==VOLTAGE_PIN)
    #error You cannot read voltage from the OTC pin, disable one or change the layout.
   #endif
#endif

#ifdef USE_OTSM
  #ifndef OTSM_PIN
    #error You must define an OTSM_PIN in fr-tk-attiny.h or undefine USE_OTSM
  #endif
#endif

#if ATTINY==13 && defined(READ_VOLTAGE_FROM_VCC)
  #error You cannot use READ_VOLTAGE_FROM_VCC with the attiny 13.
#endif

#if ATTINY==13 && (defined(PWM3_LVL)||defined(PWM4_LVL))
  #error attiny13 does not support PWM3 or PWM4, modify layout or select a new one.
#endif

#if defined(USE_INDICATOR)&&!defined(INDICATOR_PIN)
  #error You have defined USE_INDICATOR without defining  INDICATOR_PIN
#endif

#if (defined(READ_VOLTAGE_FROM_DIVIDER)||defined(USE_OTSM))&&!defined(VOLTAGE_PIN)
  #error You have defined READ_VOLTAGE_FROM_DIVIDER or USE_OTSM without defining VOLTAGE_PIN
#endif

#if defined(VOLTAGE_MON)&&!(defined(READ_VOLTAGE_FROM_VCC)||defined(READ_VOLTAGE_FROM_DIVIDER))
  #error You must define READ_VOLTAGE_FROM_VCC or READ_VOLTAGE_FROM_DIVIDER when VOLTAGE_MON is defined
#endif

#if defined(VOLTAGE_MON)&&defined(READ_VOLTAGE_FROM_VCC)&&defined(READ_VOLTAGE_FROM_DIVIDER)
  #error You cannot define READ_VOLTAGE_FROM_VCC at the same time as READ_VOLTAGE_FROM_DIVIDER
#endif

#if defined(READ_VOLTAGE_FROM_DIVIDER)&&!defined(REFERENCE_DIVIDER_READS_TO_VCC)&&defined(USE_OTSM)&&(OTSM_PIN==VOLTAGE_PIN)&&defined(VOLTAGE_MON)
  #error You cannot use 1.1V reference to read voltage on the same pin as the OTSM.  OTSM power-on voltage must be >1.8V.
  #error FOR LDO builds use REFERENCE_DIVIDER_READS_TO_VCC instead and set divider resistors to provide > 1.8V on-voltage to voltage pin.
  #error For 1S non-LDO or 1-S 5.0V LDO (LDO below regulation) lights you can use READ_VOLTAGE_FROM_VCC instead
#endif

#if defined(READ_VOLTAGE_FROM_DIVIDER)&&!defined(REFERENCE_DIVIDER_READS_TO_VCC)&&defined(USE_ESWITCH)&&(ESWITCH_PIN==VOLTAGE_PIN)&&defined(VOLTAGE_MON)
  #error You cannot use 1.1V reference to read voltage on the same pin as the e-switch.  E-switch on-voltage must be >1.8V.
  #error Use REFERENCE_DIVIDER_READS_TO_VCC instead and set divider resistors to provide > 1.8V on-voltage to voltage pin.
  #error For 1S non-LDO or 1-S 5.0V-LDO lights you can use READ_VOLTAGE_FROM_VCC instead.
#endif

#if !defined(USE_OTC)&&!defined(USE_OTSM)&&!defined(USE_ESWITCH)&&defined(OFFTIM3)
  #error You must define either USE_OTC or USE_OTSM or USE_ESWITCH when OFFTIM3 is defined.
#endif

/* If the eswitch is used it's impossible to know if there's also a clicky switch
So this warning doesn't catch everything */
#if !defined(USE_OTC)&&!defined(USE_OTSM)&&!defined(USE_SAFE_PRESSES)&&!defined(USE_ESWITCH)
  #warning It is highly recommend to define USE_SAFE_PRESSES for capacitor-less off-time detection
#endif

#if defined(USE_INDICATOR) // check for conflicts
  #if defined(USE_OTSM)&&(OTSM_PIN==INDICATOR_PIN)
   #error "You cannot use OTSM and an indicator on the same pin.  Disable one, or change the layout."
  #endif
  #if defined(USE_ESWITCH)&&(ESWITCH_PIN==INDICATOR_PIN)
   #error "You cannot use an ESWITCH and an indicator on the same pin.  Disable one, or change the layout."
  #endif
  #if defined(READ_VOLTAGE_FROM_DIVIDER)&&(VOLTAGE_PIN==INDICATOR_PIN)
   #error "You cannot use the indicator and voltage reads on the same pin.  Disable one or change the layout."
  #endif
  #if defined(USE_OTC)&&(CAP_PIN==INDICATOR_PIN)
   #error "You cannot use OTC and an indicator on the same PIN, and the two options don't make sense together."  
   #error "Disable one or change the layout."
  #endif
#endif



  
// Not really used
//#define DETECT_ONE_S  // auto-detect 1s vs multi-s light make prev.
// requiers 5V LDO for multi-s light and will use VCC for 1S.
// incomplete, needs calibration table switching, costs memory, probably 160 bytes when done.


/****************************END OF PREPROCESSOR DEFINES**************************/

// check_stars: including from BLFA6
// Configure options based on stars
// 0 being low for soldered, 1 for pulled-up for not soldered
#ifdef USE_STARS
inline void check_stars() { 
    #if 0  // not implemented and broken, STAR2_PIN is used for second PWM channel
    // Moon
    // enable moon mode?
    if ((PINB & (1 << STAR2_PIN)) == 0) {
        modes[mode_cnt++] = MODE_MOON;
    }
    #endif
    #if 0  // broken. Mode order not as important as mem/no-mem
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
#endif

#ifdef SAFE_PRESSES 
// FR 2017 adds redundancy to check RAM decay without flakiness.
// Expand fast_presses to mulitple copies.  Set and increment all.
// Check that all are equal to see if RAM has is still intact (short click) and value still valid.
void clear_presses() {
    for (uint8_t i=0;i<N_SAFE_BYTES;++i){
        // if NEXT_LOCKOUT is 0 this just gets optimized to =0
        // else we only clear last 7 bits, preserving lockout bit
           fast_presses_array[i]=clear_presses_value;
    }
}


inline uint8_t check_presses(){
    uint8_t i=N_SAFE_BYTES-1;
    while (i&&(fast_presses_array[i]==fast_presses_array[i-1])){
        --i;
    }
    return !i; // if we got to 0, they're all equal
}
// increment all fast_presses variables by 1
inline void inc_presses(){
    for (uint8_t i=0;i<N_SAFE_BYTES;++i){
        ++fast_presses_array[i];
    }
}

// probably unused, and too direct now:
inline void set_presses(uint8_t val){
    for (uint8_t i=0;i<N_SAFE_BYTES;++i){
        fast_presses_array[i]=val;
    }
}

#else 
  #if NEXT_LOCKOUT == 0
 // determines if a short click was done
 // The theory is for long clicks, ram decays and random chance will produce a higher value.
 // FR finds this theory suspect, but this is all 
 // irrelevant when safe_presses is used or noinit is not used.
    #define check_presses() (fast_presses < 0x20)
  #else //if next mode lockout is used, we need to pass the short-press test
       // when disable_next is set. Disable_next is achieved through the short press logic.
    #define check_presses() ( fast_presses < 0x20 || fast_presses == DISABLE_NEXT )
  #endif
  inline void clear_presses() {fast_presses=clear_presses_value;}
  #define inc_presses() fast_presses=(fast_presses+1)& 0x1f	
  #define set_presses(val) fast_presses=(val)	
#endif

// Sets the reset value of clear_presses to a magic number
//  that signals to skip next mode on startup.
//  Directly setting fast_presses would interfere with clear_presses in main loop.
// So just let clear_presses do it, turns the problem into the solution.
#if NEXT_LOCKOUT == 1
inline void disable_next() {
    clear_presses_value=DISABLE_NEXT;
}
#endif

// Sets the reset value of clear_presses to a magic number
//  that signals e-switch power lockout
//#if POWER_LOCKOUT == 1
//inline void disable_next() {
    //clear_presses_value=DISABLE_NEXT;
//}
//#endif


//initial_values(): Intialize default configuration values
inline void initial_values() {//FR 2017
    //for (uint8_t i=1;i<n_toggles+1;i++){ // disable all toggles, enable one by one.
        //OPT_array[i]=255;
    //}
     // configure initial values here, 255 disables the toggle... doesn't save space.
    #ifdef USE_FIRSTBOOT
     firstboot = FIRSTBOOT;               // detect initial boot or factory reset

// else we're not using it, but if it's in the range of used toggles  we have to disable it:
    #elif firstboot_IDX <= final_toggles
     firstboot=255;                       // disables menu toggle
    #endif

    #ifdef USE_MOON
     enable_moon = INIT_ENABLE_MOON;      // Should we add moon to the set of modes?
    #elif enable_moon_IDX <= final_toggles
     enable_moon=255;                   //disable menu toggle,	 
    #endif

    #if defined(USE_REVERSE_MODES) && defined(OFFTIM3)
     reverse_modes = INIT_REVERSE_MODES;  // flip the mode order?
    #elif reverse_modes_IDX <= final_toggles
     reverse_modes=255;                   //disable menu toggle,
    #endif
     memory = INIT_MEMORY;                // mode memory, or not

    #ifdef OFFTIM3
     offtim3 = INIT_OFFTIM3;              // enable medium-press?
    #elif offtim3_IDX <= final_toggles
     offtim3=255;                         //disable menu toggle, 
    #endif

    #ifdef USE_MUGGLE_MODE
     muggle_mode = INIT_MUGGLE_MODE;      // simple mode designed for muggles
    #elif muggle_mode_IDX <= final_toggles
     muggle_mode=255;                     //disable menu toggle,
    #endif

    #ifdef USE_LOCKSWITCH
     lockswitch=INIT_LOCKSWITCH;          // E-swtich enabled.
    #elif lockswitch_IDX <= final_toggles
     lockswitch=255;
    #endif


// Handle non-toggle saves.
    modegroup = INIT_MODEGROUP;          // which mode group will be default, mode groups below start at zero, select the mode group you want and subtract one from the number to get it by defualt here
    #ifdef TEMPERATURE_MON
    maxtemp = INIT_MAXTEMP;              // temperature step-down threshold, each number is roughly equal to 4c degrees
    #endif

// Some really uncharacteristically un-agressive (poor even) compiler optimizing 
//  means it saves bytes to do all the overrides initializations after 
//  the OPT_ARRAY initializations, to free up the best registers.
// required turning this upside down, and making it confusing.
    #ifdef LOOP_TOGGLES
      // If we're not using the temp cal mode:
       #if !( defined(TEMPERATURE_MON)&&defined(USE_TEMP_CAL) )
         // and if the temp cal mode is within the range of used toggles
         #if TEMP_CAL_IDX <= final_toggles
            // then disable the toggle:
           OPT_array[7]=255; 
         #endif
       #else // we're using temp cal mode, set the override mode for menu to toggle into: 
        overrides[7]=TEMP_CAL_MODE; // enable temp cal mode menu
       #endif
       // We always need the group select mode toggle:
       overrides[5]=GROUP_SELECT_MODE; 
    #endif

    #ifdef USE_STARS
      check_stars();
    #endif
}

// save the mode_idx variable:
void save_mode() {  // save the current mode index (with wear leveling)
    uint8_t oldpos=eepos;

    eepos = (eepos+1) & ((EEPSIZE/2)-1);  // wear leveling, use next cell

    eeprom_write_byte((uint8_t *)((uint16_t)eepos),mode_idx);  // save current state
    eeprom_write_byte((uint8_t *)((uint16_t)oldpos), 0xff);     // erase old state
}


#define gssWRITE 1
#define gssREAD 0

// save and restore all other configuration variables:
void getsetstate(uint8_t rw){// double up the function to save space.
   uint8_t i; 
   i=1;
   do{
       if (rw==gssREAD){// conditional inside loop for space.
         // -1 isn't needed but could help avoid bugs later
         //  and must match with firstboot address define above.
           OPT_array[i]=eeprom_read_byte((uint8_t *)(EEPSIZE-i-1));
       } else {
           eeprom_write_byte((uint8_t *)(EEPSIZE-i-1),OPT_array[i]);
       }
       i++;
    } while(i<=n_saves); 
}

// pretty obvious, just saves the mode and all other variables:
void save_state() {  // central method for writing complete state
    save_mode();
    getsetstate(gssWRITE);
}



//Recalls all saved variables, generally on startup, 
//unless this is the firt boot after flash or reset, then save the defaults.  
inline void restore_state() {
#ifdef USE_FIRSTBOOT
    uint8_t eep;
     //check if this is the first time we have powered on
    eep = eeprom_read_byte((uint8_t *)OPT_firstboot);
    if (eep != FIRSTBOOT) {// confusing: if eep = FIRSTBOOT actually means the first boot already happened
                           // so eep != FIRSTBOOT means this IS the first boot.
        // not much to do; the defaults should already be set
        // while defining the variables above
        mode_idx=0;
        save_state(); // this will save FIRSTBOOT to the OPT_firstboot byte.
        return;
    }
#endif
    eepos=0; 
    do { // Read until we get a result. Tightened this up slightly -FR
        mode_idx = eeprom_read_byte((const uint8_t *)((uint16_t)eepos));
    } while((mode_idx == 0xff) && eepos++<(EEPSIZE/2) ) ; // the && left to right eval prevents the last ++.
#ifndef USE_FIRSTBOOT

    // if no mode_idx was found, assume this is the first boot
    if (mode_idx==0xff) {
          mode_idx=0;
          save_state(); //redundant with check below -FR
        return; 
    }
#endif
    // load other config values
    getsetstate(gssREAD);

#ifndef USE_FIRSTBOOT
//    if (modegroup >= NUM_MODEGROUPS) reset_state(); // redundant with check above
// With this commented out, disabling firstboot just eeks out 12 bytes of saving,
// largely because reset_state can now become = save_state above.
// before doing getsetstate, the initial values are still good for the save.
// this is a nice safety check, but that's not what it's here for.
#endif
}

// change to the next mode:
inline void next_mode() {
    mode_idx += 1;
    if (mode_idx >= solid_modes) {
        // Wrap around, skipping the hidden modes
        // (note: this also applies when going "forward" from any hidden mode)
        // FIXME? Allow this to cycle through hidden modes?
        mode_idx = 0;
    }
}

#ifdef OFFTIM3
// change to the previous mode:
inline void prev_mode() {
    // simple mode has no reverse
    #ifdef USE_MUGGLE_MODE
      if (muggle_mode) { return next_mode(); }
    #endif
    if (mode_idx == solid_modes) { 
        // If we hit the end of the hidden modes, go back to first mode
        mode_idx = 0;
    } else if (mode_idx > 0) {
        // Regular mode: is between 1 and TOTAL_MODES
        mode_idx -= 1;
    } else {
        // Otherwise, wrap around (this allows entering hidden modes)
        mode_idx = mode_cnt - 1;
    }
}
#endif

// Helper for count_modes. 
// Copies hidden modes into present modes array
// At one point seemed potentially
// useful to break it out for preprocessor choices.  -FR
// Copy starting from end, so takes mode_cnt as input.
inline void copy_hidden_modes(uint8_t mode_cnt){
    for(uint8_t j=mode_cnt, i=sizeof(hiddenmodes);i>0; )
    {
        // predecrement to get the minus 1 for free and avoid the temp,
        // probably optimizer knows all this anyway.
        modes[--j] = pgm_read_byte((uint8_t *)(hiddenmodes+(--i)));
    }
}


inline void count_modes() {
    /*
     * Determine how many solid and hidden modes we have.
     *
     * (this matters because we have more than one set of modes to choose
     *  from, so we need to count at runtime)
     */
    /*  
     * FR: What this routine really does is combine the moon mode, the defined group modes, 
     * and the hidden modes into one array according to configuration options
     * such as mode reversal, enable mood mode etc.
     * The list should like this:
     * modes ={ moon, solid1, solid2,...,last_solid,hidden1,hidden2,...,last_hidden}
     * unless reverse_modes is enabled then it looks like this:
     * modes={last_solid,last_solid-1,..solid1,moon,hidden1,hidden2,...last_hidden}
     * or if moon is disabled it is just left out of either ordering.
     * If reverse clicks are disabled the hidden modes are left out.
     * mode_cnt will hold the final count of modes  -FR.
     *
     * Now updated to construct the modes in one pass, even for reverse modes.
     * Reverse modes are constructed backward and pointer gets moved to begining when done.
     */	 
    
    // copy config to local vars to avoid accidentally overwriting them in muggle mode
    // (also, it seems to reduce overall program size)
    uint8_t my_modegroup = modegroup;
    #ifdef USE_MOON
      uint8_t my_enable_moon = enable_moon;
    #else
      #define my_enable_moon 0
    #endif
    #if defined(USE_REVERSE_MODES) && defined(OFFTIM3)
     uint8_t my_reverse_modes = reverse_modes;
    #else
      #define my_reverse_modes 0
    #endif

    // override config if we're in simple mode
    #ifdef USE_MUGGLE_MODE
    if (muggle_mode) {
        my_modegroup = NUM_MODEGROUPS;
      #ifdef USE_MOON
        my_enable_moon = 0;
      #endif
      #if defined(USE_REVERSE_MODES)&&defined(OFFTIM3)
        my_reverse_modes = 0;
      #endif
    }
    #endif
    //my_modegroup=11;
    //my_reverse_modes=0; // FOR TESTING
    //my_enable_moon=1;
    uint8_t i=0;
//    const uint8_t *src = modegroups + (my_modegroup<<3);

    uint8_t *src=(void *)modegroups;
    // FR adds 0-terminated variable length mode groups, saves a bunch in big builds.
    // some inspiration from gchart here too in tightening this up even more.
    // scan for enough 0's, leave the pointer on the first mode of the group.
    i=my_modegroup+1;
    uint8_t *start=0;
    solid_modes=0;

    // now find start and end in one pass
    // for 
    while(i) {
        start=src; // set start to beginning of mode group
        #if defined(USE_REVERSE_MODES)&&defined(OFFTIM3) //Must find end before we can reverse the copy
                                        // so copy will happen in second pass below.
          while(pgm_read_byte(src++)){
        #else // if no reverse, can do the copy here too.
          while((modes[solid_modes+my_enable_moon]=pgm_read_byte(src++))){
        #endif
                         solid_modes=(uint8_t)(src-start);
                } // every 0 starts a new modegroup, decrement i
        i--;
    }


//     Now handle reverse order in same step. -FR
    #if defined(USE_REVERSE_MODES)&&defined(OFFTIM3)
    uint8_t j;
    j=solid_modes; // this seems a little redundant now.
      do{
     //note my_enable_moon and my_reverse_modes are either 1 or 0, unlike solid_modes.
      modes[(my_reverse_modes?solid_modes-j:j-1+my_enable_moon)] = pgm_read_byte(start+(j-1));
       j--;
      } while (j);
    #endif

    #ifdef USE_MOON
    solid_modes+=my_enable_moon;
    if (my_enable_moon) {// moon placement already handled, but have to fill in the value.
        modes[(my_reverse_modes?solid_modes-1:0)] = 1;
    }
    #endif
    

// add hidden modes
#ifdef OFFTIM3
    mode_cnt = solid_modes+sizeof(hiddenmodes);
    copy_hidden_modes(mode_cnt); // copy hidden modes starts at the end.

    mode_cnt-=!(modes[0]^modes[mode_cnt-1]);// cheapest syntax I could find, but the check still costs 20 bytes. -FR

#else
    mode_cnt = solid_modes; 
#endif
}

// This is getting too silly
// and tk already removed all usages of it anyway, except from set_level, so just do it there.
//#ifdef PWM4_LVL
  //inline void set_output(uint8_t pwm1, uint8_t pwm2, uint8_t pwm3, uint8_t pwm4) {
//#elif defined(PWM3_LVL)
  //inline void set_output(uint8_t pwm1, uint8_t pwm2, uint8_t pwm3) {
//#elif defined(PWM2_LVL)
  //inline void set_output(uint8_t pwm1, uint8_t pwm2) {
//#else
  //inline void set_output(uint8_t pwm1,) {
//#endif
//#endif
  //inline void set_output() {
    ///* This is no longer needed since we always use PHASE mode.
    //// Need PHASE to properly turn off the light
    //if ((pwm1==0) && (pwm2==0)) {
        //TCCR0A = PHASE;
    //}
    //*/
    //FET_PWM1_LVL = pwm1;
    //PWM1_LVL = pwm2;
//#ifdef ALT_PWM1_LVL
      //ALT_PWM1_LVL = pwm3;
//#endif
////return;
//}

// Set the level to the mode provided
// The mode specifies a PWM level for each channel, defined by the channel "ramps"
void set_level(uint8_t level) {
    if (level == 0) {
#ifdef USE_PWM4
        gbit_pwm4zero=1; // we can't disable the interrupt because we use it for other timing
                         // so have to signal the interrupt instead.
        PWM4_LVL=0;
#endif
#if defined(USE_PWM3)
        PWM3_LVL=0;
#endif
#if defined(USE_PWM2)
        PWM2_LVL=0;
#endif
#ifdef USE_PWM1
        PWM1_LVL=0;
#endif
    } else {
        level -= 1;
#ifdef USE_PWM4
       PWM4_LVL=pgm_read_byte(ramp_pwm4   + level);
       gbit_pwm4zero=0;
#endif
#ifdef USE_PWM3
       PWM3_LVL=pgm_read_byte(ramp_pwm3   + level);
#endif
#ifdef USE_PWM2
       PWM2_LVL=pgm_read_byte(ramp_pwm2   + level);
#endif
#ifdef USE_PWM1
       PWM1_LVL=pgm_read_byte(ramp_pwm1   + level);
#endif
    }
}

// if soft start, ramp to the new mode
// else just an alias for set_level:
void set_mode(uint8_t mode) {
#ifdef SOFT_START
    static uint8_t actual_level = 0;
    uint8_t target_level = mode;
    int8_t shift_amount;
    int8_t diff;
    do {
        diff = target_level - actual_level;
        shift_amount = (diff >> 2) | (diff!=0);
        actual_level += shift_amount;
        set_level(actual_level);
        _delay_ms(RAMP_SIZE/20);  // slow ramp
        //_delay_ms(RAMP_SIZE/4);  // fast ramp
    } while (target_level != actual_level);
#else
#define set_mode set_level
    //set_level(mode);
#endif  // SOFT_START
}

//Blinks val number of times at speed
void blink(uint8_t val, uint16_t speed)
{
    for (; val>0; val--)
    {
        set_level(BLINK_BRIGHTNESS);
        _delay_ms(speed);
        set_level(0);
        _delay_ms(speed);
        _delay_ms(speed);
    }
}

// utility routine to blink out 8bit value as a 3 digit decimal.
// The compiler will omit it if it's not used.  Good for debugging.  -FR.
// uses much memory though.
inline void blink_value(uint8_t value){
    blink (10,20);
    _delay_ms(500);
    blink((value/100)%10,200); // blink hundreds
    _delay_s();
    blink((value/10)%10,200); // blink tens
    _delay_s();
    blink(value % 10,200); // blink ones
    _delay_s();
    blink(10,20);
    _delay_ms(500);
}


// A basic strobe generator defined by the time on and time off for each blink:
INLINE_STROBE void strobe(uint8_t ontime, uint8_t offtime) {
// TK added this loop.  The point seems to be to slow down one strobe cycle
// so fast_presses timeout is not reached too quickly.
// FR has a partial implementation of a global clock in tk-delay
// that would solve this more generally, but for now this is slightly smaller and does the job.
    uint8_t i;
    for(i=0;i<8;i++) {
        set_level(RAMP_SIZE);
        _delay_ms(ontime);
        set_level(0);
        _delay_ms(offtime);
    }
}

// SOS srobe:
#ifdef SOS
inline void SOS_mode() {
#define SOS_SPEED 200
    blink(3, SOS_SPEED);
    _delay_ms(SOS_SPEED*5/2);
    blink(3, SOS_SPEED*5/2);
    //_delay_ms(SOS_SPEED);
    blink(3, SOS_SPEED);
    _delay_s(); _delay_s();
}
#endif

// Looped version of main menu to toggle config variables:
#ifdef LOOP_TOGGLES
inline void toggles() {
    // New FR version loops over all toggles, avoids variable passing, saves ~50 bytes.
    // mode_overrides ( toggles that cause startup in a special mode or menu) 
    // are handled by an array entry for each toggle that provides the  mod_idx to set.
    // For normal config toggles, the array value is 0, so the light starts in 
    // the first normal mode.
    // For override toggles the array value is an identifier for the special mode
    // All special modes have idx values above MINUMUM_OVERRIDE_MODE.
    // On startup mode_idx is checked to be in the override range or not. 
    //
    // Used for config mode
    // Changes the value of a config option, waits for the user to "save"
    // by turning the light off, then changes the value back in case they
    // didn't save.  Can be used repeatedly on different options, allowing
    // the user to change and save only one at a time.
    uint8_t i=0;
    do {
        i++;
        if(OPT_array[i]!=255){
            blink(i, BLINK_SPEED/8);  // indicate which option number this is
            mode_idx=overrides[i];
            OPT_array[i] ^= 1;
            save_state();
            // "buzz" for a while to indicate the active toggle window
            blink(32, 500/32);
            OPT_array[i] ^= 1;
            mode_idx=0;
            save_state();
            _delay_s();
            #ifdef USE_MUGGLE_MODE
            if (muggle_mode) {break;} // go through once on muggle mode
            // this check adds 10 bytes. Is it really needed?
            #endif
        }
    }while((i<n_toggles));
}

#else // use the old way, can come close in space for simple menus:

// Define the toggle UI
// old way, probably permanently obsolete
void toggle(uint8_t *var, uint8_t num) {
    // Used for config mode
    // Changes the value of a config option, waits for the user to "save"
    // by turning the light off, then changes the value back in case they
    // didn't save.  Can be used repeatedly on different options, allowing
    // the user to change and save only one at a time.
    blink(num, BLINK_SPEED/8);  // indicate which option number this is
    *var ^= 1;
    save_state();
    // "buzz" for a while to indicate the active toggle window
    blink(32, 500/32);
    /*
    for(uint8_t i=0; i<32; i++) {
        set_level(BLINK_BRIGHTNESS * 3 / 4);
        _delay_ms(20);
        set_level(0);
        _delay_ms(20);
    }
    */
    // if the user didn't click, reset the value and return
    *var ^= 1;
    save_state();
    _delay_s();
}


// The old way to do the toggles, in case we need it back.
// Probably permanently obsolete though:
inline void toggles(){
// Enter or leave "muggle mode"?
   #ifdef USE_MUGGLE_MODE
    toggle(&muggle_mode, 1);
    if (muggle_mode)  return;  // don't offer other options in muggle mode
  #endif
    toggle(&memory, 2);
  #ifdef USE_MOON
    toggle(&enable_moon, 3);
  #endif
  #if defined(USE_REVERSE_MODES) && defined(OFFTIM3)
    toggle(&reverse_modes, 4);
  #endif
// Enter the mode group selection mode?
    mode_idx = GROUP_SELECT_MODE;
    toggle(&mode_override, 5);
    mode_idx = 0;

  #ifdef OFFTIM3
    toggle(&offtim3, 6);
  #endif

  #ifdef TEMPERATURE_MON
// Enter temperature calibration mode?
    mode_idx = TEMP_CAL_MODE;
    toggle(&mode_override, 7);
    mode_idx = 0;
  #endif

  #ifdef USE_FIRSTBOOT
    toggle(&firstboot, 8);
  #endif
  
  #ifdef USE_LOCKSWITCH
      toggle(&lockswitch, 9);
  #endif
}

#endif


#ifdef TEMPERATURE_MON
uint8_t  get_temperature() {
    // average a few values; temperature is noisy
    uint16_t temp = 0;
    uint8_t i;
    ADC_on_temperature(); // do it before calling, inconsistent with get_voltage and
                           
    for(i=0; i<16; i++) {
        _delay_ms(5);  // 
        temp += get_voltage();
    }
    temp >>= 4;
    return temp;
}
#endif  // TEMPERATURE_MON

// read the off-time capacitor.
// High values will imply little discharge, so short press.
#if defined(OFFTIM3)&&defined(USE_OTC)
inline uint8_t read_otc() {
    // Read and return the off-time cap value
    // Start up ADC for capacitor pin
    // disable digital input on ADC pin to reduce power consumption
    //DIDR0 |= (1 << CAP_DIDR); // will just do it for all pins on init.
    // 1.1v reference, left-adjust, ADC3/PB3
    ADMUX  = (1 << V_REF) | (1 << ADLAR) | CAP_PIN;
    // enable, start, prescale
     ADCSRA = (1 << ADEN ) | (1 << ADSC ) | ADC_PRSCL;
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    // Start again as datasheet says first result is unreliable
      ADCSRA |= (1 << ADSC);
    // Wait for completion
    while (ADCSRA & (1 << ADSC));
    // ADCH should have the value we wanted
    return ADCH;
}
#endif

// init_mcu: Setup all our initial mcu configuration, pin states and interrupt configuration.
//
// Complete revamp by FR to allow use of any combination of 4 outputs, OTSM, and more
// 2017 by Flintrock (C)
//
inline void init_mcu(){ 
    //DDRB, 1 bit per pin, 1=output default =0.
    //PORTB 1 is ouput-high or input-pullup.  0 is output-low or input floating/tri-stated.  Default=0.
    //DIDR0 Digital input disable, 1 disable, 0 enable. disable for analog pins to save power. Default 0 (enabled) 

// Decide which clocks to enable, note OTSM sleep uses clock0 unless PWM4 interrupts are 
//  running anyway, then it uses those.
#if defined(USE_PWM1) || defined (USE_PWM2) || (defined POWERSAVE&&!defined(USE_PWM4))
   #define USE_CLCK0
#endif   
#if defined(USE_PWM3) || defined (USE_PWM4)
   #define USE_CLCK1
#endif

    // setup some temp variables.  This allows to build up values while only assigning to the real
    // volatile I/O once at the end.
    // The constant temp variable calculations get optimized out. Saves ~30 bytes on big builds.
    uint8_t t_DIDR0=0;
    uint8_t t_DDRB=0;
    uint8_t t_PORTB=0;
#ifdef USE_CLCK0
    uint8_t t_TCCR0B=0;
    uint8_t t_TCCR0A=0;
#endif
#ifdef USE_CLCK1
    uint8_t t_TCCR1=0;
#endif

    // start by disabling all digital input.
    // PWM pins will get set as output anyway.
    
    t_DIDR0=0b00111111 ; // tests say makes no difference during sleep,
                       // but datasheet claims it helps drain in middle voltages, so during power-off detection maybe.
    
    // Charge up the capacitor by setting CAP_PIN to output
#ifdef CAP_PIN
    // if using OTSM but no OTC cap, probably better to stay input high on the unused pin.
    // ESWITCH will override cap pin setting later, if USE_ESWITCH is defined
    // If USE_ESWITCH and eswitch pin is defined same as cap pin, the eswitch define overrides, 
    //  so again, leave it as input.
    #if (defined(USE_OTSM) && !defined(OTSM_USES_OTC) )
       // just leave cap_pin as an input pin (raised high by default lower down)
    #else
    // Charge up the capacitor by setting CAP_PIN to output
      t_DDRB  |= (1 << CAP_PIN);    // Output
      t_PORTB |= (1 << CAP_PIN);    // High
    #endif
#endif
    
            
    // Set PWM pins to output 
    // Regardless if in use or not, we can't set them input high (might have a chip even if unused in config)
    // so better set them as output low.
    #ifdef PWM_PIN
       t_DDRB |= (1 << PWM_PIN) ;    
    #endif
    #ifdef PWM2_PIN
       t_DDRB |= (1 << PWM2_PIN);
    #endif 
    #ifdef PWM3_PIN
       t_DDRB |= (1 << PWM3_PIN); 
    #endif
    #ifdef PWM4_PIN
       t_DDRB |= (1 << PWM4_PIN); 
    #endif
      
    
    // Set Timer 0 to do PWM1 and PWM2 or for OTSM powersave timing
    #ifdef USE_CLCK0
     
       t_TCCR0A = PHASE; // phase means counter counts up and down and 
                   // compare register triggers on and off symmetrically at same value.
                   // This doubles the time so half the max frequency.
    // Set prescaler timing
    // This and the phase above now directly impact the delay function in tk-delay.h..
    //  make changes there to compensate if this is changed
         t_TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)  1 => 16khz in phase mode or 32 in fast mode.

       #ifdef  USE_PWM1
         t_TCCR0A |= (1<<COM0B1);  // enable PWM on PB1
       #endif
       #ifdef USE_PWM2
         t_TCCR0A |= (1<<COM0A1);  // enable PWM on PB0
       #endif
    #endif       

   //Set Timer1 to do PWM3 and PWM4
    #ifdef USE_CLCK1
      OCR1C = 255;  
    // FR: this timer does not appear to have any phase mode.
    // The frequency appears to be just system clock/255
    #endif
    #ifdef USE_PWM3
      t_TCCR1 = _BV (CS10);  // prescale of 1 (32 khz PWM)
      GTCCR = (1<<COM1B1) | (1<<PWM1B) ; // enable pwm on OCR1B but not OCR1B compliment (PB4 only). 
                                        // FR: this timer does not appear to have any phase mode.
                                        // The frequency appears to be just system clock/255
    #endif										
    #ifdef USE_PWM4
      t_TCCR1 = _BV (CS11);  // override, use prescale of 2 (16khz PWM) for PWM4 because it will have high demand from interrupts.
                           // This will still be just as fast as timer 0 in phase mode.
      t_TCCR1 |=  _BV (PWM1A) ;           // for PB3 enable pwm for OCR1A but leave it disconnected.  
                                        // Its pin is already in use.
                                        // Must handle it with an interrupt.
      _TIMSK_ |= (1<<OCIE1A) | (1<<TOIE1);        // enable interrupt for compare match and overflow on channel 4 to control PB3 output
      // Note: this will then override timer0 as the delay clock interrupt for tk-delay.h.
    #endif
    
        
    
      t_PORTB|=(~t_DDRB); // Anything that is not an output gets a pullup resistor
      // except...
#if defined(USE_OTSM)||defined(READ_VOLTAGE_FROM_DIVIDER)
      t_PORTB&=~(1<<VOLTAGE_PIN); // The voltage sense pin does not get a pullup resistor
#endif

    // again, no change measured during sleep, but maybe helps with shutdown?
    // exceptions handled below.
#ifdef USE_ESWITCH
   #if  (!defined(READ_VOLTAGE_FROM_DIVIDER)&&!defined(USE_OTSM)) || (ESWITCH_PIN != VOLTAGE_PIN)
     //if eswitch is on voltage divider, input power pulls pin to correct level
     // else we need pull it up.
     // pullup set on E-switch pin
      t_DDRB  |= t_DDRB&~(1 << ESWITCH_PIN);    // Input
      t_PORTB |= (1 << ESWITCH_PIN);     // High
   #endif
   PCMSK |= 1<< ESWITCH_PIN;  // catch pin change on eswitch pin
#endif

#if defined(USE_OTSM) 
//  no pull-up on OTSM-PIN. 
      t_PORTB=t_PORTB&~(1<<OTSM_PIN); 
      PCMSK |= 1<< OTSM_PIN;  // catch pin change on OTSM PIn
#endif

#ifdef USE_INDICATOR
    #ifdef INDICATOR_PIN
    // Turn on the INDICATOR 
    t_DDRB  |= (1 << INDICATOR_PIN);    // Output
    t_PORTB |= (1 << INDICATOR_PIN);    // High
    #endif
#endif 

// save the compiled results out to the I/O registers:
     DIDR0=t_DIDR0;
     DDRB=t_DDRB;
     PORTB=t_PORTB;
#ifdef USE_CLCK0
     TCCR0B=t_TCCR0B;
     TCCR0A=t_TCCR0A;
#endif
#ifdef USE_CLCK1
     TCCR1=t_TCCR1;
#endif


#if defined(USE_ESWITCH) || defined(USE_OTSM)
     GIMSK |= (1<<PCIE);	// enable PIN change interrupt

     GIFR |= PCIF;  //    Clear pin change interrupt flag
#endif

#if defined(USE_ESWITCH) || defined(USE_OTSM) || defined(POWERSAVE)
     sleep_enable();     // just leave it enabled.  It's fine and will help elsewhere.
     sei();		//Enable global interrupts.

#endif

//timer overflow interrupt is presently setup in fr-delay.h for OTS_powersave

}

// change_mode:  interpret click timing information
//               and decide what mode to change into
//  If noinit is used, this checks for RAM decay (check_presses) to determine
//      click length.
//  If Eswitch, OTSM, or OTC is used it uses the global wake count variable instead
//  OTC and OTSM values or thresholds have already been translated as needed.
inline void change_mode() {  // just use global  wake_count variable

// if a special override mode is set, do nothing here:
  if (mode_idx<MINIMUM_OVERRIDE_MODE) {
      
//******** DETECT SHORT PRESS: two methods:
   #if defined(USE_OTSM) || defined(USE_ESWITCH) || defined(USE_OTC) // the wake count method
       if (wake_count < wake_count_short   )  { 
   #else                                         // the noinit method
//		if (fast_presses < 0x20) { // fallback, but this trick needs improvements.
        if ( check_presses() ) {// Indicates they did a short press, go to the next mode
   #endif
                // after turbo timeout, forward click "stays" in turbo (never truly left).
                // conditional gets optimized out if NEXT_LOCKOUT is 0
                if ( NEXT_LOCKOUT && fast_presses == DISABLE_NEXT ){
                    clear_presses(); // one time only.
                } else {
                   next_mode(); // Will handle wrap arounds
                }
                //// We don't care what the fast_presses value is as long as it's over 15
                //				fast_presses = (fast_presses+1) & 0x1f;
                inc_presses();

//********** DETECT MEDIUM PRESS.
   #ifdef OFFTIM3
         } else if (wake_count < wake_count_med   )  {

            // User did a medium press, go back one mode
//			fast_presses = 0;
            clear_presses();
            if (offtim3) {
                prev_mode();  // Will handle "negative" modes and wrap-arounds
            } else {
                next_mode();  // disabled-med-press acts like short-press
                    // (except that fast_presses isn't reliable then)
            }
      #endif
      
//********** ELSE LONG PRESS
        } else {
                // Long press, keep the same mode
                // ... or reset to the first mode
//				fast_presses = 0;
            clear_presses();
            #ifdef USE_MUGGLE_MODE
            if (muggle_mode  || (! memory)) {
            #else 
            if (!memory) {
            #endif
                    // Reset to the first mode
                    mode_idx = 0;
            }
       } // short/med/long
  }// mode override
}

// Blinks ADC value for battcheck builds
#ifdef VOLTAGE_CAL
inline void voltage_cal(){
    ADC_on();
    while(1){
      _delay_ms(300);
      blink_value(get_voltage());
    }
}
#endif


/*********************ISRs***************************************************/
/******One more is in FR-delay.h*******************************************/

#ifdef USE_PWM4
// 2017 by Flintrock.
// turn on PWM4_PIN at bottom of clock ramp
// this will also replace the delay-clock interrupt used in fr-delay.h
// Added a control flag for 0 level to disable the light. We need the interrupts running for the delay clock.
// Since SBI,CBI, and SBIC touch no registers, not even SREG, so no register protection is needed.
// PWM this way without a phase-mode counter would not shut off the light completely
// Added a control flag for 0 level to disable the light. We need the interrupts running for the delay clock.

ISR(TIMER1_OVF_vect,ISR_NAKED){ // hits when counter 1 reaches max
    __asm__ __volatile__ (    
    "sbic   %[aGbit_addr]  , %[aPwm4zero_bit] \n\t" // if pwm4zero isn't set...
    "rjmp .+2 " "\n\t"
    "sbi   %[aPortb]  ,  %[aPwm4_pin] \n\t"  //set bit in PORTB (0x18) to make PW4_PIN high.
    "reti" "\n\t"
    :: [aGbit_addr] "I" (_SFR_IO_ADDR(gbit)), 
       [aPwm4zero_bit] "I" (gbit_pwm4zero_bit), 
       [aPortb] "I" (_SFR_IO_ADDR(PORTB)), 
       [aPwm4_pin] "I" (PWM4_PIN)
       :);
}

// Turn off PWM4_PIN at COMPA register match
ISR(TIMER1_COMPA_vect, ISR_NAKED){ //hits when counter 1 reaches compare value
      __asm__ __volatile__ (    //
      "cbi   %[aPortb]  ,  %[aPwm4_pin]  \n\t"
      "reti" "\n\t"
      ::[aPortb] "I" (_SFR_IO_ADDR(PORTB)), 
        [aPwm4_pin] "I" (PWM4_PIN)
        :);
}
#endif

#if defined(USE_OTSM) || defined(USE_ESWITCH)

inline void wdt_sleep (uint8_t ste) { //Flintrock 2017
    //Setup a watchdog sleep for 16ms * 2^ste.
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    // reset watchdog counter so it doesn't trip during clear window.
    // and so the clock starts at 0 for next sleep loop.
    __asm__ __volatile__ ("wdr");
    // 		WDTCR = (1<<WDCE) |(1<<WDE); // Enable WDE 4-cycle clear window
    // set the watchdog timer: 2^n *16 ms datasheet pg 46 for table.
    WDTCR = (1<<WDIE) |(!!(ste&8)<<WDP3)|(ste&7<<WDP0); // also clears WDE 4-cycle window is activated.
    sleep_bod_disable();
    // clear the watchdog interrupt, not needed, if it went of 4 us ago fine,
    //  could as well go of 4us later.  Process it either way.
    //    MCUSR&=~(1<<WDRF);
    sei();
    sleep_cpu();
    // return from wake ISR's here, both watchdog and pin change.
    cli();
    WDTCR &= ~(1<<WDIE); // disable watchdog interrupt (prevents glitching the strobe sleeps later).
}

// Off-time sleep mode by -FR, thanks to flashy Mike and Mike C for early feasibility test with similar methods.

//need to declare main here to jump to it from ISR.
int main();
void Button_press();


// OS_task is a bit safer than naked,still allocates locals on stack if needed, although presently avoided.

void __attribute__((naked, used, externally_visible)) PCINT0_vect (void) { 
//ISR(PCINT0_vect) {
// Dual watchdog pin change power-on wake by FR

    // 2017 by Flintrock (C)
    //
//PIN change interrupt.  This serves as the power down and power up interrupt, and starts the watchdog.
//  Now split into a short ISR and a separate e-switch OTSM subtroutine: Button_press().
//
    /* IS_NAKED (or OS_main) IS ***DANGEROUS** remove it if you edit this functions significantly
     *  and aren't sure. You can remove it, and change the asm reti call below to a return.
     * and remove the CLR R1. However, it saves about 40 bytes, which matters.
     * The idea here, the compiler normally stores all registers used by the interrupt and
     * restores them at the end of the interrupt.  This costs instructions (space).
     * This ISR handles initial button presses, which all Button_press.
     * And button release interrupts, activated while in button press.
     * The button release interrupt carefullly does nothing, so register protection isn't needed.
     * SREG is probably changed but since it only breaks out of the sleep in Button_press, there are
     * no pending checks of result flags occurring.
     * The button press call will never come back.  It will do a soft reset, clearing the
     * stack pointer and jumping back to main.  Again, no protection required.
     * This is a little similar to a tail call optimization.
     *
     */

#if defined(USE_ESWITCH)&&!(defined(USE_OTSM))
// used for de-bouncing eswitch lights.
// distinguishes pin-change wakes from watchdog wakes to decide when to debounce.
//  writes to a gpio-register to avoid clobbering working registers.
//  can't debounce in here without clobbering registers. 
    gbit_pinchange=1;
#endif

    if( wake_count ){// if we've already slept, so we're waking up.
        // This is a register variable; no need to clean space for it.
        // we are waking up, do nothing and continue from where we slept (below).
        // with ISR_naked, can use the asm reti,

        __asm__ __volatile__ ("reti");
//        return;
    } else {
        Button_press();
    }
}


void __attribute__((OS_main, used, externally_visible)) Button_press (void) {
//void Button_press() {  
    /* Naked has no stack frame,
     * but seems to properly force variables to register (better anyway) until it can't,
     * and then fails to compile (gcc 4.9.2) OS_main however will create a 
     * stack frame if needed for local variables and is
     * more documented/gauranteed/future-proof probably.
     */
    __asm__ __volatile__ ("CLR R1"); // We do need a clean zero-register (yes, that's R1).
    
    #ifdef USE_ESWITCH // for eswtich we'll modify the sleep loop after a few seconds.
        register uint8_t sleep_time_exponent=SLEEP_TIME_EXPONENT; 
    #else
        #define sleep_time_exponent SLEEP_TIME_EXPONENT
    #endif
    
// we only pass this section once, can do initialization here:	

    ADCSRA = 0; //disable the ADC.  We're not coming back, no need to save it.
    set_level(0); // Set all the outputs low.	
    // Just the output-low of above, by itself, doesn't play well with 7135s.  Input high doesn't either
    //  Even though in high is high impedance, I guess it's still too much current.
    // What does work.. is input low:

    uint8_t inputs=0; // temp variable designed to get optimized out.
    #ifdef PWM_PIN
        inputs |= (1 << PWM_PIN) ;
    #endif
    #ifdef PWM2_PIN
        inputs |= (1 << PWM2_PIN) ;
    #endif
    #ifdef PWM3_PIN
        inputs |= (1 << PWM3_PIN) ;
    #endif
    #ifdef PWM4_PIN
        inputs |= (1 << PWM4_PIN) ;
    #endif
    DDRB&= ~inputs;

#ifdef USE_INDICATOR
// turn off the indicator to acknowledge button is pressed:
    PORTB &= ~(1<<INDICATOR_PIN);
#endif 
    

#ifdef USE_ESWITCH
     register uint8_t e_standdown=1; // pressing off, where we start
     register uint8_t e_standby=0;  // pressing on
     register uint8_t switch_is_pressed; // 1 means any switch is pressed.
#endif

    while(1) { // keep going to sleep until power is on

     #if defined(USE_ESWITCH)

//***************DO DEBOUNCING ******************

      asm volatile ("" ::: "memory"); // just in case, no i/o reordering around these. It's a no-op.
      #if !defined(USE_OTSM)
       asm volatile ("" ::: "memory"); // just in case, no i/o reordering around these. It's a no-op.
       //debounce, OTSM probably has enough capacitance not to need it
       // and the extra on-time is bad for OTSM, may just change to WDT sleep though.
       if (gbit_pinchange){ //don't debounce after watchdog
                          // only after pin-change.
        //cannot use delay_ms() with powersave without micro-managing interrupts.					  
         _delay_loop_2(0); // let pin settle for 32ms. 0=> 0xffff
//         _delay_loop_2(0x6000);  // delay about 10ms
         GIFR |= PCIF;  //    Clear pin change interrupt flag
         asm volatile ("" ::: "memory"); // just in case, no i/o reordering around these. It's a no-op.
       }
     #else    // if ewitch AND OTSM     
//        // Does OTSM+ eswitch even need debounce?
//        // Is it C2 alone that fixes debounce or something about clicky switches?
//        // if so it will need to use watchdog wakes to save power.

//        asm volatile ("" ::: "memory"); // just in case, no i/o reordering around these. It's a no-op.
//        if (gbit_pinchange){ //don't debounce after watchdog
//            // only after pin-change.
//          GIMSK &= ~(1<<PCIE);  // disable PIN change interrupt
////          debounce time exponent for watchdog prescale
////          2^n *16 ms datasheet pg 46 for table.
//          uint8_t dbe=1; // 32ms.
//          wdt_sleep(dbe);
//          GIMSK |= (1<<PCIE);  // re-enable PIN change interrupt
//          GIFR |= PCIF;  //    Clear pin change interrupt flag
//          gbit_pinchange=0; // clear wake was pin change flag
//       }
//       asm volatile ("" ::: "memory"); // just in case, no i/o reordering around these. It's a no-op.
       #endif
     
//**** DETERMINE PRESENT STATE AND RESTART DECISION, EVOLVED THROUGH CLICK HISTORY

     /*  Three phases, 
       * 1:standown: button was pressed while on, not released yet.
       *    If released in short or med click will restart
       * 2:full off. (neither standby nor standown). Long press time-out ocurerd, and button was released.
       *           -Pressing the button now arms standby mode.
       * 3:standby:  Button is pressed for turn on from off-sleep, restart on release. No timing presently.
       */

//**** First figure out if buttons are pressed, and to some extent which ones: *******
       #if defined(USE_OTSM) && (OTSM_PIN != ESWITCH_PIN) //more than one switch pin 
          // define only  on first entry to identify press type.
          // This is not a proper bool, it's 0 or ESWITCH_PIN.
          // Could store this is the GPIOR if we need to know on wake up.
          register uint8_t e_was_pressed = ((PINB^(1<<ESWITCH_PIN)));

          // define on every entry to see if still pressed:
          // if either switch is "pressed" (0) this is 0.
          // is right shifting cheaper than &&? Is the compiler smart?
          switch_is_pressed = ( (PINB^(1<<ESWITCH_PIN)) || (PINB^(1<<OTSM_PIN)) ); 
       #else // only one switch pin:
          switch_is_pressed =  (PINB^(1<<ESWITCH_PIN)); 
             //this gets optomized out of conditional checks:
          register uint8_t e_was_pressed=1; 
       #endif
       
//****** If a switch is released, what do we do? *******
// **  Restart or go to off-sleep?
       
//      if ( !switch_is_pressed ) { 
 //  If wake count==0, first entry, check pin state
 //  If it's already open,process the release.  If not so initially closed,
 //    then assume any later pin change must have produced a release at some point
 //    even if we missed it, so don't check pin again, just check for pinchange event.
      if ( (gbit_pinchange && (wake_count !=0)) || !switch_is_pressed ) {
        if ( wake_count<wake_count_med || !e_was_pressed || e_standby ){
           // If clicky switch is released ever, or if eswitch is released
           // before long click, or if switch is released in standby we restart.
               break; // goto reset 
         // otherwise if e-switch is released after wake_count_med..
        } else { // switch released in long press
            //exit standown, go to off-sleep.
            e_standdown=0; // allows next press to enter standby
            sleep_time_exponent=9; // 16ms*2^9=8s, maximum watchod timeout.
        }		  
      }
      

      // increment click time if still in short or med click
      // go one past, why? on == we burn out the OTSM cap below,
      // Then we go one past, so we don't repeat the burnout.
      wake_count+= (wake_count <= wake_count_med); 
                     // then continue.. sleep more.
                     
//***Things to do at start of long-press timeout:
      if ( wake_count == wake_count_med){// long press is now inevitable

      // if e-switch and OTSM-clicky switch are the same.  Prevent releasing clicky into off-sleep 
      // by draining the clicky OTSM before user has a chance to release, while interrupts are disabled.
      // This will do a full restart (clicky long press).
        #if ( defined(USE_OTSM) && (OTSM_PIN == ESWITCH_PIN) ) 
          // _delay_loop_2 waits for 4 clocks. so F_CPU/400 is 10ms.
          _delay_loop_2(F_CPU/400); // if this was a clicky switch.. this will bring it down,
        #endif

        #ifdef USE_INDICATOR
          // re-light the indicator to acknowledge turn-off
          PORTB |= (1<<INDICATOR_PIN);
        #endif
      }

//*** Handle pressing the switch again from off ***//

      if (!e_standdown && !e_standby && switch_is_pressed   ) { // if button pressed while off, standby for restart
            e_standby=1; // going to standby, will light when button is released
         #ifdef USE_INDICATOR
           // cut the indicator to acknowledge button press
           PORTB &= ~(1<<INDICATOR_PIN);
         #endif
      }

//**** Finally what do for OTSM-only, no e-switch.  ***********************					 
     #else // if not defined USE_ESWITCH: for simple OTSM, 
     //  if power comes back, we're on.  Wake_count will tell main about the click length:
        if(PINB&(1<<OTSM_PIN)) {
             break; // goto reset
        }  
        wake_count++;
     #endif

/***************** Wake on either a power-up (pin change) or a watchdog timeout************/
// At every watchdog wake, increment the wake counter and go back to sleep.
         gbit_pinchange=0; // clear wake was pin change flag
        wdt_sleep(sleep_time_exponent);		
//        wdt_(sleep_time_exponent>>(wake_count<5));// for first 4 sleeps, cut wake time in half (right shift 1 if wake_count is <5)
     }	 

/******************* Do a soft "reset" of sorts. ********************/
     //
     asm volatile ("" ::: "memory"); // just in case, no i/o reordering around these. It's a noop.
     SPL = (RAMEND&0xff); // reset stack pointer, so if we keep going around we don't get a stack overflow. 
             // Scary looking perhaps, but a true reset does exactly this (same assembly code).
#if (ATTINY == 45 ) || ( ATTINY == 85 )
     SPH = RAMEND>>8; 
#endif
     SREG=0;  // clear status register.
     main(); // start over, bypassing initialization.
} 

ISR(WDT_vect, ISR_NAKED) // watchdog interrupt handler -FR
{ // gcc avr stacks empty interrupts with a bunch of un-needed initialization.
  // do this in assembly and we get one instruction instead about 15.
        __asm__ volatile("reti");
}

#endif // OTSM/E-SWITCH




// initializer added by Flintrock.  This goes into a "secret" code segment that gets run before main. 
// It cannot be called. It's cheaper than breaking main into another sub.
// It initializes "cold boot" values. A "reset" instead jumps to main after a switch press
// and must then bypass the wake_counter initializer.
// be careful in here, local variables are not technically allowed (no stack exists), but subroutine calls are.
void __attribute__ ((naked)) __attribute__ ((section (".init8"))) \
main_init () { 
     initial_values(); // load defaults for eeprom config variables.
   // initialize register variables, since gcc stubornly won't do it for us.
     eepos=0;
#if defined(USE_OTSM) || defined(USE_ESWITCH)
     wake_count=255; // initial value on cold reboot, represents large click time.
#endif
}

int __attribute__((OS_task)) main()  { // Jump here from a wake interrupt, skipping initialization of wake_count.

#ifndef clear_presses_value // if it's not a compile time constant
// requires initialization on re-entry to main from OTSM sleep.
  clear_presses_value=0; 
#endif
#ifdef OTSM_debug // for testing your OTSM hardware
  //store the wake_count value to blink it out after mcu initialization.
  uint8_t tempval=wake_count;
#endif

// Handle some click timing scenarios.  We can't processes click timings until we restore the last mode save.
// but need to read OTC as early as possible anyway.
// Also need to translate different timing methods to compatible forms for change_modes().

// Still not loving this.  Very difficult to make this flexible, pretty, and compact.
// But I think the long term aim could be to allow to configure actions for every click type.
//  LIke #define SHORT_POWER_CLICK  REVERSE_RESET  to have a 
//   short power switch act like a long press with reversed modes.
// Otherwise we have to just keep adding boutique conditionals here:
#if defined(OFFTIM3)
    #if defined(USE_ESWITCH)
       // handle dual switch cases:
        #if defined(DUMB_CLICK) // LEXEL mode, power click just turns the light on the way it came off.
           if(wake_count==255) {// we had a full reset, aka power switch.
              wake_count++; // set to 0, short click.
              disable_next(); //forces short click and disables mode advance.
           }
        #elif defined(TURBO_CLICK)// power switch always starts in first hidden mode
           if(wake_count==255) {// we had a full reset, aka power switch.
                  mode_idx=0; // set to first (0) mode,
                  save_mode();
                  wake_count=wake_count_short; // med press, will move back into hidden mode.
           }		
        #elif defined(REVERSE_CLICK)// power switch reverses modes.
              //not implemented, 
              // a little tricky.
        #elif defined(USE_OTC)
        // We have an eswitch and OTC on the clicking switch.
        // If Eswitch was pressed, nothing to do,
        //   but if click switch was pressed:
           if(wake_count==255) { //255 is re-initialized value, power-outage implies clicky press, must use OTC
            // We can't translate the CAP thresholds, because they are compile-time constants,
            // and this is a run-time conditional.
            // Instead translate the cap values to wake_count values.
            // A matching preprocessor conditional above stores 255-CAP_MED and 255-CAP_SHORT in the wake_count macros.
            //     in the case that OTC is used.  
              uint8_t cap_val = read_otc();
              if (cap_val>CAP_MED) {
                 wake_count=wake_count_med; // long press (>= is long)
              }else if (cap_val>CAP_SHORT){
                 wake_count=wake_count_short; // med press
              }else{
                 wake_count=0;               // short press
              }
           }
        #elif  defined(USE_OTSM) // eswitch and OTSM clicking switch
           // anything to do here?
           // These switches use the same timing format already.
        #elif  defined(USE_ESWITCH)  // eswitch and noinit or no second switch configured.
          #if  defined(DUAL_SWITCH_NOINIT)
              wake_count=255+check_presses(); // 0 if check_presses is 1 (short click),  255 if 0 (long click).
          #endif
        #endif 
    #elif defined(USE_OTC) 
       #if !defined(USE_OTSM)
        // OTC only.
        // check the OTC immediately before it has a chance to charge or discharge
        // We re-use the wake_count variable that measures off time for OTSM:
        // But high cap is short time, so have to invert.
          wake_count = 255-read_otc(); // 255 - OTC adc value.
          // define wake_count thresholds from cap thresholds:
       #else 
         #error You cannot define USE_OTSM and USE_OTC at the same time.
       #endif //handle eswitch and OTC below
    // end of OTC possibilities
    #endif // end OTC without e-switch.
#endif
    
    restore_state(); // loads user configurations

// Disable e-switch interrupt if it's locked out:
#if defined(USE_ESWITCH_LOCKOUT_TOGGLE) && (ESWITCH_PIN != OTSM_PIN)
    if (lockswitch) {
      PCMSK&=~(1<<ESWITCH_PIN);
    }
#endif

    count_modes(); // build the working mode group using configured choices.
 
    change_mode(); // advance, reverse, stay put?

// reset the wake counter, must happen after we proccess 
//  the click timing in change_mode and before releasing interrupts in init_mcu().
#if defined(USE_OTSM) || defined(USE_ESWITCH)
    wake_count=0; // reset the wake counter.
#endif


// initialize the pin states and interrupts, and enable interrupts.
// First entry into pin change interrupt must have wake_count of 0
// or the interrupt will kick back out with interrupts disabled 
// causing sleeps to freeze, so enable interrupts after resetting wake_count.

// debounce at power-on?
// Not needed because there was already a pin change parse before restart
// with a debounce time then. Next pin read after interrupts are re-enabled.
// So far OTSM clicky lights don't seem to need debounce.
//   _delay_loop_2(0); // let pin settle for 32ms. 0=> 0xff


    init_mcu();

#ifdef OTSM_debug // for testing your OTSM hardware 
    if (tempval!=255) {
      blink_value(tempval);
    } 
#endif

#ifdef OTC_debug
    blink_value(cap_val);
    return 0;
#endif

#ifdef VOLTAGE_CAL
    voltage_cal();
#endif


#if defined(VOLTAGE_MON) || defined(USE_BATTCHECK)
    ADC_on(); 
#endif

    uint8_t output; // defines the nominal mode for program control.
    uint8_t actual_level; // allows modifications for stepdown etc.
#if defined(TEMPERATURE_MON)
    uint8_t temp=0;
    uint8_t overheat_count = 0;
#elif defined(USE_TURBO_TIMEOUT)
    uint8_t overheat_count = 0;
#endif

    uint8_t i = 0;
#ifdef VOLTAGE_MON
    uint8_t lowbatt_cnt = 0;
#endif
    //  
    // modegroups, mode_idx, output, and actual_level explained:
    //
    // Modegroups are an array of "output" values.  For normal modes these values are normally the same as actual_level values 
    //  which are just ramp index values.  (The ramp arrays convert an actual_level into PWM values for all the channels in use).
    //
    // In normal modes actual_level can become different from nominal "output" value if temperature or voltage regulation has kicked in
    //  and this is why there are two separate values.  output remembers the nominal value and actual_level holds the present corrected value.
    //  
    // mode_idx normally tells which mode we're in within the modegroup array and thus which output value to use from the modegroup.
    // Some output values (with high values) in the modegroups correspond to special modes like strobes, not nominal actual_level.  
    // Those magic values will get filtered and processed before/instead-of selecting a ramp value.
    //
    // Still other times mode_idx itself refers to a special "output" value, not a modegroup index. 
    // These are values not listed within the modegroup and only selectable from menu functions. 
    // They are identified when mode_idx has been set greater than MINIMUM_OVERRIDE_MODE, thus overriding the modegroup lookup.
    // In these cases we must first copy the mode_idx value to the output value.  It then gets treated like any other special output value
    //    and again filtered before/instead-of selecting a ramp value.
    //
    //
//    if (mode_override) {
    if (mode_idx>=MINIMUM_OVERRIDE_MODE) {
    // handle mode overrides, like mode group selection and temperature calibration
        // do nothing; mode is already set
//        fast_presses = 0;
//		clear_presses(); // redundant, already clear on menu entry
                         // and had to go through menu to get to an override mode.
                         // at most fast_presses now is 1, saves 6 bytes.
        output = mode_idx; // not a typo. override modes don't get converted to an actual output level.

        // exit these mode after one use by default:
        //   (Also prevents getting stuck in a bogus override mode from data corruption, etc.)
        mode_idx = 0;  // requires save_mode call below to take effect.
    } else {
      if (fast_presses > 0x0f) {  // Config mode
             _delay_s();       // wait for user to stop fast-pressing button
             clear_presses(); // exit this mode after one use
             //            mode_idx = 0;
             toggles(); // this is the main menu
      }
      output = modes[mode_idx];
      actual_level = output;
    }
    
    save_mode();
  //************Finished startup**********************	
    
        
  //****************************************************
  //*********** The main on-time loop *****************
    
    while(1) {
//#if defined(VOLTAGE_MON) || defined(USE_BATTCHECK)
//         ADC_on();  // switch (back) to voltage mode before the strobe or regular mode delay.
//#endif
#if !defined(NO_STROBES)

// Tried saving some space on these strobes, but it's not easy. gcc does ok though.
// Using a switch does create different code (more jump-table-ish, kind of), but it's not shorter.
        if (0) {} // dummy to get the conditionals started,
                  // will optimize out.
#ifdef USE_STROBE_10HZ
        else if (output == STROBE_10HZ) {
            // 10Hz tactical strobe
            strobe(33,67);
        }
#endif // ifdef STROBE_10HZ
#ifdef USE_STROBE_16HZ
        else if (output == STROBE_16HZ) {
            // 16.6Hz tactical strobe
            strobe(20,40);
        }
#endif // ifdef STROBE_16HZ
#ifdef USE_STROBE_8HZ
        else if (output == STROBE_8HZ) {
            // 8.3Hz Tactical strobe
            strobe(40,80);
        }
#endif // ifdef STROBE_8HZ
#ifdef USE_STROBE_OLD_MOVIE
        else if (output == STROBE_OLD_MOVIE) {
            // Old movie effect strobe, like you some stop motion?
            strobe(1,41);
        }
#endif // ifdef STROBE_OLD_MOVIE
#ifdef USE_STROBE_CREEPY
        else if (output == STROBE_CREEPY) {
            // Creepy effect strobe stop motion effect that is quite cool or quite creepy, dpends how many friends you have with you I suppose.
            strobe(1,82);
        }
#endif // ifdef STROBE_CREEPY

#ifdef USE_POLICE_STROBE
        else if (output == POLICE_STROBE) {

            // police-like strobe
            //for(i=0;i<8;i++) {
                 strobe(20,40);
            //}
            //for(i=0;i<8;i++) {
                 strobe(40,80);
            //}
        }
#endif // ifdef POLICE_STROBE
#ifdef USE_RANDOM_STROBE
        else if (output == RANDOM_STROBE) {
            // pseudo-random strobe
            uint8_t ms = 34 + (pgm_rand() & 0x3f);
            strobe(ms, ms);
            //strobe(ms, ms);
        }
#endif // ifdef RANDOM_STROBE
#ifdef USE_BIKING_STROBE
        else if (output == BIKING_STROBE) {
            // 2-level stutter beacon for biking and such
#ifdef USE_FULL_BIKING_STROBE
            // normal version
            for(i=0;i<4;i++) {
                set_level(TURBO);
                //set_output(255,0,0);
                _delay_ms(5);
                set_level(ONE7135);
                //set_output(0,0,255);
                _delay_ms(65);
            }
            _delay_ms(720);
#else
            // small/minimal version
            set_level(TURBO);
            //set_output(255,0,0);
            _delay_ms(10);
            set_level(ONE7135);
            //set_output(0,0,255);
            _delay_s();
#endif
        }
#endif  // ifdef BIKING_STROBE
#ifdef USE_SOS
        else if (output == SOS) { SOS_mode(); }
#endif // ifdef SOS
#ifdef USE_RAMP
        else if (output == RAMP) {
            int8_t r;
            // simple ramping test
            for(r=1; r<=RAMP_SIZE; r++) {
                set_level(r);
                _delay_ms(40);
            }
            for(r=RAMP_SIZE; r>0; r--) {
                set_level(r);
                _delay_ms(40);
            }
        }
#endif  // ifdef RAMP
#endif // if !defined(NO_STROBES)
#ifdef USE_BATTCHECK
        else if (output == BATTCHECK) {
#ifdef BATTCHECK_VpT
            //blink_value(get_voltage()); // for debugging
            // blink out volts and tenths
            _delay_ms(100);
            uint8_t result = battcheck();
            blink(result >> 5, BLINK_SPEED/6);
            _delay_ms(BLINK_SPEED);
            blink(1,5);
            _delay_ms(BLINK_SPEED*3/2);
            blink(result & 0b00011111, BLINK_SPEED/6);
#else  // ifdef BATTCHECK_VpT
            // blink zero to five times to show voltage
            // (~0%, ~25%, ~50%, ~75%, ~100%, >100%)
            blink(battcheck(), BLINK_SPEED/6);
#endif  // ifdef BATTCHECK_VpT
            // wait between readouts
            _delay_s(); _delay_s();
        }
#endif // ifdef USE_BATTCHECK

// **********output values here correspond to override modes. ************

        else if (output == GROUP_SELECT_MODE) {
            // exit this mode after one use
            //  Now done as default at top for all override modes.
            //		mode_idx = 0;
//            mode_override = 0; // old way

            for(i=0; i<NUM_MODEGROUPS; i++) {
                modegroup = i;
                save_state();

                blink(1, BLINK_SPEED/3);
            }
            _delay_s(); _delay_s();
        }
#ifdef USE_TEMP_CAL
#ifdef TEMPERATURE_MON
        else if (output == TEMP_CAL_MODE) {
            // make sure we don't stay in this mode after button press
            //  Now done as default at top for all override modes.
            // mode_idx = 0;
//            mode_override = 0; // old way

            // Allow the user to turn off thermal regulation if they want
            maxtemp = 255;
            save_state(); 
            set_mode(RAMP_SIZE/4);  // start somewhat dim during turn-off-regulation mode
            _delay_s(); _delay_s();

            // run at highest output level, to generate heat
            set_mode(RAMP_SIZE);

            // measure, save, wait...  repeat
            while(1) {
                maxtemp = get_temperature();
                save_state();
                _delay_s(); _delay_s();
            }
        }
#endif
#endif  // TEMP_CAL_MODE

     //********* Otherwise, regular non-hidden solid modes: ****************

        else {  
//  moved this before temp check.  Temp check result will still apply on next loop.
// reason is with Vcc reads, we switch back and forth between ADC channels.
//  The get temperature includes a delay to stabilize ADC (maybe not needed)
//   That delay results in hesitation at turn on.
//  So turn on first.
            set_mode(actual_level);		   
#ifdef TEMPERATURE_MON
            temp=get_temperature();
  #if defined(VOLTAGE_MON) || defined(USE_BATTCHECK)
            ADC_on(); // switch back to voltage mode.
  #endif
#endif		 
        // If get_temp were done at the end of loop, could move temp conditionals outside 
        // mode check to affect all modes including strobes,
        // but get temp is too slow for strobes, would need to implement it in interrupts.
        // Requires interrupt timing of temp, voltage, and delays between them though. :(	 

// Temp control methods:
// Notice we don't actually have a temp reading yet on first loop, will be zero, that's ok.
#ifdef CLASSIC_TEMPERATURE_MON //this gets set by default.
            // Tries to regulate temparature in all modes.
            // This has issues.  First, it's very hard to get right and very hardware dependent
            // If you get it right you likely don't get a turbo boost at all
            // but just stabilize at a sustainable temp.  Even if you do get a 
            //  turbo boost, then you get either
            // eventual stabilization at max temp, no more turbo boosts possible after that really,
            // Or you get up and down and up and down at random times.
            // If we want sustainable regulation we should do that.  
            // If we want controlled tubro boosts we should do that, 
            //  and come back down to cool off and be ready for another turbo on demand.
            // So FR dividing these needs going forward with new options below.
            //
            // step down? (or step back up?)
             if (temp > maxtemp) {
                overheat_count ++;
                // reduce noise, and limit the lowest step-down level
                if ((overheat_count > 8) && (actual_level > (RAMP_SIZE/8))) {
                    actual_level --;
                    //_delay_ms(5000);  // don't ramp down too fast
                    overheat_count = 0;  // don't ramp down too fast
                }
             } else {
                // if we're not overheated, ramp up  the user-requested level
                overheat_count = 0;
                if ((temp < maxtemp - 2) && (actual_level < output)) {
                    actual_level ++;
                }
             }
 #elif defined(TEMP_STEP_DOWN)|| defined (USE_TURBO_TIMEOUT) // BLFA6 inspired stepdown modes, by FR
             // now using this to count minimum up time before step down is activated.
             // We do NOT need to be overheated that long though.
             overheat_count ++;			 
         // choose the step_down condition:
         #ifdef TEMP_STEP_DOWN
            // For stepdown to occur,
            // need to be overheated AND past the MINIMUM_TURBO_TIME and in a level higher than the step down level:
             if (temp >= maxtemp && actual_level > TURBO_STEPDOWN 
                   && overheat_count > (uint8_t)((double)(MINIMUM_TURBO_TIME)*(double)1000/(double)LOOP_SLEEP)) {
         #elif defined(USE_TURBO_TIMEOUT)
           // for non-temp based method, need to be in TURBO mode and past TURBO_TIMEOUT for step down:
             if ((output == TURBO) && overheat_count > (uint8_t)((double)(TURBO_TIMEOUT)*(double)1000/(double)LOOP_SLEEP) ) {
         #endif
                   // can't use BLFA6 mode change since we don't have predictable modes.
                   // Must change actual_level, and lockout mode advance on next click.
//	               output=TURBO_STEPDOWN;
                   actual_level=TURBO_STEPDOWN;
                   disable_next(); // next forward press will keep turbo mode.
                                              // this value is immune to clear_presses.
             } 
             
// end of temperature control methods.
#endif
        //the loop delay for regular modes:
         _delay_ms(LOOP_SLEEP);
        } 

        // We've taken our time, either with regular mode delay or one strobe cycle.
        // So the user isn't fast pressing anymore.

        clear_presses();
        // ********end of mode conditional************

        //****** Now things to do in loop after every mode **************/


#ifdef VOLTAGE_MON
            // See if voltage is lower than what we were looking for
            if (get_voltage() < ADC_LOW) {
                lowbatt_cnt ++;
            } else {
                lowbatt_cnt = 0;
            }
            // See if it's been low for a while, and maybe step down
            // Why do we need 8 checks?  Battcheck works fine with one.
            // Is there some corner condition when this goes wrong?
            if (lowbatt_cnt >= 8) {
                if (actual_level > RAMP_SIZE) {  // hidden / blinky modes
                    // step down from blinky modes to medium
                    actual_level = RAMP_SIZE / 2;
                } else if (actual_level > 1) {  // regular solid mode
                    // step down from solid modes somewhat gradually
                    // drop by 25% each time
                    actual_level = (actual_level >> 2) + (actual_level >> 1);
                } else { // Already at the lowest mode
                    // Turn off the light
                    set_level(0);
                    // Power down as many components as possible
                    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                    cli(); // make sure not to wake back up.
                #if (ATTINY>13) // could macro define it in headers, but this is more clear:
                    sleep_bod_disable();// power goes from 25uA to 4uA.
                #endif
                    sleep_mode(); 
                }
//                set_mode(actual_level); // redundant
                output = actual_level; //This mostly just pulls out of strobes.
                lowbatt_cnt = 0;
                // Wait before lowering the level again
                //_delay_ms(250); // redundant
            }
#endif  // ifdef VOLTAGE_MON



    }// end of main while loop
}// end of main.

//TODO -FR
// Change OTSM debounce to watchdog sleep... maybe (implemented in comments)
// fix mode_save commit.
//Ideas for farther ahead:
//Try shortening medium click for OTSM, may require adding finer timing resulotion for all times, or possibly just early times. (has?a cap performance?cost, but it's performing better than needed now).
//Maybe rework dual switch stuff so every click type can be given assigned an action type in the configs. ?Hard to keep the programming tight, and clean, but avoids endless options.
//In particular consider reverse click order for power switch, maybe separately from above plan.
