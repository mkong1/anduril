
/****************************************************************************
*																			*
*							X85 v1 - Mike C									*
*																			*
*				ATtiny85 firmware for Mike C drivers.						*
*				Support for Dual Switch and Off Switch setups.				*
*																			*
*				Driver type and setup selected by #defines					*
*																			*
****************************************************************************/

// ======================== DEFAULT SETTINGS ===============================

	#define F8																	//	F8 & ZY-T08
//	#define G12																	//	G-12
//	#define G16																	//	G-16 & BMF-SRK
//	#define LOW																	//	Low output modes for development testing.

	#define	DUALSWITCH
//	#define	OFFSWITCH

//	#define	DOUBLE_CELL															//	Comment out for single cell drivers.
//	#define PB5_IO																//	Comment out if PB5 is NOT enabled as IO.

	#define UI					1												//	Default UI, 1 to 4.
	#define MODECOUNT			4												//	Default mode count, max 4.

	#define MODE1				20,0											//	PWM, AMC. (PWM on single dedicated AMC, number of AMCs turned on in constant current).
	#define MODE2				100,0											//	MODECOUNT=3 skips MODE3, MODECOUNT=2 skips MODE2 & MODE3, MODECOUNT=1 is MODE1.
	#define MODE3				0,2
	#define BOOST				0,MAX											//	MAX for max output. Differs depending on driver, defined per driver type.
	#define	RAMP1				15,0											//	Default output in UI 3.
	#define	RAMP2				10,0											//	Default output in UI 4.

	#define	BOOSTTIMER			5												//	0, 10, 20, 30, 45, 1:00, 1:30, 2:00, 3:00 ,5:00.
	#define GENERICSETTINGS		0b111111										//	CRITSHUTDOWN, LOWVOLTMON, TEMPMONITOR, MODEDIRL2H, BOOSTMEM, MODEMEM


// ======================== DUAL SWITCH INTERFACE ==========================
/*
	UI 1
	----
	Next mode:			Short E or P press.
	Previous mode:		Long E or P press.
	Boost:				In mode cycle.

	UI 2
	----
	Next mode:			Short P press.
	Previous mode:		Long P press.
	Boost:				E press (not in mode cycle). Hold for 1.5 seconds to lock.

	UI 3
	----
	Adjust mode:		Short P press enables mode adjustment.
	Boost:				E press. Hold for 1.5 seconds to lock.

	UI 4
	--------
	Adjust mode:		Constantly adjustable.
	Boost:				N/A.

	Adjusting output
	----------------
	Increase:			E press and hold.
	Decrease:			Double E press, hold on second press.

// ======================== OFF SWITCH INTERFACE ===========================

	UI 1
	----
	Next mode:			Short P press.
	Previous mode:		Long P press.
	Boost:				In mode cycle.

	UI 2
	----
	Next mode:			Short P press.
	Previous mode:		N/A.
	Boost:				Long P press (not in mode cycle).

	UI 3
	----
	Adjust mode:		Long P press enables mode adjustment.
	Boost:				Short P press.

	UI 4
	--------
	Adjust mode:		Constantly adjustable.
	Boost:				N/A.

	Adjusting output
	----------------
	Increase:			Short press starts ramp up. Stop with short press.
	Decrease:			Double short press starts ramp down. Stop with short press.


// ======================== DUAL SWITCH *SETUP* ============================

  --- *MENU ---    Accessed by holding E-switch on startup.
                   Menu selection by blinks, rest by E-switch presses.

    READOUTS       1 = Voltage level 1 to 5.
                   2 = Real voltage X,XX.
                   3 = Temperature.
                   4 = Mode level.
                   5 = Calibration values (after SOS confirmation timeout, endless loop).

    SPECIALMODES   5 = SOS (confirmation required).

1   LOCK / UI      L = Safety lock.
                   1-4 = UI.
                   9 = Noob mode on/off.

2   SPECIALMODES   1 = Beacon.
                   2 = Strobe.
                   3 = Prog mode.
                  (4)= RND Strobe.

3   MODESETTINGS   1 = ModeCount:    1-4 = Mode count (UI 1/2).
                                     L = Toggle mode direction (UI 1/2).
                   2 = ModeMem:      L = No memory.
                                     1 = Ex Boost.
                                     2 = Inc Boost.
                   3 = BoostTimer:   1-9 = 10, 20, 30, 45, 1:00, 1:30, 2:00, 3:00, 5:00.
                                     L = No timer.
                                     0 = Readout timer.

4   MONITORING     1 = MaxTemp:      1 = Start (turn off at max temp).
                                     L = Toggle Temp Monitor on/off.
                                     0 = Readout max temp.
                   2 = LowVolt:      X.X = V, readout, confirm.
                                     L = Toggle Low Volt Monitor on/off.
                                     0 = Readout threshold value.
                   3 = CritVolt:     X.X = V, readout, confirm.
                                     L = Toggle Crit shutdown on/off.
                                     0 = Readout threshold value.

5   CALIBRATIONS   1 = VoltCalib:    XXX = X.XX Volt, readout, confirm.
                                     L = Reset INTREFVOLT to default.
                                     0 = Readout INTREFVOLT.
                   2 = TempCalib:    XX = Temp, readout, confirm.
                                     L = Reset TEMPOFFSET to default.
                                     0 = Readout TEMPOFFSET.
                   3 = TempProfile:  1 = Starts temp profiling.
                                     L = Reset TEMPPROFILE to default.
                                     0 = Readout TEMPPROFILE.

6   RESET2DEFAULT  1 = Reset modes and settings but not calibrations or offsets.
                   L = Full reset.
                   Additional confirmation required.


// ======================== OFF SWITCH *SETUP* =============================

  --- *MENU ---    Accessed by 3 to 9 short presses directly after cold start.
                   All selections by off switch presses.

    READOUTS       3 = Voltage level.
                   4 = Real voltage.
                   5 = Temperature (after SOS confirmation timeout).
                   6 = Mode level.
                   7 = Calibration values (endless loop).

    SPECIALMODES   5 = SOS (confirmation required).
                   9 = NOOB mode.

3   LOCK / UI      L = Safety lock.
                   1-4 = UI.

4   SPECIALMODES   1 = Beacon.
                   2 = Strobe.
                   3 = Prog mode.

5   MODESETTINGS   1 = ModeCount:    1-4 = Mode count (UI 1/2).
                                     L = Toggle mode direction (UI 1/2).
                   2 = ModeMem:      L = No memory.
                                     1 = Ex Boost.
                                     2 = Inc Boost.
                   3 = BoostTimer:   1-9 = 10, 20, 30, 45, 1:00, 1:30, 2:00, 3:00, 5:00.
                                     L = No timer.
                                     0 = Readout timer.
                   0 = SOS on/off    Confirmation required.

6   MONITORING     1 = MaxTemp:      1 = Start (turn off at max temp).
                                     L = Toggle Temp Monitor on/off.
                                     0 = Readout max temp.
                   2 = LowVolt:      X.X = V, readout, confirm.
                                     L = Toggle Low Volt Monitor on/off.
                                     0 = Readout threshold value.
                   3 = CritVolt:     X.X = V, readout, confirm.
                                     L = Toggle Crit shutdown on/off.
                                     0 = Readout threshold value.

7   CALIBRATIONS   1 = VoltCalib:    XXX = X.XX Volt, readout, confirm.
                                     L = Reset INTREFVOLT to default.
                                     0 = Readout INTREFVOLT.
                   2 = TempCalib:    XX = Temp, readout, confirm.
                                     L = Reset TEMPOFFSET to default.
                                     0 = Readout TEMPOFFSET.
                   3 = TempProfile:  1 = Starts temp profiling.
                                     L = Reset TEMPPROFILE to default.
                                     0 = Readout TEMPPROFILE.

8   RESET2DEFAULT  1 = Reset modes and settings but not calibrations or offsets.
                   L = Full reset.
                   Additional confirmation required.

9   NOOB MODE      Noob mode on/off.


// ======================== COMMON SHARED INTERFACE ========================

	Mode level programming
	----------------------
	UI 1 & 2:			Blink writes mode, then double blink exits programming mode.
	UI 3:				Single blink writes mode and exists programming mode.
	UI 4:				Constant programming mode, writes mode without blinks.

	Memory
	------
	No memory:			Always starts in lowest mode on cold start.
	Mode memory:		Mode restored on cold start. Does not include boost by default.
	Boost memory:		Includes boost mode in mode memory.
	UI change:			Lowest mode set on UI change, regardless of mode memory setting.

	Low voltage or high temp
	------------------------
	On low voltage, high temperature or critical levels the light steps down to a mode
	or level that is at or below the defined allowed limits after blinking warning.
	2 blinks for voltage, 3 blinks for temperature, followed by 3 quick flashes if critical.
	The light continues to warn every 30 seconds without interaction.
	If levels are critical and the critical shutdown setting is enabled the light will enter
	sleep mode. A power off cycle will wake up the light from sleep.

*/
// =========================================================================

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

// ==================== DRIVER DEFAULT *SETTINGS* ==========================

//	GENERAL SETTINGS

#define	LVL_READOUT				18												//	Level for programming blinks
#define	LVL_READOUTLOW			11												//	Lower level for programming blinks
#define	LVL_ONOROFF				9												//	Level threshold to blink on or off.

#define	NOOBMODE1				20												//	Noob mode levels, 255 is max.
#define	NOOBMODE2				75
#define	NOOBMODE3				255

// -------------------- COMMON DEFAULT *SETTINGS* --------------------------

#ifndef TEMPOFFSET
	#define	TEMPOFFSET			0												//	Default uncalibrated temperature offset.
#endif
#ifndef MAXTEMP
	#define	MAXTEMP				50												//	Default temperature threshold. One below does step down, one above activates critical mode.
#endif
#ifndef TEMPPROFILE
	#define	TEMPPROFILE			15												//	Time in seconds to resume temp monitoring after temp step-down/warning. *EXPERIMENTAL*
#endif
#define UIHOLD					UI												//	UI memory used for toggle SOS & NOOB mode.
#define MODE					0
#define MODEHOLD				0
#define STROBESPEED				0
#define INTREFVOLTB1			INTREFVOLT % 256
#define INTREFVOLTB2			INTREFVOLT / 256

// -------------------- SINGLE CELL VOLTAGE LEVELS -------------------------

#ifndef DOUBLE_CELL
	#ifndef INTREFVOLT
		#define INTREFVOLT		11000											//	Default uncalibrated internal ref voltage: 11000 = 1.1V.
	#endif
	#ifndef	OFF_SHORT
		#define OFF_SHORT		205												//	Voltage threshold for off time cap short press detection.
	#endif
	#ifndef	OFF_LONG
		#define OFF_LONG		165												//	Voltage threshold for off time cap long press detection.
	#endif
	#ifndef	LOWVOLT
		#define LOWVOLT			32
	#endif
	#ifndef	CRITVOLT
		#define CRITVOLT		29
	#endif

	#define VOLTAGE_LEVEL5		400
	#define VOLTAGE_LEVEL4		380
	#define VOLTAGE_LEVEL3		360
	#define VOLTAGE_LEVEL2		330
	#define VOLTAGE_LEVEL1		300

	#ifndef	INTREFVOLT
		#define INTREFVOLT		11000											//	Default uncalibrated internal reference voltage. 11000 = 1.1V.
	#endif

// -------------------- DOUBLE CELL VOLTAGE LEVELS -------------------------

#else
	#ifndef INTREFVOLT
		#define INTREFVOLT		25600											//	Default uncalibrated internal ref voltage: 25600 = 2.56V.
	#endif
	#ifndef	OFF_SHORT
		#define OFF_SHORT		410												//	Voltage threshold of off time cap for short off press detection.
	#endif
	#ifndef	OFF_LONG
		#define OFF_LONG		310												//	Voltage threshold of off time cap for long off press detection.
	#endif
	#ifndef	LOWVOLT
		#define LOWVOLT			65
	#endif
	#ifndef	CRITVOLT
		#define CRITVOLT		60
	#endif

	#define VOLTAGE_LEVEL5		800
	#define VOLTAGE_LEVEL4		760
	#define VOLTAGE_LEVEL3		720
	#define VOLTAGE_LEVEL2		660
	#define VOLTAGE_LEVEL1		600

	#ifndef	INTREFVOLT
		#define INTREFVOLT		25600											//	Default uncalibrated internal reference voltage. 25600 = 2.56V.
	#endif

#endif

// ------------------------ LOW V/HIGH T MODE LIMITS -----------------------

	#ifndef LOW
		#define	LOWVOLTMAX		2*256											//	Highest output level allowed when voltage is low. AMC * 256 + PWM.
		#define	CRITICALMAX		1*256											//	Highest output level allowed when voltage is critical.
	#endif

// ------------------------ LOW OUTPUT SETTINGS ----------------------------

	#ifdef LOW
		#define MODE1			9,0
		#define MODE2			15,0
		#define MODE3			40,0
		#define BOOST			80,0

		#define	LOWVOLTMAX		35
		#define	CRITICALMAX		15
	#endif

// ======================== DRIVER DEFINITIONS =============================

#ifdef LOW
	#define	RAMPMAX				1*256
	#define MAX					1
#endif

#ifdef F8
	#ifdef PB5_IO
		#define	RAMPMAX			8*256
	#endif
	#define MAX					111
#endif

#ifdef G12
	#ifdef PB5_IO
		#define	RAMPMAX			12*256
	#endif
	#define MAX					12
#endif

#ifdef G16
	#ifdef PB5_IO
		#define	RAMPMAX			16*256
	#endif
	#define MAX					16
#endif

//	PB5 NOT IO:

#ifndef RAMPMAX
	#define	RAMPMAX				4*256
