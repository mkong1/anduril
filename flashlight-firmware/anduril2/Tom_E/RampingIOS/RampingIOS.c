//-------------------------------------------------------------------------------------
// RampingIOS - simple ramping UI for the Atmel ATTiny85/ATTiny85V
// ==========
//
// Copyright (C) 2017 Tom Elfers and Selene Scriven
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//
//  originally based on STAR_momentary.c v1.6 from JonnyC as of Feb 14th, 2015, Modified by Tom E
//
// Capabilities:
//  - full e-switch "only" support with a low standby power mode (low parasitic drain)
//  - lock-out feature - uses a special sequence to lock out the e-switch (prevent accidental turn-on's)
//  - tactical mode, beacon mode
//  - LVP and temperature regulation using the internal temp sensor
//  - no voltage divider resistors, very low parasitic drain
//
// Change History
// --------------
// Vers 1.1.0 2017-07-22:  (ToyKeeper)
//   - added full thermal regulation with user-calibrated ceiling
//   - added mode memory on click-from-off (default 100% 7135)
//   - made beacon use current ramp level
//   - made double-click toggle turbo (not just one-way any more)
//   - made LVP drop down in smaller steps
//   - calibrated moon to ~0.3 lm on Emisar D4-219c hardware
//   - blink when passing the 100% 7135 level, for reference
//   - fixed display of "zero" digits
//   - fixed potential eeprom corruption
//   - some code refactoring
//   - cleaned whitespace, pruned dead code
//
// Vers 1.0.1 2017-05-03:
//   - changed ramping table to start at 7135 PWM value of 10 rather than 4 (4 was too low for the IOS 7135's)
//   - delay a double-click action in ramping so that all multi-clicks don't flash MAX brightness
//   - bug fixes in tactical: all multi clicks were not available, they are now
//   - bug fixes in tactical: 4 extra blinks happened when powering up in tactical mode -- removed that
//   - bug fixes in tactical: from a lockout, ramping got resotred when tactical should have
//   - for the 10 clicks and 11th hold for thermal step-down, ensure no extra blinks
//
// Vers 1.0 2017-03-21/27:
//   - original release
//-------------------------------------------------------------------------------------

/*
 * ATtiny 25/45/85 Diagram
 *              ---
 *   Reset  1 -|   |- 8  VCC
 *  switch  2 -|   |- 7  Voltage ADC
 * Ind.LED  3 -|   |- 6  FET PWM
 *     GND  4 -|   |- 5  7135 PWM
 *              ---
 *
 * FUSES
 *  See this for fuse settings:
 *    http://www.engbedded.com/cgi-bin/fcx.cgi?P_PREV=ATtiny85&P=ATtiny85&M_LOW_0x3F=0x22&M_HIGH_0x07=0x07&M_HIGH_0x20=0x00&B_SPIEN=P&B_SUT0=P&B_CKSEL3=P&B_CKSEL2=P&B_CKSEL0=P&V_LOW=E2&V_HIGH=DE&V_EXTENDED=FF&O_HEX=Apply+values
 *
 *  Following is the command options for the fuses used for BOD enabled (Brown-Out Detection), recommended:
 *      -Ulfuse:w:0xe2:m -Uhfuse:w:0xde:m -Uefuse:w:0xff:m
 *    or BOD disabled:
 *      -Ulfuse:w:0xe2:m -Uhfuse:w:0xdf:m -Uefuse:w:0xff:m
 *
 *      Low: 0xE2 - 8 MHz CPU without a divider, 15.67kHz phase-correct PWM
 *     High: 0xDE - enable serial prog/dnld, BOD enabled (or 0xDF for no BOD)
 *    Extra: 0xFF - self programming not enabled
 *
 *  --> Note: BOD enabled fixes lockups intermittently occurring on power up fluctuations, but adds slightly to parasitic drain
 *
 * STARS  (not used)
 *
 * VOLTAGE
 *      Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 *
 */

#define ATTINY 85
#include "tk-attiny.h"

#define byte uint8_t
#define word uint16_t

#define PHASE 0xA1          // phase-correct PWM both channels
#define FAST 0xA3           // fast PWM both channels

// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

//------------- Driver Board Settings -------------------------------------
//
#define VOLTAGE_MON         // Comment out to disable - ramp down and eventual shutoff when battery is low
//#define VOLT_MON_PIN7       // Use voltage divider on pin7 instead of raw voltage on VCC pin.
#define VOLT_MON_INTREF     // uses the 1.1V internal reference

//
//------------- End Driver Board Settings ---------------------------------


//-------------------------------------------------------------------------
//                  Settings to modify per driver
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

// v1.1 (in bits: mmm.nnnnn, mm=major @, nnnnn=minor#)
#define FIRMWARE_VERS ((1<<5) | 1)

// Choose one or the other below. Neither set will be normal power-up in OFF state
//#define STARTUP_LIGHT_ON        // powerup/startup at max level
#define STARTUP_2BLINKS         // enables the powerup/startup two blinks

#define USE_LVP                 // enables Low Voltage Protection

// Temperature monitoring setup
//-----------------------------
// Temperature Calibration Offset - tiny85 chips vary per-chip
#define TEMP_CAL_OFFSET (0)     // err on the safe side for factory production purposes
//   -12  is about right on the DEL DDm-L4 board in the UranusFire C818 light
//   -11  On the TA22 board in SupFire M2-Z, it's bout 11-12C too high,reads 35C at room temp, 23C=73.4F
//   -8   For the Manker U11 - at -11, reads 18C at 71F room temp (22C)
//   -2   For the Lumintop SD26 - at -2, reading a solid 19C-20C (66.2F-68F for 67F room temp)

#define MAX_THERM_CEIL 70
#define DEFAULT_THERM_CEIL 45
#define TEMP_CRITICAL (DEFAULT_THERM_CEIL)      // Temperature in C to step the light down
// (40C=104F, 45C=113F, 50C=122F, 55C=131F, 60C=140F)
// use 50C for smaller size hosts, or a more conservative level (SD26, U11, etc.)
// use 55C for larger size hosts, maybe C8 and above, or for a more aggressive setting

#define TEMP_ADJ_PERIOD 32     // Thermal regulation frequency: 0.5 sec(s) (in 16 msec ticks)
//-----------------------------

//#define USING_3807135_BANK    // (default OFF) sets up ramping for 380 mA 7135's instead of a FET

#define RAMPING_REVERSE         // (default ON) reverses ramping direction for every click&hold

#define RAMP_SWITCH_TIMEOUT 75  // make the up/dn ramp switch timeout 1.2 secs (same as IDLE_TIME)

//#define ADV_RAMP_OPTIONS        // In ramping, enables "mode set" additional method for lock-out and battery voltage display, comment out to disable

#define TRIPLE_CLICK_BATT       // enable a triple-click to display Battery status

#define SHORT_CLICK_DUR 18      // Short click max duration - for 0.288 secs
#define RAMP_MOON_PAUSE 23      // this results in a 0.368 sec delay, paused in moon mode

#define D1 2                    // Drop over reverse polarity protection diode = 0.2 V

// ----- 2/14 TE: One-Click Turn OFF option --------------------------------------------
#define IDLE_TIME         75    // make the time-out 1.2 seconds (Comment out to disable)


// Switch handling:
#define LONG_PRESS_DUR    24    // Prev Mode time - long press, 24=0.384s (1/3s: too fast, 0.5s: too slow)
#define XLONG_PRESS_DUR   68    // strobe mode entry hold time - 68=1.09s (any slower it can happen unintentionally too much)
#define CONFIG_ENTER_DUR 160    // Config mode entry hold time - 160=2.5s, 128=2s

#define DBL_CLICK_TICKS   15    //  fast click time for enable/disable of Lock-Out, batt check,
                                // and double/triple click timing (15=0.240s, was 14=0.224s)

