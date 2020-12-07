//-------------------------------------------------------------------------------------
// NarsilM.c - "the sword wielded by Isildur that cut the One Ring from Sauron's hand"
// =========
//			(Narsil is reforged as Andúril for Aragorn)
//
//  based on STAR_momentary.c v1.6 from JonnyC as of Feb 14th, 2015, Modified by Tom E
//
//  ***************  ATtiny 85 Version  ***************
//
//  This version is the base "eswitch" firmware merged in with STAR_NOINIT version to
// combine full e-switch support with a power switch capability to do mode changing, in
// addition to other enhancements.
//
// Capabilities:
//  - full e-switch support in standby low power mode
//  - full mode switching off a power switch (tail) w/memory using "NOINIT" or brownout method
//  - lock-out feature - uses a special sequence to lock out the e-switch (prevent accidental turn-on's)
//  - battery voltage level display in n.n format (ex: 4.1v blinks 4 times followed by 1 blink)
//  - multiple strobe and beacon modes
//  - 1, 2 or 3 output channel support
//  - mode configuration settings:
//			- ramping
//			- 12 mode sets to choose from
//			- moonlight mode on/off plus setting it's PWM level 1-7
//			- lo->hi or hi->lo ordering
//			- mode memory on e-switch on/off
// 		- timed/thermal stepdown setting
//			- blinkie mode configuration (off, strobe only, all blinkies)
//
// Change History - see Rev History.txt
//-------------------------------------------------------------------------------------

/*
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
 *		  Low: 0xE2 - 8 MHz CPU without a divider, 15.67kHz phase-correct PWM
 *	    High: 0xDE - enable serial prog/dnld, BOD enabled (or 0xDF for no BOD)
 *    Extra: 0xFF - self programming not enabled
 *
 *  --> Note: BOD enabled fixes lockups intermittently occurring on power up fluctuations, but adds slightly to parasitic drain
 *
 * VOLTAGE
 *		Resistor values for voltage divider (reference BLF-VLD README for more info)
 *		Reference voltage can be anywhere from 1.0 to 1.2, so this cannot be all that accurate
 *
 * For drivers with R1 and R2 before (or after, depending) the diode, I've been using:
 *   R1: 224  --> 220,000 ohms
 *   R2: 4702 -->  47,000 ohms
 *
 *  The higher values reduce parasitic drain - important for e-switch lights.
 *      
 */
#define FIRMWARE_VERS (13)	// v1.3 (in 10ths)

#define FET_7135_LAYOUT
#define ATTINY 85

//-----------------------------------------------------------------------------
// SETUPS Header File defined here:
//-----------------------------------------------------------------------------
#include "Setups_X9_2C1S.h"
//#include "Setups_2HL_1C1S.h"
//#include "Setups_ZY-T115_2C1S.h"
//#include "Setups_D1_2C1S.h"
//#include "Setups_ZY-T114_2C1S.h"
//#include "Setups_D4Ti.h"
//#include "Setups_X1R_2C1S.h"
//#include "Setups_AX3_2C1Son3C.h"
//#include "Setups_C20C#2_2C1S.h"
//#include "Setups_X7R_2C1S.h"
//#include "Setups_C8FXPLHI_2C1S.h"
//#include "Setups_C8F21700_2C1S.h"
//#include "Setups_X6R_2C1Son3C.h"
//#include "Setups_SparkSF3_2C1S.h"
//#include "Setups_Spark_2C1S.h"
//#include "setups_NA40_2C1S.h"
//#include "setups_M8_2C1Son3C.h"
//#include "setups_PK26_2C1S.h"
//#include "setups_JM70_2C1S.h"
//#include "setups_SP03.h"
//#include "setups_C8F_2C1S.h"
//#include "setups_SDMini_2C1S.h"
//#include "setups_2C1S.h"
//#include "setups_D4_2C1S.h"
//#include "Setups_OTRM3.h"
//#include "setups_S42_2C1S.h"
//#include "setups_2C2S.h"
//#include "setups_GTBuck.h"
//#include "setups_Q8_2C1S.h"
//#include "setups_3C1S.h"
//#include "setups_3C2S.h"

#if OUT_CHANNELS == 3
 #ifdef VOLT_MON_R1R2
  #ifdef ONBOARD_LED
   #error SETUPS.H ERROR - Cannot set both R1/R2 voltage monitoring and Onboard LED for 3 channels!!
  #endif
 #endif
#endif

#include "RegisterSettings.h"

#define byte uint8_t
#define word uint16_t
 
// Ignore a spurious warning, we did the cast on purpose
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#include <avr/pgmspace.h>
#include <avr/io.h>
//#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>	
#include <avr/eeprom.h>
#include <avr/sleep.h>
//#include <avr/power.h>


#define F_CPU 8000000UL			// 8 Mhz
#define BOGOMIPS (F_CPU/4000)	// set to 2000 for 8 Mhz
#define OWN_DELAY					// Don't use stock delay functions.
#define USE_DELAY_S				// Also use _delay_s(), not just _delay_ms()

#include "tk-delay.h"
#include "tk-random.h"

// Comment out all tk-calib headers if not using R1/R2 (for example: for the BLF Q8)
//#include "tk-calib.h"				// use this for the GT-buck driver or 2S cells using 360K or 220K 
//#include "tk-calibMTN17DDm.h"	// use this for the MtnE MTN17DDm v1.1 driver with external R1/R2


//#include "RampingTables_HiPerf.h"	// for very high performance, high output custom mods
//#include "ModeSets_HiPerf.h"		// for very high performance, high output custom mods
#include "RampingTables.h"
#include "ModeSets.h"

#define DEBOUNCE_BOTH				// Comment out if you don't want to debounce the PRESS along with the RELEASE
											//   PRESS debounce is only needed in special cases where the switch can experience errant signals
#define DB_PRES_DUR 0b00000001	// time before we consider the switch pressed (after first realizing it was pressed)
#define DB_REL_DUR  0b00001111	// time before we consider the switch released. Each bit of 1 from the right equals 16ms, so 0x0f = 64ms


// Timed stepdown values, 16 msecs each: 60 secs, 90 secs, 120 secs, 3 mins, 5 mins, 7 mins
PROGMEM const word timedStepdownOutVals[] = {0, 0, 3750, 5625, 7500, 11250, 18750, 26250};

const byte specialModes[] =    { SPECIAL_MODES_SET };
byte specialModesCnt = sizeof(specialModes);		// total count of modes in specialModes above
byte specModeIdx;

//----------------------------------------------------------------
// Config Settings via UI, with default values:
//----------------------------------------------------------------
volatile byte rampingMode;					// 1: ramping mode in effect, 0: mode sets
#if USING_3807135_BANK
volatile byte moonlightLevel;			// 0..7, 0: disabled, usually set to 3 (350 mA) or 5 (380 mA) - 2 might work on a 350 mA
#else
volatile byte moonlightLevel;			// 0..7, 0: disabled, usually set to 3 (350 mA) or 5 (380 mA) - 2 might work on a 350 mA
#endif
volatile byte stepdownMode;			// 0=disabled, 1=thermal, 2=60s, 3=90s, 4=120s, 5=3min, 6=5min, 7=7min (3 mins is good for production)
volatile byte blinkyMode;				// blinky mode config: 1=strobe only, 2=all blinkies, 0=disable

volatile byte modeSetIdx;				// 0..11, mode set currently in effect, chosen by user (3=4 modes)
volatile byte moonLightEnable;		// 1: enable moonlight mode, 0: disable moon mode
volatile byte highToLow;				// 1: highest to lowest, 0: modes go from lowest to highest
volatile byte modeMemoryEnabled;		// 1: save/recall last mode set, 0: no memory

volatile byte locatorLedOn;			// Locator LED feature (ON when light is OFF) - 1=enabled, 0=disabled
volatile byte bvldLedOnly;				// BVLD (Battery Voltage Level Display) - 1=BVLD shown only w/onboard LED, 0=both primary and onboard LED's
volatile byte onboardLedEnable;		// On Board LED support - 1=enabled, 0=disabled
//----------------------------------------------------------------

volatile byte OffTimeEnable = OFFTIME_ENABLE;	// 1: Do OFF time mode memory for Mode Sets on power switching (tailswitch), 0: disabled
volatile byte momentaryState = 0;					// 1: momentary mode in effect, 0: ramping or mode sets


// Ramping :
#define MAX_RAMP_LEVEL (RAMP_SIZE)

#ifdef TURBO_LEVEL_SUPPORT
 #define TURBO_LEVEL (MAX_RAMP_LEVEL+1)
#else
 #define TURBO_LEVEL (MAX_RAMP_LEVEL)
#endif


volatile byte rampingLevel = 0;		// 0=OFF, 1..MAX_RAMP_LEVEL is the ramping table index, 255=moon mode
byte rampState = 0;						// 0=OFF, 1=in lowest mode (moon) delay, 2=ramping Up, 3=Ramping Down, 4=ramping completed (Up or Dn)
byte rampLastDirState = 0;				// last ramping state's direction: 2=up, 3=down
byte  dontToggleDir = 0;				// flag to not allow the ramping direction to switch//toggle
byte savedLevel = 0;						// mode memory for ramping (copy of rampingLevel)
byte outLevel;								// current set rampingLevel (see rampinglevel above)
volatile byte byDelayRamping = 0;	// when the ramping needs to be temporarily disabled
byte rampPauseCntDn;						// count down timer for ramping support


// State and count vars:
byte modesCnt;			// total count of modes based on 'modes' arrays below

volatile byte byLockOutSet = 0;		// System is in LOCK OUT mode

volatile byte ConfigMode = 0;		// config mode is active: 1=init, 2=mode set,
											//   3=moonlight, 4=lo->hi, 5=mode memory, 6=done!)
byte prevConfigMode = 0;
volatile byte configClicks = 0;
volatile byte configIdleTime = 0;

volatile byte modeIdx = 0;			// current mode selected
volatile byte prevModeIdx = 0;	// used to restore the initial mode when exiting special/strobe mode(s)
volatile byte prevRamping = 0;	// used to restore the ramping state when exiting special/strobe mode(s)
word wPressDuration = 0;

