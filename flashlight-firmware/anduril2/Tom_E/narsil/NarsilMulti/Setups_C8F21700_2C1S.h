/****************************************************************************************
 * Setups_C8F21700_2C1S.h - for the Sofirn C8F 21700 triple host from Ali, 2 channels, 1S battery
 * ======================
 *
 * Created: 11/10/2018 9:52 AM
 *  Author: Tom E
 ****************************************************************************************/
#ifndef SETTINGS_H_
#define SETTINGS_H_

//----------------------------------------------------------------------------------------
//			Driver Board Settings
//----------------------------------------------------------------------------------------
#define OUT_CHANNELS 2			// define the output channels as 1, 2 or 3

//#define TWOCHAN_3C_PINS			// special setting for using a FET+1 config on a 3 channel TA driver
//#define TURBO_LEVEL_SUPPORT	// set if you want a max turbo level above the max ramping level

#define VOLTAGE_MON				// Comment out to disable - ramp down and eventual shutoff when battery is low
//#define VOLT_MON_R1R2			// uses external R1/R2 voltage divider, comment out for 1.1V internal ref

// For voltage monitoring on pin #7, only uncomment one of the two def's below:
//#define USING_220K	// for using the 220K resistor
//#define USING_360K	// for using a 360K resistor (LDO and 2S cells)

// This is added to reading, so the higher the #, the higher the reading
#define D1_DIODE 24				// Drop over rev. polarity protection diode: using 0.24V

// For 2 channel (FET+1) boards:
 //#define USING_3807135_BANK	// (default OFF) sets up ramping for 380 mA 7135's instead of a FET

// For 3 channel (triple) boards:
//#define TRIPLE_3_7135			// Configure for 3 7135's
//#define TRIPLE_8_7135			// Configure for 8 7135's

#define ONBOARD_LED				// Enable the LED support
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
//			Temperature Monitoring
//----------------------------------------------------------------------------------------
// Temperature Calibration Offset - value is added to reading, higher the #, higher the reading:
#define TEMP_CAL_OFFSET (-12)
//   -12  rough guess for the C8F 21700 triple
//   -12  rough guess for the X6R triple
//   -14  about for the TA driver for the M8
//   -3   try for SP03
//    1   about right for the C8F #1
//   -12  guess for the JM70 #2
//   -19  is about right for the Lumintop SD Mini, IOS proto driver
//   -3   Decided to use this for Q8 production
//   -6   BLF Q8 Round 3 - blinks 29C w/3 setting for 20C (68F) room temp
//   -2   try for the Manker U21 (LJ)
//   -2   works for the Warsun X60 (robo) using the 17 mm DEL driver
//   -1   try this for proto #1, OSHPark BLF Q8 driver
//    3   about right for BLF Q8 proto #2 and #3, reads ~20 for ~68F (18C)
//   -12  this is about right on the DEL DDm-L4 board in the UranusFire C818 light
//   -11  On the TA22 board in SupFire M2-Z, it's bout 11-12C too high,reads 35C at room temp, 23C=73.4F
//   -8   For the Manker U11 - at -11, reads 18C at 71F room temp (22C)
//   -2   For the Lumintop SD26 - at -2, reading a solid 19C-20C (66.2F-68F for 67F room temp)

#define DEFAULT_STEPDOWN_TEMP (70)	// default for stepdown temperature (50C=122F, 55C=131F)
// use 50C for smaller size hosts, or a more conservative level (SD26, U11, etc.)
// use 55C to 60C for larger size hosts, maybe C8 and above, or for a more aggressive setting

#define TEMP_ADJ_PERIOD 2812	// Over Temp adjustment frequency: 45 secs (in 16 msec ticks)
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
#ifndef VOLT_MON_R1R2	// if using R1/R2, change the LVP settings in the calibration header file
#define BATT_LOW			  30	// Cell voltage to step light down = 3.0 V
#define BATT_CRIT			  28	// Cell voltage to shut the light off = 2.8 V
#endif
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
//			Stored Configuration Setting Defaults
//----------------------------------------------------------------------------------------
#define DEF_RAMPING			1		// 1: ramping, 0: mode sets
#if USING_3807135_BANK
 #define DEF_MOON_LEVEL		5		// 0..7, 0: disabled, usually set to 3 (350 mA) or 5 (380 mA) - 2 might work on a 350 mA
#else
 #define DEF_MOON_LEVEL		3		// 0..7, 0: disabled, usually set to 3 (350 mA) or 5 (380 mA) - 2 might work on a 350 mA