#ifdef VOLT_MON_PIN7
// 1S cell configuration:
#define BATT_LOW   30  // Cell voltage to step light down = 3.0 V
#define BATT_CRIT  28  // Cell voltage to shut the light off = 2.8 V
// measure values for a full and empty battery, then plug them in
#define VOLTS_MAX  42  // 4.2V
#define ADC_MAX   172
#define VOLTS_MIN  28  // 2.8V
#define ADC_MIN   116
// 2S cell configuration:
//#define BATT_LOW   61  // Cell voltage to step light down = 6.1 V
//#define BATT_CRIT  59  // Cell voltage to shut the light off = 5.9 V
// measure values for a full and empty battery, then plug them in
//#define VOLTS_MAX  84  // 8.4V
//#define ADC_MAX   225
//#define VOLTS_MIN  50  // 5.0V
//#define ADC_MIN   134
#else
#define BATT_LOW          30    // Cell voltage to step light down = 3.0 V
#define BATT_CRIT         28    // Cell voltage to shut the light off = 2.8 V
#endif

#define ADC_DELAY        312    // Delay in ticks between low-batt ramp-downs (312=5secs, 250=4secs, was: 188 ~= 3s)

// output to use for blinks on battery check mode (primary PWM level, alt PWM level)
// Use 20,0 for a single-channel driver or 0,40 for a two-channel driver
#define BLINK_BRIGHTNESS  (RAMP_SIZE/5)
#define BEACON_BRIGHTNESS (RAMP_SIZE/4)

// States (in modeState):
#define RAMPING_ST      (0)
#define TACTICAL_ST     (1)
#define LOCKOUT_ST      (2)
#define BEACON_ST       (3)
#define THERMAL_REG_ST  (4)
#define THERMAL_CAL_ST  (5)
#define BATT_READ_ST    (6)
#define TEMP_READ_ST    (7)


#define FIRM_VERS_MODE  81

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#include <avr/pgmspace.h>
#include <avr/io.h>
//#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#define OWN_DELAY           // Don't use stock delay functions.
#define USE_DELAY_S         // Also use _delay_s(), not just _delay_ms()

#include "tk-delay.h"

// MCU I/O pin assignments (most are now in tk-attiny.h):
#define SWITCH_PIN PB3      // Star 4,  MCU pin #2 - pin the switch is connected to

#define ADCMUX_TEMP 0b10001111  // ADCMUX register setup to read temperature
#ifdef VOLT_MON_PIN7
// ADCMUX register setup to read PIN7 referenced divider
//#define ADC_CHANNEL 0x01  // MUX 01 corresponds with PB2 (pin 7)
//#define ADCMUX_VCC	((1 << V_REF) | (0 << ADLAR) | ADC_CHANNEL)
#define ADCMUX_VCC	0b10000001
#else
#define ADCMUX_VCC  0b00001100  // ADCMUX register setup to read Vbg referenced to Vcc
#endif

#define DEBOUNCE_BOTH          // Comment out if you don't want to debounce the PRESS along with the RELEASE
                               // PRESS debounce is only needed in special cases where the switch can experience errant signals
#define DB_PRES_DUR 0b00000001 // time before we consider the switch pressed (after first realizing it was pressed)
#define DB_REL_DUR  0b00001111 // time before we consider the switch released
                               // each bit of 1 from the right equals 16ms, so 0x0f = 64ms


//---------------------------------------------------------------------------------------
#define RAMP_SIZE  sizeof(ramp_7135)
#define TURBO_DROP_MIN 115
// min level in ramping the turbo timeout will engage,
//    level 115 = 106 PWM, this is ~43%
#define TURBO_DROP_SET 102
// the level turbo timeout will set,
//    level 102 = 71 PWM, this is ~32%

//----------------------------------------------------
// Ramping Mode Levels, 150 total entries (2.4 secs)
//
// For FET+1: FET and one 350 mA 7135 (tested/verified):
//    level_calc.py 2 150 7135 10 0.3 150 FET 1 1 1500
//
// Note: PWM value of 4 should be ~5.5 mA, but a value of 10 actually measures ~5 ma
//----------------------------------------------------

PROGMEM const byte ramp_7135[] = {
    // original ramp, 1.7 lm to 133 lm
    //10,10,10,11,11,11,12,12,            13,13,14,15,15,16,17,18,
    //19,21,22,23,25,27,28,30,            32,34,37,39,42,44,47,50,
    //53,56,60,63,67,71,75,79,            84,88,93,98,103,108,114,120,
    //126,132,138,145,152,159,166,174,    182,190,198,206,215,224,234,243,
    //253,255,255,255,255,255,255,255,    255,255,255,255,255,255,255,255,
    // lower lows (0.2-0.4 lm moon), exact 100% 7135 at level=65
    4,4,5,5,5,6,6,7,                    8,8,9,10,11,12,13,14,
    15,17,18,20,22,23,25,27,            30,32,34,37,40,42,45,48,
    52,55,59,62,66,70,74,79,            83,88,93,98,104,109,115,121,
    127,133,140,146,153,160,168,175,    183,191,200,208,217,226,236,245,
    255,255,255,255,255,255,255,255,    255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,    255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,    255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,    255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,    255,255,255,255,255,255,255,255,
    255,255,255,255,255,0
};
PROGMEM const byte ramp_FET[]  = {
    0,0,0,0,0,0,0,0,                    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,                    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,                    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,                    0,0,0,0,0,0,0,0,

// 100%
    0,2,3,4,5,7,8,9,                    11,12,14,15,17,18,20,22,
    23,25,27,29,30,32,34,36,            38,40,42,44,47,49,51,53,
    56,58,60,63,66,68,71,73,            76,79,82,85,87,90,93,96,
    100,103,106,109,113,116,119,123,    126,130,134,137,141,145,149,153,
    157,161,165,169,173,178,182,186,    191,196,200,205,210,214,219,224,
    229,234,239,244,250,255

// 95%
//    0,1,2,3,4,6,7,8,10,11,13,14,16,17,19,20,21,23,25,27,28,30,32,34,36,38,39,41,44,46,48,50,53,55,57,59,62,64,67,69,72,75,77,80,82,85,88,91,95,97,100,103,107,110,113,116,119,123,127,130,133,137,141,145,149,152,156,160,164,169,172,176,181,186,190,194,199,203,208,212,217,222,227,231,237,242

// 90%
//    0,1,2,3,4,6,7,8,9,10,12,13,15,16,18,19,20,22,24,26,27,28,30,32,34,36,37,39,42,44,45,47,50,52,54,56,59,61,63,65,68,71,73,76,78,81,83,86,90,92,95,98,101,104,107,110,113,117,120,123,126,130,134,137,141,144,148,152,155,160,163,167,171,176,180,184,189,192,197,201,206,210,215,219,225,229

// 85%
//    0,1,2,3,4,5,6,7,9,10,11,12,14,15,17,18,19,21,22,24,25,27,28,30,32,34,35,37,39,41,43,45,47,49,51,53,56,57,60,62,64,67,69,72,73,76,79,81,85,87,90,92,96,98,101,104,107,110,113,116,119,123,126,130,133,136,140,143,147,151,154,158,162,166,170,174,178,181,186,190,194,198,203,207,212,216

// 80%
//    0,1,2,3,4,5,6,7,8,9,11,12,13,14,16,17,18,20,21,23,24,25,27,28,30,32,33,35,37,39,40,42,44,46,48,50,52,54,56,58,60,63,65,68,69,72,74,76,80,82,84,87,90,92,95,98,100,104,107,109,112,116,119,122,125,128,132,135,138,142,145,148,152,156,160,164,168,171,175,179,183,187,191,195,200,204

// 75%
//    0,1,2,3,3,5,6,6,8,9,10,11,12,13,15,16,17,18,20,21,22,24,25,27,28,30,31,33,35,36,38,39,42,43,45,47,49,51,53,54,57,59,61,63,65,67,69,72,75,77,79,81,84,87,89,92,94,97,100,102,105,108,111,114,117,120,123,126,129,133,136,139,143,147,150,153,157,160,164,168,171,175,179,183,187,191

// 70%
//    0,1,2,2,3,4,5,6,7,8,9,10,11,12,14,15,16,17,18,20,21,22,23,25,26,28,29,30,32,34,35,37,39,40,42,44,46,47,49,51,53,55,57,59,60,62,65,67,70,72,74,76,79,81,83,86,88,91,93,95,98,101,104,107,109,112,115,118,121,124,127,130,133,137,140,143,147,149,153,156,160,163,167,170,175,178

// 66.66%
//    0,1,1,2,3,4,5,5,7,7,9,9,11,11,13,14,15,16,17,19,19,21,22,23,25,26,27,29,31,32,33,35,37,38,39,41,43,45,47,48,50,52,54,56,57,59,61,63,66,68,70,72,75,77,79,81,83,86,89,91,93,96,99,101,104,107,109,112,115,118,121,123,127,130,133,136,139,142,145,149,152,155,159,162,166,169
};

