/****************************************************************************************
 * channels.h - output channel support with ramping tables
 * ==========
 *
 *    ATtiny85 Diagrams
 *    -----------------
 *
 * For TA style triples, 3 channel:
 *              ---
 *   Reset  1 -|   |- 8  VCC
 *  switch  2 -|   |- 7  Voltage ADC or Indicator LED
 * FET PWM  3 -|   |- 6  bank of 7135s PWM
 *     GND  4 -|   |- 5  one 7135 PWM
 *              ---
 *
 * For standard FET+1, 2 channels:
 *              ---
 *   Reset  1 -|   |- 8  VCC
 *  switch  2 -|   |- 7  Voltage ADC
 * Ind.LED  3 -|   |- 6  FET PWM
 *     GND  4 -|   |- 5  7135 PWM
 *              ---
 *
 * For 1 channel, using a NANJG or FET+1:
 *              ---
 *   Reset  1 -|   |- 8  VCC
 *  switch  2 -|   |- 7  Voltage ADC
 * Ind.LED  3 -|   |- 6  FET or 7135s PWM
 *     GND  4 -|   |- 5  Not Used (7135 PWM)
 *              ---
 *
 *
 ****************************************************************************************/
#ifndef CHANNELS_H_
#define CHANNELS_H_

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#if   OUT_CHANNELS == 2			// FET+1 or Buck driver
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

#ifdef ONBOARD_LED
/**************************************************************************************
* TurnOnBoardLed
* ==============
**************************************************************************************/
void TurnOnBoardLed(byte on)
{
	if (onboardLedEnable)
	{
		if (on)
		{
			DDRB = (1 << ONE7135_PWM_PIN) | (1 << FET_PWM_PIN) | (1 << ONBOARD_LED_PIN);
		  #ifndef GRND_TO_LED
			PORTB |= (1 << ONBOARD_LED_PIN);
		  #endif
		}
		else
		{
			DDRB = (1 << ONE7135_PWM_PIN) | (1 << FET_PWM_PIN);
		  #ifndef GRND_TO_LED
			PORTB &= 0xff ^ (1 << ONBOARD_LED_PIN);
		  #endif
		}
	}
}
#endif

/**************************************************************************************
* DefineModeSet
* =============
**************************************************************************************/
void DefineModeSet()
{
	byte offset = 1;

	modesCnt = pgm_read_byte(modeSetCnts+modeSetIdx);

	// Set OFF mode states (index 0)
	by7135Modes[0] = byFETModes[0] = 0;

	if (moonLightEnable)
	{
		offset = 2;
		by7135Modes[1] = moonlightLevel;	// PWM value to use for moonlight mode
	  #ifdef GT_BUCK
		byFETModes[1] = 25;					// for GT-buck, set analog dimming to 10%
	  #else
		byFETModes[1] = 0;
	  #endif
	}

	// Populate the RAM based current mode set
	int i;									// winAVR does not like this inside the for loop (weird!!)
	for (i = 0; i < modesCnt; i++)
	{
		by7135Modes[offset+i] = pgm_read_byte(modeTable7135[modeSetIdx]+i);
		byFETModes[offset+i] = pgm_read_byte(modeTableFet[modeSetIdx]+i);
	}

	modesCnt += offset;		// adjust to total mode count
}

/**************************************************************************************
* SetOutput - sets the PWM output values directly for the two channels
* =========
**************************************************************************************/
inline void SetOutput(byte pwm7135, byte pwmFet)
{
	ONE7135_PWM_LVL = pwm7135;
	FET_PWM_LVL = pwmFet;
}

