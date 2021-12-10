/****************************************************************************************
 * ModeSets_HiPerf.h - sets of fixed mode levels for High Performance
 * ================= (dont PWM both 7135 and FET)
 *
 ****************************************************************************************/

#ifndef MODESETS_H_
#define MODESETS_H_

//---------------------------------------------------------------------------------------
#if   OUT_CHANNELS == 2			// FET+1 or Buck driver
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------
//  Standard FET+1 2 channel modes
//---------------------------------------------------------------------

// 1 mode (max)                         max
PROGMEM const byte modeFetSet1[] =  {   255};
PROGMEM const byte mode7135Set1[] = {     0};

// 2 modes (10-max)                     100%///max
PROGMEM const byte modeFetSet2[] =  {     0,   255 };
PROGMEM const byte mode7135Set2[] = {   255,     0 };

// 3 modes (5-35-max)                   100% |  30%   max
PROGMEM const byte modeFetSet3[] =  {     0,    75,   255 };	// Must be low to high
PROGMEM const byte mode7135Set3[] = {   255,     0,     0 };	// for secondary (7135) output

// ************************************************************
//             ***  TEST ONLY  ***
// ************************************************************
//PROGMEM const byte modeFetSet3[] =  {     3,  64,  128,   255 };	// Must be low to high
//PROGMEM const byte mode7135Set3[] = {     0,   0,   0,     0 };	// for secondary (7135) output
// ************************************************************

// 4 modes                               12%   100% |  40%   max
PROGMEM const byte modeFetSet4[] =  {     0,     0,   102,   255 };	// Must be low to high
PROGMEM const byte mode7135Set4[] = {    30,   255,     0,     0 };	// for secondary (7135) output

// 5 modes                               15%   100% |  10%    40%   max
PROGMEM const byte modeFetSet5[] =  {     0,     0,    25,   102,   255 };	// Must be low to high
PROGMEM const byte mode7135Set5[] = {    37,   255,     0,     0,     0 };	// for secondary (7135) output

// 6 modes                               12%   100% |  10%    25%    50%   max
PROGMEM const byte modeFetSet6[] =  {     0,     0,    25,    64,   128,   255};
PROGMEM const byte mode7135Set6[] = {    30,   255,     0,     0,     0,     0};

// 7 modes                                8%    30%   100% |  8%    20%    60%   max
PROGMEM const byte modeFetSet7[] =  {     0,     0,     0,    20,    51,   153,   255};
PROGMEM const byte mode7135Set7[] = {    20,    75,   255,     0,     0,     0,     0};

// 8 modes                               8%     24%    60%   100% |   6%    25%    70%   max
PROGMEM const byte modeFetSet8[] =  {     0,     0,     0,     0,    15,    64,   178,   255};
PROGMEM const byte mode7135Set8[] = {    20,    61,   153,   255,     0,     0,     0,     0};

// 6 modes                               12%   100% |  10%    25%    50%   max
PROGMEM const byte modeFetSet9[] =  {     0,     0,    24,    56,   137,   255};
PROGMEM const byte mode7135Set9[] = {    30,   255,     0,     0,     0,     0};

// 6 modes                               12%   100% |  10%    25%    50%   max
PROGMEM const byte modeFetSet10[] =  {     0,     0,    24,    56,   137,   255};
PROGMEM const byte mode7135Set10[] = {    30,   255,     0,     0,     0,     0};

// 6 modes                               12%   100% |  10%    25%    50%   max
PROGMEM const byte modeFetSet11[] =  {     0,     0,    24,    56,   137,   255};
PROGMEM const byte mode7135Set11[] = {    30,   255,     0,     0,     0,     0};

// 6 modes                               12%   100% |  10%    25%    50%   max
PROGMEM const byte modeFetSet12[] =  {     0,     0,    24,    56,   137,   255};
PROGMEM const byte mode7135Set12[] = {    30,   255,     0,     0,     0,     0};

PROGMEM const byte modeSetCnts[] = {
		sizeof(modeFetSet1), sizeof(modeFetSet2), sizeof(modeFetSet3), sizeof(modeFetSet4), sizeof(modeFetSet5), sizeof(modeFetSet6),
		sizeof(modeFetSet7), sizeof(modeFetSet8),	sizeof(modeFetSet9), sizeof(modeFetSet10), sizeof(modeFetSet11), sizeof(modeFetSet12)};

const byte *(modeTableFet[]) =  { modeFetSet1, modeFetSet2, modeFetSet3, modeFetSet4,  modeFetSet5,  modeFetSet6,
		modeFetSet7, modeFetSet8, modeFetSet9, modeFetSet10, modeFetSet11, modeFetSet12};
const byte *modeTable7135[] =   { mode7135Set1, mode7135Set2, mode7135Set3, mode7135Set4,  mode7135Set5,  mode7135Set6,
		mode7135Set7, mode7135Set8, mode7135Set9, mode7135Set10, mode7135Set11, mode7135Set12};

// Index 0 value must be zero for OFF state (up to 8 modes max, including moonlight)
byte byFETModes[10];		// FET output
byte by7135Modes[10];	// 7135 output

volatile byte currOutLvl1;			// set to current: by7135modes[mode]
volatile byte currOutLvl2;			// set to current: byFETmodes[mode]

// Common Output Settings:
#define MAX_BRIGHTNESS 0,255
#define MAX_7135 255,0
//---------------------------------------------------------------------------------------------------
// Note: The MT03 w/std 7135 does not work well, so use the following 2 settings to use the FET only
//---------------------------------------------------------------------------------------------------
//#define BLINK_BRIGHTNESS 0,3	// output to use for blinks on battery check mode (use only the FET)
//#define CLICK_BRIGHTNESS 0,2	// button click blink brightness (use only the FET)
#define BLINK_BRIGHTNESS 40,0	// output to use for blinks on battery check mode (7135)
#define CLICK_BRIGHTNESS 20,0	// button click blink brightness (7135)
#define OFF_OUTPUT 0,0

//---------------------------------------------------------------------------------------
#elif OUT_CHANNELS == 3		// Triple Channel
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#elif OUT_CHANNELS == 1		// single FET or single bank of 7135's
//---------------------------------------------------------------------------------------
#endif

#endif /* MODESETS_H_ */