#if 0       // ------------------   Do Not Use   --------------------
// level_calc.py 2 150 7135 4 0.3 150 FET 1 1 1500
PROGMEM const byte ramp_7135[] = {
    4,4,4,5,5,5,6,6,                  7,7,8,9,10,10,11,13,
    14,15,16,18,19,21,23,25,          27,29,31,34,36,39,42,45,
    48,51,55,58,62,66,70,75,          79,84,89,94,99,105,111,116,
    123,129,136,142,149,157,164,172,  180,188,197,205,214,224,233,243,  // 49-64
    253,255,255,255,255,255,255,255,  255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,  255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,  255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,  255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,  255,255,255,255,255,255,255,255,
    255,255,255,255,255,0                                               // 145-150
};
PROGMEM const byte ramp_FET[]  = {
    0,0,0,0,0,0,0,0,                  0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,                  0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,                  0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,                  0,0,0,0,0,0,0,0,                  // 49-64
    0,2,3,4,5,7,8,9,                  11,12,14,15,17,18,20,22,          // 65
    23,25,27,29,30,32,34,36,          38,40,42,44,47,49,51,53,          // 81
    56,58,60,63,66,68,71,73,          76,79,82,85,87,90,93,96,          // 97
    100,103,106,109,113,116,119,123,  126,130,134,137,141,145,149,153,  // 113-128
    157,161,165,169,173,178,182,186,  191,196,200,205,210,214,219,224,  // 129-144
    229,234,239,244,250,255                                             // 145-150
};
#endif  // -----------------------------------------------------

//---------------------------------------------------------------------------------------

// Turbo timeout values, 16 msecs each: 30secs, 60 secs, 90 secs, 120 secs, 3 mins, 5 mins, 10 mins
// unused
//PROGMEM const word turboTimeOutVals[] = {0, 1875, 3750, 5625, 7500, 11250, 18750, 37500};


//----------------------------------------------------------------
// Config Settings via UI, with default values:
//----------------------------------------------------------------
byte tacticalSet = 0;       // 1: system is in Tactical Mode
//byte tempAdjEnable = 1;     // 1: enable active temperature adjustments, 0: disable
// thermal regulation ceiling
byte tempCeil = DEFAULT_THERM_CEIL;
// unused
//byte turboTimeoutMode = 0;  // 0=disabled, 1=30s,2=60s,3=90s, 4=120s, 5=3min,6=5min,7=10min (2 mins is good for production)

//----------------------------------------------------------------
// Global state options
//----------------------------------------------------------------

// Ramping :
#define MAX_RAMP_LEVEL (RAMP_SIZE)
#define MAX_7135_LEVEL 65

volatile byte rampingLevel = 0;     // 0=OFF, 1..MAX_RAMP_LEVEL is the ramping table index, 255=moon mode
volatile byte TargetLevel = 0;      // what the user requested (may be higher than actual level, due to thermal regulation)
volatile byte ActualLevel;          // current brightness (may differ from rampingLevel or TargetLevel)
byte memorizedLevel = 65;           // mode memory (ish, not saved across battery changes)
byte preTurboLevel = 65;            // only used to return from double-click turbo
byte rampState = 0;                 // 0=OFF, 1=in lowest mode (moon) delay, 2=ramping Up, 3=Ramping Down, 4=ramping completed (Up or Dn)
byte rampLastDirState = 0;          // last ramping state's direction: 2=up, 3=down
byte dontToggleDir = 0;             // flag to not allow the ramping direction to switch//toggle
//volatile byte byDelayRamping = 0;   // when the ramping needs to be temporarily disabled
byte rampPauseCntDn;                // count down timer for ramping support


// State and count vars:

volatile byte modeState;            // Current operating state: 0=Ramping, 1=Tactical, 2=Lock Out, 3=Beacon mode, 4=Battery check, 5=Temp check

word wPressDuration = 0;

volatile byte fastClicks = 0;       // fast click counts for dbl-click, triple-click, etc.

volatile word wIdleTicks = 0;

volatile byte LowBattSignal = 0;    // a low battery has been detected - trigger/signal it

byte byStartupDelayTime;            // Used to delay the WDT from doing anything for a short period at power up

// Keep track of cell voltage in ISRs, 10-bit resolution required for impedance check
volatile int16_t voltageTenths = 0; // in volts * 10
volatile int16_t temperature = 0;   // in degC
volatile byte adc_step = 0;         // steps 0, 1 read voltage, steps 2, 3 reads temperature - only use steps 1, 3 values

volatile word wTempTicks = 0;       // set to TEMP_ADJ_PERIOD, and decremented

byte b10Clicks = 0;                 // special 10 click state for turning thermal reg ON/OFF

// Configuration settings storage in EEPROM
word eepos = 0; // (0..n) position of mode/settings stored in EEPROM (128/256/512)


/**************************************************************************************
* SetOutput - sets the PWM output value directly
* =========
* Don't use this directly; use SetLevel or SetActualLevel instead.
**************************************************************************************/
// TODO: add support for 1-channel and 3-channel drivers
void SetOutput(byte pwm1, byte pwm2)
{
    PWM_LVL = pwm1;
    ALT_PWM_LVL = pwm2;
}

/**************************************************************************************
* SetActualLevel - uses the ramping levels to set the PWM output
* ========      (0 is OFF, 1..RAMP_SIZE is the ramping index level)
**************************************************************************************/
void SetActualLevel(byte level)
{
    ActualLevel = level;
    if (level == 0)
    {
        SetOutput(0,0);
    }
    else
    {
        level -= 1;  // make it 0 based
        SetOutput(pgm_read_byte(ramp_FET  + level), pgm_read_byte(ramp_7135 + level));
    }
}

// Use this one if you also want to adjust the maximum brightness it'll step up to when cold.
void SetLevel(byte level)
{
    TargetLevel = level;
    SetActualLevel(level);
}

#define SetLevelSoft SetLevel
#ifndef SetLevelSoft
// FIXME: this interferes with handling double-clicks
// FIXME: looks bad when turning off, if ramp has repeated values at the bottom
// Like SetLevel, but with a ramp-up / ramp-down transition
void SetLevelSoft(byte level)
{
    uint8_t target_level = level;
    int8_t shift_amount;
    int16_t diff;
    do {
        diff = target_level - ActualLevel;
        shift_amount = (diff >> 2) | (diff!=0);
        ActualLevel += shift_amount;
        SetLevel(ActualLevel);
        _delay_ms(RAMP_SIZE/8);  // fast ramp
    } while (target_level != ActualLevel);
}
#endif

/**************************************************************************************
* Strobe
* ======
**************************************************************************************/
void Strobe(byte ontime, byte offtime)
{
    SetLevel(MAX_RAMP_LEVEL);
    _delay_ms(ontime);
    SetLevel(0);
    _delay_ms(offtime);
}