#endif

// ------------------------ COMMON DEFINITIONS -----------------------------

#define RAW						0
#define CAP						1
#define VOLT					2
#define VOLT_READOUT			3
#define TEMP					4
#define TEMP_READOUT			5

#define VOLTAGE_LOW				sLOWVOLT*10
#define VOLTAGE_CRIT			sCRITVOLT*10

#define ALERT_LOWVOLT			1
#define ALERT_CRITVOLT			2
#define ALERT_HIGHTEMP			3
#define ALERT_CRITTEMP			4

#define FLASH_ONOROFF			254												//	Flash routine needs to use BlinkOnOffThreshold.
#define FLASH_LIGHTMODE			255

// -------------------- PIN & E-SWITCH CONFIGURATION -----------------------

#define ESWITCH					(eswLevel < 20)									//	Check if E-switch is pressed.
#define NO_PRESS				0
#define LONG_PRESS				255
#define PWM						OCR0A											//	PWM output.

// ======================== MODE VARIABLES =================================

uint8_t MODE_AMC =				0;
uint8_t MODE_PWM =				0;

#define BOOSTMODE				3												//	Mode number for boost.
#define RAMPMODE1				4
#define RAMPMODE2				5

// ------------------------ UI VARIABLES -----------------------------------

#define SAFETYLOCK				(sUI & (1 << 7))								//	Highest bit in UI byte used for safety lock.

#define SOS						6
#define BEACON					7
#define STROBE					8
#define UI_NOOB					9

// ------------------------ *SETTINGS* -------------------------------------

uint8_t SettingsDefault[] =		{MODE1, MODE2, MODE3, BOOST, RAMP1, RAMP2,						//	0-11	All mode levels use two bytes each.
								UI, UIHOLD, MODE, MODEHOLD, MODECOUNT-1, STROBESPEED,			//	12-17
								GENERICSETTINGS, BOOSTTIMER, LOWVOLT, CRITVOLT, MAXTEMP,0,		//	18-23
								INTREFVOLTB1, INTREFVOLTB2,TEMPOFFSET, TEMPPROFILE,				//	24-27
								0,0,0,0};														//	28-31	0 = Unused bytes.

uint8_t NoobModes[] =			{NOOBMODE1,NOOBMODE2,NOOBMODE3};
uint8_t BoostTimeouts[] =		{0,2,4,6,9,12,18,24,36,60};						//	X * 5 sec = 0, 10, 20, 30, 45, 1:00, 1:30, 2:00, 3:00 ,5:00.
uint8_t StrobeSpeeds[] =		{127,100,80,60,45,30,20,14,10,7,5,3,2,1};		//	Ticks.

#define sUI						Settings[12]
#define sUIHOLD					Settings[13]
#define sMODE					Settings[14]
#define sMODEHOLD				Settings[15]
#define sMODECOUNT				Settings[16]
#define sSTROBESPEED			Settings[17]

#define sGENERICSETTINGS		Settings[18]
#define sBOOSTTIMER				Settings[19]
#define sLOWVOLT				Settings[20]
#define sCRITVOLT				Settings[21]
#define sMAXTEMP				Settings[22]

#define sINTREFVOLT				Modes[12]										//	Settings[24] & [25].
#define sTEMPOFFSET				Settings[26]
#define sTEMPPROFILE			Settings[27]

// ------------------------ GENERIC SETTINGS -------------------------------

#define gsModeMemory			0
#define gsBoostMemory			1
#define gsModeDirection			2
#define gsTempMonitor			3
#define gsLowVoltMon			4
#define gsCritShutdown			5
//#define -						6												//	Available bit, currently not used.
//#define -						7												//	Available bit, currently not used.

#define sMODEMEM				(sGENERICSETTINGS % 4)							//	Mode memory.
#define sMODEDIRL2H				(sGENERICSETTINGS & (1 << gsModeDirection))
#define sTEMPMONITOR			(sGENERICSETTINGS & (1 << gsTempMonitor))
#define sLOWVOLTMON				(sGENERICSETTINGS & (1 << gsLowVoltMon))
#define sCRITSHUTDOWN			(sGENERICSETTINGS & (1 << gsCritShutdown))

// ------------------------ TIMER DEFINITIONS ------------------------------

#define SEC						66												//	WDT ticks per second.
#define BOOST_PERIOD			165												//	1 Boost period = 2.5 seconds.

#define MULTIPRESSTIME			15												//	Time to consider if presses are multi-presses.
#define LONGPRESSTIME			27												//	Time to consider if press is long press.
#define BOOSTFREERUN			80												//	Time to set boost free running.
#define FUNCTIONBLINKTIME		1*SEC											//	Time between E-switch setup blinks.

#define MENUSTARTTIME			20

#define PROGMODETIMER			(2*SEC)											//	Programming mode save/exit timeout, seconds.

// ------------------------ *DELAY* DEFINITIONS ----------------------------

#define	DELAY_FLASH				2
#define DELAY_FADE				3
#define	DELAY_BLINK				4
#define DELAY_ENTRY				6
#define DELAY_SHORT				16
#define DELAY_MEDIUM			33
#define DELAY_LONG				66

#define DELAY_SW				100												//	Duration for switch input.
#define DELAY_VOLTCALIB			2*SEC											//	Cell recovery delay for voltage calibration.
#define DELAY_VOLTREADOUT		DELAY_VOLTCALIB-DELAY_SW						//	Cell recovery delay for voltage readout.

#define DELAY_SETUPWINDOW		66												//	Window of opportunity to enter setup from cold start.
#define DELAY_SETUPPRESS		33												//	Window of opportunity for multiple off switch press detection.

#define STARTDELAY_NONE			0
#define STARTDELAY_SHORT		251
#define STARTDELAY_MEDIUM		252
#define STARTDELAY_LONG			253
#define STARTDELAY_LONG_KEEPMEM	254

// ------------------------ SETUP DEFINITIONS ------------------------------

//	SETUP MENU

#define sSELECTUI				10
#define sSPECIALMODES			20
#define sMODESETTINGS			30
#define sMONITORING				40
#define sCALIBRATIONS			50
#define sRESET2DEF				60

//	MODE SETTINGS

#define sSETMODECNT				31
#define sSETMODEMEM				32
#define	sSETBOOSTIME			33
#define sSOS					34

//	MONITORING SETTINGS

#define sSETMAXTEMP				41
#define sSETLOWVMON				42
#define sSETCRITVMON			43

//	CALIBRATIONS

#define sVOLTCALIB				51
#define sTEMPCALIB				52
#define sTEMPPROFILING			53

//	OFFSWITCH INPUT ENTRY

#define RESET_CONFIRM_1			61
#define RESET_CONFIRM_SHORT		62
#define RESET_CONFIRM_LONG		63

#define SECOND_DIGIT			100
#define THIRD_DIGIT				150
#define CONFIRMATION			200

// ------------------------ EEPROM VARIABLES -------------------------------	*EEPROM*

#define eeBlockSize				32												//	Size of settings array. Must be evenly multipliable to 512.
uint16_t eePos =				0;												//	Address in memory block.
uint8_t eeSettingsRead =		0;												//	Flag that indicates settings have been read from EEPROM.
uint8_t Settings[eeBlockSize]=	{0};											//	Settings array.
uint16_t* Modes =				(uint16_t*)Settings;							//	Settings array as uint16 values.

// ------------------------ TIMER VARIABLES --------------------------------	//	*ISR*

volatile uint8_t WdtTicks =		0;												//	Watchdog timer counter.
volatile uint8_t WdtSecs =		0;												//	Watchdog timer counter in seconds.
volatile uint8_t WdtStartup =	0;												//	Startup delay.
volatile uint8_t DelayTicks =	0;												//	Timer for Delay routine.

volatile uint8_t SwitchTicks =	0;												//	Timer for E-switch.
volatile uint8_t NoSwitchTicks= 0;												//	E-switch not pressed counter for detecting multiple short E-presses.
volatile uint8_t swPressed =	0;												//	E-switch press counter for quick or multi-presses. Set but not cleared in ISR.
volatile uint8_t swLongPressed= 0;												//	Flag for E-switch long pressed. Set but not cleared in ISR.
volatile uint8_t swPressCounter=0;												//	E-switch press counter with no timeout. Set but not cleared in ISR.
volatile uint8_t swMultiDetect= 0;												//	Flag for E-switch detecting multiple presses.

volatile uint8_t BOOSTSTATUS =	0;												//	Integer for boost mode status.
volatile uint16_t BoostTicks =	0;												//	Timer for boost.
volatile uint16_t ProgModeTicks=0;												//	Timer for Mode programming mode.

#ifdef OFFSWITCH
	volatile uint8_t SetupTicks=0;												//	Timer for settings menu access.
#endif

// ------------------------ VOLT VARIABLES ---------------------------------

volatile uint16_t Voltage =		0;												//	Converted voltage level from ADC.
volatile uint16_t NormVoltTicks=0;												//	Normal voltage level tick counter.
volatile uint16_t LowVoltTicks= 0;												//	Low voltage level tick counter.
volatile uint16_t CritVoltTicks=0;												//	Critical voltage level tick counter.

// ------------------------ TEMP VARIABLES ---------------------------------

volatile uint8_t Temperature =	0;												//	For storing current temperature.
volatile uint16_t NormTempTicks=0;												//	Normal temperature level tick counter.
volatile uint16_t CritTempTicks=0;												//	Critical temperature level tick counter.

// ------------------------ NO-INIT VARIABLES ------------------------------

volatile uint8_t bVOLTSTATUS __attribute__ ((section (".noinit")));				//	0 = normal, 1 = low, 2 = low warned, 3 = critical, 4 = critical warned.
volatile uint8_t bTEMPSTATUS __attribute__ ((section (".noinit")));				//	0 = normal, 1 = high, 2 = critical.
volatile uint8_t bPROGSTATUS __attribute__ ((section (".noinit")));				//	Programming mode status.

#ifdef OFFSWITCH
	volatile uint8_t swSETUP __attribute__ ((section (".noinit")));				//	Settings menu access status.
	volatile uint8_t swCOUNT __attribute__ ((section (".noinit")));				//	Settings menu press counter.
	volatile uint16_t swVALUE __attribute__ ((section (".noinit")));			//	Settings menu value input.
	volatile uint8_t swRAMP __attribute__ ((section (".noinit")));				//	Used for ramp up or down in mode programming.
	volatile uint16_t bMODE __attribute__ ((section (".noinit")));				//	Mode level storage in no init, used in mode programming and mode readout.
#endif

// ------------------------ MISC VARIABLES ---------------------------------

volatile uint8_t eswLevel =		100;											//	Raw value from each ADC volt read before conversion to volt, used for ESWITCH.
volatile uint8_t StatusAlert =	0;												//	To trigger actions based on volt/temp monitoring.
volatile uint8_t BreakStrobe =	0;												//	Breaks strobe delays on E-switch presses.

// ======================== READ FROM *EEPROM* =============================

inline void eeRead() {
	while (!eeSettingsRead && eePos < 512) {									//	Loop through EEPROM until settings are read or end is reached.
		while (EECR & (1<<EEPE));												//	Halt while EEPROM is busy.
		eeprom_read_block(&Settings, (const void*)(eePos), eeBlockSize);		//	Read current block of EEPROM.
		while (EECR & (1<<EEPE));												//	Halt while EEPROM is busy.
		if (Settings[0] != 0xff) eeSettingsRead = 1;							//	Set flag that settings have been read.
		else eePos += eeBlockSize;
	}

	if (!eeSettingsRead) {														//	If no settings where found in EEPROM:
		eePos = 0;																//	Reset eePos 0.
		for (uint8_t i=0; i < eeBlockSize; i++) {
			Settings[i] = SettingsDefault[i];									//	Load default settings into settings array.
		}
	}
}

// ======================== WRITE TO *EEPROM* ==============================

void eeWrite() {

	uint16_t eePosOld = eePos;

	if (eeSettingsRead) {														//	Increase memory address only if settings have been read.
		eePos += eeBlockSize;													//	Go to next block to write settings array (wear leveling).
		if (eePos == 512) eePos = 0;											//	If it end, wrap around to start of EEPROM memory.
	}

	while (EECR & (1<<EEPE));													//	Halt while EEPROM is busy.
	eeprom_write_block(&Settings, (void *)(eePos), eeBlockSize);				//	Write settings array to EEPROM before erasing old.

	if (eeSettingsRead) {														//	Erase old only if settings have been read. Erases only first byte of old array.
		while (EECR & (1<<EEPE));												//	Halt while EEPROM is busy.
		EECR = (0<<EEPM1) | (1<<EEPM0);											//	Mode erase: 01 = erase, 10 = write, 00 = erase & write.
		EEAR = eePosOld;
		EECR |= (1<<EEMPE);														//	Enable writing.
		EECR |= (1<<EEPE);														//	Start writing.
	}
}

// ------------------------ RESET MODE -------------------------------------

void ResetMode() {
	sMODE = 0;
	sMODEHOLD = 0;
	if (sUI == 3) sMODE = RAMPMODE1;
	if (sUI == 4) sMODE = RAMPMODE2;
	eeWrite();
}

// ======================== SET/RESET TO DEFAULT ===========================

void ResetToDefault(uint8_t iSwConfirm) {

	uint8_t iSettings = eeBlockSize;
	if (iSwConfirm == 1) iSettings = 24;										//	iSettings = 24 does not restore calibrations.

	for (uint8_t i=0; i < iSettings; i++) {
		Settings[i] = SettingsDefault[i];										//	Load default settings into settings array, except last three.
	}

	ResetMode();																//	Reset modes and write default settings to EEPROM.
}