#endif
#define DEF_STEPDOWN_MODE	1		// 0=disabled, 1=thermal, 2=60s, 3=90s, 4=120s, 5=3min, 6=5min, 7=7min (3 mins is good for production)
#define DEF_BLINKY_MODE		2		// blinky mode config: 1=strobe only, 2=all blinkies, 0=disable

#define DEF_MODE_SET_IDX	3		// 0..11, mode set currently in effect, chosen by user (3=4 modes)
#define DEF_MOON_MODE		1		// 1: enable moonlight mode, 0: disable moon mode
#define DEF_HIGH_TO_LOW		0		// 1: highest to lowest, 0: modes go from lowest to highest
#define DEF_MODE_MEMORY		0		// 1: save/recall last mode set, 0: no memory

#define DEF_LOCATOR_LED		1		// Locator LED feature (ON when light is OFF) - 1=enabled, 0=disabled
#define DEF_BVLD_LED_ONLY	0		// BVLD (Battery Voltage Level Display) - 1=BVLD shown only w/onboard LED, 0=both primary and onboard LED's
#define DEF_ONBOARD_LED		1		// On Board LED support - 1=enabled, 0=disabled
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
//			Operational Settings Enable/Disable
//----------------------------------------------------------------------------------------
#define STARTUP_LIGHT_OFF		// (default ON) for ramping from power up, main light OFF, otherwise set to max
#define STARTUP_2BLINKS			// enables the powerup/startup two blinks
#define LOCKOUT_ENABLE			// (default ON) Enable the "Lock-Out" feature
#define TRIPLE_CLICK_BATT		// (default ON) enable a triple-click to display Battery status
#define RAMPING_REVERSE			// (default ON) reverses ramping direction for every click&hold
#define OFFTIME_ENABLE 0		// 1: Do OFF time mode memory for Mode Sets on power switching (tailswitch), 0: disabled

//#define BLINK_ONLY_IND_LED	// blink the Ind. LED, not main LED for: startup, config settings, Enter/Exit lockout blinks
//#define ADV_RAMP_OPTIONS		// In ramping, enables "mode set" additional method for lock-out and battery voltage display, comment out to disable
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
//			Timed Settings
//----------------------------------------------------------------------------------------
#define RAMP_SWITCH_TIMEOUT 75 // make the up/dn ramp switch timeout 1.2 secs (same as IDLE_TIME)

#define STROBE_SPEED	14,41		// 14,41: 18Hz, 16,47: 16Hz, 20,60: 12.5Hz

#define SHORT_CLICK_DUR 18		// Short click max duration - for 0.288 secs
#define RAMP_MOON_PAUSE 23		// this results in a 0.368 sec delay, paused in moon mode

// One-Click Turn OFF option:
#define IDLE_TIME         75	// make the time-out 1.2 seconds (Comment out to disable)

// Switch handling:
#define LONG_PRESS_DUR    24	// Prev Mode time - long press, 24=0.384s (1/3s: too fast, 0.5s: too slow)
#define XLONG_PRESS_DUR   75	// strobe mode entry hold time - 75=1.2s, 68=1.09s (any slower it can happen unintentionally too much)
#define CONFIG_ENTER_DUR 200	// In Mode Sets Only: Config mode entry hold time - 200=3.2s, 160=2.5s

#define DBL_CLICK_TICKS   14	//  fast click time for enable/disable of Lock-Out, batt check,
										// and double/triple click timing (14=0.224s, was 16=0.256s)

#define ADC_DELAY        312	// Delay in ticks between low-batt ramp-downs (312=5secs, was 250=4secs)
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
//			Strobe/Blinky mode Configuration
//----------------------------------------------------------------------------------------
#define BATT_CHECK_MODE	  80
#define TEMP_CHECK_MODE	  81
#define FIRM_VERS_MODE	  82
#define SPECIAL_MODES	  90		// base/lowest value for special modes
#define STROBE_MODE		  SPECIAL_MODES+1
//#define RANDOM_STROBE	  SPECIAL_MODES+2	// not used for now...
#define POLICE_STROBE	  SPECIAL_MODES+2
#define BIKING_STROBE	  SPECIAL_MODES+3
#define BEACON_2S_MODE	  SPECIAL_MODES+4
#define BEACON_10S_MODE	  SPECIAL_MODES+5

// Custom define your blinky mode set here:
#define SPECIAL_MODES_SET	STROBE_MODE, POLICE_STROBE, BIKING_STROBE, BEACON_2S_MODE, BEACON_10S_MODE
//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

#endif /* SETTINGS_H_ */