/**************************************************************************************
* Blink - do a # of blinks with a speed in msecs
* =====
**************************************************************************************/
void Blink(byte val, word speed)
{
    for (; val>0; val--)
    {
        SetLevel(BLINK_BRIGHTNESS);
        _delay_ms(speed);

        SetLevel(0);
        _delay_ms(speed<<2);    // 4X delay OFF
    }
}

/**************************************************************************************
* delay_unless_modeState_changed - like _delay_ms(), but aborts on mode change
* =========
* ms       : milliseconds to delay
* expected : abort if/when modeState doesn't match this value
**************************************************************************************/
uint8_t delay_unless_modeState_changed(uint16_t ms, uint8_t expected)
{
    //uint8_t expected = modeState;  // automatic mode; buggy
    while (ms > 0) {
        _delay_ms(1);
        ms --;
        if (modeState != expected) return 0;
    }
    return 1;
}

/**************************************************************************************
* DigitBlink - do a # of blinks with a speed in msecs
* =========
**************************************************************************************/
byte DigitBlink(byte val, byte blinkModeState)
{
    if (modeState != blinkModeState)    // if the mode changed, bail out
        return 0;

    // zeroes get only a short blink
    if (val == 0) {
        SetLevel(BLINK_BRIGHTNESS);
        delay_unless_modeState_changed(8, blinkModeState);
        SetLevel(0);
        if(!delay_unless_modeState_changed(375, blinkModeState)) return 0;
    }

    for (; val>0; val--)
    {
        SetLevel(BLINK_BRIGHTNESS);
        delay_unless_modeState_changed(250, blinkModeState);
        SetLevel(0);
        if(!delay_unless_modeState_changed(375, blinkModeState)) return 0;
    }
    return 1;   // Ok, not terminated
}

/**************************************************************************************
* BlinkOutNumber - blinks out a 2 digit #
* ==============
**************************************************************************************/
void BlinkOutNumber(byte num, byte blinkModeState)
{
    // FIXME: handle 1-digit and 3-digit numbers
    // FIXME: don't use "/" or "%"; this MCU doesn't have those
    if (!DigitBlink(num / 10, blinkModeState))
        return;
    _delay_ms(800);
    if (!DigitBlink(num % 10, blinkModeState))
        return;
    _delay_ms(2000);
}

/**************************************************************************************
* BlinkLVP - blinks the specified time for use by LVP
* ========
*  Supports both ramping and mode set modes.
**************************************************************************************/
void BlinkLVP(byte BlinkCnt, byte level)
{
    int nMsecs = 200;
    if (BlinkCnt > 5)
        nMsecs = 125;

    // Flash 'n' times before lowering
    while (BlinkCnt-- > 0)
    {
        SetLevel(0);
        _delay_ms(nMsecs);
        SetLevel(level);
        _delay_ms(nMsecs<<1);
    }
}

/**************************************************************************************
* IsPressed - debounce the switch release, not the switch press
* ==========
**************************************************************************************/
// Debounce switch press value
#ifdef DEBOUNCE_BOTH
int IsPressed()
{
    static byte pressed = 0;
    // Keep track of last switch values polled
    static byte buffer = 0x00;
    // Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
    buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);

    if (pressed) {
        // Need to look for a release indicator by seeing if the last switch status has been 0 for n number of polls
        pressed = (buffer & DB_REL_DUR);
    } else {
        // Need to look for pressed indicator by seeing if the last switch status was 1 for n number of polls
        pressed = ((buffer & DB_PRES_DUR) == DB_PRES_DUR);
    }

    return pressed;
}
#else
static int IsPressed()
{
    // Keep track of last switch values polled
    static byte buffer = 0x00;
    // Shift over and tack on the latest value, 0 being low for pressed, 1 for pulled-up for released
    buffer = (buffer << 1) | ((PINB & (1 << SWITCH_PIN)) == 0);

    return (buffer & DB_REL_DUR);
}
#endif


/**************************************************************************************
* PCINT_on - Enable pin change interrupts
* ========
**************************************************************************************/
inline void PCINT_on()
{
    // Enable pin change interrupts
    GIMSK |= (1 << PCIE);
}

/**************************************************************************************
* PCINT_off - Disable pin change interrupts
* =========
**************************************************************************************/
inline void PCINT_off()
{
    // Disable pin change interrupts
    GIMSK &= ~(1 << PCIE);
}

// Need an interrupt for when pin change is enabled to ONLY wake us from sleep.
// All logic of what to do when we wake up will be handled in the main loop.
EMPTY_INTERRUPT(PCINT0_vect);

/**************************************************************************************
* WDT_on - Setup watchdog timer to only interrupt, not reset, every 16ms
* ======
**************************************************************************************/
void WDT_on()
{
    // Setup watchdog timer to only interrupt, not reset, every 16ms.
    cli();                          // Disable interrupts
    wdt_reset();                    // Reset the WDT
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    WDTCR = (1<<WDIE);              // Enable interrupt every 16ms (was 1<<WDTIE)
    sei();                          // Enable interrupts
}

/**************************************************************************************
* WDT_off - turn off the WatchDog timer
* =======
**************************************************************************************/
void WDT_off()
{
    wdt_reset();                    // Reset the WDT
    MCUSR &= ~(1<<WDRF);            // Clear Watchdog reset flag
    WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
    WDTCR = 0x00;                   // Disable WDT
}

/**************************************************************************************
* ADC_on - Turn the AtoD Converter ON
* ======
**************************************************************************************/
void ADC_on()
{
    // Turn ADC on (13 CLKs required for conversion, go max 200 kHz for 10-bit resolution)
    ADMUX  = ADCMUX_VCC;                         // 1.1 V reference, not left-adjust, Vbg
    DIDR0 |= (1 << ADC1D);                       // disable digital input on ADC1 pin to reduce power consumption
    ADCSRA = (1 << ADEN ) | (1 << ADSC ) | 0x07; // enable, start, ADC clock prescale = 128 for 62.5 kHz
}

/**************************************************************************************
* ADC_off - Turn the AtoD Converter OFF
* =======
**************************************************************************************/
inline void ADC_off()
{
    ADCSRA &= ~(1<<ADEN); //ADC off
}

/**************************************************************************************
* SleepUntilSwitchPress - only called with the light OFF
* =====================
**************************************************************************************/
inline void SleepUntilSwitchPress()
{
    // This routine takes up a lot of program memory :(

    cli();          // Disable interrupts

    //  Turn the WDT off so it doesn't wake us from sleep. Will also ensure interrupts
    // are on or we will never wake up.
    WDT_off();

    ADC_off();      // Save more power -- turn the AtoD OFF

    // Need to reset press duration since a button release wasn't recorded
    wPressDuration = 0;

    sei();          // Enable interrupts

    //  Enable a pin change interrupt to wake us up. However, we have to make sure the switch
    // is released otherwise we will wake when the user releases the switch
    while (IsPressed()) {
        _delay_ms(16);
    }

    PCINT_on();

    //-----------------------------------------
    sleep_enable();
    sleep_bod_disable();  // try to disable Brown-Out Detection to reduce parasitic drain
    sleep_cpu();          // go to sleepy beddy bye
    sleep_disable();
    //-----------------------------------------

    // Hey, to get here, someone must have pressed the switch!!

    PCINT_off();    // Disable pin change interrupt because it's only used to wake us up

    ADC_on();       // Turn the AtoD back ON

    WDT_on();       // Turn the WDT back on to check for switch presses

}   // Go back to main program