// ======================== *INITIALIZING* ROUTINES =========================

// ------------------------ *PWM* ON/OFF ------------------------------------

void PWM_on(uint8_t MODE_PWM) {
	TCCR0A = 0b10000001;														//	Clear 0C0A on match and set phase correct PWM (WGM01 = 0, WGM00 = 1).
	TCCR0B = 0b00000001;														//	Prescaler for timer (1 => 1, 2 => 8, 3 => 64...).
	PWM = MODE_PWM;
}

void PWM_off() {
	PWM = 0;
	TCCR0A = 0;
	TCCR0B = 0;
}

// ------------------------ WATCHDOG TIMER ON ------------------------------

inline void WDT_on() {															//	Set watchdog timer only to interrupt, not reset, every 500ms.
	cli();																		//	Disable interrupts.
	wdt_reset();																//	Reset the WDT.
	WDTCR |= (1<<WDCE) | (1<<WDE);												//	Start timed sequence.
	WDTCR = (1<<WDIE);															//	Enable interrupt every 16ms.
	sei();																		//	Enable interrupts.
}

// ------------------------ WATCHDOG TIMER OFF -----------------------------

inline void WDT_off() {
	cli();
	wdt_reset();
	MCUSR = 0;																	//	Clear MCU status register.
	WDTCR |= (1<<WDCE) | (1<<WDE);												//	Start timed sequence.
	WDTCR = 0x00;																//	Disable WDT.
	cli();
}

// ======================== CRITICAL SHUTDOWN ==============================

void Shutdown() {
	PWM_off();
	PORTB = 0;																	//	Turn off PORTB pins.
	ADCSRA &= ~(1 << ADEN);														//	Make sure ADC is off.
	WDT_off();																	//	Turn off watchdog timer.

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);										//	Power down as many components as possible.

	uint8_t mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);							//	Turn off brown-out detection.
    uint8_t mcucr2 = mcucr1 & ~_BV(BODSE);
    MCUCR = mcucr1;
    MCUCR = mcucr2;

	sleep_enable();
	sleep_cpu();
	while (1) {}																//	ZZzzZZzz...
}

// ======================== *DELAY* AND RESETS =============================

void Delay(uint8_t iDelay) {
	DelayTicks = 0;
	while (DelayTicks < iDelay && !BreakStrobe) {}
	BreakStrobe = 0;
}

void ResetTimer() {
	WdtTicks = 0;
	WdtSecs = 0;
}

void ResetVoltTimers() {
	NormVoltTicks = 0;
	LowVoltTicks = 0;
	CritVoltTicks = 0;
}

void WriteAndResetVoltStatus() {
	bVOLTSTATUS = 0;															//	Set voltage status to normal.
	ResetVoltTimers();
	eeWrite();
}

void ResetTempTimers() {
	NormTempTicks = 0;
	CritTempTicks = 0;
}

void ClearSwitch() {															//	Clears all switch flags and counters.
	#ifdef OFFSWITCH
		if (swSETUP != 1) {
			swSETUP =	0;
			swCOUNT =	0;
		}
		swVALUE =		0;
	#else
		SwitchTicks =	0;
		swPressed =		0;
		swLongPressed =	0;
		swPressCounter= 0;
		swMultiDetect =	0;
	#endif
}

void ColdStart() {
	bVOLTSTATUS = 0;
	bTEMPSTATUS = 0;
	if (bPROGSTATUS != 6) bPROGSTATUS = 0;										//	PROGSTATUS = 6 is when programming mode is selected from setup.
	#ifdef OFFSWITCH
		swSETUP = 1;
		swCOUNT = 0;
		swVALUE = 0;
		swRAMP = 0;
	#endif
}

void StatusAfterSwitch() {
	if (bVOLTSTATUS == 2 || bVOLTSTATUS == 4) bVOLTSTATUS--;					//	Forces warning blink after every E-switch press.
	if (bTEMPSTATUS) CritTempTicks = (sTEMPPROFILE * SEC) - 2*SEC;				//	If TEMP status is critical force flash after two seconds.
	ResetVoltTimers();															//	Reset voltage counters if E-switch pressed under normal operation.
}

// ------------------------ STATUS CHECKS ----------------------------------

uint8_t CheckStatus() {
	uint8_t iStatus = 0;
	if (bVOLTSTATUS || bTEMPSTATUS) iStatus++;
	if (bVOLTSTATUS >= 3 || bTEMPSTATUS >= 2) iStatus++;
	return iStatus;
}

// ------------------------ SPLIT MODE LEVEL -------------------------------

void ModeToBytes() {															//	Splits 16 bit mode value into two separate bytes. In separate function to optimize space.
	MODE_AMC = Modes[sMODE] / 256;
	MODE_PWM = Modes[sMODE] % 256;
}

// ================ TURN *LIGHT* ON ACCORDING TO MODE ======================

void LightMode() {

	ModeToBytes();

	if (sUI == UI_NOOB) {
		MODE_AMC = 0;
		MODE_PWM = NoobModes[sMODE];
	}

	if (MODE_PWM) PWM_on(MODE_PWM);
	else PWM_off();

// -------------------- PB5 NOT ENABLED AS IO ------------------------------

	#ifndef PB5_IO
		if (MODE_AMC < 4) PORTB = MODE_AMC * 2;
		else {
			if (MODE_AMC == 4) PORTB = 7;
			else PORTB = 15;
		}

	#else

// -------------------- PB5 IS ENABLED AS IO -------------------------------

		#ifdef F8
			if (MODE_AMC < 8) PORTB = MODE_AMC * 2;
			else {
				if (MODE_AMC == 8) PORTB = 15;
				else PORTB = 47;
			}
		#endif


		#ifdef G12
			if (MODE_AMC < 8) PORTB = MODE_AMC * 2;
			else {
				if (MODE_AMC < 12) PORTB = ((MODE_AMC - 8) * 2) + 40;
				else PORTB = 47;
			}
		#endif


		#ifdef G16
			if (MODE_AMC < 8) PORTB = MODE_AMC * 2;
			else {
				if (MODE_AMC < 16) PORTB = (MODE_AMC * 2) + 16;
				else PORTB = 47;
			}
		#endif

	#endif
}

// ------------------------ LED OFF ---------------------------------------

void LED_off() {
	PWM_off();
	PORTB = 0;
}

// ======================== *BLINKOUT ROUTINES* ============================

// ------------------------ SHORT FLASHES/BLINKS ---------------------------

inline void FlashOn() {
	LightMode();
	Delay(DELAY_FLASH);
	LED_off();
}

inline void FlashOff() {
	LED_off();
	Delay(DELAY_FLASH);
}

inline void FlashLow() {
	PWM_on(LVL_READOUTLOW);
	Delay(DELAY_FLASH);
	PWM_off();
}

void FlashReadout() {
	PWM_on(LVL_READOUT);
	Delay(DELAY_FLASH);
	PWM_off();
}

inline void BlinkOn() {
	LightMode();
	Delay(DELAY_BLINK);
	LED_off();
}

inline void BlinkOff() {
	LED_off();
	Delay(DELAY_BLINK);
}

// ------------------------ BLINK ON/OFF THRESHOLD -------------------------

void BlinkOnOffThreshold(uint8_t iLightMode) {
	if (Modes[sMODE] > LVL_ONOROFF) {
		if (iLightMode) FlashOn();
		else FlashOff();
	}
	else {
		FlashReadout();
	}
}

// ------------------------ *FLASH* ----------------------------------------

void Flash(uint8_t iTimes, uint8_t iStrength, uint8_t iDuration) {
	while (iTimes--) {
		PWM_on(iStrength);
		if (!iStrength) LED_off();
		if (iStrength == FLASH_LIGHTMODE) LightMode();
		if (iStrength == FLASH_ONOROFF) BlinkOnOffThreshold(1);
		else Delay(iDuration);

		if (!iStrength) LightMode();
		else LED_off();
		Delay(iDuration);
	}
}

// ------------------------ BLINK VALUE ------------------------------------

void BlinkValue(uint8_t iValue, uint8_t iDigits) {
	PWM_on(LVL_READOUT);
	if (iValue) {
		while (iValue--) Flash(1,LVL_READOUT,DELAY_SHORT);
	}
	else {		
		if (iDigits) {
			Delay(DELAY_FLASH);
			PWM = 0;
			Delay(DELAY_SHORT);
		}
		else {
			Delay(DELAY_SHORT);
			for (uint8_t i=LVL_READOUT; i; i--){
				PWM = i;
				Delay(DELAY_FADE);
			}
			Delay(DELAY_SHORT);
		}
	}
	PWM_off();
}

//	------------------- BLINK MULTI DIGIT NUMBER ---------------------------

void BlinkNumValue(uint16_t iValue, uint8_t iDigits) {							//	iDigits = digits to blink out, X, 0X, 00X, 000X, etc etc.

	if (!(iValue + iDigits)) BlinkValue(0,0);
	uint16_t iDivider = 10000;
	for (uint8_t i=5; i; i--) {
		uint8_t vValue = iValue / iDivider;
		if (vValue || iDigits >= i) {
			iDigits = i;
			BlinkValue(vValue,1);
			Delay(DELAY_MEDIUM);
		}
		iValue %= iDivider;
		iDivider /= 10;
	}
}

//	------------------- BLINK WITH MINUS NUMBER ----------------------------

void BlinkMinusValue(uint8_t iValue) {
	PWM_on(0);

	if (iValue >= 128) {
		iValue = 256 - iValue;
		PWM = LVL_READOUTLOW;													//	If value is -, start with long low 0.
		Delay(DELAY_LONG);
		PWM = 0;
		Delay(DELAY_LONG);
	}
	BlinkNumValue(iValue,1);

	PWM_off();
}

// ------------------------ BLINK CONFIRMATION -----------------------------

void BlinkConfirm() {
	PWM_on(LVL_READOUTLOW - 2);
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW;
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW * 2;
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW * 3;
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW * 4;
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW * 3;
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW * 2;
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW;
	Delay(DELAY_FADE);
	PWM = LVL_READOUTLOW - 2;
	Delay(DELAY_FADE);
	PWM_off();
	Delay(DELAY_BLINK);
}

// ------------------------ BLINK SETTING STATUS ---------------------------

inline void BlinkSettingUp() {
	PWM_on(LVL_READOUTLOW);
	Delay(DELAY_MEDIUM);
	PWM = LVL_READOUT;
	Delay(DELAY_MEDIUM);
	PWM_off();
}

void BlinkSettingDown() {
	PWM_on(LVL_READOUT);
	Delay(DELAY_MEDIUM);
	PWM = LVL_READOUTLOW;
	Delay(DELAY_MEDIUM);
	PWM_off();
}

// ------------------------ FLASH ON ERROR ---------------------------------

void BlinkError() {
	ClearSwitch();

	PWM_on(LVL_READOUTLOW);
	Delay(DELAY_SHORT);
	PWM = LVL_READOUT;
	Delay(DELAY_SHORT);
	PWM = LVL_READOUTLOW;
	Delay(DELAY_SHORT);
	PWM = LVL_READOUT;
	Delay(DELAY_SHORT);
	for (uint8_t i=LVL_READOUT; i; i--){
		PWM = i;
		Delay(DELAY_FADE);
	}
	PWM_off();
}

// ------------------------ GET VOLTAGE BLINKS -----------------------------

uint8_t GetVoltBlinks(uint16_t v) {
	uint8_t i = 0;
	if (v >= VOLTAGE_LEVEL1) i++;
	if (v >= VOLTAGE_LEVEL2) i++;
	if (v >= VOLTAGE_LEVEL3) i++;
	if (v >= VOLTAGE_LEVEL4) i++;
	if (v >= VOLTAGE_LEVEL5) i++;
	return i;
}

// ======================== *MODE* FUNCTIONS ===============================

// ------------------------ BOOST IN CYCLE ---------------------------------

void BoostIfInCycle() {															//	Made as function to save space as it's called on from two functions.
	if (!CheckStatus() && (sUI == 1 || (sUI >= SOS && sUI <= STROBE) || bPROGSTATUS)) {
		BOOSTSTATUS = 3;
		sMODE = BOOSTMODE;
	}
}

// ------------------------ *MODE* UP --------------------------------------

void ModeUp() {
	uint8_t iMODECOUNT = sMODECOUNT;
	if (sUI == UI_NOOB) iMODECOUNT = 3;

	if (iMODECOUNT) {
		sMODEHOLD = sMODE;
		sMODE++;
		BOOSTSTATUS = 0;
		if (sMODE == BOOSTMODE+1) sMODE = 0;
		if (sMODE == iMODECOUNT) {
			sMODE = 0;
			BoostIfInCycle();													//	Changes mode from 0 to BOOST if in cycle.
		}
		if ((CheckStatus() == 1 && Modes[sMODE] > LOWVOLTMAX) || (CheckStatus() == 2 && Modes[sMODE] > CRITICALMAX)) sMODE = 0;
	}
}

// ------------------------ *MODE* DOWN ------------------------------------

void ModeDwn() {
	uint8_t iMODECOUNT = sMODECOUNT;
	if (sUI == UI_NOOB) iMODECOUNT = 3;

	if (iMODECOUNT) {
		sMODEHOLD = sMODE;
		if (sMODE == BOOSTMODE) sMODE = iMODECOUNT;
		sMODE--;
		BOOSTSTATUS = 0;
		if (sMODE == 255) {
			sMODE = iMODECOUNT - 1;
			BoostIfInCycle();													//	Changes mode from sMODECOUNT - 1 to BOOST if in cycle.
		}
		while ((CheckStatus() == 1 && Modes[sMODE] > LOWVOLTMAX) || (CheckStatus() == 2 && Modes[sMODE] > CRITICALMAX)) sMODE--;
	}
}