/**************************************************************************************
* SetLevel - uses the ramping levels to set the PWM output
* ========		(0 is OFF, 1..TURBO_LEVEL is the ramping index level)
**************************************************************************************/
void SetLevel(byte level)
{
	if (level == 0)
	{
		SetOutput(OFF_OUTPUT);
		
	  #ifdef ONBOARD_LED
		if (locatorLed)
			TurnOnBoardLed(1);
	  #endif
	}
	else
	{
	  #ifdef TURBO_LEVEL_SUPPORT
		if (level == TURBO_LEVEL)
		{
		  #ifdef GT_BUCK
			SetOutput(255, 255);
		  #else
			SetOutput(0, 255);
		  #endif
		}
		else
		{
			level -= 1;	// make it 0 based
			SetOutput(pgm_read_byte(ramp_7135 + level), pgm_read_byte(ramp_FET  + level));
		}
	  #else
		level -= 1;	// make it 0 based
		SetOutput(pgm_read_byte(ramp_7135 + level), pgm_read_byte(ramp_FET  + level));
	  #endif
		
	  #ifdef ONBOARD_LED
		if (locatorLed)
			TurnOnBoardLed(0);
	  #endif
	}
}

/**************************************************************************************
* SetMode
* =======
**************************************************************************************/
void inline SetMode(byte mode)
{
	currOutLvl1 = by7135Modes[mode];
	currOutLvl2 = byFETModes[mode];
	
	SetOutput(currOutLvl1, currOutLvl2);

  #ifdef ONBOARD_LED
	if ((mode == 0) && locatorLed)
		TurnOnBoardLed(1);
	else if (last_modeIdx == 0)
		TurnOnBoardLed(0);
  #endif
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#elif OUT_CHANNELS == 3		// Triple Channel
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

#ifdef ONBOARD_LED
/**************************************************************************************
* TurnOnBoardLed
* ==============
**************************************************************************************/
void TurnOnBoardLed(byte on)
{
	if (onboardLedEnable)
	{
		if (on)
		{
			DDRB = (1 << ONE7135_PWM_PIN) | (1 << BANK_PWM_PIN) | (1 << FET_PWM_PIN) | (1 << ONBOARD_LED_PIN);
		
		  #ifndef GRND_TO_LED
			PORTB |= (1 << ONBOARD_LED_PIN);
		  #endif
		}
		else
		{
			DDRB = (1 << ONE7135_PWM_PIN) | (1 << BANK_PWM_PIN) | (1 << FET_PWM_PIN);
		  #ifndef GRND_TO_LED
			PORTB &= 0xff ^ (1 << ONBOARD_LED_PIN);
		  #endif
		}
	}
}
#endif

/**************************************************************************************
* DefineModeSet
* =============
**************************************************************************************/
void DefineModeSet()
{
	byte offset = 1;

	modesCnt = pgm_read_byte(modeSetCnts+modeSetIdx);

	// Set OFF mode states (index 0)
	by7135Modes[0] = by7135sModes[0] = byFETModes[0] = 0;

	if (moonLightEnable)
	{
		offset = 2;
		by7135Modes[1] = moonlightLevel;	// PWM value to use for moonlight mode
		by7135sModes[1] = byFETModes[1] = 0;
	}

	// Populate the RAM based current mode set
	for (int i = 0; i < modesCnt; i++)
	{
		by7135Modes[offset+i] = pgm_read_byte(modeTable7135[modeSetIdx]+i);
		by7135sModes[offset+i] = pgm_read_byte(modeTable7135s[modeSetIdx]+i);
		byFETModes[offset+i] = pgm_read_byte(modeTableFet[modeSetIdx]+i);
	}

	modesCnt += offset;		// adjust to total mode count
}

/**************************************************************************************
* SetOutput - sets the PWM output value directly
* =========
**************************************************************************************/
inline void SetOutput(byte pwm7135, byte pwm7135s, byte pwmFet)
{
	ONE7135_PWM_LVL = pwm7135;
	BANK_PWM_LVL = pwm7135s;
	FET_PWM_LVL = pwmFet;
}

/**************************************************************************************
* SetLevel - uses the ramping levels to set the PWM output
* ========		(0 is OFF, 1..RAMP_SIZE is the ramping index level)
**************************************************************************************/
void SetLevel(byte level)
{
	if (level == 0)
	{
		SetOutput(0,0,0);
	  #ifdef ONBOARD_LED
		if (locatorLed)
			TurnOnBoardLed(1);
	  #endif
	}
	else
	{
		level -= 1;	// make it 0 based
		SetOutput(pgm_read_byte(ramp_7135 + level), pgm_read_byte(ramp_7135s + level), pgm_read_byte(ramp_FET  + level));
	  #ifdef ONBOARD_LED
		if (locatorLed)
			TurnOnBoardLed(0);
	  #endif
	}
}

/**************************************************************************************
* SetMode
* =======
**************************************************************************************/
void inline SetMode(byte mode)
{
	currOutLvl1 = by7135Modes[mode];
	currOutLvl2 = by7135sModes[mode];
	currOutLvl3 = byFETModes[mode];
	
	SetOutput(currOutLvl1, currOutLvl2, currOutLvl3);
  #ifdef ONBOARD_LED
	if ((mode == 0) && locatorLed)
		TurnOnBoardLed(1);
	else if (last_modeIdx == 0)
		TurnOnBoardLed(0);
  #endif
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#elif OUT_CHANNELS == 1		// single FET or single bank of 7135's
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

#ifdef ONBOARD_LED
/**************************************************************************************
* TurnOnBoardLed
* ==============
**************************************************************************************/
void TurnOnBoardLed(byte on)
{
	if (onboardLedEnable)
	{
		if (on)
		{
			DDRB = (1 << MAIN_PWM_PIN) | (1 << ONBOARD_LED_PIN);
		  #ifndef GRND_TO_LED
			PORTB |= (1 << ONBOARD_LED_PIN);
		  #endif
		}
		else
		{
			DDRB = (1 << MAIN_PWM_PIN);
		  #ifndef GRND_TO_LED
			PORTB &= 0xff ^ (1 << ONBOARD_LED_PIN);
		  #endif
		}
	}
}
#endif

/**************************************************************************************
* DefineModeSet
* =============
**************************************************************************************/
void DefineModeSet()
{
	byte offset = 1;

	modesCnt = pgm_read_byte(modeSetCnts+modeSetIdx);

	// Set OFF mode states (index 0)
	byMainModes[0] = 0;

	if (moonLightEnable)
	{
		offset = 2;
		byMainModes[1] = moonlightLevel;	// PWM value to use for moonlight mode
	}

	// Populate the RAM based current mode set
	for (int i = 0; i < modesCnt; i++)
	{
		byMainModes[offset+i] = pgm_read_byte(modeTable1Chan[modeSetIdx]+i);
	}

	modesCnt += offset;		// adjust to total mode count
}

/**************************************************************************************
* SetOutput - sets the PWM output values directly for the one channel
* =========
**************************************************************************************/
inline void SetOutput(byte pwmChan)
{
	MAIN_PWM_LVL = pwmChan;
}

/**************************************************************************************
* SetLevel - uses the ramping levels to set the PWM output
* ========		(0 is OFF, 1..RAMP_SIZE is the ramping index level)
**************************************************************************************/
void SetLevel(byte level)
{
	if (level == 0)
	{
		SetOutput(OFF_OUTPUT);
		
		#ifdef ONBOARD_LED
		if (locatorLed)
		TurnOnBoardLed(1);
		#endif
	}
	else
	{
		level -= 1;	// make it 0 based
		SetOutput(pgm_read_byte(ramp_1Chan + level));
		
		#ifdef ONBOARD_LED
		if (locatorLed)
		TurnOnBoardLed(0);
		#endif
	}
}

/**************************************************************************************
* SetMode
* =======
**************************************************************************************/
void inline SetMode(byte mode)
{
	currOutLvl = byMainModes[mode];
	
	SetOutput(currOutLvl);

	#ifdef ONBOARD_LED
	if ((mode == 0) && locatorLed)
	TurnOnBoardLed(1);
	else if (last_modeIdx == 0)
	TurnOnBoardLed(0);
	#endif
}

#endif

#endif /* CHANNELS_H_ */