/**************************************************************************************
* LoadConfig - gets the configuration settings from EEPROM
* ==========
*  config1 - 1st byte of stored configuration settings:
*   bits   0: tactical mode
*  config2 - 2nd byte of stored configuration settings
*   all bits: user-configured thermal ceiling value
*
**************************************************************************************/
inline void LoadConfig()
{
    uint8_t found = 0;
    uint8_t config1, config2;

    // find the config data
    for (eepos=0; (!found) && (eepos < 127); eepos++)
    {
        config1 = eeprom_read_byte((const byte *)eepos);
        config2 = eeprom_read_byte((const byte *)eepos+1);

        // Only use the data if a valid marker is found
        if (config1 < 0x80) {
            found = 1;
            break;
        }
    }

    // unpack the config data
    if (found)
    {
        tacticalSet = modeState = config1 & 0x01;   // ramping or tactical
        //tempAdjEnable = (config1 >> 1) & 0x01;      // temp regulation
        tempCeil = config2;                         // temperature limit
        // make sure nothing crazy in eeprom can cause a fire
        if (tempCeil > MAX_THERM_CEIL) {
            tempCeil = DEFAULT_THERM_CEIL;
        }
    }
    else
    {
        tacticalSet = 0;
        modeState = RAMPING_ST;
        eepos = 0;
    }
}

/**************************************************************************************
* SaveConfig - save the current mode with config settings
* ==========
*  Central method for writing (with wear leveling)
*
*  config1 - 1st byte of stored configuration settings:
*   bits   0: tactical mode
*  config2 - 2nd byte of stored configuration settings
*   all bits: user-configured thermal ceiling value
*
**************************************************************************************/
void SaveConfig()
{
    uint8_t config1;
    uint8_t config2;
    // Pack all settings into one byte
    //config1 = (byte)(tacticalSet | (tempAdjEnable << 1));
    config1 = (byte)(tacticalSet);
    config2 = tempCeil;

    byte oldpos = eepos;

    eepos = (eepos+1) & 127;  // wear leveling, use next cell

    cli();  // don't interrupt while writing eeprom; that's bad

    // Erase the last settings
    eeprom_write_byte((uint8_t *)(oldpos), 0xff);
    eeprom_write_byte((uint8_t *)(oldpos+1), 0xff);

    // Write the current settings
    eeprom_write_byte((uint8_t *)(eepos), config1);
    eeprom_write_byte((uint8_t *)(eepos+1), config2);

    sei();  // okay, interrupts are fine now
}

/****************************************************************************************
* ADC_vect - ADC done interrupt service routine
* ========
* This should execute every 64ms  (every 4 WDT ticks)
* So, voltage and temperature are each measured 4X per second.
****************************************************************************************/
ISR(ADC_vect)
{
    // (probably 4 values, since this happens 4X per second)
    #define NUM_TEMP_VALUES 4
    static int16_t temperature_values[NUM_TEMP_VALUES];  // for lowpass filter

    // Read in the ADC 10 bit value (0..1023)
    int16_t adcin = ADC;

    // Voltage Monitoring
    if (adc_step == 1)                          // ignore first ADC value from step 0
    {
        // Calculate cell voltage from ADC value
        #ifdef VOLT_MON_PIN7
        // ADC / 1.1V : ADC goes up with voltage
        // requires voltage divider on driver, to adjust volts to 0-1.1V range
        // ADC_MIN and ADC_MAX are 8-bit values, but we need 10-bit
        adcin = VOLTS_MIN + \
                 ((adcin - (ADC_MIN<<2)) * (VOLTS_MAX - VOLTS_MIN) / ((ADC_MAX<<2) - (ADC_MIN<<2)));
        voltageTenths = adcin;
        #else
        // 1.1V / ADC : ADC goes down with voltage
        // requires raw battery voltage on VCC pin, so 5.5V or less
        adcin = (11264 + adcin/2)/adcin + D1;   // in volts * 10: 10 * 1.1 * 1024 / ADC + D1, rounded
        if (voltageTenths > 0)
        {
            if (voltageTenths < adcin)          // crude low pass filter
                ++voltageTenths;
            if (voltageTenths > adcin)
                --voltageTenths;
        }
        else
            voltageTenths = adcin;              // prime on first read
        #endif
    }

    // Temperature monitoring
    if (adc_step == 3)                          // ignore first ADC value from step 2
    {
        //----------------------------------------------------------------------------------
        // Read the MCU temperature, applying a calibration offset value
        //----------------------------------------------------------------------------------
        // cancelled: adjust temperature values to fit in 0-255 range
        //            (should be able to cover about -40 C to 100 C)
        //            (cancelled because 13.2 fixed-point precision works better)
        adcin = adcin - 275 + TEMP_CAL_OFFSET;  // 300 = 25 degC

        // average the last few values to reduce noise
        // (noise will be amplified by predictive thermal algorithm, so noise is bad)
        if (temperature != 0)
        {
            uint8_t i;
            uint16_t total=0;

            // rotate array
            for(i=0; i<NUM_TEMP_VALUES-1; i++) {
                temperature_values[i] = temperature_values[i+1];
                total += temperature_values[i];
            }
            temperature_values[i] = adcin;
            total += adcin;

            // Original method, divide back to original range:
            //temperature = total >> 2;
            // More precise method: use noise as extra precision
            // (values are now basically fixed-point, signed 13.2)
            temperature = total;
        }
        else {  // prime on first read
            uint8_t i;
            for(i=0; i<NUM_TEMP_VALUES; i++)
                temperature_values[i] = adcin;
            // convert to 13.2 fixed-point
            //temperature = adcin<<2;
            // ... or not; all that matters here is that it's non-zero
            temperature = adcin;
        }
    }

    adc_step = (adc_step +1) & 0x03;             // increment but keep in range of 0..3

    if (adc_step < 2)                           // steps 0, 1 read voltage, steps 2, 3 read temperature
        ADMUX  = ADCMUX_VCC;
    else
        ADMUX  = ADCMUX_TEMP;
}