// ---------------- NEXT/PREV MODE ACCORDING TO DIRECTION ------------------

void NextMode() {
	if (sMODEDIRL2H) ModeUp();
	else ModeDwn();
}

void PrevMode() {
	if (sMODEDIRL2H) ModeDwn();
	else ModeUp();
}

// ------------------------ *STEPDOWN* ROUTINE -----------------------------

void StepDown(uint8_t iBlinks, uint8_t iCritical) {

	Flash(iBlinks,0,DELAY_SHORT);												//	Step-down blink.
	Delay(DELAY_SHORT);

	if (!iCritical) {
		if (BOOSTSTATUS) {														//	Boost mode all UIs.
			BOOSTSTATUS = 0;
			sMODE = sMODEHOLD;
		}
		else {
			while ((sUI <= 2 || sUI == BEACON || sUI == STROBE) && sMODE && Modes[sMODE]>LOWVOLTMAX) {
				sMODE--;														//	UI 1/2/Strobe mode level higher than allowed, lower until below limit or lowest mode.
			}
			if ((sUI == 3 || sUI == 4) && Modes[sMODE] > LOWVOLTMAX) {
				Modes[sMODE] = LOWVOLTMAX;
			}

		}
	}

	else {
		Flash(3,0,DELAY_BLINK);													//	Critical blink.
		if (sCRITSHUTDOWN || sUI == UI_NOOB) Shutdown();						//	Shut down if set to turn off when levels are critical, or Noob mode is active.

		while ((sUI <=2 || sUI == BEACON || sUI==STROBE) && sMODE && Modes[sMODE] > CRITICALMAX) {
			sMODE--;															//	UI 1/2/Strobe mode level higher than allowed, lower until below limit or lowest mode.
		}
		if ((sUI == 3 || sUI == 4) && Modes[sMODE] > CRITICALMAX) {
			Modes[sMODE] = CRITICALMAX;
		}
	}

	eeWrite();																	//	Save new lower mode to memory.
	LightMode();
}

// ======================== READ *ADC* =====================================

uint16_t ADC_read(uint8_t adc) {

// ------------------------ ADC ON -----------------------------------------

	DIDR0 |= (1 << ADC2D);														//	Disable digital input on ADC2/PB4.
	if (adc <= VOLT_READOUT) {													//	ADC is for voltage reading.
		#ifndef DOUBLE_CELL
			ADMUX = (0<<REFS0) | (1<<REFS1) | (0<<REFS2) | (0<<ADLAR) | 0b0010;	//	1.1v reference (single cell), right-adjust, connect ADC2/PB4 to ADC.
		#else
			ADMUX = (0<<REFS0) | (1<<REFS1) | (1<<REFS2) | (0<<ADLAR) | 0b0010;	//	2.56v reference (double cell), right-adjust, connect ADC2/PB4 to ADC.
		#endif
	}
	else {																		//	ADC is for temperature reading.
		ADMUX = (0<<REFS0) | (1<<REFS1) | (0<<REFS2) | (0<<ADLAR) | 0b1111;		//	1.1v reference, right-adjust, connect ADC4/TEMP to ADC.
	}
	ADCSRA = (1 << ADEN ) | 0b111;												//	ADC on, prescaler division factor 128.

// ------------------------ ADC READ ---------------------------------------

	uint16_t iADC = 0;
	uint8_t iReadings = 20;														//	20 readings to average readouts and calibrations.
	if (adc == VOLT || adc == TEMP) iReadings = 5;								//	5 readings to average level monitoring.

	for (uint8_t i=0; i <= iReadings; i++) {									//	Loop ADC readings.
		ADCSRA |= (1 << ADSC);													//	Start conversion.
		while (ADCSRA & (1 << ADSC));											//	Wait for completion.
		uint8_t L = ADCL; uint8_t H = ADCH;										//	Obtain the result.
		if (i) iADC += ((H << 8) | L);											//	If not the first reading, add to stack of readings.
	}

	iADC /= iReadings;															//	Get the average.

// ------------------------ ADC OFF ----------------------------------------

	ADCSRA &= ~(1 << ADEN);

// ------------------------ RETURN VALUE -----------------------------------

	if (adc && adc <= VOLT_READOUT) {
		#ifdef DUALSWITCH
			eswLevel = iADC / 4;												//	Left adjust raw value for E-switch reading.
		#endif

		iADC=(uint16_t)(((uint32_t)iADC*(uint32_t)sINTREFVOLT)/(uint32_t)20480);//	Convert raw ADC value to voltage value.
	}

	if (adc >= TEMP) {
		iADC = (iADC - 285) + (int8_t)sTEMPOFFSET;								//	Convert raw ADC to temperature-ish value apply offset.
		if (iADC > 0x7FFF) iADC = 0;											//	Result is below scope of measurement.
		if (iADC > 255) iADC = 255;												//	Result is above scope of measurement.
	}

	return iADC;																//	Return value.
}

// ======================== *ISR* WATCHDOG TIMER ===========================

ISR(WDT_vect) {

// ------------------------ GENERAL TIMER ----------------------------------

	DelayTicks++;																//	Tick counter for Delay routine.

	if (++WdtTicks == SEC) {													//	WdtTicks per second.
		WdtTicks = 0;
		if (WdtSecs < 255) WdtSecs++;											//	Increase watchdog timer seconds, max 255.
	}

// -------------------- *OFFSWITCH* FUNCTION TIMING ------------------------

	#ifdef OFFSWITCH
		if (SetupTicks < 255) SetupTicks++;
		if (SetupTicks >= DELAY_SETUPWINDOW && swSETUP == 1) swSETUP = 0;		//	Window of opportunity to enter setup with off-switch.
		if (SetupTicks >= DELAY_SETUPPRESS) swRAMP = 0;
	#endif

// ------------------------ WAIT STARTUP DELAY -----------------------------

	if (WdtStartup) {

// ------------------------ READ *VOLTAGE* LEVEL ---------------------------

		Voltage = ADC_read(VOLT);												//	Also determines E-switch status.
//		Voltage = 300;															//	*TEST* emulated voltage.

		if (!ESWITCH && WdtStartup == 2) {

		//	NORMAL VOLTAGE LEVEL:

			if ((sLOWVOLTMON && Voltage > VOLTAGE_LOW) || (!sLOWVOLTMON && Voltage > VOLTAGE_CRIT)) {
				if (NormVoltTicks++ >= SEC) {
					bVOLTSTATUS = 0;											//	Set voltage status to normal.
					ResetVoltTimers();
				}
			}

		//	LOW VOLTAGE LEVEL:

			if (sLOWVOLTMON && (Voltage <= VOLTAGE_LOW && Voltage > VOLTAGE_CRIT) && BOOSTSTATUS < 3) {
				if ((LowVoltTicks++ >= 2*SEC && bVOLTSTATUS <= 1) || (LowVoltTicks >= 30*SEC && bVOLTSTATUS >= 2)) {
					if (!bVOLTSTATUS) bVOLTSTATUS = 1;
					StatusAlert = ALERT_LOWVOLT;
					ResetVoltTimers();
				}
			}

		//	CRITICAL VOLTAGE LEVEL:

			if (Voltage <= VOLTAGE_CRIT) {
				if ((CritVoltTicks++ >= 2*SEC && bVOLTSTATUS <= 3) || (CritVoltTicks >= 30*SEC && bVOLTSTATUS >= 4)) {

					if (BOOSTSTATUS >= 3) {
						bVOLTSTATUS = 1;										//	If Boost, set voltage status to low, not critical.
						StatusAlert = ALERT_LOWVOLT;
					}
					else {
						if (bVOLTSTATUS < 3) bVOLTSTATUS = 3;
						StatusAlert = ALERT_CRITVOLT;
					}
					ResetVoltTimers();
				}
			}
		}

// ------------------------ E-SWITCH TIMER ---------------------------------

	//	DUALSWITCH:

		#ifdef DUALSWITCH

			if (ESWITCH) {														//	E-switch pressed:
				if (!swPressed) swPressed = 1;									//	Set switch pressed for main loop to handle.
				if (swMultiDetect) {											//	Multiple press detection.
					swPressed++;
					swMultiDetect = 0;
				}
				if (!SwitchTicks) swPressCounter++;
				if (SwitchTicks < 255) SwitchTicks++;
				if (SwitchTicks == LONGPRESSTIME) swLongPressed = 1;			//	Set switch long pressed for main loop to handle.
				if (SwitchTicks == LONGPRESSTIME*2) swLongPressed = 3;			//	Set switch longer pressed for main loop to handle.

				NoSwitchTicks = 0;
				ProgModeTicks = 0;												//	Always reset ProgModeTicks on E-switch press.
			}
			else {
				SwitchTicks = 0;
				swMultiDetect = 0;
				if (NoSwitchTicks < 255) NoSwitchTicks++;						//	Increase ticks not pressed but do not wrap around to 0.
				if (NoSwitchTicks < MULTIPRESSTIME) swMultiDetect = 1;			//	Set for multiple press detection.
			}

		#endif

	//	OFFSWITCH:

		#ifdef OFFSWITCH

			if (ESWITCH) {
				if (SwitchTicks < 255) SwitchTicks++;
				ProgModeTicks = 0;												//	Always reset ProgModeTicks on E-switch press.
			}
			else SwitchTicks = 0;

		#endif

// ------------------------ BOOST TIMER ------------------------------------

		if (BOOSTSTATUS >= 3 && BoostTimeouts[sBOOSTTIMER]) {
			if (++BoostTicks >= (BoostTimeouts[sBOOSTTIMER]*5)*SEC ) {			//	If boost timer enabled and times out:
				BoostTicks = 0;
				BOOSTSTATUS = 1;												//	Turn off boost and set boost status to timed out.
			}
		}
		else BoostTicks = 0;

// ------------------------ PROGRAMMING MODE TIMER -------------------------

		if (bPROGSTATUS == 2 || bPROGSTATUS == 4) {
			ProgModeTicks++;
			if (ProgModeTicks == PROGMODETIMER) bPROGSTATUS--;
		}
		else ProgModeTicks = 0;

// -------------------- *STROBE* SPEED CHANGE ------------------------------

		#ifdef DUALSWITCH

			if ((sUI >= SOS && sUI <= STROBE) && swPressed && !ESWITCH && WdtStartup == 2) {
				if (sUI == SOS) {
					if (!swLongPressed) NextMode();
					else PrevMode();
				}
				else {
					if (!swLongPressed) {
						if (sSTROBESPEED++ == 13) sSTROBESPEED = 0;
					}
					else  {
						if (sSTROBESPEED-- == 0) sSTROBESPEED = 13;
					}
				}
				BreakStrobe = 1;													//	Setting to 1 breaks waiting for long strobe/sos blinks to finish.
				eeWrite();															//	Save mode or speed change.
				ClearSwitch();
			}

		#endif

// -------------------- READ *TEMPERATURE* ---------------------------------

		Temperature = (uint8_t)ADC_read(TEMP);
//		Temperature = 80;														//	*TEST* emulated temperatures.

		if (sTEMPMONITOR && WdtStartup == 2) {

		//	NORMAL TEMPERATURE LEVEL:

			if (Temperature < sMAXTEMP) {
				if (NormTempTicks++ >= SEC/2) {
					bTEMPSTATUS = 0;											//	Set Temperature status to normal.
					ResetTempTimers();
				}
			}

		//	HIGH TEMPERATURE LEVEL:

			else {
				CritTempTicks++;
				if ((CritTempTicks >= 1*SEC && bTEMPSTATUS == 0) || CritTempTicks >= sTEMPPROFILE * SEC) {
					if (Temperature < sMAXTEMP+2) {
						bTEMPSTATUS = 1;										//	Set Temperature status to high.
						StatusAlert = ALERT_HIGHTEMP;
					}
					else {
						StatusAlert = ALERT_CRITTEMP;
						bTEMPSTATUS = 2;										//	Set Temperature status to critical.
					}
					ResetTempTimers();
				}
			}
		}

// ======================== END OF ISR ROUTINE =============================

	}
}

// ======================== *ESWITCH* FUNCTIONS ============================

#ifdef DUALSWITCH

// ------------------------ E-SWITCH PRESS COUNTER -------------------------

inline uint8_t EswPressCounter() {												//	Detect and return number of short presses or single long press during a delay.
	DelayTicks = 0;
	uint8_t iEswCounter = 0;
	while (DelayTicks < DELAY_SW) {
		if (ESWITCH) {
			DelayTicks = 0;
		}
		iEswCounter = swPressCounter;
		if (swPressCounter == 1 && swLongPressed) {								//	Long press.
			while (ESWITCH);													//	Hold until switch release.
			Delay(DELAY_MEDIUM);												//	Delay before exiting.
			ClearSwitch();
			return 255;															//	Exit, return long press.
		}
		else {
			if (iEswCounter > 9) iEswCounter = 9;								//	Highest number 9.
		}
	}
	ClearSwitch();
	return iEswCounter;
}

// ------------------------ E-SWITCH ENTER VALUE ---------------------------