volatile byte last_modeIdx;		// last value for modeIdx


volatile byte fastClicks = 0;		// fast click counts for dbl-click, triple-click, etc.

volatile byte modeMemoryLastModeIdx = 0;

volatile word wIdleTicks = 0;

word wTimedStepdownTickLimit = 0;			// current set timed stepdown limit (in 16 msec increments)

byte holdHandled = 0;	// 1=a click/hold has been handled already - ignore any more hold events

volatile byte LowBattSignal = 0;	// a low battery has been detected - trigger/signal it

volatile byte LowBattState = 0;	// in a low battery state (it's been triggered)
volatile byte LowBattBlinkSignal = 0;	// a periodic low battery blink signal

volatile byte locatorLed;			// Locator LED feature (ON when light is OFF) - 1=enabled, 0=disabled

byte byStartupDelayTime;			// Used to delay the WDT from doing anything for a short period at power up

// Keep track of cell voltage in ISRs, 10-bit resolution required for impedance check
volatile byte byVoltage = 0;		// in volts * 10
volatile byte byTempReading = 0;// in degC
volatile byte adc_step = 0;		// steps 0, 1 read voltage, steps 2, 3 reads temperature - only use steps 1, 3 values

volatile word wThermalTicks = 0;		// used in thermal stepdown and thermal calib -
												//   after a stepdown, set to TEMP_ADJ_PERIOD, and decremented
												//   in calib, use it to timeout a click response
byte byStepdownTemp = DEFAULT_STEPDOWN_TEMP;	// the current temperature in C for doing the stepdown

byte byBlinkActiveChan = 0;		// schedule a onboard LED blink to occur for showing the active channel


// Configuration settings storage in EEPROM
word eepos = 0;	// (0..n) position of mode/settings stored in EEPROM (128/256/512)

byte config1;	// keep these global, not on stack local
byte config2;
byte config3;


// OFF Time Detection
volatile byte noinit_decay __attribute__ ((section (".noinit")));

// channel specific versions
#include "Channels.h"


#ifdef VOLT_MON_R1R2
// Map the voltage level to the # of blinks, ex: 3.7V = 3, then 7 blinks
PROGMEM const uint8_t voltage_blinks[] = {
		ADC_22,22,	// less than 2.2V will blink 2.2
		ADC_23,23,		ADC_24,24,		ADC_25,25,		ADC_26,26,
		ADC_27,27,		ADC_28,28,		ADC_29,29,		ADC_30,30,
		ADC_31,31,		ADC_32,32,		ADC_33,33,		ADC_34,34,
		ADC_35,35,		ADC_36,36,		ADC_37,37,		ADC_38,38,
		ADC_39,39,		ADC_40,40,		ADC_41,41,		ADC_42,42,
		ADC_43,43,		ADC_44,44,
		255,   11,  // Ceiling, don't remove
};

/**************************************************************************************
* BattCheck - Returns number in 10ths of a volt for approximate battery voltage level
* =========
*  Uses the table: voltage_blinks above for return values
**************************************************************************************/
inline byte BattCheck()
{
	byte i;

	// The ADC values are the max value to use for the associated voltage value to report
	for (i=0; ; i += 2)
		if (byVoltage <= pgm_read_byte(voltage_blinks + i))
			break;

	// Drop one full level -- not sure why
	//if (i > 0)
	//	i -= 2;
	
	return pgm_read_byte(voltage_blinks + i + 1);
}
#endif


/**************************************************************************************
* SetConfigDefaults - set the stored configuration setting defaults
* =================
*  inline for now, but may be called by user invocation to reset to factory defaults
**************************************************************************************/
void SetConfigDefaults()
{
	rampingMode = DEF_RAMPING;					// 1: ramping mode in effect, 0: mode sets
	moonlightLevel = DEF_MOON_LEVEL;		// 0..7, 0: disabled, usually set to 3 (350 mA) or 5 (380 mA) - 2 might work on a 350 mA
	stepdownMode = DEF_STEPDOWN_MODE;	// 0=disabled, 1=thermal, 2=60s, 3=90s, 4=120s, 5=3min, 6=5min, 7=7min (3 mins is good for production)
	blinkyMode = DEF_BLINKY_MODE;			// blinky mode config: 1=strobe only, 2=all blinkies, 0=disable

	modeSetIdx = DEF_MODE_SET_IDX;				// 0..11, mode set currently in effect, chosen by user (3=4 modes)
	moonLightEnable = DEF_MOON_MODE;		// 1: enable moonlight mode, 0: disable moon mode
	highToLow = DEF_HIGH_TO_LOW;			// 1: highest to lowest, 0: modes go from lowest to highest
	modeMemoryEnabled = DEF_MODE_MEMORY;// 1: save/recall last mode set, 0: no memory

	locatorLedOn = DEF_LOCATOR_LED;		// Locator LED feature (ON when light is OFF) - 1=enabled, 0=disabled
	bvldLedOnly = DEF_BVLD_LED_ONLY;		// BVLD (Battery Voltage Level Display) - 1=BVLD shown only w/onboard LED, 0=both primary and onboard LED's
	onboardLedEnable = DEF_ONBOARD_LED;	// On Board LED support - 1=enabled, 0=disabled
}


/**************************************************************************************
* Strobe
* ======
**************************************************************************************/
void Strobe(byte ontime, byte offtime)
{
	FET_PWM_LVL = 255;

#ifdef GT_BUCK
	ONE7135_PWM_LVL = 255;	//  for GT-buck
	_delay_ms(ontime);
	ONE7135_PWM_LVL = 0;		//  for GT-buck
#else	
	_delay_ms(ontime);
	FET_PWM_LVL = 0;
#endif
	_delay_ms(offtime);
}

/**************************************************************************************
* EnterSpecialModes - enter special/strobe(s) modes
* =================
**************************************************************************************/
void EnterSpecialModes ()
{
	prevRamping = rampingMode;
	rampingMode = 0;				// disable ramping while in special/strobe modes
	
	specModeIdx = 0;
	modeIdx = specialModes[specModeIdx];
										
  #ifdef ONBOARD_LED
	TurnOnBoardLed(0);	// be sure the on board LED is OFF here
  #endif
}

/**************************************************************************************
* ExitSpecialModes - exit special/strobe mode(s)
* ================
**************************************************************************************/
void ExitSpecialModes ()
{
	rampingMode = prevRamping;	// restore ramping state
	rampingLevel = 0;
	
	if (rampingMode)				// for ramping, force mode back to 0
		modeIdx = 0;
	else
		modeIdx = prevModeIdx;
}

/**************************************************************************************
* Blink - do a # of blinks with a speed in msecs
* =====
**************************************************************************************/
void Blink(byte val, word speed)
{
	for (; val>0; val--)
	{
	  #ifdef ONBOARD_LED
		TurnOnBoardLed(1);
	  #endif
	  #ifndef BLINK_ONLY_IND_LED
		SetOutput(BLINK_BRIGHTNESS);
	  #endif

		_delay_ms(speed);
		
	  #ifdef ONBOARD_LED
		TurnOnBoardLed(0);
	  #endif

	  #ifndef BLINK_ONLY_IND_LED
		SetOutput(OFF_OUTPUT);
	  #endif

		_delay_ms(speed<<2);	// 4X delay OFF
	}
}

/**************************************************************************************
* NumBlink - do a # of blinks with a speed in msecs
* ========
**************************************************************************************/
void NumBlink(byte val, byte blinkModeIdx)
{
	for (; val>0; val--)
	{
		if (modeIdx != blinkModeIdx)	// if the mode changed, bail out
			break;
			
#ifdef ONBOARD_LED
		TurnOnBoardLed(1);
#endif
		
		if ((onboardLedEnable == 0) || (bvldLedOnly == 0))
			SetOutput(BLINK_BRIGHTNESS);
			
		_delay_ms(250);
		
#ifdef ONBOARD_LED
		TurnOnBoardLed(0);
#endif

		SetOutput(OFF_OUTPUT);
		_delay_ms(375);
		
		if (modeIdx != blinkModeIdx)	// if the mode changed, bail out
			break;
	}
}

/**************************************************************************************
* BlinkOutNumber - blinks out a # in 2 decimal digits
* ==============
**************************************************************************************/
void BlinkOutNumber(byte num, byte blinkMode)
{
	NumBlink(num / 10, blinkMode);
	if (modeIdx != blinkMode)		return;
	_delay_ms(800);
	NumBlink(num % 10, blinkMode);
	if (modeIdx != blinkMode)		return;
	_delay_ms(2000);
}

/**************************************************************************************
* BlinkLVP - blinks the specified time for use by LVP
* ========
*  Supports both ramping and mode set modes.
**************************************************************************************/
void BlinkLVP(byte BlinkCnt)
{
	int nMsecs = 250;
	if (BlinkCnt > 5)
		nMsecs = 150;
		
	// Flash 'n' times before lowering
	while (BlinkCnt-- > 0)
	{
		SetOutput(OFF_OUTPUT);
		_delay_ms(nMsecs);

#ifdef ONBOARD_LED
		TurnOnBoardLed(1);
#endif

		if (rampingMode)
			SetLevel(outLevel);
		else
		{
		  #if OUT_CHANNELS == 3		// Triple Channel
			SetOutput(currOutLvl1, currOutLvl2, currOutLvl3);
		  #elif OUT_CHANNELS == 2	// two channels
			SetOutput(currOutLvl1, currOutLvl2);
		  #else			
			SetOutput(currOutLvl);
		  #endif
		}
		_delay_ms(nMsecs*2);

#ifdef ONBOARD_LED
		TurnOnBoardLed(0);
#endif
	}
}