/**************************************************************************************
* WDT_vect - The watchdog timer - this is invoked every 16ms
* ========
**************************************************************************************/
ISR(WDT_vect)
{
    //static word wTurboTicks = 0;

    //static byte byBattCheck = 0;
    static byte byModeForMultiClicks = 0;   // the mode that originally was running before the 1st click

    #ifdef VOLTAGE_MON
    static byte byInitADCTimer = 0;
    static word adc_ticks = ADC_DELAY;
    static byte lowbatt_cnt = 0;
    #endif

    // Enforce a start-up delay so no switch actions processed initially
    if (byStartupDelayTime > 0)
    {
        --byStartupDelayTime;
        return;
    }

    if (wTempTicks > 0) // decrement each tick if active
        --wTempTicks;

    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    // Button is pressed
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    if (IsPressed())
    {
        if (wPressDuration < 2000)
            wPressDuration++;


        // Enter thermal calibration mode
        if (b10Clicks)
        {
            if (wPressDuration >= 62)   // if held more than 1 second
            {
                b10Clicks = 0;
                // enter thermal calibration mode
                modeState = THERMAL_REG_ST;
            }
        }

        // For Tactical, turn on MAX while button is depressed
        // FIXME: tactical mode acts weird when user fast-clicks 3 times
        //        (goes to battcheck mode, then next click goes back to tactical mode)
        else if ((modeState == TACTICAL_ST) && (wPressDuration == 1) && (fastClicks < 2))
        {
            rampingLevel = MAX_RAMP_LEVEL;
            SetLevel(rampingLevel);
        }

        // long-press for ramping
        else if ((wPressDuration >= SHORT_CLICK_DUR) && (modeState == RAMPING_ST))  // also in there was:  && !byDelayRamping
        {
            {
                switch (rampState)
                {
                    case 0:        // ramping not initialized yet
                        if (rampingLevel == 0)
                        {
                            rampState = 1;
                            rampPauseCntDn = RAMP_MOON_PAUSE;   // delay a little on moon

                            // set this to the 1st level
                            rampingLevel = 1;
                            SetLevel(1);

                            dontToggleDir = 0;                  // clear it in case it got set from a timeout
                        }
                        else
                        {
                            #ifdef RAMPING_REVERSE
                                if (dontToggleDir)
                                {
                                    rampState = rampLastDirState;      // keep it in the same
                                    dontToggleDir = 0;                 // clear it so it can't happen again til another timeout
                                }
                                else
                                    rampState = 5 - rampLastDirState;  // 2->3, or 3->2
                            #else
                                rampState = 2;  // lo->hi
                            #endif
                            if (rampingLevel == MAX_RAMP_LEVEL)
                            {
                                rampState = 3;  // hi->lo
                                SetLevel(MAX_RAMP_LEVEL);
                            }
                            else if (rampingLevel == 255)  // If stopped in ramping moon mode, start from lowest
                            {
                                rampState = 2; // lo->hi
                                rampingLevel = 1;
                                SetLevel(rampingLevel);
                            }
                            else if (rampingLevel == 1)
                                rampState = 2;  // lo->hi
                        }
                        break;

                    case 1:        // in moon mode
                        if (--rampPauseCntDn == 0)
                        {
                            rampState = 2; // lo->hi
                        }
                        break;

                    case 2:        // lo->hi
                        rampLastDirState = 2;
                        if (rampingLevel < MAX_RAMP_LEVEL)
                        {
                            rampingLevel ++;
                        }
                        else
                        {
                            rampState = 4;
                        }
                        if ((rampingLevel == MAX_RAMP_LEVEL) || (rampingLevel == MAX_7135_LEVEL)) {
                            SetActualLevel(0);  // Do a quick blink
                            _delay_ms(7);
                        }
                        SetLevel(rampingLevel);
                        dontToggleDir = 0;
                        break;

                    case 3:        // hi->lo
                        rampLastDirState = 3;
                        if (rampingLevel > 1)
                        {
                            rampingLevel --;
                        }
                        else
                        {
                            rampState = 4;
                        }
                        if ((rampingLevel == 1) || (rampingLevel == MAX_7135_LEVEL)) {
                            SetActualLevel(0);  // Do a quick blink
                            _delay_ms(7);
                        }
                        SetLevel(rampingLevel);
                        dontToggleDir = 0;
                        break;

                    case 4:        // ramping ended - 0 or max reached
                        break;

                } // switch on rampState
                // ensure turbo toggle will return to ramped level, even if the ramp was at turbo
                preTurboLevel = rampingLevel;

            } // switch held longer than single click
        } // else (not b10Clicks)

        //wTurboTicks = 0;      // Just always reset turbo timer whenever the button is pressed

        #ifdef VOLTAGE_MON
        adc_ticks = ADC_DELAY;  // Same with the ramp down delay
        #endif
    }

    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    // Not pressed (debounced qualified)
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    else
    {

        // Was previously pressed - button released
        if (wPressDuration > 0)
        {

            rampState = 0;  // reset state to not ramping

            // user got too hot and let go of button in thermal calibration mode
            // (or otherwise aborted that mode)
            if ((modeState == THERMAL_REG_ST) || (modeState == THERMAL_CAL_ST)) {
                // change modeState to let main() know it can save the calibration
                modeState = RAMPING_ST;
            }

            // Momentary / tactical mode
            if (modeState == TACTICAL_ST)
            {
                rampingLevel = 0;  // turn off output as soon as the user lets the switch go
                SetLevel(0);
            }

            // normal short click processing
            if (wPressDuration < SHORT_CLICK_DUR)
            {

                ++fastClicks;   // track fast clicks in a row from OFF or ON (doesn't matter in ramping)

                //-----------------------------------------------------------------------------------
                //-----------------------------------------------------------------------------------
                switch (modeState)
                {
                    case RAMPING_ST:
                        if (fastClicks == 1)
                        {
                            byModeForMultiClicks = modeState;       // save current mode

                            // if we were off, turn on at memorized level
                            if (rampingLevel == 0) {
                                rampingLevel = memorizedLevel;
                                // bugfix: make off->turbo work when memorizedLevel == turbo
                                preTurboLevel = memorizedLevel;
                                // also set up other ramp vars to make sure ramping will work
                                rampState = 2;  // lo->hi
                                rampLastDirState = 2;
                                if (rampingLevel == MAX_RAMP_LEVEL) {
                                    rampState = 3;  // hi->lo
                                    rampLastDirState = 3;
                                }
                                dontToggleDir = 0;
                            // if we were on, turn off
                            } else {
                                memorizedLevel = rampingLevel;
                                rampingLevel = 0;
                            }
                            SetLevelSoft(rampingLevel);

                            //byDelayRamping = 1;       // don't act on ramping button presses
                        }
                        else if (fastClicks == 10)      // --> 10 clicks: flip thermal regulation control
                            b10Clicks = 1;
                        else                            // --> 2+ clicks: turn off the output
                        {
                            // prevent flashes while entering long click sequences
                            rampingLevel = 0;
                            SetLevel(rampingLevel);
                        }
                        break;

                    case TACTICAL_ST:
                        if (fastClicks == 1)
                            byModeForMultiClicks = modeState;   // save current mode
                        else if (fastClicks == 10)              // --> 10 clicks: flip thermal regulation control
                            b10Clicks = 1;
                        break;

                    case BEACON_ST:
                    case BATT_READ_ST:
                    case TEMP_READ_ST:
                        if (fastClicks == 1)
                        {
                            byModeForMultiClicks = modeState;   // save current mode

                            modeState = tacticalSet;            // set to Ramping or Tactical
                            rampingLevel = 0;
                            SetLevel(rampingLevel);
                            //byDelayRamping = 1;                 // don't act on ramping button presses
                        }
                        break;
                } // switch

            } // short click
            else
            {
                fastClicks = 0; // reset fast clicks for long holds
                b10Clicks = 0;  // reset special 10 click flag
            }

            wPressDuration = 0;

            //byDelayRamping = 0;   // restore acting on ramping button presses, if disabled

            wIdleTicks = 0; // reset idle time

        } // previously pressed
        else
        {
            //---------------------------------------------------------------------------------------
            //---------------------------------------------------------------------------------------
            // Not previously pressed
            //---------------------------------------------------------------------------------------
            //---------------------------------------------------------------------------------------
            if (++wIdleTicks > 30000)
                wIdleTicks = 30000;     // max it out at 30,000 (8 minutes)

            if (wIdleTicks > DBL_CLICK_TICKS)   // Too much delay between clicks - time out the fast clicks
            {
                //-----------------------------------------------------------------------------------
                // Process multi clicks here, after it times out
                //-----------------------------------------------------------------------------------
                switch (modeState)
                {

                    // FIXME: disable extra states from tactical/momentary mode
                    //        (only do this in ramping mode, or from off)
                    case RAMPING_ST:
                    case TACTICAL_ST:
                        if (fastClicks == 2)            // --> double click
                        {
                            // --> battcheck to tempcheck
                            // FIXME: why is this handled here instead of below under battcheck state?
                            if (byModeForMultiClicks == BATT_READ_ST)
                            {
                                modeState = TEMP_READ_ST;
                            }
                            // --> ramp direct to MAX/turbo (or back)
                            else if (modeState == RAMPING_ST)
                            {
                                // make double-click toggle turbo, not a one-way trip
                                // Note: rampingLevel is zero here,
                                //       because double-click turns the light off and back on
                                if (memorizedLevel == MAX_RAMP_LEVEL) {
                                    rampingLevel = preTurboLevel;
                                } else {
                                    preTurboLevel = memorizedLevel;
                                    rampingLevel = MAX_RAMP_LEVEL;
                                }
                                SetLevelSoft(rampingLevel);
                            }
                        }

                        #ifdef TRIPLE_CLICK_BATT
                        else if (fastClicks == 3)       // --> triple click: display battery check/status
                        {
                            modeState = BATT_READ_ST;
                            //byDelayRamping = 1;         // don't act on ramping button presses
                        }
                        #endif

                        else if (fastClicks == 4)       // --> 4 clicks: set toggle Ramping/Tactical Mode
                        {
                            if (modeState == RAMPING_ST)
                                modeState = TACTICAL_ST;
                            else
                                modeState = RAMPING_ST;
                            //byDelayRamping = 1;         // don't act on ramping button presses
                        }

                        else if (fastClicks == 6)       // --> 6 clicks: set Lockout Mode
                        {
                            modeState = LOCKOUT_ST;
                            //byDelayRamping = 1;         // don't act on ramping button presses
                        }

                        else if (fastClicks == 8)       // --> 8 clicks: set Beacon Mode
                        {
                            modeState = BEACON_ST;
                            //byDelayRamping = 1;         // don't act on ramping button presses
                        }
                        break;

                    case LOCKOUT_ST:
                        if (fastClicks == 6)            // --> 6 clicks: clear Lock Out Mode
                        {
                            modeState = tacticalSet;    // set to Ramping or Tactical
                            //byDelayRamping = 1;         // don't act on ramping button presses
                        }
                        break;

                    case BEACON_ST:
                        break;

                    case BATT_READ_ST:
                    case TEMP_READ_ST:
                        break;
                }

                fastClicks = 0;
            }

            // For ramping, timeout the direction toggling
            if ((rampingLevel > 1) && (rampingLevel < MAX_RAMP_LEVEL))
            {
                if (wIdleTicks > RAMP_SWITCH_TIMEOUT)
                    dontToggleDir = 1;
            }

            //------------------------------------------------------------------------------
            // Do voltage monitoring when the switch isn't pressed
            //------------------------------------------------------------------------------
            #ifdef VOLTAGE_MON
            if (adc_ticks > 0)
                --adc_ticks;
            if (adc_ticks == 0)
            {
                // See if conversion is done
                {
                    byte atLowLimit;
                    atLowLimit = ((ActualLevel == 1));

                    // See if voltage is lower than what we were looking for
                    if (voltageTenths < (atLowLimit ? BATT_CRIT : BATT_LOW))
                        ++lowbatt_cnt;
                    else
                        lowbatt_cnt = 0;
                }

                // See if it's been low for a while
                if (lowbatt_cnt >= 4)
                {
                    LowBattSignal = 1;

                    lowbatt_cnt = 0;
                    // If we reach 0 here, main loop will go into sleep mode
                    // Restart the counter to when we step down again
                    adc_ticks = ADC_DELAY;
                }

            }

            #endif
        } // not previously pressed

        wPressDuration = 0;
    } // Not pressed

    // Schedule an A-D conversion every 4th timer (64 msecs)
    if ((++byInitADCTimer & 0x03) == 0)
        ADCSRA |= (1 << ADSC) | (1 << ADIE);    // start next ADC conversion and arm interrupt
}