uint16_t EswGetValue(uint8_t iDigits) {											//	Overly complicated routine to allow different results depending on conditions.
																				//	1,2 or 3 expects 1,2 or 3 digits to be entered. Exceptions:
	uint8_t i = iDigits;														//	Long press on first digit returns 255, returns 0 on following digits.
																				//	No press on first returns 0. No press on following returns error (except if Temperature).

	if (iDigits == TEMP) i = 2;													//	Temperature takes 1 or 2 digits.
	uint8_t iFirst = i - 1;
	uint16_t iNumber = 0;

	while (i--) {
		uint8_t iEswDigit = EswPressCounter();
		if (iEswDigit == 255 && i == iFirst) return 255;						//	First long press exits and returns 255.
		if (!iEswDigit) {														//	No input for digit:
			if (i == iFirst) return 0;											//	Was first digit, return 0.
			else {																//	Was a following digit when no input.
				if (iDigits == TEMP) return iNumber;							//	No input on following digits only allowed for temperature values.
				else return 1;													//	Not a temperature value, return error.
			}
		}
		if (iEswDigit == 255) iEswDigit = 0;
		iNumber *= 10;
		iNumber += iEswDigit;
		if (i) FlashLow();
	}
	if (iDigits == 3) iNumber+= 1000;											//	Adding 1000 to 3 digit values at end so 255 can be entered.
	return iNumber;
}

#endif

// ------------------------ SWITCH CONFIRMATION ----------------------------

uint8_t SwConfirm() {															//	Confirmation flash.

	#ifdef OFFSWITCH

		if (!swCOUNT) {
			Delay(DELAY_MEDIUM);
			PWM_on(0);
			ResetTimer();
			while (!WdtSecs) Flash(1,LVL_READOUT,DELAY_FLASH);
			PWM_off();
		}
		return 0;																//	Non void function, return something.

	#else

		Delay(DELAY_MEDIUM);
		PWM_on(0);
		ResetTimer();
		while (!ESWITCH && !WdtSecs) Flash(1,LVL_READOUT,DELAY_FLASH);
		PWM_off();
		if (!WdtSecs) {															//	Switch was pressed during flashing.
			while (ESWITCH) {}													//	Make sure switch is released before continuing.
			if (!swLongPressed) {
				ClearSwitch();
				return 1;														//	Short confirmation press returns 1.
			}
			else {
				ClearSwitch();
				return 255;														//	Long confirmation press returns 255.
			}
		}
		else {
			return 0;															//	No confirmation press returns 0.
		}

	#endif
}

// ======================== *OFFSWITCH* FUNCTIONS ==========================

#ifdef OFFSWITCH

// ------------------------ O-SWITCH INPUT DELAY ---------------------------

void OswDelay() {
	if (swCOUNT < LONG_PRESS) Delay(DELAY_SW);									//	If press count not long press, delay for switch input.
	else Delay(DELAY_SHORT);													//	Else short delay.
}

#endif

// ************************ *MAIN* ROUTINE *********************************