#ifdef ONBOARD_LED
/**************************************************************************************
* BlinkIndLed - blinks the indicator LED given time and count
* ===========
**************************************************************************************/
void BlinkIndLed(int nMsecs, byte nBlinkCnt)
{
	while (nBlinkCnt-- > 0)
	{
		TurnOnBoardLed(1);
		_delay_ms(nMsecs);
		TurnOnBoardLed(0);
		_delay_ms(nMsecs >> 1);		// make it 1/2 the time for OFF
	}
}
#endif

/**************************************************************************************
* ConfigBlink - do 2 quick blinks, followed by num count of long blinks
* ===========
**************************************************************************************/
void ConfigBlink(byte num)
{
	Blink(2, 40);
	_delay_ms(240);
	Blink(num, 100);

	configIdleTime = 0;		// reset the timeout after the blinks complete
}

/**************************************************************************************
* ClickBlink
* ==========
**************************************************************************************/
inline static void ClickBlink()
{
	SetOutput(CLICK_BRIGHTNESS);
	_delay_ms(100);
	SetOutput(OFF_OUTPUT);
}

/**************************************************************************************
* BlinkActiveChannel - blink 1 or 2 times to show which of 2 output channels is
*		active. Only the Ind. LED is used.
**************************************************************************************/
inline static void BlinkActiveChannel()
{
#ifdef ONBOARD_LED
	byte cnt = 1;
	
	if (outLevel == 0)	// if invalid, bail. outLevel should be 1..RAMP_SIZE or 255
		return;

  #if OUT_CHANNELS == 2
	if ((outLevel >= FET_START_LVL) && (outLevel < 255))	// 255 is special value for moon mode (use 1)
		cnt = 2;
  #endif
		
  #if OUT_CHANNELS == 3
	if (outLevel < BANK_START_LVL)
		cnt = 1;
	else if (outLevel < FET_START_LVL)
		cnt = 2;
	else if (outLevel < 255)	// 255 is special value for moon mode (use 1)
		cnt = 3;
  #endif
	
	_delay_ms(300);				// delay initially so a button LED can be seen
	for (; cnt>0; cnt--)
	{
		TurnOnBoardLed(1);
		_delay_ms(175);
		TurnOnBoardLed(0);
		if (cnt > 1)
			_delay_ms(175);
	}
#endif
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
* NextMode - switch's to next mode, higher output mode
* =========
**************************************************************************************/
void NextMode()
{
	if (modeIdx < 16)	// 11/16/14 TE: bug fix to exit strobe mode when doing a long press in strobe mode
		prevModeIdx	= modeIdx;

	if (++modeIdx >= modesCnt)
	{
		// Wrap around
		modeIdx = 0;
	}	
}

/**************************************************************************************
* PrevMode - switch's to previous mode, lower output mode
* =========
**************************************************************************************/
void PrevMode()
{
	prevModeIdx	 = modeIdx;

	if (modeIdx == 0)
		modeIdx = modesCnt - 1;	// Wrap around
	else
		--modeIdx;
}

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
inline void WDT_on()
{
	// Setup watchdog timer to only interrupt, not reset, every 16ms.
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = (1<<WDIE);				// Enable interrupt every 16ms (was 1<<WDTIE)
	sei();							// Enable interrupts
}

/**************************************************************************************
* WDT_off - turn off the WatchDog timer
* =======
**************************************************************************************/
inline void WDT_off()
{
	cli();							// Disable interrupts
	wdt_reset();					// Reset the WDT
	MCUSR &= ~(1<<WDRF);			// Clear Watchdog reset flag
	WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	WDTCR = 0x00;					// Disable WDT
	sei();							// Enable interrupts
}

/**************************************************************************************
* ADC_on - Turn the AtoD Converter ON
* ======
**************************************************************************************/
inline void ADC_on()
{
	// Turn ADC on (13 CLKs required for conversion, go max 200 kHz for 10-bit resolution)
  #ifdef VOLT_MON_R1R2
	ADMUX = ADCMUX_VCC_R1R2;			// 1.1v reference, not left-adjust, ADC1/PB2
  #else
	ADMUX  = ADCMUX_VCC_INTREF;		// not left-adjust, Vbg
  #endif
	DIDR0 |= (1 << ADC1D);					// disable digital input on ADC1 pin to reduce power consumption
	ADCSRA = (1 << ADEN ) | (1 << ADSC ) | 0x07;// enable, start, ADC clock prescale = 128 for 62.5 kHz
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
	
	wIdleTicks = 0;		// reset here
	wThermalTicks = 0;	// reset to allow thermal stepdown to go right away
	
	//  Turn the WDT off so it doesn't wake us from sleep. Will also ensure interrupts
	// are on or we will never wake up.
	WDT_off();
	
	ADC_off();		// Save more power -- turn the AtoD OFF
	
	// Need to reset press duration since a button release wasn't recorded
	wPressDuration = 0;
	
	//  Enable a pin change interrupt to wake us up. However, we have to make sure the switch
	// is released otherwise we will wake when the user releases the switch
	while (IsPressed()) {
		_delay_ms(16);
	}

	PCINT_on();
	
	//-----------------------------------------
   sleep_enable();
	sleep_bod_disable();		// try to disable Brown-Out Detection to reduce parasitic drain
	sleep_cpu();				// go to sleepy beddy bye
	sleep_disable();
	//-----------------------------------------
	
	// Hey, to get here, someone must have pressed the switch!!
	
	PCINT_off();	// Disable pin change interrupt because it's only used to wake us up

	ADC_on();		// Turn the AtoD back ON
	
	WDT_on();		// Turn the WDT back on to check for switch presses
	
}	// Go back to main program

/**************************************************************************************
* LoadConfig - gets the configuration settings from EEPROM
* ==========
*  config1 - 1st byte of stored configuration settings:
*   bits 0-2: mode index (0..7), for clicky mode switching
*   bits 3-6: selected mode set (0..11)
*   bit 7:    ramping mode
*
*  config2 - 2nd byte of stored configuration settings:
*   bit    0: mode ordering, 1=hi to lo, 0=lo to hi
*   bit    1: mode memory for the e-switch - 1=enabled, 0=disabled
*   bit  2-4: moonlight level, 1-7 enabled on the PWM value of 1-7, 0=disabled
*   bits 5-7: stepdown: 0=disabled, 1=thermal, 2=60s, 3=90s, 4=2min, 5=3min, 6=5min, 7=7min
*
*  config3 - 3rd byte of stored configuration settings:
*   bit    0: 1: Do OFF time mode memory on power switching (tailswitch), 0: disabled
*   bit    1: On Board LED support - 1=enabled, 0=disabled
*   bit    2: Locator LED feature (ON when light is OFF) - 1=enabled, 0=disabled
*   bit    3: BVLD LED Only - 1=BVLD only w/onboard LED, 0=both primary and onboard LED's are used
*   bit    4: 1: moonlight mode - 1=enabled, 0=disabled
*   bit  5-6: blinky mode config: 1=strobe only, 2=all blinkies, 0=disable
*
**************************************************************************************/
inline void LoadConfig()
{
	byte byMarker;

   // find the config data
   for (eepos=0; eepos < 128; eepos+=4)
	{    
	   config1 = eeprom_read_byte((const byte *)eepos);
		config2 = eeprom_read_byte((const byte *)eepos+1);
		config3 = eeprom_read_byte((const byte *)eepos+2);
		byMarker = eeprom_read_byte((const byte *)eepos+3);
		
		// Only use the data if a valid marker is found
	   if (byMarker == 0x5d)
   		break;
   }

   // unpack the config data
   if (eepos < 128)
	{
	   modeIdx = config1 & 0x7;
		modeSetIdx = (config1 >> 3) & 0x0f;
		rampingMode = (config1 >> 7);
		
	   highToLow = config2 & 1;
	   modeMemoryEnabled = (config2 >> 1) & 1;
		moonlightLevel = (config2 >> 2) & 0x07;
		stepdownMode = (config2 >> 5) & 0x07;
	
		OffTimeEnable = config3 & 1;
		onboardLedEnable = (config3 >> 1) & 1;
		locatorLedOn = (config3 >> 2) & 1;
		bvldLedOnly = (config3 >> 3) & 1;
		moonLightEnable = (config3 >> 4) & 1;
		blinkyMode = (config3 >> 5) & 0x03;
	}
	else
		eepos = 0;

	locatorLed = locatorLedOn;
}

/**************************************************************************************
* SaveConfig - save the current mode with config settings
* ==========
*  Central method for writing (with wear leveling)
*
*  config1 - 1st byte of stored configuration settings:
*   bits 0-2: mode index (0..7), for clicky mode switching
*   bits 3-6: selected mode set (0..11)
*   bit 7:    ramping mode
*
*  config2 - 2nd byte of stored configuration settings:
*   bit    0: mode ordering, 1=hi to lo, 0=lo to hi
*   bit    1: mode memory for the e-switch - 1=enabled, 0=disabled
*   bit  2-4: moonlight level, 1-7 enabled on the PWM value of 1-7, 0=disabled
*   bits 5-7: stepdown: 0=disabled, 1=thermal, 2=60s, 3=90s, 4=2min, 5=3min, 6=5min, 7=7min
*
*  config3 - 3rd byte of stored configuration settings:
*   bit    0: 1: Do OFF time mode memory on power switching (tailswitch), 0: disabled
*   bit    1: On Board LED support - 1=enabled, 0=disabled
*   bit    2: Locator LED feature (ON when light is OFF) - 1=enabled, 0=disabled
*   bit    3: BVLD LED Only - 1=BVLD only w/onboard LED, 0=both primary and onboard LED's are used
*   bit    4: 1: moonlight mode - 1=enabled, 0=disabled
*   bit  5-6: blinky mode config: 1=strobe only, 2=all blinkies, 0=disable
*
**************************************************************************************/
void SaveConfig()
{
	// Pack all settings into one byte
	config1 = (byte)(word)((modeIdx & 0x07) | ((modeSetIdx & 0x0f) << 3) | (rampingMode << 7));
	config2 = (byte)(word)(highToLow | (modeMemoryEnabled << 1) | (moonlightLevel << 2) | (stepdownMode << 5));
	config3 = (byte)(word)(OffTimeEnable | (onboardLedEnable << 1) | (locatorLedOn << 2) | (bvldLedOnly << 3) | (moonLightEnable << 4) | (blinkyMode << 5));
	
	byte oldpos = eepos;
	
	eepos = (eepos+4) & 127;  // wear leveling, use next cell

   cli();  // don't interrupt while writing eeprom; that's bad

	// Write the current settings (3 bytes)
	eeprom_write_byte((uint8_t *)(eepos), config1);
	eeprom_write_byte((uint8_t *)(eepos+1), config2);
	eeprom_write_byte((uint8_t *)(eepos+2), config3);
	
	// Erase the last settings
	eeprom_write_byte((uint8_t *)(oldpos), 0xff);
	eeprom_write_byte((uint8_t *)(oldpos+1), 0xff);
	eeprom_write_byte((uint8_t *)(oldpos+2), 0xff);
	eeprom_write_byte((uint8_t *)(oldpos+3), 0xff);
	
	// Write the validation marker byte out
	eeprom_write_byte((uint8_t *)(eepos+3), 0x5d);
	
	sei();  // okay, interrupts are fine now
}

/**************************************************************************************** 
* ADC_vect - ADC done interrupt service routine
* ========
****************************************************************************************/
ISR(ADC_vect)
{
	word wAdcVal = ADC;	// Read in the ADC 10 bit value (0..1023)

	// Voltage Monitoring
	if (adc_step == 1)							// ignore first ADC value from step 0
	{
	  #ifdef VOLT_MON_R1R2
		byVoltage = (byte)(wAdcVal >> 2);	// convert to 8 bits, throw away 2 LSB's
		//byVoltage = (byte)((wAdcVal + 2) >> 2);	// divide by 4, rounded
		//byVoltage = 218;	// test only!!
	  #else
		// Read cell voltage, applying the equation
		uint32_t voltage100ths;
		
		voltage100ths = (112640 + (wAdcVal >> 1))/wAdcVal + D1_DIODE;		// in 100th's volts: 100 * 1.1 * 1024 / ADC + D1_DIODE, rounded
		
		wAdcVal = voltage100ths / 10;	// divide by 10, rounded (converts to voltage in tenths)
		
		if (byVoltage > 0)
		{
			if (byVoltage < wAdcVal)			// crude low pass filter
				++byVoltage;
			if (byVoltage > wAdcVal)
				--byVoltage;
		}
		else
			byVoltage = (byte)wAdcVal;			// prime on first read
	  #endif
	} 
	
	// Temperature monitoring
	if (adc_step == 3)							// ignore first ADC value from step 2
	{
		//----------------------------------------------------------------------------------
		// Read the MCU temperature, applying a calibration offset value
		//----------------------------------------------------------------------------------
		wAdcVal = wAdcVal - 275 + TEMP_CAL_OFFSET;			// 300 = 25 degC
		
		if (byTempReading > 0)
		{
			if (byTempReading < wAdcVal)	// crude low pass filter
				++byTempReading;
			if (byTempReading > wAdcVal)
				--byTempReading;
		}
		else
			byTempReading = wAdcVal;						// prime on first read
	}
	
	//adc_step = (adc_step +1) & 0x3;	// increment but keep in range of 0..3
	if (++adc_step > 3)		// increment but keep in range of 0..3
		adc_step = 0;
		
	if (adc_step < 2)							// steps 0, 1 read voltage, steps 2, 3 read temperature
	{
	  #ifdef VOLT_MON_R1R2
		ADMUX = ADCMUX_VCC_R1R2;			// 1.1v reference, not left-adjust, ADC1/PB2
	  #else
		ADMUX  = ADCMUX_VCC_INTREF;		// not left-adjust, Vbg
	  #endif
	}
	else
		ADMUX = ADCMUX_TEMP;	// temperature
}

/**************************************************************************************
* WDT_vect - The watchdog timer - this is invoked every 16ms
* ========
**************************************************************************************/
ISR(WDT_vect)
{
	static word wStepdownTicks = 0;
	static byte byModeForClicks = 0;
	static byte initialLevelForClicks = 0;
	static byte turboRestoreLevel = 0;
	
  #ifdef VOLTAGE_MON
	static word wLowBattBlinkTicks = 0;
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

	if (wThermalTicks > 0)	// decrement each tick if active
		--wThermalTicks;

	//---------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------
   // Button is pressed
	//---------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------
	if (IsPressed())
	{
		if (wPressDuration < 2000)
			wPressDuration++;

		//---------------------------------------------------------------------------------------
		// Handle "button stuck" safety timeout
		//---------------------------------------------------------------------------------------
		if ((wPressDuration == 1250) && !momentaryState)	// 20 seconds
		{
			modeIdx = outLevel = rampingLevel = 0;
			rampState = 0;
		  #ifdef LOCKOUT_ENABLE
			byLockOutSet = 1;		// set "LOCK OUT"
		  #endif
			ConfigMode = 0;
			return;
		}
		
		//---------------------------------------------------------------------------------------
		// Handle config mode specially right here:
		//---------------------------------------------------------------------------------------
		if (ConfigMode > 0)
		{
			configIdleTime = 0;
			
			if (!holdHandled)
			{
				if (wPressDuration == 35)		// hold time for skipping: 35*16 = 560 msecs
					++ConfigMode;
				else if (wPressDuration == 70)	// hold time for bailing out: 70*16 = 1.1 secs
				{
					holdHandled = 1;		// suppress more hold events on this hold
					ConfigMode = 15;		// Exit Config mode
				}
			}
			return;
		}

		if (!holdHandled)
		{
			// For Tactical, turn on MAX while button is depressed
			if (momentaryState)
			{
				if ((wPressDuration == 1) && !byLockOutSet)
				{
					outLevel = rampingLevel = TURBO_LEVEL;
					SetLevel(outLevel);
				}
			}
			
			//------------------------------------------------------------------------------
			//	Ramping - pressed button
			//------------------------------------------------------------------------------
			else if (rampingMode)
			{
				if ((wPressDuration >= SHORT_CLICK_DUR) && !byLockOutSet && !byDelayRamping && (modeIdx < BATT_CHECK_MODE))
				{
					turboRestoreLevel = 0;	// reset on press&holds (for 2X click exit of turbo feature)
					
					switch (rampState)
					{
					 case 0:		// ramping not initialized yet
						if (rampingLevel == 0)
						{
							rampState = 1;
							rampPauseCntDn = RAMP_MOON_PAUSE;	// delay a little on moon

						 #if OUT_CHANNELS == 3		// Triple Channel
							SetOutput(moonlightLevel,0,0);
						 #elif OUT_CHANNELS == 2	// two channels
						  #ifdef GT_BUCK
							SetOutput(moonlightLevel,25);    // 10% for GT-buck analog control channel
						  #else
							SetOutput(moonlightLevel,0);
						  #endif
						 #else
							SetOutput(moonlightLevel);
						 #endif

#ifdef ONBOARD_LED
							if (locatorLed)
								TurnOnBoardLed(0);
#endif
								
							// set this to the 1st level for the current mode
							outLevel = rampingLevel = 255;
							
							if (savedLevel == 0)
								savedLevel = rampingLevel;

							dontToggleDir = 0;						// clear it in case it got set from a timeout
						}
						else
						{
							#ifdef RAMPING_REVERSE
								if (dontToggleDir)
								{
									rampState = rampLastDirState;			// keep it in the same
									dontToggleDir = 0;						//clear it so it can't happen again til another timeout
								}
								else
									rampState = 5 - rampLastDirState;	// 2->3, or 3->2
							#else
								rampState = 2;	// lo->hi
							#endif

							if (rampingLevel == 255)	// If stopped in ramping moon mode, start from lowest
							{
								rampState = 2; // lo->hi
								outLevel = rampingLevel = 1;
								SetLevel(outLevel);
							}
							else if (rampingLevel >= MAX_RAMP_LEVEL)
							{
								rampState = 3;	// hi->lo
								outLevel = MAX_RAMP_LEVEL;
								SetLevel(outLevel);
							}
							else if (rampingLevel == 1)
								rampState = 2;	// lo->hi
						}
						break;
						
					 case 1:		// in moon mode
						if (--rampPauseCntDn == 0)
						{
							rampState = 2; // lo->hi
							outLevel = rampingLevel = 1;
							SetLevel(outLevel);
						}
						break;
						
					 case 2:		// lo->hi
						rampLastDirState = 2;
						if (rampingLevel < MAX_RAMP_LEVEL)
						{
							outLevel = ++rampingLevel;
							savedLevel = rampingLevel;
						}
						else
						{
							rampState = 4;
							SetLevel(0);		// Do a quick blink
							_delay_ms(7);
						}
						SetLevel(outLevel);
						break;
						
					 case 3:		// hi->lo
						rampLastDirState = 3;
						if (rampingLevel > 1)
						{
							outLevel = --rampingLevel;
							savedLevel = rampingLevel;
						}
						else
						{
							rampState = 4;
							SetLevel(0);		// Do a quick blink
							_delay_ms(7);
						}
						SetLevel(outLevel);
						break;
					
					 case 4:		// ramping ended - 0 or max reached
						break;
						
					} // switch on rampState
					
				} // switch held longer than single click
				
			} // ramping
			
			//---------------------------------------------------------------------------------------
			// LONG hold - for previous mode
			//---------------------------------------------------------------------------------------
			if (!rampingMode && (wPressDuration == LONG_PRESS_DUR) && !byLockOutSet)
			{
				if ((modeIdx < 16) && !momentaryState)
				{
					// Long press
					if (highToLow)
						NextMode();
					else
					{
						if (modeIdx == 1)	// wrap around from lowest to highest
							modeIdx = 0;
						PrevMode();
					}
				}
				else if (modeIdx > SPECIAL_MODES)
				{
					if (specModeIdx > 0)
						--specModeIdx;
					else
						specModeIdx = specialModesCnt - 1;	// wrap around to last special mode
					modeIdx = specialModes[specModeIdx];
				}
			}

			//---------------------------------------------------------------------------------------
			// XLONG hold - for strobes, battery check, or lock-out (depending on preceding quick clicks)
			//---------------------------------------------------------------------------------------
			if (wPressDuration == XLONG_PRESS_DUR)
			{
			  #ifdef LOCKOUT_ENABLE
				if ((fastClicks == 2) && (wIdleTicks < DBL_CLICK_TICKS))
				{
				  #ifndef ADV_RAMP_OPTIONS
					if (!rampingMode || (byLockOutSet == 1))
				  #endif
					{
						modeIdx = outLevel = rampingLevel = 0;
						rampState = 0;
						byLockOutSet = 1 - byLockOutSet;		// invert "LOCK OUT"
						byDelayRamping = 1;		// don't act on ramping button presses
					}
				}
				else if (byLockOutSet == 0)
			  #endif
				{
					if ((fastClicks == 1) && (wIdleTicks < DBL_CLICK_TICKS))
					{
					  #ifndef ADV_RAMP_OPTIONS
						if (!rampingMode)
					  #endif
						{
							modeIdx = BATT_CHECK_MODE;
							byDelayRamping = 1;		// don't act on ramping button presses
							SetOutput(OFF_OUTPUT);	// suppress main LED output
						}
					}
					else if (modeIdx > SPECIAL_MODES)
					{
						ExitSpecialModes ();		// restore previous state (normal mode and ramping)
					}
					else if (modeIdx == BATT_CHECK_MODE)
					{
						modeIdx = 0;		// clear main mode
						SetOutput(OFF_OUTPUT);	// suppress main LED output
			
						ConfigMode = 21;		// Initialize Advanced Config mode
						configClicks = 0;
						holdHandled = 1;		// suppress more hold events on this hold
					}
					else if (modeIdx == FIRM_VERS_MODE)
					{
						modeIdx = 0;			// clear main mode
						SetOutput(OFF_OUTPUT);	// suppress main LED output
						
						ConfigMode = 14;		// Reset to Factory Defaults
						configClicks = 0;
						holdHandled = 1;		// suppress more hold events on this hold
					}
					else if (!rampingMode && (blinkyMode > 0) && (modeIdx != BATT_CHECK_MODE) && !momentaryState)
					{
						// Engage first special mode!
						EnterSpecialModes ();
					}
				}
			}
			
			//---------------------------------------------------------------------------------------
			// CONFIG hold - if it is not locked out or lock-out was just exited on this hold
			//---------------------------------------------------------------------------------------
			if ((byLockOutSet == 0) && !momentaryState)
			{
				if ((!rampingMode && (wPressDuration == CONFIG_ENTER_DUR) && (fastClicks != 2) && (modeIdx != BATT_CHECK_MODE))
											||
					 (rampingMode && (wPressDuration == 500)))	// 8 secs in ramping mode
				{
					modeIdx = 0;
					rampingLevel = 0;
					rampState = 0;
					
					// turn the light off initially
					SetOutput(OFF_OUTPUT);	// suppress main LED output
				
					ConfigMode = 1;
					configClicks = 0;
				}
			}
		}

		wStepdownTicks = 0;		// Just always reset stepdown timer whenever the button is pressed

		//LowBattState = 0;		// reset the Low Battery State upon a button press (NO - keep it active!)

	  #ifdef VOLTAGE_MON
		adc_ticks = ADC_DELAY;	// Same with the ramp down delay
	  #endif
	}
	
	//---------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------
   // Not pressed (debounced qualified)
	//---------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------
	else
	{
		holdHandled = 0;		// free up any hold suppressed state

		if (ConfigMode > 0)
		{
			if (wPressDuration > 0)
			{
				if (wPressDuration < LONG_PRESS_DUR)
					++configClicks;
				configIdleTime = 0;
			}
			else
			{
				++configIdleTime;
			}
			wPressDuration = 0;
		} // config mode
		
		else if (wPressDuration > 0)
		{
   		// Was previously pressed

			if (momentaryState)
			{
				outLevel = rampingLevel = 0;		// turn off output as soon as the user lets the switch go
				SetLevel(outLevel);
			}
			
			//------------------------------------------------------------------------------
			//	Ramping - button released
			//------------------------------------------------------------------------------
			if (rampingMode)
			{
				rampState = 0;	// reset state to not ramping
				
				if (wPressDuration < SHORT_CLICK_DUR)
				{
					// normal short click processing

					++fastClicks;	// track quick clicks in a row from OFF or ON (doesn't matter in ramping)

					if (fastClicks == 1)
					{
						byModeForClicks = modeIdx;		// save current mode
						if ((modeIdx < 16) && (rampingLevel == TURBO_LEVEL))
							byModeForClicks = 255;
						
						if ((modeIdx == BATT_CHECK_MODE) || (modeIdx == TEMP_CHECK_MODE) || (modeIdx == FIRM_VERS_MODE))
						{
							modeIdx = 0;			// battery check - reset to OFF
							outLevel = rampingLevel = 0;	// 08/28/16 TE: zero out outLevel here as well (used in LVP)
							byDelayRamping = 1;		// don't act on ramping button presses
						}
						
						initialLevelForClicks = rampingLevel;

						if (!byLockOutSet && !byDelayRamping && !momentaryState)
						{
							//---------------------------------
							// Normal click ON in ramping mode
							//---------------------------------
							if (rampingLevel == 0)  // light is OFF, turn it ON
							{
								if (savedLevel == 0)
									savedLevel = 1;

								dontToggleDir = 1;		// force direction to be lo->hi for a switch ON
								rampLastDirState = 2;	// lo->hi

								// Restore the saved level (last mode memory)
								outLevel = rampingLevel = savedLevel;

							  #ifdef ONBOARD_LED
								// Blink the Ind. LED from restoring last level
								if (onboardLedEnable)
									byBlinkActiveChan = 1;
							  #endif
							  
								wThermalTicks = TEMP_ADJ_PERIOD/3;	// Delay a temperature step down at least 15 secs or so
							}
							else        				// light is ON, turn it OFF
							{
								outLevel = rampingLevel = 0;
							}

							if (outLevel == 255)
							{
							 #if OUT_CHANNELS == 3		// Triple Channel
								SetOutput(moonlightLevel,0,0);
							 #elif OUT_CHANNELS == 2	// two channels
							  #ifdef GT_BUCK
								SetOutput(moonlightLevel,25); // for GT-buck
							  #else
								SetOutput(moonlightLevel,0);
							  #endif
							 #else
								SetOutput(moonlightLevel);
							 #endif

							  #ifdef ONBOARD_LED
								if (locatorLed)
									TurnOnBoardLed(0);
							  #endif
							}
							else
								SetLevel(outLevel);
						} // !byLockOutSet && !byDelayRamping
					} // fastClicks == 1
				} // short click
				else
				{
					fastClicks = 0;	// reset quick clicks for long holds
					
				  #ifdef ONBOARD_LED
					// Blink the Ind. LED from a ramping hold
					if (onboardLedEnable)
						byBlinkActiveChan = 1;
				  #endif
				}
				
				wPressDuration = 0;
				
				byDelayRamping = 0;	// restore acting on ramping button presses, if disabled
			} // ramping

			//------------------------------------------------------------------------------
			//	Non-Ramping - button released
			//------------------------------------------------------------------------------
			else
			{
				if (wPressDuration < LONG_PRESS_DUR)
				{
					// normal short click
					if ((modeIdx == BATT_CHECK_MODE) || (modeIdx == TEMP_CHECK_MODE) || (modeIdx == FIRM_VERS_MODE))		// battery check/firmware vers - reset to OFF
					{
						byModeForClicks = modeIdx;		// save current mode
						modeIdx = 0;
						++fastClicks;
					}
					else
					{
						// track quick clicks in a row from OFF
						if ((modeIdx == 0) && !fastClicks)
							fastClicks = 1;
						else if (fastClicks)
							++fastClicks;

						if (byLockOutSet == 0)
						{
							if (modeMemoryEnabled && (modeMemoryLastModeIdx > 0) && (modeIdx == 0))
							{
								if (!momentaryState)
								{
									modeIdx = modeMemoryLastModeIdx;
									modeMemoryLastModeIdx = 0;
								}
							}
							else if (modeIdx < 16)
							{
								if (!momentaryState)
								{
									if ((modeIdx > 0) && (wIdleTicks >= IDLE_TIME))
									{
										modeMemoryLastModeIdx = modeIdx;
										prevModeIdx = modeIdx;
										modeIdx = 0;	// Turn OFF the light
									}
									else
									{
										// Short press - normal modes
										if (highToLow)
											PrevMode();
										else
											NextMode();
									}
								}
							}
							else  // special modes
							{
								// Bail out if timed out the click
								if (wIdleTicks >= IDLE_TIME)
									ExitSpecialModes ();		// restore previous state (normal mode and ramping)
								
								// Bail out if configured for only one blinky mode
								else if (blinkyMode == 1)
									ExitSpecialModes ();		// restore previous state (normal mode and ramping)
								
								// Bail out if at last blinky mode
								else if (++specModeIdx >= specialModesCnt)
									ExitSpecialModes ();		// restore previous state (normal mode and ramping)
								else
									modeIdx = specialModes[specModeIdx];
							}
						}
					} // ...

					wPressDuration = 0;
				} // short click (wPressDuration < LONG_PRESS_DUR)
				else
				{
				  #ifdef ONBOARD_LED
					if (wPressDuration < XLONG_PRESS_DUR)
					{
						// Special Locator LED toggle sequence: quick click then click&hold
						if ((fastClicks == 1) && (wIdleTicks < DBL_CLICK_TICKS) && (modeIdx == 0))
						{
							locatorLed = 1 - locatorLed;
							TurnOnBoardLed(locatorLed);
						}
					}
				  #endif
				  
					fastClicks = 0;	// reset quick clicks for long holds
				}
			
			} // non-ramping
	
			wIdleTicks = 0;	// reset idle time
			
		} // previously pressed (wPressDuration > 0)
		
		else
		{
			//---------------------------------------------------------------------------------------
			//---------------------------------------------------------------------------------------
			// Not previously pressed
			//---------------------------------------------------------------------------------------
			//---------------------------------------------------------------------------------------
			if (++wIdleTicks == 0)
				wIdleTicks = 30000;		// max it out at 30,000 (8 minutes)

			if (wIdleTicks > DBL_CLICK_TICKS)
			{
				//-----------------------------------------------------------------------------------
				// Process multi clicks here, after it times out
				//-----------------------------------------------------------------------------------

				if (fastClicks == 5)	// --> 5X clicks: toggle momentary mode (click&hold - light ON at max)
				{
					if (rampingMode && !byLockOutSet && !momentaryState)
					{
						momentaryState = 1;
						modeIdx = outLevel = rampingLevel = 0;
						SetOutput(OFF_OUTPUT);	// suppress main LED output
					}
				}

				// For Lock-Out, always support 4 quick clicks regardless of mode/state
			  #ifdef LOCKOUT_ENABLE
				else if (fastClicks == 4)	// --> 4X clicks: do a lock-out
				{
					if ((rampingMode || byLockOutSet) && !momentaryState)	// only for ramping, or un-locking out
					{
						modeIdx = outLevel = rampingLevel = 0;
						rampState = 0;
						byLockOutSet = 1 - byLockOutSet;		// invert "LOCK OUT"
						byDelayRamping = 1;		// don't act on ramping button presses (ramping ON/OFF code below)
					}
				}
			  #endif

				// Support 2X clicks for Batt and Temp blink outs in ramping
				else if ((byModeForClicks == BATT_CHECK_MODE) || (byModeForClicks == TEMP_CHECK_MODE))	// battery check - multi-click checks
				{
					if (fastClicks == 2)			// --> double click: blink out the firmware vers #
					{
						if (byModeForClicks == BATT_CHECK_MODE)
							modeIdx = TEMP_CHECK_MODE;
						else
							modeIdx = FIRM_VERS_MODE;
						byDelayRamping = 1;		// don't act on ramping button presses
						SetOutput(OFF_OUTPUT);	// suppress main LED output
					}
				}
				else if (rampingMode && !byLockOutSet  && !momentaryState)
				{
				  #ifdef TRIPLE_CLICK_BATT
					if (fastClicks == 3)						// --> triple click: display battery check/status
					{
						modeIdx = BATT_CHECK_MODE;
						byDelayRamping = 1;		// don't act on ramping button presses
						SetOutput(OFF_OUTPUT);	// suppress main LED output
					}
				  #endif

					if (fastClicks == 2)						// --> double click: set to MAX level
					{
						if (byModeForClicks == 255)	// Turbo
						{
							if (blinkyMode > 0) // must be enabled
								EnterSpecialModes ();	// Engage first special mode!
							else
							{
								// Restore the saved level (last mode memory)
								if (savedLevel != MAX_RAMP_LEVEL)
									outLevel = rampingLevel = turboRestoreLevel;
								else
									outLevel = rampingLevel = 0;
								SetLevel(outLevel);
							}
						}
						else
						{
							rampingLevel = TURBO_LEVEL;
							outLevel = rampingLevel;
							SetLevel(outLevel);
							turboRestoreLevel = initialLevelForClicks;

							wThermalTicks = TEMP_ADJ_PERIOD/3;	// Delay a temperature step down at least 15 secs or so
						}
					}
				} // ramping && !byLockOutSet

				fastClicks = 0;	// clear it after processing it
				
			} // wIdleTicks > DBL_CLICK_TICKS

			// Only do timed stepdown check when switch isn't pressed
			if (stepdownMode > 1)
			{
				if (rampingMode)  // ramping
				{
					if ((outLevel > TIMED_STEPDOWN_MIN) && (outLevel < 255))	// 255= moon, so must add this check!
						if (++wStepdownTicks > wTimedStepdownTickLimit)
						{
							outLevel = rampingLevel = TIMED_STEPDOWN_SET;
							SetLevel(outLevel);
						}
				}
				else          // regular modes
				{
					if (modeIdx < 16)
						if (byFETModes[modeIdx] == 255)
						{
							if (++wStepdownTicks > wTimedStepdownTickLimit)
								PrevMode();		// Go to the previous mode
						}
				}
			}
			
			// For ramping, timeout the direction toggling
			if (rampingMode && (rampingLevel > 1) && (rampingLevel < MAX_RAMP_LEVEL))
			{
				if (wIdleTicks > RAMP_SWITCH_TIMEOUT)
					dontToggleDir = 1;
			}
			
			// Only do voltage monitoring when the switch isn't pressed
		  #ifdef VOLTAGE_MON
			if (adc_ticks > 0)
				--adc_ticks;
			if (adc_ticks == 0)
			{
				byte atLowLimit = (modeIdx == 1);
				if (rampingMode)
					atLowLimit = ((outLevel == 1) || (outLevel == 255));	// 08/28/16 TE: add check for moon mode (255)
				
				// See if voltage is lower than what we were looking for
			  #ifdef VOLT_MON_R1R2
				if (byVoltage < (atLowLimit ? ADC_CRIT : ADC_LOW))
			  #else
				if (byVoltage < (atLowLimit ? BATT_CRIT : BATT_LOW))
			  #endif
					++lowbatt_cnt;
				else
				{
					lowbatt_cnt = 0;
					LowBattState = 0;		// clear it if battery level has returned
				}
				
				// See if it's been low for a while (8 in a row)
				if (lowbatt_cnt >= 8)
				{
					LowBattSignal = 1;
					
					LowBattState = 1;
					
					lowbatt_cnt = 0;
					// If we reach 0 here, main loop will go into sleep mode
					// Restart the counter to when we step down again
					adc_ticks = ADC_DELAY;
				}
			}
			
			if (LowBattState)
			{
				if (++wLowBattBlinkTicks == 500)		// Blink every 8 secs
				{
					LowBattBlinkSignal = 1;
					wLowBattBlinkTicks = 0;
				}
			}
			
		  #endif
		} // not previously pressed
		
		wPressDuration = 0;
	} // Not pressed

	// Schedule an A-D conversion every 4th timer (64 msecs)
	if ((++byInitADCTimer & 0x03) == 0)
		ADCSRA |= (1 << ADSC) | (1 << ADIE);	// start next ADC conversion and arm interrupt
}

/**************************************************************************************
* main - main program loop. This is where it all happens...
* ====
**************************************************************************************/
int main(void)
{	
	byte i;

	// Set all ports to input, and turn pull-up resistors on for the inputs we are using
	DDRB = 0x00;

	PORTB = (1 << SWITCH_PIN);		// Only the switch is an input

	// Set the switch as an interrupt for when we turn pin change interrupts on
	PCMSK = (1 << SWITCH_PIN);
	
	// Set LED PWM pins for output
  #if OUT_CHANNELS == 3		// Triple Channel
	DDRB = (1 << ONE7135_PWM_PIN) | (1 << BANK_PWM_PIN) | (1 << FET_PWM_PIN);
  #elif OUT_CHANNELS == 2	// two channels
	DDRB = (1 << ONE7135_PWM_PIN) | (1 << FET_PWM_PIN);
  #else
	DDRB = (1 << MAIN_PWM_PIN);
  #endif

	TCCR0A = PHASE;		// set this once here - don't use FAST anymore

	// Set timer to do PWM for correct output pin and set prescaler timing
	TCCR0B = 0x01; // pre-scaler for timer (1 => 1, 2 => 8, 3 => 64...)
	
  #if OUT_CHANNELS == 3		// Triple Channel
	// Second PWM counter is ... weird (straight from bistro-tripledown.c)
	TCCR1 = _BV (CS10);
	GTCCR = _BV (COM1B1) | _BV (PWM1B);
	OCR1C = 255;  // Set ceiling value to maximum
  #endif

  #ifdef S42_PINS  			// special S42 driver (uses PWM on PB4 for the 7135)
  // Second PWM counter is ... weird (straight from bistro-tripledown.c)
  TCCR1 = _BV (CS10);
  GTCCR = _BV (COM1B1) | _BV (PWM1B);
  OCR1C = 255;  // Set ceiling value to maximum
  #endif

  #ifdef TWOCHAN_3C_PINS	// using a triple channel board for 2 channels
  // Second PWM counter is ... weird (straight from bistro-tripledown.c)
  TCCR1 = _BV (CS10);
  GTCCR = _BV (COM1B1) | _BV (PWM1B);
  OCR1C = 255;  // Set ceiling value to maximum
  #endif


	// Turn features on or off as needed
	ADC_on();

	ACSR   |=  (1<<7);	// Analog Comparator off

	// Enable sleep mode set to Power Down that will be triggered by the sleep_mode() command.
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	// Shut things off now, just in case we come up in a partial power state
	cli();							// Disable interrupts
	PCINT_off();

	{	// WDT_off() in line, but keep ints disabled
	 wdt_reset();					// Reset the WDT
	 MCUSR &= ~(1<<WDRF);			// Clear Watchdog reset flag
	 WDTCR |= (1<<WDCE) | (1<<WDE);  // Start timed sequence
	 WDTCR = 0x00;					// Disable WDT
	}
	
	SetConfigDefaults();
	
	// Load config settings: mode, mode set, lo-hi, mode memory
	LoadConfig();
	
	// Load the configured temperature to use as the threshold to step down output
   byte inValue = eeprom_read_byte((const byte *)250);	// use address 250 in the EEPROM space
	if ((inValue > 0) && (inValue < 255))
		byStepdownTemp = inValue;

	DefineModeSet();

	wTimedStepdownTickLimit = pgm_read_word(timedStepdownOutVals+stepdownMode);
	
	if (OffTimeEnable && !rampingMode)
	{
		if (!noinit_decay)
		{
			// Indicates they did a short press, go to the next mode
			NextMode(); // Will handle wrap arounds
			SaveConfig();
		}
	}
	else
	{
	  #ifdef STARTUP_LIGHT_OFF
		modeIdx = 0;
	  #else
		modeIdx = modesCnt - 1;
		if (ramping)
		{
			outLevel = rampingLevel = TURBO_LEVEL;
			SetLevel(outLevel);
		}
	  #endif
	}

  #ifdef STARTUP_2BLINKS
	if (modeIdx == 0)
	{
		// If tail mode switching is enabled for mode set operation, don't blink the LED
		if (rampingMode || !OffTimeEnable)
			Blink(2, 80);
	}
  #endif

	// set noinit data for next boot
	noinit_decay = 0;  // will decay to non-zero after being off for a while

	last_modeIdx = 250;	// make it invalid for first time
	
   byte byPrevLockOutSet = 0;			// copy of byLockOutSet to detect transitions
	byte byPrevMomentaryState = 0;	// copy of momentaryState to detect transitions

   byte prevConfigClicks = 0;

	byStartupDelayTime = 25;	// 400 msec delay in the WDT interrupt handler
	
	WDT_on();		// Turn it on now (mode can be non-zero on startup)

	//------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------	
	//  We will never leave this loop.  The WDT will interrupt to check for switch presses
	// and will change the mode if needed. If this loop detects that the mode has changed,
	// run the logic for that mode while continuing to check for a mode change.
	//------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------
	while(1)		// run forever
	{
      //---------------------------------------------------------------------------------------
		if (ConfigMode == 0)					// Normal mode
      //---------------------------------------------------------------------------------------
		{
			// Handle Lockout mode transtions
			if (byPrevLockOutSet != byLockOutSet)
			{
				byPrevLockOutSet = byLockOutSet;

				SetOutput(OFF_OUTPUT);
				Blink(4, 75);
				
				byDelayRamping = 0;		// restore acting on ramping button presses
				
				if (byLockOutSet)
					last_modeIdx = modeIdx;	// entering - no need to configure mode 0 (keep the locator LED off)
				else
					last_modeIdx = 255;		// exiting - force a mode handling to turn on locator LED
			}

			// Handle momentary/tactical mode transitions
			if (byPrevMomentaryState != momentaryState)
			{
				byPrevMomentaryState = momentaryState;

				Blink(2, 60);		// Special blink sequence for momentary mode (into or out of)
				_delay_ms(250);
				Blink(3, 75);
			}

			//---------------------------------------------------
			// Mode Handling - did the WDT change the mode?
			//---------------------------------------------------
			if (modeIdx != last_modeIdx)
			{
				if (modeIdx < 16)
				{
					SetMode(modeIdx);      // Set a solid mode here!!
					last_modeIdx = modeIdx;
				}
				else
				{
					last_modeIdx = modeIdx;

					// If coming from a standard mode, suppress alternate PWM output
					SetOutput(OFF_OUTPUT);	// suppress main LED output

					if (modeIdx == BATT_CHECK_MODE)
					{
						_delay_ms(400);	// delay a little here to give the user a chance to see a full blink sequence

						byDelayRamping = 0;		// restore acting on ramping button presses
						
						while (modeIdx == BATT_CHECK_MODE)	// Battery Check
						{
							// blink out volts and tenths
						  #ifdef VOLT_MON_R1R2
							byte voltageNum = BattCheck();
							BlinkOutNumber(voltageNum, BATT_CHECK_MODE);
						  #else
							BlinkOutNumber((byte)byVoltage, BATT_CHECK_MODE);
						  #endif
						}
					}

					else if (modeIdx == TEMP_CHECK_MODE)
					{
						_delay_ms(250);
						byDelayRamping = 0;		// restore acting on ramping button presses

						while (modeIdx == TEMP_CHECK_MODE)	// Temperature Check
						{
							// blink out temperature in 2 digits
							BlinkOutNumber(byTempReading, TEMP_CHECK_MODE);
						}
					}

					else if (modeIdx == FIRM_VERS_MODE)
					{
						_delay_ms(250);
						byDelayRamping = 0;		// restore acting on ramping button presses
						
						while (modeIdx == FIRM_VERS_MODE)	// Battery Check
						{
							// blink out volts and tenths
							byte vers = FIRMWARE_VERS;
							BlinkOutNumber(vers, FIRM_VERS_MODE);
						}
					}

					else if (modeIdx == STROBE_MODE)
					{
						while (modeIdx == STROBE_MODE)      // strobe at 16 Hz
						{
							Strobe(STROBE_SPEED);	// 14,41: 18Hz, 16,47: 16Hz, 20,60: 12.5Hz
						}
					}

#if RANDOM_STROBE
					else if (modeIdx == RANDOM_STROBE)
					{
						while (modeIdx == RANDOM_STROBE)		// pseudo-random strobe
						{
							byte ms = 34 + (pgm_rand() & 0x3f);
							Strobe(ms, ms);
							Strobe(ms, ms);
						}
					}
#endif

					else if (modeIdx == POLICE_STROBE)
					{
						while (modeIdx == POLICE_STROBE)		// police strobe
						{
							for(i=0;i<8;i++)
							{
								if (modeIdx != POLICE_STROBE)		break;
								Strobe(20,40);
							}
							for(i=0;i<8;i++)
							{
								if (modeIdx != POLICE_STROBE)		break;
								Strobe(40,80);
							}
						}
					}

					else if (modeIdx == BIKING_STROBE)
					{
						while (modeIdx == BIKING_STROBE)		// police strobe
						{
							// normal version
							for(i=0;i<4;i++)
							{
								if (modeIdx != BIKING_STROBE)		break;
								SetOutput(MAX_BRIGHTNESS);
								_delay_ms(5);
								SetOutput(MAX_7135);
								_delay_ms(65);
							}
							for(i=0;i<10;i++)
							{
								if (modeIdx != BIKING_STROBE)		break;
								_delay_ms(72);
							}
						}
						SetOutput(OFF_OUTPUT);
					}

					else if (modeIdx == BEACON_2S_MODE)
					{
						while (modeIdx == BEACON_2S_MODE)		// Beacon 2 sec mode
						{
							_delay_ms(300);	// pause a little initially
						
							Strobe(125,125);		// two flash's
							Strobe(125,125);
						
							for (i=0; i < 15; i++)	// 1.5 secs delay
							{
								if (modeIdx != BEACON_2S_MODE)		break;
								_delay_ms(100);
							}
						}
					}

					else if (modeIdx == BEACON_10S_MODE)
						while (modeIdx == BEACON_10S_MODE)		// Beacon 10 sec mode
						{
							_delay_ms(300);	// pause a little initially

							Strobe(240,240);		// two slow flash's
							Strobe(240,240);

							for (i=0; i < 100; i++)	// 10 secs delay
							{
								if (modeIdx != BEACON_10S_MODE)		break;
								_delay_ms(100);
							}
						}
				}
			} // mode change detected
			

			//---------------------------------------------------------------------
			// Perform low battery indicator tests
			//---------------------------------------------------------------------
			if (LowBattSignal)
			{
				if (rampingMode)
				{
					if (outLevel > 0)
					{
						if ((outLevel == 1) || (outLevel == 255))		// 8/27/16 TE: 255 is special moon mode
						{
							// Reached critical battery level
							outLevel = 7;	// bump it up a little for final shutoff blinks
							BlinkLVP(8);	// blink more and quicker (to get attention)
							outLevel = rampingLevel = 0;	// Shut it down
							rampState = 0;
						}
						else
						{
							// Drop the output level
							BlinkLVP(3);	// normal 3 blinks
							if (outLevel > MAX_RAMP_LEVEL/16)
								outLevel = ((int)outLevel * 4) / 5;	// drop the level (index) by 20%
							else
								outLevel = 1;
						}
						SetLevel(outLevel);
						_delay_ms(250);		// delay a little here before the next drop, if it happens quick
					}
				}
				else   // Not ramping
				{
					if (modeIdx > 0)
					{
						if (modeIdx == 1)
						{
							// Reached critical battery level
							BlinkLVP(8);	// blink more and quicker (to get attention)
						}
						else
						{
							// Drop the output level
							BlinkLVP(3);	// normal 3 blinks
						}
						if (modeIdx < 16)
							PrevMode();
					}
				}
				LowBattSignal = 0;
			}
			else if (LowBattBlinkSignal)
			{
#ifdef ONBOARD_LED
				// Blink the Indicator LED twice
				if (onboardLedEnable)
				{
					// Be sure to leave the indicator LED in it's proper state after blinking twice
					if ((locatorLed != 0) &&
							((rampingMode && (outLevel != 0)) || (!rampingMode && (modeIdx > 0))))
					{
						BlinkIndLed(500, 2);
					}
					else
					{
						TurnOnBoardLed(0);
						_delay_ms(500);
						TurnOnBoardLed(1);
						_delay_ms(500);
						TurnOnBoardLed(0);
						_delay_ms(500);
						TurnOnBoardLed(1);
					}
				}
#endif
				LowBattBlinkSignal = 0;
			} // low battery signal


			//---------------------------------------------------------------------
			// Temperature monitoring - step it down if too hot!
			//---------------------------------------------------------------------
			if ((stepdownMode == 1) && (byTempReading >= byStepdownTemp) && (wThermalTicks == 0))
			{
				if (rampingMode)
				{
					if ((outLevel >= FET_START_LVL) && (outLevel < 255))	// 255 is special value for moon mode (use 1)
					{
						int newLevel = outLevel - outLevel/6;	// reduce by 16.7%
					
						if (newLevel >= FET_START_LVL)
							outLevel = newLevel;	// reduce by 16.7%
						else
							outLevel = FET_START_LVL - 1;	// make it the max of the 7135
					
						SetLevel(outLevel);
						
						wThermalTicks = TEMP_ADJ_PERIOD;
					  #ifdef ONBOARD_LED
						BlinkIndLed(500, 3);	// Flash the Ind LED to show it's lowering
					  #endif
					}
				}
				else	// modes
				{
					if ((modeIdx > 0) && (modeIdx < 16))
					{
						PrevMode();	// Drop the output level
						
						wThermalTicks = TEMP_ADJ_PERIOD;
					  #ifdef ONBOARD_LED
						BlinkIndLed(500, 3);	// Flash the Ind LED to show it's lowering
					  #endif
					}
				}
			} // thermal step down

			//---------------------------------------------------------------------
			// Be sure switch is not pressed and light is OFF for at least 5 secs
			//---------------------------------------------------------------------
			word wWaitTicks = 310;	// 5 secs
			if (LowBattState)
				wWaitTicks = 22500;	// 6 minutes
			
			if (((!rampingMode && (modeIdx == 0)) || (rampingMode && outLevel == 0))
									 &&
				 !IsPressed() && (wIdleTicks > wWaitTicks))
			{
#ifdef ONBOARD_LED
				// If the battery is currently low, then turn OFF the indicator LED before going to sleep
				//  to help in saving the battery
			  #ifdef VOLT_MON_R1R2
				if (byVoltage < ADC_LOW)
			  #else
				if (byVoltage < BATT_LOW)
			  #endif
				if (locatorLed)
					TurnOnBoardLed(0);
#endif

				_delay_ms(1); // Need this here, maybe instructions for PWM output not getting executed before shutdown?

				SleepUntilSwitchPress();	// Go to sleep
			}
			
			//---------------------------------------------------------------------
			// Check for a scheduled output channel blink indicator
			//---------------------------------------------------------------------
			if (byBlinkActiveChan)
			{
				byBlinkActiveChan = 0;	// disable first so a subsequent one is not missed
				BlinkActiveChannel();
			}
		}
		
      //---------------------------------------------------------------------------------------
		else                             // Configuration mode in effect
      //---------------------------------------------------------------------------------------
		{
			if (configClicks != prevConfigClicks)
			{
				prevConfigClicks = configClicks;
				if (configClicks > 0)
					ClickBlink();
			}
			
			if (ConfigMode != prevConfigMode)
			{
				prevConfigMode = ConfigMode;
				configIdleTime = 0;

				switch (ConfigMode)
				{
				 case 1:
					_delay_ms(400);

					ConfigBlink(1);
					++ConfigMode;
					break;
					
				 case 3:	// 1 - (exiting) ramping mode selection
					if ((configClicks > 0) && (configClicks <= 8))
					{
						rampingMode = 1 - (configClicks & 1);
						SaveConfig();
					}
					ConfigBlink(2);
					break;

				case 14:	// Reset to Factory Default settings
					SetConfigDefaults();
					SaveConfig();
					
					locatorLed = locatorLedOn;
					DefineModeSet();
					byStepdownTemp = DEFAULT_STEPDOWN_TEMP;	// the current temperature in C for doing the stepdown
					
					eeprom_write_byte((uint8_t *)(250), byStepdownTemp);	// save it at the fixed address of 250 in the EEPROM space

					wTimedStepdownTickLimit = pgm_read_word(timedStepdownOutVals+stepdownMode);
				
					ConfigMode = 15;			// all done, go to exit
					break;	
					
				case 15:	// exiting config mode
					ConfigMode = 0;		// Exit Config mode
					Blink(5, 40);
					outLevel = rampingLevel = 0;
					modeIdx = 0;
					
					if (rampingMode)
						SetLevel(rampingLevel);
					break;
				
				//-------------------------------------------------------------------------
				//			Advanced Config Modes (from Battery Voltage Level display)
				//-------------------------------------------------------------------------
					
				case 21:	// Start Adv Config mode
					_delay_ms(400);

					ConfigBlink(1);
					++ConfigMode;
					configClicks = 0;
					break;
					
				case 23:	// 1 - (exiting) locator LED ON selection
					if (configClicks)
					{
						locatorLedOn = 1 - (configClicks & 1);
						locatorLed = locatorLedOn;
						SaveConfig();
					}
					ConfigBlink(2);
					break;
					
				case 24:	// 2 - (exiting) BVLD LED config selection
					if (configClicks)
					{
						bvldLedOnly = 1 - (configClicks & 1);
						SaveConfig();
					}
					ConfigBlink(3);
					break;
					
				case 25:	// 3 - (exiting) Indicator LED enable selection
					if (configClicks)
					{
						onboardLedEnable = 1 - (configClicks & 1);
						SaveConfig();
					}
					ConfigMode = 15;			// all done, go to exit
					break;

				case 100:	// thermal calibration in effect
					Blink(3, 40);		 	// 3 quick blinks
					SetOutput(MAX_BRIGHTNESS);	// set max output
					wThermalTicks = 312;	// set for 5 seconds as the minimum time to set a new stepdown temperature
					break;

				case 101:	// exiting thermal calibration
					SetOutput(OFF_OUTPUT);
				
					if (wThermalTicks == 0)	// min. time exceeded
					{
						// Save the current temperature to use as the threshold to step down output
						byStepdownTemp = byTempReading;
						
						eeprom_write_byte((uint8_t *)(250), byStepdownTemp);	// save it at the fixed address of 250 in the EEPROM space
					}
				
					// Continue to next setting (blinky modes)
					if (rampingMode)
					{
						ConfigBlink(4);
						prevConfigMode = ConfigMode = 5;
					}
					else
					{
						ConfigBlink(8);
						prevConfigMode = ConfigMode = 9;
					}
					break;

				case 40:		// timed stepdown calibration in effect (do nothing here)
					break;

				case 41:	// (exiting) timed stepdown
					if ((configClicks > 0) && (configClicks <= 6))
					{
						stepdownMode = configClicks + 1;
						SaveConfig();
						
						// Set the updated timed stepdown tick (16 msecs) limit
						wTimedStepdownTickLimit = pgm_read_word(timedStepdownOutVals+stepdownMode);
					}

					// Continue to next setting (blinky modes)
					if (rampingMode)
					{
						ConfigBlink(4);
						prevConfigMode = ConfigMode = 5;
					}
					else
					{
						ConfigBlink(8);
						prevConfigMode = ConfigMode = 9;
					}
					break;
				
				default:
					if (rampingMode)
					{
						//---------------------------------------------------------------------
						// Ramping Configuration Settings
						//---------------------------------------------------------------------
						switch (ConfigMode)
						{
						case 4:	// 2 - (exiting) moonlight level selection
							if ((configClicks > 0) && (configClicks <= 7))
							{
								moonlightLevel = configClicks;
								DefineModeSet();
								SaveConfig();
							}
							ConfigBlink(3);
							break;

						case 5:	// 3 - (exiting) stepdown setting
							if (configClicks == 1)			// disable it
							{
								stepdownMode = 0;
								SaveConfig();
								ConfigBlink(4);
							}
							else if (configClicks == 2)	// thermal stepdown
							{
								stepdownMode = 1;
								SaveConfig();
								ConfigMode = 100;		// thermal configuration in effect!!
							}
							else if (configClicks == 3)   // timed stepdown
							{
									Blink(3, 40);			// 3 quick blinks
									ConfigMode = 40;
							}
							else
								ConfigBlink(4);
							break;
							
						case 6:	// 4 - (exiting) blinky mode setting (0=disable, 1=strobe only, 2=all blinkies)
							if ((configClicks > 0) && (configClicks <= 3))
							{
								blinkyMode = configClicks - 1;
								SaveConfig();
							}
							ConfigMode = 15;			// all done, go to exit
							break;
						}
					}
					else
					{					
						//---------------------------------------------------------------------
						// Mode Set Configuration Settings
						//---------------------------------------------------------------------
						switch (ConfigMode)	
						{
						case 4:	// 2 - (exiting) mode set selection
							if ((configClicks > 0) && (configClicks <= sizeof(modeSetCnts)))
							{
								modeSetIdx = configClicks - 1;
								DefineModeSet();
								SaveConfig();
							}
							ConfigBlink(3);
							break;
							
						case 5:	// 3 - (exiting) moonlight enabling
							if (configClicks)
							{
								moonLightEnable = 1 - (configClicks & 1);
								DefineModeSet();
								SaveConfig();
							}
							ConfigBlink(4);
							break;

						case 6:	// 4 - (exiting) mode order setting
							if (configClicks)
							{
								highToLow = 1 - (configClicks & 1);
								SaveConfig();
							}
							ConfigBlink(5);
							break;

						case 7:	// 5 - (exiting) mode memory setting
							if (configClicks)
							{
								modeMemoryEnabled = 1 - (configClicks & 1);
								SaveConfig();
							}
							ConfigBlink(6);
							break;

						case 8:	// 6 - (exiting) moonlight level selection
							if ((configClicks > 0) && (configClicks <= 7))
							{
								moonlightLevel = configClicks;
								DefineModeSet();
								SaveConfig();
							}
							ConfigBlink(7);
							break;

						case 9:	// 7 - (exiting) stepdown setting
							if (configClicks == 1)			// disable it
							{
								stepdownMode = 0;
								SaveConfig();
								ConfigBlink(8);
							}
							else if (configClicks == 2)	// thermal stepdown
							{
								stepdownMode = 1;
								SaveConfig();
								ConfigMode = 100;		// thermal configuration in effect!!
							}
							else if (configClicks == 3)   // timed stepdown
							{
									Blink(3, 40);			// 3 quick blinks
									ConfigMode = 40;
							}
							else
								ConfigBlink(8);
							break;
							
						case 10:	// 8 - (exiting) blinky mode setting (0=disable, 1=strobe only, 2=all blinkies)
							if ((configClicks > 0) && (configClicks <= 3))
							{
								blinkyMode = configClicks - 1;
								SaveConfig();
							}
							ConfigMode = 15;			// all done, go to exit
							break;
						}
					}
					break;	
				} // switch ConfigMode
				
				configClicks = 0;

			} // ConfigMode changed

			else if (
					((configClicks > 0) && (configIdleTime > 125))		// least 1 click: 2 second wait is enough
												||
					((ConfigMode != 100) && (configClicks == 0) && (configIdleTime > 219)))	// no clicks: 3.5 secs (make it a little quicker, was 4 secs)
			{
				++ConfigMode;
			}
			
		} // config mode
	} // while(1)

   return 0; // Standard Return Code
}