/**************************************************************************************
* main - main program loop. This is where it all happens...
* ====
**************************************************************************************/
int main(void)
{

    // Set all ports to input, and turn pull-up resistors on for the inputs we are using
    DDRB = 0x00;
    //PORTB = (1 << SWITCH_PIN) | (1 << STAR3_PIN);

    PORTB = (1 << SWITCH_PIN);  // Only the switch is an input

    // Set the switch as an interrupt for when we turn pin change interrupts on
    PCMSK = (1 << SWITCH_PIN);

    // Set primary and alternate PWN pins for output
    // FIXME: add support for 1-channel and 3-channel drivers
    DDRB = (1 << PWM_PIN) | (1 << ALT_PWM_PIN);

    TCCR0A = PHASE;  // set this once here - don't use FAST anymore

    // Set timer to do PWM for correct output pin and set prescaler timing
    TCCR0B = 0x01;  // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)

    // Turn features on or off as needed
    #ifdef VOLTAGE_MON
    ADC_on();
    #else
    ADC_off();
    #endif

    ACSR |= (1<<7);  // Analog Comparator off

    // Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Shut things off now, just in case we come up in a partial power state
    cli();  // Disable interrupts
    PCINT_off();

    WDT_off();

    // Load config settings: tactical mode and temperature regulation
    LoadConfig();

    // Not really intended to be configurable;
    // only #defined for convenience
    #define THERM_HISTORY_SIZE 8
    // this allows us to measure change over time,
    // which allows predicting future state
    uint16_t temperatures[THERM_HISTORY_SIZE];
    // track whether array has been initialized yet
    uint8_t first_temp_reading = 1;


    #ifdef STARTUP_LIGHT_ON
    rampingLevel = MAX_RAMP_LEVEL;
    SetLevel(MAX_RAMP_LEVEL);
    #elif defined STARTUP_2BLINKS
    Blink(2, 80);           // Blink twice upon power-up
    #else
    SetLevel(0);
    #endif

    byte byPrevModeState = modeState;

    byStartupDelayTime = 25;  // 400 msec delay in the WDT interrupt handler

    WDT_on();  // Turn it on now (mode can be non-zero on startup)

    //------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------
    //  We will never leave this loop.  The WDT will interrupt to check for switch presses
    // and will change the mode if needed. If this loop detects that the mode has changed,
    // run the logic for that mode while continuing to check for a mode change.
    //------------------------------------------------------------------------------------
    //------------------------------------------------------------------------------------
    while(1)  // run forever
    {
        //---------------------------------------------------------------------------------------
        // State Changes and Main Mode Handling
        //---------------------------------------------------------------------------------------
        if (byPrevModeState != modeState)
        {
            byte savedPrevMode = byPrevModeState;
            byPrevModeState = modeState;

            switch (modeState)
            {
                case RAMPING_ST:
                    // user released button during thermal calibration:
                    if ((savedPrevMode == THERMAL_REG_ST) || (savedPrevMode == THERMAL_CAL_ST)) {
                        // set limit to whatever they requested
                        SaveConfig();
                        // ... and show them the new value
                        SetLevel(0);
                        _delay_ms(500);
                        BlinkOutNumber(tempCeil, RAMPING_ST);
                    }
                    else if (savedPrevMode == TACTICAL_ST)
                    {
                        SetLevel(0);
                        _delay_ms(250);
                        Blink(2, 60);

                        tacticalSet = 0;    // Save the state for ramping
                        SaveConfig();
                    }
                    else if (savedPrevMode == LOCKOUT_ST)
                    {
                        SetLevel(0);
                        _delay_ms(250);
                        Blink(2, 60);
                    }
                    break;

                case TACTICAL_ST:  // Entering Tactical
                    if (savedPrevMode == RAMPING_ST)
                    {
                        SetLevel(0);
                        _delay_ms(250);
                        Blink(4, 60);

                        tacticalSet = 1;
                        SaveConfig();
                    }
                    // FIXME: We can go from lockout to tactical???
                    //        (this code should probably be removed)
                    else if (savedPrevMode == LOCKOUT_ST)
                    {
                        SetLevel(0);
                        _delay_ms(250);
                        Blink(2, 60);
                    }
                    break;

                case LOCKOUT_ST:       // Entering Lock Out
                    SetLevel(0);
                    _delay_ms(250);
                    Blink(4, 60);

                    //byDelayRamping = 0;       // restore acting on ramping button presses
                    break;

                case BEACON_ST:
                    _delay_ms(300); // pause a little initially
                    while (modeState == BEACON_ST)  // Beacon mode
                    {
                        SetLevel(memorizedLevel);   // allow user to set level
                        // on for 1/4 second
                        delay_unless_modeState_changed(250, BEACON_ST);
                        SetLevel(0);

                        // 2.5 s per cycle
                        delay_unless_modeState_changed(2250, BEACON_ST);
                    }
                    break;

                case THERMAL_REG_ST:
                    {
                        // First part of thermal calibration mode:
                        // check current value, option to set maximum,
                        // then proceed to actual calibration

                        // show previous ceiling value
                        byte oldCeil = tempCeil;
                        byte aborted = 0;
                        SetLevel(0);     // blink ON/OFF
                        _delay_ms(500);
                        BlinkOutNumber(oldCeil, THERMAL_REG_ST);
                        _delay_ms(500);
                        aborted = ! IsPressed();
                        if (aborted) {
                            // exit with no changes
                            modeState = RAMPING_ST;
                            break;
                        }

                        // buzz at a low level to give user time to set maximum level
                        tempCeil = MAX_THERM_CEIL;
                        for(uint8_t i=0; i<20; i++) {
                            SetLevel(MAX_RAMP_LEVEL/8);
                            _delay_ms(50);
                            SetLevel(MAX_RAMP_LEVEL/7);
                            _delay_ms(50);
                            aborted = ! IsPressed();
                            if (aborted) break;
                        }
                        if (aborted) {
                            // save and exit
                            modeState = RAMPING_ST;
                            break;
                        }

                        // else calibrate normally
                        tempCeil = oldCeil;
                        modeState = THERMAL_CAL_ST;
                    }
                    break;

                case BATT_READ_ST:
                    _delay_ms(400);  // delay a little here to give the user a chance to see a full blink sequence

                    //byDelayRamping = 0;       // restore acting on ramping button presses

                    while (modeState == BATT_READ_ST)  // Battery Check
                    {
                        // blink out volts and tenths
                        BlinkOutNumber(voltageTenths, BATT_READ_ST);
                    }
                    break;

                case TEMP_READ_ST:
                    _delay_ms(400);  // delay a little here to give the user a chance to see a full blink sequence
                    while (modeState == TEMP_READ_ST)  // Temperature Check
                    {
                        // blink out temperature in 2 digits
                        // (divide by 4 to get an integer because the value is 13.2 fixed-point)
                        uint8_t temp = 0;
                        // don't try to show negative numbers
                        if (temperature > 0) {
                            temp = temperature>>2;
                        }
                        BlinkOutNumber(temp, TEMP_READ_ST);
                    }
                    break;
            } // switch

        } // mode changed


        //---------------------------------------------------------------------
        // Perform low battery indicator tests
        //---------------------------------------------------------------------
        #ifdef USE_LVP
        if (LowBattSignal)
        {
            if (ActualLevel > 0)
            {
                if (ActualLevel == 1)
                {
                    // Reached critical battery level
                    BlinkLVP(8, RAMP_SIZE/6);    // blink more, brighter, and quicker (to get attention)
                    ActualLevel = rampingLevel = 0;    // Shut it down
                    rampState = 0;
                    // TODO: test if it actually shuts off as far as possible after LVP
                }
                else
                {
                    // Drop the output level when voltage is low
                    BlinkLVP(3, ActualLevel);  // 3 blinks as a warning
                    // decrease by half current ramp level
                    ActualLevel = (ActualLevel>>1);
                    if (ActualLevel < 1) { ActualLevel = 1; }
                }
                SetLevelSoft(ActualLevel);  // soft ramp down
            }
            LowBattSignal = 0;
        }
        #endif  // ifdef USE_LVP

        //---------------------------------------------------------------------
        // Temperature monitoring - step it down if too hot!
        //---------------------------------------------------------------------
        // Init thermal calibration mode, if we're in that mode.
        if (modeState == THERMAL_CAL_ST) {
            SetLevelSoft(MAX_RAMP_LEVEL);
        }
        // don't run unless a measurement has completed
        // don't run unless the light is actually on!
        if ((ActualLevel > 0) && (temperature > 0)) {

            if (first_temp_reading) {  // initial setup
                first_temp_reading = 0;
                uint8_t i;
                for (i=0; i<THERM_HISTORY_SIZE; i++)
                    temperatures[i] = temperature;
            }

            // should happen every half-second or so
            if (wTempTicks == 0) {
                // reset timer
                wTempTicks = TEMP_ADJ_PERIOD;

                uint8_t i;
                // only adjust if several readings in a row are outside desired range
                static uint8_t overheat_count = 0;
                static uint8_t underheat_count = 0;
                int16_t projected;  // Fight the future!
                int16_t diff;

                // algorithm tweaking; not really intended to be modified
                // how far ahead should we predict?
                #define THERM_PREDICTION_STRENGTH 4
                // how proportional should the adjustments be?
                #define THERM_DIFF_ATTENUATION 4
                // how low is the lowpass filter?
                #define THERM_LOWPASS 8
                // lowest ramp level; never go below this (sanity check)
                #define THERM_FLOOR (MAX_RAMP_LEVEL/8)
                // highest temperature allowed
                // (convert configured value to 13.2 fixed-point)
                #define THERM_CEIL (tempCeil<<2)
                // acceptable temperature window size in C
                #define THERM_WINDOW_SIZE 8

                // rotate measurements and add a new one
                for (i=0; i<THERM_HISTORY_SIZE-1; i++) {
                    temperatures[i] = temperatures[i+1];
                }
                temperatures[THERM_HISTORY_SIZE-1] = temperature;

                // guess what the temp will be several seconds in the future
                diff = temperatures[THERM_HISTORY_SIZE-1] - temperatures[0];
                projected = temperatures[THERM_HISTORY_SIZE-1] + (diff<<THERM_PREDICTION_STRENGTH);

                // never step down in thermal calibration mode
                if (modeState == THERMAL_CAL_ST) {
                    // use the current temperature as the new ceiling value
                    //tempCeil = projected >> 2;
                    // less aggressive prediction
                    tempCeil = (temperatures[THERM_HISTORY_SIZE-1]
                                + (diff<<(THERM_PREDICTION_STRENGTH-1))) >> 2;
                    //tempCeil = (temperature + (diff<<(THERM_PREDICTION_STRENGTH-1))) >> 2;
                    // Don't let user exceed maximum limit
                    if (tempCeil > MAX_THERM_CEIL) {
                        tempCeil = MAX_THERM_CEIL;
                    }
                }
                // too hot, step down (maybe)
                else if (projected >= THERM_CEIL) {
                    underheat_count = 0;  // we're definitely not too cold
                    if (overheat_count > THERM_LOWPASS) {
                        overheat_count = 0;

                        // how far above the ceiling?
                        int16_t exceed = (projected - THERM_CEIL) >> THERM_DIFF_ATTENUATION;
                        if (exceed < 1) { exceed = 1; }
                        uint8_t stepdown = ActualLevel - exceed;
                        // never go under the floor; zombies in the cellar
                        if (stepdown < THERM_FLOOR) {
                            stepdown = THERM_FLOOR;
                        }
                        // avoid a bug: stepping "down" from moon to THERM_FLOOR
                        // if user turned light down to moon during lowpass period
                        if (stepdown > TargetLevel) {
                            stepdown = TargetLevel;
                        }
                        // really, don't try to regulate below the floor
                        if (ActualLevel > THERM_FLOOR) {
                            SetActualLevel(stepdown);
                        }
                    } else {
                        overheat_count ++;
                    }
                }
                else {  // not too hot
                    overheat_count = 0;  // we're definitely not too hot
                    // too cold?  step back up?
                    if (projected < (THERM_CEIL - (THERM_WINDOW_SIZE<<2))) {
                        if (underheat_count > (THERM_LOWPASS/2)) {
                            underheat_count = 0;
                            // never go above the user's requested level
                            if (ActualLevel < TargetLevel) {
                                SetActualLevel(ActualLevel + 1);   // step up slowly
                            }
                        } else {
                            underheat_count ++;
                        }
                    }
                }
            }
        }  // thermal regulation

        //---------------------------------------------------------------------
        // Be sure switch is not pressed and light is OFF for at least 5 secs
        //---------------------------------------------------------------------
        word wWaitTicks = 300;   // ~5 secs

        if ((ActualLevel == 0) && !IsPressed() && (wIdleTicks > wWaitTicks))
        {
            wIdleTicks = 0;
            _delay_ms(1); // Need this here, maybe instructions for PWM output not getting executed before shutdown?
            SleepUntilSwitchPress();  // Go to sleep
        }

    } // while(1)

   return 0;  // Standard Return Code
}