int main(void) {

	#ifdef DUALSWITCH
		uint8_t swSETUP = 0;													//	Settings menu access status.
		uint8_t swCOUNT = 0;
		uint16_t swVALUE = 0;
	#endif
	
	uint8_t offTime = 0;
	uint8_t swStartup = 0;														//	Used for switch startup menu, then startup delays.

// ======================== *SETUP* FUNCTIONS ==============================	//	Inline functions that don't take any memory if not called on.

// ------------------------ *READOUTS* -------------------------------------

	//	VOLT LEVEL:

	void Readout_VoltLevel() {
		ClearSwitch();
		Delay(DELAY_VOLTREADOUT);												//	Long delay so voltage reading is taken from cell under no load.
		BlinkValue(GetVoltBlinks(ADC_read(VOLT_READOUT)),0);					//	Convert voltage value to voltage level blinks.
		swStartup = STARTDELAY_LONG;
	}

	//	REAL VOLTAGE:

	void Readout_RealVolt() {
		ClearSwitch();
		Delay(DELAY_VOLTREADOUT);												//	Long delay so voltage reading is taken from cell under no load.
		BlinkNumValue(ADC_read(VOLT_READOUT),3);								//	Readout full voltage value X.XX volts.
		swStartup = STARTDELAY_LONG;
	}

	//	TEMPERATURE:

	void Readout_Temperature() {
		ClearSwitch();
		uint8_t iTemp = ADC_read(TEMP_READOUT);
		if (iTemp) BlinkNumValue(iTemp,2);
		else BlinkValue(0,0);
		swStartup = STARTDELAY_LONG;
	}

	//	MODE LEVEL:

	void Readout_ModeLevel() {
		ClearSwitch();

		#ifdef OFFSWITCH
			sMODE = (uint8_t)bMODE;												//	Copy stored mode to keep mode in mode level readout when memory disabled.
			eeWrite();
		#endif

		ModeToBytes();

		if (MODE_AMC) {
			Delay(DELAY_MEDIUM);
			BlinkNumValue(MODE_AMC,1);											//	Readout AMC value first.
		}

		if (MODE_PWM) {
			Delay(DELAY_LONG);
			Flash(3,LVL_READOUTLOW,DELAY_FADE);									//	Flash to indicate next readout is PWM level.
			Delay(DELAY_LONG);
			BlinkNumValue(MODE_PWM,3);
		}

		swStartup = STARTDELAY_LONG_KEEPMEM;
	}

	//	CALIBRATIONS:

	void Readout_Calibrations() {
		ClearSwitch();
		while (1) {																//	Endless loop, easier to count and write down.
			Delay(DELAY_SW);
			BlinkNumValue(sINTREFVOLT,5);
			Delay(DELAY_SW);
			BlinkMinusValue(sTEMPOFFSET);
		}
	}

// ------------------------ TOGGLE *SOS* -----------------------------------

	void ToggleSOS() {
		if (sUI != SOS) {
			sUIHOLD = sUI;
			sUI = SOS;
		}
		else sUI = sUIHOLD;
		eeWrite();
		BlinkConfirm();
		swStartup = STARTDELAY_SHORT;
	}

// ------------------------ TOGGLE *NOOB* ----------------------------------

	void ToggleNoob() {
		if (sUI != UI_NOOB) {
			sUIHOLD = sUI;
			sUI = UI_NOOB;
		}
		else sUI = sUIHOLD;
		ResetMode();
		eeWrite();
		BlinkConfirm();
		swStartup = STARTDELAY_SHORT;
	}

// ------------------------ SELECT *UI* ------------------------------------

	void SelectUI() {

		#ifdef OFFSWITCH
			OswDelay();
		#else
			swCOUNT = EswGetValue(1);
		#endif

		#ifdef DUALSWITCH
			if (sUI == UI_NOOB || (sUI != UI_NOOB && swCOUNT == 9)) {
				if (swCOUNT == 9) ToggleNoob();
				else {
					swCOUNT = 0;
					swSETUP = 0;
				}
			}
			else {
		#endif

		if (swCOUNT == NO_PRESS) Readout_VoltLevel();

		//	SAFETY LOCK:

		else {
			if (swCOUNT == LONG_PRESS) {									//	Long E-switch press for safety lock.
				sUI ^= (1 << 7);											//	Toggle safety lock bit.
				eeWrite();													//	Write safety lock setting.
				BlinkConfirm();
				if (SAFETYLOCK) {
					ResetTimer();
					WdtSecs = 2;											//	Set so first safety lock blink after engaging is after 1 second.
				}
			}
			else {

		//	SELECT UI:

				if (swCOUNT <= 4) {
					sUI &= ~(1 << 7);										//	UI change always disengages safety lock.
					sUI = swCOUNT;
					BlinkConfirm();
					ResetMode();
					swStartup = STARTDELAY_SHORT;							//	Short startup delay.
				}


	//	ERROR: PRESS COUNT OUT OF RANGE

				else BlinkError();
			}
		}
		#ifdef DUALSWITCH
		}
		#endif
	}

// ------------------------ *SPECIAL* MODES --------------------------------

	void SpecialModes() {

		#ifdef OFFSWITCH
			OswDelay();
		#else
			swCOUNT = EswGetValue(1);
		#endif

		if (swCOUNT == NO_PRESS) Readout_RealVolt();
		else {

	//	PRESS COUNT: SPECIAL MODES

			if (swCOUNT <= 3) {

				if (swCOUNT == 1) {												//	BEACON mode.
					sUI = BEACON;
					if (sMODE >= RAMPMODE1) sMODE = 0;
					eeWrite();
					swStartup = STARTDELAY_NONE;
				}

				if (swCOUNT == 2) {												//	STROBE mode.
					sUI = STROBE;
					if (sMODE >= RAMPMODE1) sMODE = 0;
					eeWrite();
					swStartup = STARTDELAY_NONE;
				}

				if (swCOUNT == 3) {												//	*PROGRAMMING* mode.
					if (sUI > 2) {
						Delay(DELAY_MEDIUM);
						BlinkError();
					}
					else {
						bPROGSTATUS = 6;
						#ifdef OFFSWITCH
							sMODE = (uint8_t)bMODE;								//	Copy stored mode to keep current mode even if mode memory is disabled.
							eeWrite();
						#endif
						BlinkConfirm();
					}
				}
			}

	//	ERROR: PRESS COUNT OUT OF RANGE

			else BlinkError();
		}
	}

// ------------------------ *SETTINGS* -------------------------------------

	void Setup() {

		#ifdef OFFSWITCH
			OswDelay();
		#else
			swCOUNT = EswGetValue(1);
		#endif

		if (swCOUNT == NO_PRESS) {												//	*SOS AND *READOUTS*

			#ifdef OFFSWITCH

				if (swSETUP == sMODESETTINGS) {
					swSETUP = sSOS;
					SwConfirm();

				//	NO CONFIRMATION: READOUT TEMPERATURE

					if (swCOUNT == NO_PRESS) {
						Delay(DELAY_LONG);
						Readout_Temperature();
					}
				}

				if (swSETUP == sMONITORING) Readout_ModeLevel();
				if (swSETUP == sCALIBRATIONS) Readout_Calibrations();

			#else

				if (swSETUP == sMODESETTINGS) Readout_Temperature();
				if (swSETUP == sMONITORING) Readout_ModeLevel();

				if (swSETUP == sCALIBRATIONS) {
					swSETUP = sSOS;
					swCOUNT = SwConfirm();

				//	NO CONFIRMATION: READOUT TEMPERATURE

					if (swCOUNT == NO_PRESS) {
						Delay(DELAY_LONG);
						Readout_Calibrations();
						swSETUP = 0;
					}
				}

			#endif
		}
		else {

	//	PRESS COUNT: SETTING

			if (swCOUNT <= 3) {
				swSETUP += swCOUNT;
				swCOUNT = 0;
				if (swSETUP == sSETMODECNT && sUI > 2) {						//	ERROR if UI is not 1 or 2.
					BlinkError();
					swSETUP = 0;
				}
				else {
					if (swSETUP != sSETMAXTEMP) BlinkConfirm();
				}
			}
			else BlinkError();
		}
	}

// ------------------------ SET MODE COUNT ---------------------------------

	void SetModeCount() {

		#ifdef OFFSWITCH
			OswDelay();
		#else
			swCOUNT = EswGetValue(1);
		#endif

		if (swCOUNT) {

	//	LONG PRESS: MODE DIRECTION

			if (swCOUNT == LONG_PRESS) {
				sGENERICSETTINGS ^= (1 << gsModeDirection);						//	Toggle flag.
				if (sMODEDIRL2H) BlinkSettingUp();
				else BlinkSettingDown();
				eeWrite();
			}
			else {

	//	PRESS COUNT: MODE COUNT

				if (swCOUNT <= 4) {
					sMODECOUNT = swCOUNT - 1;
					BlinkConfirm();
					ResetMode();
				}

	//	ERROR: PRESS COUNT OUT OF RANGE

				else BlinkError();
			}
		}
	}

// ------------------------ SET MODE MEMORY --------------------------------

	void SetModeMemory() {

		#ifdef OFFSWITCH
			OswDelay();
		#else
			swCOUNT = EswGetValue(1);
		#endif

		if (swCOUNT) {

	//	PRESS COUNT OR LONG PRESS: SET MODE MEMORY

			if (swCOUNT <= 2 || swCOUNT == LONG_PRESS) {

				if (swCOUNT == LONG_PRESS) {									//	No mode memory.
					sGENERICSETTINGS &= ~(1 << gsModeMemory);					//	Clear flag.
					sGENERICSETTINGS &= ~(1 << gsBoostMemory);					//	Clear flag.
					BlinkConfirm();
				}
				if (swCOUNT == 1) {												//	Mode memory excluding boost.
					sGENERICSETTINGS |= (1 << gsModeMemory);					//	Set flag.
					sGENERICSETTINGS &= ~(1 << gsBoostMemory);					//	Clear flag.
					BlinkConfirm();
				}
				if (swCOUNT == 2) {												//	Mode memory including boost.
					sGENERICSETTINGS |= (1 << gsModeMemory);					//	Set flag.
					sGENERICSETTINGS |= (1 << gsBoostMemory);					//	Set flag.
					BlinkConfirm();
				}
				ResetMode();
			}

	//	ERROR: PRESS COUNT OUT OF RANGE

			else BlinkError();
		}
	}

// ------------------------ SET BOOST TIMER --------------------------------

	void SetBoostTimer() {

		#ifdef OFFSWITCH
			OswDelay();
		#else
			swCOUNT = EswGetValue(1);
		#endif

		if (swCOUNT == NO_PRESS) {
			BlinkValue(sBOOSTTIMER,0);
			swStartup = STARTDELAY_LONG;
		}
		else {
			if (swCOUNT == LONG_PRESS) swCOUNT = 0;
			if (swCOUNT <= 9) {													//	0-9 = 0, 10, 20, 30, 45, 1:00, 1:30, 2:00, 3:00 ,5:00.
				sBOOSTTIMER = swCOUNT;
				BlinkConfirm();
				eeWrite();
			}
			else BlinkError();
		}
	}

// ------------------------- *CALIBRATE* TEMPERATURE -----------------------

	void CalibrateTemperature() {
		sTEMPOFFSET = 0;														//	Delete current temp offset so new reading is without.
		sTEMPOFFSET = (int8_t)(swVALUE-ADC_read(TEMP_READOUT));					//	Get current temperature, calculate and store offset.
		BlinkConfirm();
		eeWrite();
	}

// ---------------- *VOLT* MONITORING / TEMP CALIBRATE ---------------------

	void VoltMonTempCalibSet() {

		#ifdef OFFSWITCH

			if (swSETUP == sSETLOWVMON+CONFIRMATION) {
				sLOWVOLT = swVALUE;												//	Copy new value.
				sGENERICSETTINGS |= (1 << gsLowVoltMon);						//	Always turn on low voltage monitoring when new value is entered.
				BlinkConfirm();
				WriteAndResetVoltStatus();										//	Save new settings and clear low voltage.
			}

			if (swSETUP == sSETCRITVMON+CONFIRMATION) {
				sCRITVOLT = swVALUE;											//	Copy new value.
				BlinkConfirm();
				WriteAndResetVoltStatus();										//	Save new settings and clear low voltage.
			}

			if (swSETUP == sTEMPCALIB+CONFIRMATION)  CalibrateTemperature();

		#else

			if (swSETUP == sSETLOWVMON) {
				sLOWVOLT = swVALUE;												//	Copy new value.
				sGENERICSETTINGS |= (1 << gsLowVoltMon);						//	Always turn on low voltage monitoring when new value is entered.
				BlinkConfirm();
				WriteAndResetVoltStatus();										//	Save new settings and clear low voltage.
			}

			if (swSETUP == sSETCRITVMON) {
				sCRITVOLT = swVALUE;											//	Copy new value.
				BlinkConfirm();
				WriteAndResetVoltStatus();										//	Save new settings and clear low voltage.
			}

			if (swSETUP == sTEMPCALIB) CalibrateTemperature();

		#endif
	}

// ---------------- VOLT MON / TEMP CALIB READ OUT -------------------------

	void VoltMonTempCalibReadout() {
		Delay(DELAY_MEDIUM);

		if (swSETUP == sSETLOWVMON) {
			if (sLOWVOLTMON) BlinkNumValue(sLOWVOLT,2);							//	Readout low voltage threshold if monitoring active.
			else BlinkValue(0,0);												//	Low voltage monitoring is deactivated.
		}
		if (swSETUP == sSETCRITVMON) BlinkNumValue(sCRITVOLT,2);				//	Readout current critical voltage threshold.
		if (swSETUP == sTEMPCALIB) {
			if (sTEMPMONITOR) BlinkMinusValue(sTEMPOFFSET);						//	Readout temperature offset if monitoring active.
			else BlinkValue(0,0);												//	Except if temperature monitor is off.
		}

		swStartup = STARTDELAY_LONG;
	}

// ---------------- VOLT MON / TEMP CALIB RESET ----------------------------

	void VoltMonTempCalibReset() {
		if (swSETUP == sSETLOWVMON) {
			sGENERICSETTINGS ^= (1 << gsLowVoltMon);							//	Toggle flag.
			if (sLOWVOLTMON) BlinkConfirm();									//	Then readout voltage monitoring on/off setting.
			else BlinkValue(0,0);
		}

		if (swSETUP == sSETCRITVMON) {
			sGENERICSETTINGS ^= (1 << gsCritShutdown);							//	Toggle flag.
			if (sCRITSHUTDOWN) BlinkValue(0,0);									//	Then readout critical shutdown setting.
			else BlinkSettingDown();
		}

		if (swSETUP == sTEMPCALIB) {
			sTEMPOFFSET = TEMPOFFSET;
			BlinkConfirm();
			eeWrite();
		}

		else WriteAndResetVoltStatus();											//	Save new settings and voltage status.
	}

// ---------------- *VOLT* MONITORING / *TEMP* *CALIBRATE* -----------------

	void VoltMonTempCalib() {

		#ifdef OFFSWITCH
			OswDelay();
			swVALUE = swCOUNT;
		#else
			swVALUE = EswGetValue(2);
		#endif

	//	NO PRESS: READOUT SETTING

		if (swVALUE == NO_PRESS) VoltMonTempCalibReadout();
		else {

	//	LONG PRESS: TOGGLE VOLT SETTING / RESET TEMPOFFSET

			if (swVALUE == LONG_PRESS) VoltMonTempCalibReset();

	//	ESWITCH CONFIRM VALUE: SAVE NEW SETTING

			#ifdef DUALSWITCH
				else {

	//	PRESS VALUE: CHECK IF IN RANGE

					if ((swVALUE < 20 || swVALUE > 90) && swSETUP != sTEMPCALIB) BlinkError();
					else {
						BlinkNumValue(swVALUE,2);
						if (SwConfirm()) VoltMonTempCalibSet();
					}
				}
			#endif

	//	OFFSWITCH: FIRST DIGIT

			#ifdef OFFSWITCH
				else {
					if (swCOUNT <= 9) {
						swVALUE = swCOUNT * 10;
						swCOUNT = 0;
						swSETUP += SECOND_DIGIT;
						FlashReadout();
						OswDelay();
						BlinkError();
					}
					else BlinkError();
				}
			#endif

		}
		ClearSwitch();
	}

	//	OFFSWITCH: SECOND DIGIT

	#ifdef OFFSWITCH

	void VoltMonTempCalib_OswDigit2() {
		OswDelay();
		if (swCOUNT == NO_PRESS) BlinkError();
		else {
			if (swCOUNT <= 9 || swCOUNT == LONG_PRESS) {
				if (swCOUNT == LONG_PRESS) swCOUNT = 0;
				swVALUE += swCOUNT;
				swCOUNT = 0;

	//	PRESS VALUE: CHECK IF IN RANGE

				if ((swVALUE < 20 || swVALUE > 90) && swSETUP != sTEMPCALIB+SECOND_DIGIT) BlinkError();
				else {
					BlinkNumValue(swVALUE,2);
					swSETUP = swSETUP - SECOND_DIGIT + CONFIRMATION;
					SwConfirm();
					ClearSwitch();
				}
			}
			else BlinkError();
		}
	}

	#endif

// ------------------------ SET MAX TEMPERATURE ----------------------------

	void SetMaxTemp() {

		#ifdef OFFSWITCH
			SwConfirm();
		#else
			swCOUNT = SwConfirm();
		#endif

	//	NO CONFIRMATION: READOUT MAX TEMPERATURE

		if (swCOUNT == NO_PRESS) {
			Delay(DELAY_LONG);
			if (sTEMPMONITOR) BlinkNumValue(sMAXTEMP,2);						//	Readout max temperature...
			else BlinkValue(0,0);												//	Except if temperature monitor is off.
			swStartup = STARTDELAY_LONG;
		}

	//	LONG CONFIRMATION: RESET MAX TEMPERATURE

		if (swCOUNT == LONG_PRESS) {
			sGENERICSETTINGS ^= (1 << gsTempMonitor);							//	Toggle flag.
			if (sTEMPMONITOR) BlinkConfirm();									//	Then readout temperature monitoring on/off setting.
			else BlinkValue(0,0);
			bTEMPSTATUS = 0;													//	Set Temperature status to normal.
			eeWrite();															//	Write setting change to EEPROM.
		}

	//	SHORT CONFIRMATION: SET MAX TEMPERATURE

		if (swCOUNT == 1) {

			if (CheckStatus()) BlinkValue(0,0);									//	Levels not normal, abort.
			else {
	
	//	LEVELS NORMAL: START SET MAX TEMPERATURE

				sGENERICSETTINGS |= (1 << gsTempMonitor);						//	Automatically enable temp monitoring.
				BlinkConfirm();
				ClearSwitch();
				Delay(DELAY_MEDIUM);
				PORTB = MAX;													//	Turn on light full blast.
				uint8_t TemperatureOld = Temperature + 1;						//	For storing previous temperature.

				while (1) {
					if (Temperature > TemperatureOld) {							//	Temperature rising one degree-ish unit.
						if (++CritTempTicks >= SEC/2) {
							CritTempTicks = 0;
							LED_off();
							sMAXTEMP = Temperature;
							TemperatureOld = Temperature;
							eeWrite();
							Delay(DELAY_FLASH);
							PORTB = MAX;										//	Turn on light full blast.
						}
					}
					else {
						CritTempTicks = 0;
					}
				}
			}
		}
	}

// ------------------------- *CALIBRATE* VOLTAGE ---------------------------

	void CalibrateVoltage() {
		Delay(DELAY_VOLTCALIB);													//	Long delay so cell recovers from load.
		uint32_t iADC = ADC_read(RAW);
		uint32_t iIntRefVolt = (uint32_t)(((uint32_t)swVALUE * (uint32_t)2048000) / iADC);

		sINTREFVOLT = (uint16_t)(iIntRefVolt / 100);
		if (iIntRefVolt % 100 >= 50) sINTREFVOLT++;

		BlinkConfirm();
		WriteAndResetVoltStatus();												//	Save new settings and clear low voltage.
	}

// ------------------------ VOLTAGE CALIBRATE ------------------------------

	void VoltCalib() {

		#ifdef OFFSWITCH
			OswDelay();
			swVALUE = swCOUNT;
		#else
			swVALUE = EswGetValue(3);
		#endif

	//	NO PRESS: READOUT INTERNAL REFERENCE VOLTAGE

		if (swVALUE == NO_PRESS) {
			Delay(DELAY_MEDIUM);
			BlinkNumValue(sINTREFVOLT,5);
			BlinkError();
			swStartup = STARTDELAY_LONG;
		}

	//	LONG PRESS: RESET VOLT OFFSET TO DEFAULT

		else {
			if (swVALUE == LONG_PRESS) {
				sINTREFVOLT = INTREFVOLT;
				BlinkConfirm();
				WriteAndResetVoltStatus();										//	Save new settings and clear low voltage.
			}

	//	ESWITCH: CHECK IF IN RANGE

			#ifdef DUALSWITCH

				else {
					swVALUE -= 1000;
					if (swVALUE < 200 || swVALUE > 900) BlinkError();				//	Minimum value 2V, maximum value 9V.

	//	CONFIRM VALUE: CALCULATE AND SAVE NEW *CALIBRATED* INTERNAL REFERENCE VOLTAGE

					else {
						BlinkNumValue(swVALUE,3);
						if (SwConfirm()) CalibrateVoltage();
					}
				}

			#endif

	//	OFFSWITCH: FIRST DIGIT

			#ifdef OFFSWITCH

				if (swCOUNT <= 9) {
					swVALUE = swCOUNT * 100;
					swCOUNT = 0;
					swSETUP += SECOND_DIGIT;
					FlashReadout();
					OswDelay();
					BlinkError();
				}
				else BlinkError();

			#endif

		}
		ClearSwitch();
	}

	//	OFFSWITCH: FOLLOWING DIGITS AND CONFIRMATION

	#ifdef OFFSWITCH

	void VoltCalib_OswPart2() {

	//	PRESS VALUE: SECOND DIGIT:

		if (swSETUP == sVOLTCALIB+SECOND_DIGIT) {
			OswDelay();
			if (swCOUNT == NO_PRESS) BlinkError();
			else {
				if (swCOUNT <= 9 || swCOUNT == LONG_PRESS) {
					if (swCOUNT == LONG_PRESS) swCOUNT = 0;
					swVALUE += swCOUNT*10;
					swCOUNT = 0;
					swSETUP = sVOLTCALIB+THIRD_DIGIT;
					FlashReadout();
					OswDelay();
					BlinkError();
				}
				else BlinkError();
			}
			ClearSwitch();
		}

	//	PRESS VALUE: THIRD DIGIT:

		if (swSETUP == sVOLTCALIB+THIRD_DIGIT) {
			OswDelay();
			if (swCOUNT == NO_PRESS) BlinkError();
			else {
				if (swCOUNT <= 9 || swCOUNT == LONG_PRESS) {
					if (swCOUNT == LONG_PRESS) swCOUNT = 0;
					swVALUE += swCOUNT;
					swCOUNT = 0;

	//	PRESS VALUE: CHECK IF IN RANGE

					if (swVALUE < 200 || swVALUE > 900) BlinkError();				//	Minimum value 2V, maximum value 9V.
					else {
						BlinkNumValue(swVALUE,3);
						swSETUP = sVOLTCALIB+CONFIRMATION;
						SwConfirm();
					}
				}
				else BlinkError();
			}
			ClearSwitch();
		}

	//	CONFIRM VALUE: CALCULATE AND SAVE NEW *CALIBRATED* INTERNAL REFERENCE VOLTAGE

		if (swSETUP == sVOLTCALIB+CONFIRMATION) CalibrateVoltage();
	}

	#endif

// ------------------------ SET TEMPERATURE PROFILE ------------------------

	void TempProfile() {

		#ifdef OFFSWITCH
			SwConfirm();
		#else
			uint8_t swCOUNT = SwConfirm();
		#endif

		Delay(DELAY_LONG);

	//	NO CONFIRMATION: READOUT TEMPERATURE PROFILE

		if (swCOUNT == NO_PRESS) {
			if (sTEMPMONITOR) BlinkNumValue(sTEMPPROFILE,1);					//	Readout temperature profile value...
			else BlinkValue(0,0);												//	Except if temperature monitor is off.
			swStartup = STARTDELAY_LONG;
			BlinkError();
		}

	//	LONG CONFIRMATION: RESET TEMPERATURE PROFILE

		if (swCOUNT == LONG_PRESS) {
			sTEMPPROFILE = TEMPPROFILE;
			BlinkConfirm();
			eeWrite();
		}

	//	SHORT CONFIRMATION: TEMPERATURE PROFILING

		if (swCOUNT == 1) {

			if (CheckStatus()) BlinkValue(0,0);									//	Levels not normal, abort.
			else {

	//	LEVELS NORMAL: START TEMPERATURE PROFILING

				BlinkConfirm();
				Delay(DELAY_LONG);
				uint8_t iTemp = Temperature + 5;
				uint8_t iMaxTemp = iTemp;
				uint8_t iWdtTicks = 0;
				uint8_t iTempReadings = 0;
				uint8_t iModeHold = sMODE;

				sMODE = BOOSTMODE;
				LightMode();
				while (iTempReadings < 5) {										//	5 readings at target before turning LED off.
					if (Temperature >= iTemp) {
						iTempReadings++;
						iWdtTicks = WdtTicks;
						while (iWdtTicks == WdtTicks);
					}
					else {
						iTempReadings = 0;
					}
				}

				iTempReadings = 0;
				LED_off();
				ResetTimer();

				while (iTempReadings < 5) {										//	5 readings at target before stopping.
					if (Temperature < iTemp) {
						iTempReadings++;
						iWdtTicks = WdtTicks;
						while (iWdtTicks == WdtTicks);
					}
					else {
						if (Temperature > iMaxTemp) {
							iMaxTemp = Temperature;								//	If temperature rises after LED off.
							FlashReadout();
						}
						iTempReadings = 0;
					}
				}

				sTEMPPROFILE = WdtSecs + 1;
				BlinkNumValue(sTEMPPROFILE,1);
				Delay(DELAY_LONG);
				BlinkNumValue(iMaxTemp,1);
				sGENERICSETTINGS |= (1 << gsTempMonitor);						//	Automatically enable temp monitoring.
				sMODE = iModeHold;
				eeWrite();														//	Write setting change to EEPROM.
			}
		}
	}

// ------------------------ RESET TO DEFAULT -------------------------------

	void Reset2Def() {

		#ifdef OFFSWITCH

			if (swSETUP == sRESET2DEF) {
				swSETUP = RESET_CONFIRM_1;
				swCOUNT = 0;
				SwConfirm();
				ClearSwitch();
			}

		#else

			uint8_t iEswConfirm = SwConfirm();

			if (iEswConfirm) {
				ResetTimer();
				PWM_on(0);
				while (!ESWITCH && WdtSecs < 4) {								//	Confirmation flash. Pressing here will reset.
					if (WdtSecs % 2) PWM = LVL_READOUT+7;
					else PWM = LVL_READOUTLOW;
					Delay(DELAY_FLASH);
					PWM = 0;
					Delay(DELAY_FLASH);
				}
				PWM_off();
				uint8_t i = WdtSecs;											//	Save WDT second counter before delay.
				Delay(DELAY_MEDIUM);
				if (i < 4) {													//	If switch was pressed during confirmation flash, reset.
					BlinkConfirm();												//	Reset to default confirmed.
					BlinkConfirm();
					if (iEswConfirm == 255) BlinkConfirm();						//	Two confirmation blinks for reset, three for full reset.
					ResetToDefault(iEswConfirm);								//	Passes on short or long confirmation press.
				}
			}

		#endif
	}

	//	OFFSWITCH CONFIRMATION:

	void Reset2Def_OswConfirm() {
		if (swSETUP == RESET_CONFIRM_1) {
			if (swCOUNT == 1) swSETUP = RESET_CONFIRM_SHORT;
			if (swCOUNT == LONG_PRESS) swSETUP = RESET_CONFIRM_LONG;
			swCOUNT = 0;
			ResetTimer();
			PWM_on(0);
			while (WdtSecs < 4) {												//	Confirmation flash. Pressing here will reset.
				if (WdtSecs % 2) PWM = LVL_READOUT+7;
				else PWM = LVL_READOUTLOW;
				Delay(DELAY_FLASH);
				PWM = 0;
				Delay(DELAY_FLASH);
			}
			ClearSwitch();
		}

		if (swSETUP == RESET_CONFIRM_SHORT && swCOUNT) {
			BlinkConfirm();
			BlinkConfirm();
			ResetToDefault(1);
		}

		if (swSETUP == RESET_CONFIRM_LONG && swCOUNT) {
			BlinkConfirm();
			BlinkConfirm();
			BlinkConfirm();
			ResetToDefault(0);
		}
	}

// ======================== *MAIN* *STARTUP* ===============================

	eeRead();																	//	Read in all settings.

	uint16_t iADC = ADC_read(CAP);												//	Read off time capacitor.
	if (iADC >= OFF_SHORT) offTime = 2;											//	Check for short off press (2), long off press (1) or cold start (0).
	else if (iADC >= OFF_LONG) offTime = 1;

	DDRB = (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB5);		//	Set PB0 to PB3 and PB5 as output.
	PORTB = 0;

	if (bVOLTSTATUS > 4) bVOLTSTATUS = 0;										//	Check Voltage status for decay.
	if (bTEMPSTATUS > 2) bTEMPSTATUS = 0;										//	Check Temperature status for decay.

	WDT_on();																	//	Watch dog timer on.
	set_sleep_mode(SLEEP_MODE_IDLE);											//	Enable sleep mode between WDT interrupts. Triggered by sleep_mode().
	while (WdtTicks < 5);														//	Slight delay so charge of COT avoids triggering ESWITCH.

	ADC_read(VOLT);																//	First read to get E-switch status.
	WdtStartup = 1;
	if (offTime == 0) ColdStart();

// ------------------------ *ESWITCH* *SETUP* ------------------------------	//	*ESETUP*

	#ifdef DUALSWITCH

	if (ESWITCH) {

		offTime = 0;															//	Clear off time cap readings.
		ColdStart();

		while (ESWITCH) {														//	Switch hold blink to number of functions before timing out.

			if (SwitchTicks == FUNCTIONBLINKTIME) {
				swStartup++;													//	Increase menu selection.

				if ((SAFETYLOCK || sUI == UI_NOOB) && swSETUP == 10) swStartup = sRESET2DEF+1;
				if (swStartup < (sRESET2DEF/10)+1) {
					FlashReadout();
					swSETUP = swStartup * 10;
					if (swSETUP == sCALIBRATIONS) {								//	Small delay before proceeding to reset2default.
						while (ESWITCH && SwitchTicks < FUNCTIONBLINKTIME + (SEC/2));
					}
				}
				else swSETUP = 0;
				SwitchTicks = 0;
			}
		}
		while (ESWITCH);
		ClearSwitch();
		swStartup = STARTDELAY_MEDIUM;
	}

	#endif

// ------------------------ *OFFSWITCH* *SETUP* ----------------------------	//	*OSETUP*

	#ifdef OFFSWITCH

	//	LONG PRESS

	if (offTime == 1) {
		if (swSETUP > 1) swCOUNT = LONG_PRESS;
		swRAMP = 0;
	}

	//	SETUP PRESS COUNT:

	if (offTime == 2 || (offTime == 1 && swSETUP == 1)) {
		if (swSETUP) {
			if (swCOUNT < 9) {
				swCOUNT++;
				Delay(DELAY_SETUPPRESS);
				if (swSETUP ==1) swSETUP = 2;
			}

	//	SETUP ACCESS: THREE TO EIGHT PRESSES AFTER COLD START

			if (swSETUP == 2) {
				if (sUI == UI_NOOB || (sUI != UI_NOOB && swCOUNT == 9)) {
					if (swCOUNT == 9) ToggleNoob();
					else {
						swCOUNT = 0;
						swSETUP = 0;
					}
				}
				else {
					if (swCOUNT >= 3 && swCOUNT <=8) {
						BlinkConfirm();
						swSETUP = (swCOUNT-2)*10;
						swCOUNT = 0;
					}
				}
			}
		}
	}

	if (swSETUP >= 10) {
		offTime = 0;
		swStartup = STARTDELAY_MEDIUM;											//	Standard startup delay after setup routine done.
	}

	#endif

// ------------------------ *SETUP* MENU -----------------------------------

	if (swSETUP == sSELECTUI) SelectUI();
	if (swSETUP == sSPECIALMODES) SpecialModes();
	if (swSETUP == sMODESETTINGS || swSETUP == sMONITORING || swSETUP == sCALIBRATIONS) Setup();
	if (swSETUP == sSOS) ToggleSOS();

	if (swSETUP == sSETMODECNT) SetModeCount();
	if (swSETUP == sSETMODEMEM) SetModeMemory();
	if (swSETUP == sSETBOOSTIME) SetBoostTimer();

	if (swSETUP == sSETMAXTEMP) SetMaxTemp();
	if (swSETUP == sTEMPPROFILING) TempProfile();
	if (swSETUP == sSETLOWVMON || swSETUP == sSETCRITVMON || swSETUP == sTEMPCALIB) VoltMonTempCalib();

	if (swSETUP == sVOLTCALIB) VoltCalib();
	if (swSETUP == sRESET2DEF) Reset2Def();

	#ifdef OFFSWITCH
		if (swSETUP == sSETLOWVMON+SECOND_DIGIT || swSETUP == sSETCRITVMON+SECOND_DIGIT || swSETUP == sTEMPCALIB+SECOND_DIGIT) VoltMonTempCalib_OswDigit2();
		VoltMonTempCalibSet();
		VoltCalib_OswPart2();
		Reset2Def_OswConfirm();
	#endif

// -------------------- END OF SETUP *FUNCTIONS* ---------------------------

	while (ESWITCH);															//	If E-switch still pressed after startup, halt to avoid going directly to boost.
	ClearSwitch();

// ------------------------ *SAFETY* LOCK CHECK ----------------------------

	if (SAFETYLOCK) {															//	If safety lock is engaged:
		uint8_t SafetyLockblinked = 0;
		while (1) {

			#ifdef DUALSWITCH
				if (ESWITCH) {
					ResetTimer();
					SafetyLockblinked = 0;
					while (ESWITCH) {}
				}
			#endif

			if (!(WdtSecs % 4)) {
				if (!SafetyLockblinked) {
					PWM_on(LVL_READOUT);
					Delay(DELAY_FLASH);
					PWM_off();
					SafetyLockblinked = 1;
					sleep_mode();
				}
			}
			else {
				SafetyLockblinked = 0;
			}
		}
	}

// ======================== LIGHT *STARTUP* ================================

	if (swStartup == STARTDELAY_SHORT) Delay(DELAY_SHORT);
	if (swStartup == STARTDELAY_MEDIUM) Delay(DELAY_MEDIUM);
	if (swStartup == STARTDELAY_LONG) Delay(DELAY_LONG);
	if (swStartup == STARTDELAY_LONG_KEEPMEM) Delay(DELAY_LONG);

	ResetTimer();

// ------------------------ COLD STARTUP ----------------------------------

	if (offTime == 0) {
		if (sUI != SOS && swStartup < STARTDELAY_LONG_KEEPMEM) {				//	SOS and mode level readout always keep current mode despite memory setting.
			#ifdef OFFSWITCH
				bMODE = sMODE;													//	Copy current mode to keep mode in mode level readout when memory disabled.
			#endif
			if (sMODEMEM) {
				if (sMODE == BOOSTMODE && sMODEMEM == 1) {						//	Boost was last mode but boost memory disabled.
					if (sUI == 3) sMODE = RAMPMODE1;
					else sMODE = sMODEHOLD;
					eeWrite();
				}
			}
			else {
				if (bPROGSTATUS != 6) ResetMode();								//	Keep mode if programming mode selected and mode memory is off.
			}
		}
	}

// ==================== ESWITCH *OFFTIME* CAP FUNCTIONS ====================

	#ifdef DUALSWITCH

		if (sUI <= 3 || (sUI >= SOS && sUI <= STROBE) || sUI == UI_NOOB) {

	//	SHORT OFF PRESS : UI 1/2/STROBE NEXT MODE, UI 3 ENABLE LEVEL PROGRAMMING.

			if (offTime == 2) {
				if (sUI == 3) bPROGSTATUS = 2;
				else {
					NextMode();
					eeWrite();
				}
			}

	//	LONG OFF PRESS : UI 1/2/STROBE PREVIOUS MODE, UI 3 EXIT LEVEL PROGRAMMING.

			if (offTime == 1) {
				if (sUI == 3) bPROGSTATUS = 0;
				else {
					PrevMode();
					eeWrite();
				}
			}
		}

	#endif

// ==================== OSWITCH *OFFTIME* CAP FUNCTIONS ====================

	#ifdef OFFSWITCH

		if ((sUI <= 3 && !bPROGSTATUS) || sUI == SOS || sUI == UI_NOOB) {

	//	SHORT OFF PRESS : UI 1/2/SOS NEXT MODE, UI 3 ENABLE LEVEL PROGRAMMING.

			if (offTime == 2) {
				if (sUI == 3) {
					if (sMODE == RAMPMODE1) sMODE = BOOSTMODE;					//	Activate boost.
					else sMODE = RAMPMODE1;										//	Deactivate boost.
					eeWrite();
				}
				else {
					if (sUI == 2 && sMODE == BOOSTMODE) sMODE = sMODEHOLD;
					else NextMode();
					eeWrite();
				}
				offTime = 0;
			}

	//	LONG OFF PRESS : UI 1/2/SOS PREVIOUS MODE.

			if (offTime == 1) {
				if (sUI == 2) {
					if (sMODE != BOOSTMODE) sMODEHOLD = sMODE;					//	Store the mode in use before boost.
					sMODE = BOOSTMODE;											//	Activate boost.
					eeWrite();
				}
				else {
					if (sUI == 3) {
						bPROGSTATUS = 2;
					}
					else {
						PrevMode();
						eeWrite();
					}
				}
				offTime = 0;
			}
		}

// ------------------------ MODE PROGRAMMING -------------------------------

		if (sUI == 4 || (sUI <= 3 && bPROGSTATUS)) {
			if (offTime == 2) {
				if (!bPROGSTATUS || bPROGSTATUS == 4 || swRAMP || (sUI <= 3 && bPROGSTATUS != 5)) {
					swRAMP++;
					eswLevel = 0;
					swPressed = 1;												//	Set switch to ramp up.
					if (swRAMP >= 2) swPressed = 2;								//	Set switch to ramp down.
					if (bPROGSTATUS == 4) Modes[sMODE] = bMODE;
				}
				else {															//	If was in programming mode:
					Modes[sMODE] = bMODE;										//	Copy mode level from no init to current mode.
					bPROGSTATUS = 4;											//	Start save mode timer.
				}
			}
			if (offTime == 1) {
				if (sUI <= 2) {													//	Long presses in UI 1 & 2 changes mode.
					NextMode();
					eeWrite();
					swPressed = 0;
					bPROGSTATUS = 2;
				}
				else bPROGSTATUS = 0;											//	If was in programming mode, abort.
			}
		}

// ------------------------ *BEACON* & *STROBE* ----------------------------

		if (sUI == BEACON || sUI == STROBE) {
			if (offTime == 2) {
				swRAMP++;
				uint8_t iRamp = swRAMP;
				Delay(DELAY_SETUPPRESS);
				if (iRamp == 1) {
					if (sSTROBESPEED++ == 13) sSTROBESPEED = 0;
				}
				if (iRamp >= 2) {
					if (sSTROBESPEED-- == 0) sSTROBESPEED = 13;
				}
				eeWrite();
			}
			if (offTime == 1) {
				NextMode();
				eeWrite();
			}
		}

// -------------------- END OF *OFFTIME* CAP FUNCTIONS ---------------------

	#endif

// ======================== *INITIALIZE* LIGHT =============================

	if (sMODE == BOOSTMODE) BOOSTSTATUS = 3;
	if (bPROGSTATUS == 6) bPROGSTATUS = 2;

	#ifdef DUALSWITCH

		if (sUI == 4) bPROGSTATUS = 0;

		if (bPROGSTATUS) {
			Delay(DELAY_BLINK);
			Flash(5,FLASH_ONOROFF,DELAY_FLASH);
			bPROGSTATUS = 2;													//	PROGSTATUS 2 sets timer to count down to exit programming mode.
			ProgModeTicks = 0;													//	Reset programming mode timer on every startup in mode programming.
		}
	#endif

	#ifdef OFFSWITCH
		if (bPROGSTATUS && bPROGSTATUS < 4 && sUI != 4 && !swPressed) {
			Delay(DELAY_BLINK);
			Flash(5,FLASH_ONOROFF,DELAY_FLASH);
			ProgModeTicks = 0;													//	Reset programming mode timer on every startup in mode programming.
		}
	#endif

	if (bTEMPSTATUS) CritTempTicks = (sTEMPPROFILE * SEC) - 2*SEC;				//	If TEMP status is critical force flash after two seconds.

	LightMode();
	WdtStartup = 2;																//	Enables voltage and temperature monitoring.
	StatusAfterSwitch();

// ------------------------ *STROBE* MODES ---------------------------------

	//	*SOS* STROBE:

	while (sUI == SOS) {
		Flash(3,FLASH_LIGHTMODE,14);
		Delay(50);
		Flash(3,FLASH_LIGHTMODE,50);
		Flash(3,FLASH_LIGHTMODE,14);
		Delay(SEC*2);
	}

	//	*BEACON* & STROBE:

	while (sUI == BEACON || sUI == STROBE) {
		if (StatusAlert) {
			if (StatusAlert == 1) StepDown(0,0);
			else StepDown(0,1);
			ResetVoltTimers();
			ResetTempTimers();
			StatusAlert = 0;
		}

		if (sUI == BEACON) {
			BlinkOn();
			Delay(DELAY_SHORT);
			BlinkOn();
			Delay(StrobeSpeeds[sSTROBESPEED] * 2);
		}

		if (sUI == STROBE) {
			Flash(1,FLASH_LIGHTMODE,StrobeSpeeds[sSTROBESPEED]);
		}
	}

// ************************ *MAIN* LOOP ************************************

	while (1) {																	//	*WHILE*

// ======================== *ESWITCH* HANDLING =============================
	
	#ifdef DUALSWITCH

		if (ESWITCH) StatusAfterSwitch();

// -------------------- UI 1 SHORT PRESS : NEXT MODE -----------------------

		if ((sUI == 1 || sUI == UI_NOOB) && !bPROGSTATUS) {

			if (swPressed && !ESWITCH && !swLongPressed) {						//	Short press with E-switch.
				NextMode();
				LightMode();
			}

// -------------------- UI 1 LONG PRESS : PREVIOUS MODE --------------------

			if (ESWITCH) {														//	Complicated routine that allows rap around to boost only from lowest mode.
				if (swLongPressed == 1) {
					PrevMode();
					LightMode();
					swLongPressed = 2;
				}
				if (swLongPressed == 3) {
					if ((sMODEDIRL2H && sMODE) || (!sMODEDIRL2H && sMODE < BOOSTMODE)) {
						PrevMode();
						LightMode();
						swLongPressed = 2;
						SwitchTicks = LONGPRESSTIME+1;
					}
				}
			}
		}

// -------------------- UI 2 & 3 : *BOOST* ---------------------------------
/*
		BOOSTSTATUS:
		5 = Switch pressed, on.
		4 = On and free running time met (switch not yet released).
		3 = On and free running (switch released).
		2 = Boost turned off while in free running (switch not yet released).
		1 = Off due to timeout.
		0 = Off and in standby.
*/

		if (((sUI == 2 && sMODECOUNT) || sUI == 3) && !bPROGSTATUS) {

			if (ESWITCH && !BOOSTSTATUS && !CheckStatus()) {					//	If E-switch, boost in standby, volt / temp levels are normal.
				BOOSTSTATUS = 5;												//	Activate boost while switch pressed.
				sMODEHOLD = sMODE;												//	Store current mode before switching to boost.
				sMODE = BOOSTMODE;
				LightMode();
			}

			if (ESWITCH && BOOSTSTATUS==5 && SwitchTicks >= BOOSTFREERUN) {		//	If E-switch, boost on and free running duration has been met:
				BOOSTSTATUS = 4;												//	Set boost in free running, button is still pressed though.
				LED_off();
				Delay(DELAY_FLASH);
				LightMode();													//	Short blink to indicate free running press time met.
			}

			if (!ESWITCH && BOOSTSTATUS == 5) {									//	If switch released before free run duration met:
				BOOSTSTATUS = 0;												//	Turn off boost.
				sMODE = sMODEHOLD;												//	Restore mode to the mode prior to engaging boost.
				LightMode();
			}

			if (!ESWITCH && BOOSTSTATUS == 4) {									//	If switch released while boost on and free run duration was met:
				BOOSTSTATUS = 3;												//	Let boost free run (until timeout or new switch press).
				LightMode();
			}

			if (ESWITCH && BOOSTSTATUS == 3) {									//	If switch pressed while boost was free running:
				BOOSTSTATUS = 2;												//	Boost off and wait until switch is released.
				sMODE = sMODEHOLD;												//	Restore mode to the mode prior to engaging boost.
				LightMode();
			}
		}

	#endif

//	------------------- UI 4 OR PROGRAM MODE -------------------------------

		if (sUI == 4 || bPROGSTATUS) {

			uint16_t *pMode = &Modes[sMODE];									//	Pointer for ramping.

		//	*ADJUST*

			if (ESWITCH && SwitchTicks >= 40) {

				bPROGSTATUS = 5;												//	PROGSTATUS 5: Switch pressed and level is adjusting.
				if (SwitchTicks >= 30) SwitchTicks = 30;
				BoostTicks = 0;													//	Boost can only be active here if in programming mode. Reset boost timer while mode programming.

				uint8_t iMultiplier = 50;

				if (swPressed == 1) {
					#if !defined(PB5_IO) || defined(F8)
						if (*pMode >= RAMPMAX) {
							*pMode = MAX * 256;
						}
						else {
							*pMode += ((*pMode + iMultiplier) / iMultiplier);
							if (*pMode > RAMPMAX) *pMode = RAMPMAX;
						}							
					#else
						*pMode += ((*pMode + iMultiplier) / iMultiplier);
						if (*pMode > RAMPMAX) *pMode = RAMPMAX;
					#endif
				}
				else {
					if (*pMode) *pMode -= ((*pMode + iMultiplier) / iMultiplier);
					if (*pMode > RAMPMAX) *pMode = RAMPMAX;
				}

				if (*pMode < 12) {												//	Slower brightness adjusting for lower values.
					SwitchTicks = 8;
				}
				else {
					if (*pMode < 22) {
						SwitchTicks = 35;
					}
					else {
						SwitchTicks = 38;
					}
				}

			//	Level limiters:

				uint8_t iRampMax = 0;

				if (CheckStatus() == 1 && *pMode > LOWVOLTMAX) {
					*pMode = LOWVOLTMAX;
					iRampMax = 1;
				}

				if (CheckStatus() == 2 && *pMode > CRITICALMAX) {
					*pMode = CRITICALMAX;
					iRampMax = 1;
				}

				#ifdef OFFSWITCH
					bMODE = *pMode;												//	Store adjusted mode level in no init, saves a lot of eeprom writing.
				#endif

				if (*pMode >= RAMPMAX || iRampMax) {
					FlashOff();													//	Short blink off to notify max level reached.
					LightMode();
					Delay(DELAY_MEDIUM);
				}
			}

			if (bPROGSTATUS == 3) {												//	PROGSTATUS 3: Level save timer timed out.
				eeWrite();
				bPROGSTATUS = 0;												//	Will exit programming mode directly if UI 3/4/5.
				if (sUI <= 2) bPROGSTATUS = 2;									//	PROGSTATUS 2: Start countdown to exit programming mode.
				if (sUI <= 3) BlinkOnOffThreshold(0);							//	Blink once on mode level save in UI 1/2/3.
				LightMode();
				if (sUI == 3) Delay(DELAY_SHORT);								//	Small delay to avoid accidental boost if pressed slightly after timeout.
				while (ESWITCH);
				ProgModeTicks = 0;
			}

			if (bPROGSTATUS == 1) {												//	PROGSTATUS 1: Programming mode timer timed out.
				bPROGSTATUS = 0;
				ProgModeTicks = 0;
				if (sUI <= 3) {
					BlinkOnOffThreshold(0);										//	UI 1/2/3 blink exit programming mode. UI 3 blinks only if level not adjusted.
					LightMode();
					if (sUI <= 2) {
						Delay(DELAY_SHORT);
						BlinkOnOffThreshold(0);									//	UI 1/2 second blink exiting programming mode.
					}
					LightMode();
					Delay(DELAY_SHORT);
					while (ESWITCH);
				}
			}

			#ifdef DUALSWITCH
				if (!ESWITCH && bPROGSTATUS == 5) bPROGSTATUS = 4;				//	PROGSTATUS 4: Switch released, start countdown to EEPROM save.
			#endif

			LightMode();
		}

// -------------------- E-SWITCH PRESS RELEASE -----------------------------

		#ifdef DUALSWITCH

			if (!ESWITCH && (swPressed || swLongPressed)) {						//	E-switch released after press:
				ClearSwitch();
				if ((sUI <= 2 || sUI == UI_NOOB || BOOSTSTATUS==3) && (CheckStatus() < 2) && !bPROGSTATUS) eeWrite();
			}

		#endif

// -------------------- *BOOST MODE TIMEOUT* -------------------------------

		if (!ESWITCH && (BOOSTSTATUS==1 || BOOSTSTATUS==2 )) StepDown(0,0);		//	Boost timed out, or switch released after turning off.

// ======================== *MONITORING* ===================================

// -------------------- *LOW* *VOLTAGE* LEVEL ------------------------------

		if (StatusAlert == ALERT_LOWVOLT) {

			if (sUI < 3) bPROGSTATUS = 0;										//	If in programming mode, exit.

			//	OUTPUT AT OR BELLOW ALLOWED LIMIT:

			if (BOOSTSTATUS <=2 && (Modes[sMODE]<=LOWVOLTMAX)) bVOLTSTATUS= 2;	//	Set voltage status to low in lowest.

			StepDown(2,0);

			StatusAlert = 0;													//	Status alert dealt with, reset.
			ResetVoltTimers();
		}

// -------------------- *CRITICAL* VOLTAGE LEVEL ---------------------------

		if (StatusAlert == ALERT_CRITVOLT) {

			if (sUI < 3) bPROGSTATUS = 0;										//	If in programming mode, exit.

			//	NOT BOOST MODE AND OUTPUT ABOVE ALLOWED LIMIT:


			if (bVOLTSTATUS < 4 || Modes[sMODE] > CRITICALMAX) {				//	If first critical warning or output above limit.
				StepDown(2,1);
				bVOLTSTATUS = 4;												//	Set voltage status to critical.
			}

			//	OUTPUT ALREADY AT OR BELLOW ALLOWED LIMIT:

			else StepDown(0,1);

			ResetVoltTimers();
			StatusAlert = 0;													//	Status alert dealt with, reset.
		}

// -------------------- *TEMPERATURE* MONITORING ---------------------------

		if (StatusAlert >= ALERT_HIGHTEMP) {

			if (sUI < 3) bPROGSTATUS = 0;										//	If in programming mode, exit.

			if (StatusAlert == ALERT_HIGHTEMP) StepDown(3,0);
			else StepDown(3,1);
			ResetTempTimers();
			StatusAlert = 0;													//	Status alert dealt with, reset.
		}

// ======================== END MONITORING =================================

		sleep_mode();
    }

// ************************ END MAIN LOOP **********************************
    return 0;
}
