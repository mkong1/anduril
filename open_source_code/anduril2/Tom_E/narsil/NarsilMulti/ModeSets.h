/****************************************************************************************
 * ModeSets.h - sets of fixed mode levels
 * ==========
 *
 ****************************************************************************************/

#ifndef MODESETS_H_
#define MODESETS_H_

//---------------------------------------------------------------------------------------
#if   OUT_CHANNELS == 2			// FET+1 or Buck driver
//---------------------------------------------------------------------------------------

#ifdef GT_BUCK
//---------------------------------------------------------------------
//  Buck driver 2 channel modes
//---------------------------------------------------------------------

// 2.0 A max modes
// 1 mode                               2.0A
PROGMEM const byte modeFetSet1[] =  {   201};	// for analog control
PROGMEM const byte mode7135Set1[] = {   255};	// for LM3409 enable/PWM

// 2 modes /4                            25%   2.0A
PROGMEM const byte modeFetSet2[] =  {    42,   201};	// for analog control
PROGMEM const byte mode7135Set2[] = {   255,   255};	// for LM3409 enable/PWM

// 3 modes /3			                 11%    33%   2.0A
PROGMEM const byte modeFetSet3[] =  {    26,    53,   201};	// for analog control
PROGMEM const byte mode7135Set3[] = {   255,   255,   255};	// for LM3409 enable/PWM

// 4 modes /2		                   12.5%    25%    50%    2.0A
PROGMEM const byte modeFetSet4[] =  {    27,    42,    77,    201};	// for analog control
PROGMEM const byte mode7135Set4[] = {   255,   255,   255,    255};	// for LM3409 enable/PWM

// 5 modes cube-oot                       1%     7%    22%    52%   2.0A
PROGMEM const byte modeFetSet5[] =  {    25,    25,    39,    80,   201};	// for analog control
PROGMEM const byte mode7135Set5[] = {    24,   170,   255,   255,   255};	// for LM3409 enable/PWM

// 6 modes cube-root                      1%     5%    15%    32%    60%   2.0A
PROGMEM const byte modeFetSet6[] =  {    25,    25,    30,    51,    94,   201};	// for analog control
PROGMEM const byte mode7135Set6[] = {    24,   126,   255,   255,   255,   255};	// for LM3409 enable/PWM

// 2.5 A max modes
// 1 mode                               2.5A
PROGMEM const byte modeFetSet7[] =  {   255};	// for analog control
PROGMEM const byte mode7135Set7[] = {   255};	// for LM3409 enable/PWM

// 2 modes /4                            25%   2.5A
PROGMEM const byte modeFetSet8[] =  {    45,   255};	// for analog control
PROGMEM const byte mode7135Set8[] = {   255,   255};	// for LM3409 enable/PWM

// 3 modes /3			                 11%    33%   2.5A
PROGMEM const byte modeFetSet9[] =  {    27,    58,   255};	// for analog control
PROGMEM const byte mode7135Set9[] = {   255,   255,   255};	// for LM3409 enable/PWM

// 4 modes /2		                   12.5%    25%    50%    2.5A
PROGMEM const byte modeFetSet10[] =  {    29,    45,    86,    255};	// for analog control
PROGMEM const byte mode7135Set10[] = {   255,   255,   255,    255};	// for LM3409 enable/PWM

// 5 modes cube-oot                       1%     7%    22%    52%   2.5A
PROGMEM const byte modeFetSet11[] =  {    25,    25,    41,    90,   255};	// for analog control
PROGMEM const byte mode7135Set11[] = {    26,   185,   255,   255,   255};	// for LM3409 enable/PWM

// 6 modes cube-root                      1%     5%    15%    32%    60%   2.5A
PROGMEM const byte modeFetSet12[] =  {    25,    25,    31,    56,   106,   255};	// for analog control
PROGMEM const byte mode7135Set12[] = {    26,   136,   255,   255,   255,   255};	// for LM3409 enable/PWM

#else
//---------------------------------------------------------------------
//  Standard FET+1 2 channel modes
//---------------------------------------------------------------------

// 1 mode (max)                         max
PROGMEM const byte modeFetSet1[] =  {   255};
PROGMEM const byte mode7135Set1[] = {     0};

// 2 modes (10-max)                     ~10%   max
PROGMEM const byte modeFetSet2[] =  {     0,   255};
PROGMEM const byte mode7135Set2[] = {   255,     0};

// 3 modes (5-35-max)                    ~5%   ~35%   max
PROGMEM const byte modeFetSet3[] =  {     0,    70,   255 };	// Must be low to high
PROGMEM const byte mode7135Set3[] = {   120,   255,     0 };	// for secondary (7135) output

// ************************************************************
//             ***  TEST ONLY  ***
// ************************************************************
//PROGMEM const byte modeFetSet3[] =  {     3,  64,  128,   255 };	// Must be low to high
//PROGMEM const byte mode7135Set3[] = {     0,   0,   0,     0 };	// for secondary (7135) output
// ************************************************************

// 4 modes (2-10-40-max)                1-2%   ~10%   ~40%   max
PROGMEM const byte modeFetSet4[] =  {     0,     0,    80,   255 };	// Must be low to high
PROGMEM const byte mode7135Set4[] = {    30,   255,   255,     0 };	// for secondary (7135) output

// 5 modes (2-10-40-max)                1-2%    ~5%   ~10%   ~40%   max
PROGMEM const byte modeFetSet5[] =  {     0,     0,     0,    80,   255 };	// Must be low to high
PROGMEM const byte mode7135Set5[] = {    30,   120,   255,   255,     0 };	// for secondary (7135) output

// 6 modes - copy of BLF A6 7 mode set
PROGMEM const byte modeFetSet6[] =  {     0,     0,     7,    56,   137,   255};
PROGMEM const byte mode7135Set6[] = {    20,   110,   255,   255,   255,     0};

// 7 modes (1-2.5-6-10-35-65-max)        ~1%  ~2.5%    ~6%   ~10%   ~35%   ~65%   max
PROGMEM const byte modeFetSet7[] =  {     0,     0,     0,     0,    70,   140,   255 };	// Must be low to high
PROGMEM const byte mode7135Set7[] = {    24,    63,   150,   255,   255,   255,     0 };	// for secondary (7135) output

// #8:  3 modes (10-25-50)              ~10%   ~25%   ~50%
PROGMEM const byte modeFetSet8[] =  {     0,    37,   110 };	// Must be low to high
PROGMEM const byte mode7135Set8[] = {   255,   255,   255 };	// for secondary (7135) output

// #9:   3 modes (2-20-max)              ~2%   ~20%   max
PROGMEM const byte modeFetSet9[] =  {     0,    25,   255 };	// Must be low to high
PROGMEM const byte mode7135Set9[] = {    30,   255,     0 };	// for secondary (7135) output

// #10:  3 modes (2-40-max)              ~2%   ~40%   max
PROGMEM const byte modeFetSet10[] =  {     0,    80,   255 };	// Must be low to high
PROGMEM const byte mode7135Set10[] = {    30,   255,     0 };	// for secondary (7135) output

// #11:  3 modes (10-35-max)             ~10%   ~35%   max
PROGMEM const byte modeFetSet11[] =  {     0,    70,   255 };	// Must be low to high
PROGMEM const byte mode7135Set11[] = {   255,   255,     0 };	// for secondary (7135) output

// #12:  4 modes - copy of BLF A6 4 mode
PROGMEM const byte modeFetSet12[] =  {     0,     0,    90,   255};
PROGMEM const byte mode7135Set12[] = {    20,   230,   255,     0};
#endif

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
#ifdef GT_BUCK
 #define MAX_BRIGHTNESS 255,255	// for GT-buck
 #define MAX_7135 255,25				// for GT-buck, about 117 mA
 #define BLINK_BRIGHTNESS 100,25	// for GT-buck, about  50 mA
 #define CLICK_BRIGHTNESS 50,25	// for GT-buck, about  25 mA
 #define OFF_OUTPUT 0,0
#else
// output to use for blinks on battery check mode
// Use 20,0 for a single-channel driver or 40,0 for a two-channel driver
 #define MAX_BRIGHTNESS 0,255
 #define MAX_7135 255,0
 #define BLINK_BRIGHTNESS 40,0
 #define CLICK_BRIGHTNESS 20,0
 #define OFF_OUTPUT 0,0
#endif


//---------------------------------------------------------------------------------------
#elif OUT_CHANNELS == 3		// Triple Channel
//---------------------------------------------------------------------------------------

//------------------- MODE SETS (must be low to high) --------------------------

// 1 mode (max)                         max
PROGMEM const byte mode7135Set1[] = {     0};		// for single 7135
PROGMEM const byte mode7135sSet1[] ={     0};		// for 7135 bank
PROGMEM const byte modeFetSet1[] =	{   255};		// FET only

// 2 modes (7135-FET)                   ~10%   max
PROGMEM const byte mode7135Set2[] = {   255,     0};
PROGMEM const byte mode7135sSet2[] ={     0,     0};
PROGMEM const byte modeFetSet2[] =  {     0,   255};

// 3 modes (7135-7135s-max)             ~10%   ~50%   max
PROGMEM const byte mode7135Set3[] = {   255,     0,     0};
PROGMEM const byte mode7135sSet3[] ={     0,   255,     0};
PROGMEM const byte modeFetSet3[] =  {     0,     0,   255};

// 4 modes (1.2-10-50-max)             ~1.2%   ~10%   ~50%   max
PROGMEM const byte mode7135Set4[] = {    30,   255,     0,     0};
PROGMEM const byte mode7135sSet4[] ={     0,     0,   255,     0};
PROGMEM const byte modeFetSet4[] =  {     0,     0,     0,   255};

// 5 modes (1.2-5-10-50-max)           ~1.2%    ~5%   ~10%   ~50%   max
PROGMEM const byte mode7135Set5[] = {    30,   120,   255,     0,     0};
PROGMEM const byte mode7135sSet5[] ={     0,     0,     0,   255,     0};
PROGMEM const byte modeFetSet5[] =  {     0,     0,     0,     0,   255};

// 6 modes 0.8-2-5-10-50-max           ~0.8%    ~2%    ~5%   ~10%   ~50%   max
PROGMEM const byte mode7135Set6[] = {    20,   110,   150,   255,     0,     0};
PROGMEM const byte mode7135sSet6[] ={     0,     0,     0,     0,   255,     0};
PROGMEM const byte modeFetSet6[] =  {     0,     0,     0,     0,     0,   255};

// 7 modes (0.5-2.5-5-10-25-50-max)    ~0.5%  ~2.5%    ~5%   ~10%   ~25%   ~50%   max
PROGMEM const byte mode7135Set7[] = {    12,    63,   150,   255,   255,     0,     0};
PROGMEM const byte mode7135sSet7[] ={     0,     0,     0,     0,   120,   255,     0};
PROGMEM const byte modeFetSet7[] =  {     0,     0,     0,     0,     0,     0,   255};

// 2 modes (all 7135s-FET)              ~60%   max
PROGMEM const byte mode7135Set8[] = {   255,     0};
PROGMEM const byte mode7135sSet8[] ={   255,     0};
PROGMEM const byte modeFetSet8[] =  {     0,   255};
	
PROGMEM const byte modeSetCnts[] = {
		sizeof(modeFetSet1), sizeof(modeFetSet2), sizeof(modeFetSet3), sizeof(modeFetSet4), sizeof(modeFetSet5), sizeof(modeFetSet6),
		sizeof(modeFetSet7),	sizeof(modeFetSet8)};

const byte *modeTable7135[] =   { mode7135Set1, mode7135Set2, mode7135Set3, mode7135Set4,
		mode7135Set5, mode7135Set6, mode7135Set7, mode7135Set8};
const byte *modeTable7135s[] =  { mode7135sSet1, mode7135sSet2, mode7135sSet3, mode7135sSet4,
		mode7135sSet5, mode7135sSet6, mode7135sSet7, mode7135sSet8};
const byte *modeTableFet[] =    { modeFetSet1, modeFetSet2, modeFetSet3, modeFetSet4,
		modeFetSet5, modeFetSet6, modeFetSet7, modeFetSet8};

// Index 0 value must be zero for OFF state (up to 8 modes max, including moonlight)
byte by7135Modes[10];	// one 7135
byte by7135sModes[10];	// bank of 7135s
byte byFETModes[10];		// FET

volatile byte currOutLvl1;			// set to current: by7135Modes[mode]
volatile byte currOutLvl2;			// set to current: by7135sModes[mode]
volatile byte currOutLvl3;			// set to current: byFetModes[mode]

// Common Output Settings:
#define MAX_BRIGHTNESS 0,0,255
#define MAX_7135 255,0,0
#define BLINK_BRIGHTNESS 50,0,0	// output to use for blinks on battery check mode, use about 20% of single 7135
#define CLICK_BRIGHTNESS 20,0,0	// use about 8% of single 7135
#define OFF_OUTPUT 0,0,0

//---------------------------------------------------------------------------------------
#elif OUT_CHANNELS == 1		// single FET or single bank of 7135's
//---------------------------------------------------------------------------------------

// 1 mode (max)                       max
PROGMEM const byte modeSet1[] =  {   255};

// 2 modes (10-max)                   10%   max
PROGMEM const byte modeSet2[] =  {    25,   255};

// 3 modes (5-35-max)                   5%    35%   max
PROGMEM const byte modeSet3[] =  {    13,    89,   255 };	// Must be low to high

// 4 modes (3-10-40-max)               3%    10%    40%   max
PROGMEM const byte modeSet4[] =  {     8,    25,   102,   255 };	// Must be low to high

// 5 modes (3-5-10-40-max)             3%     5%    10%    40%   max
PROGMEM const byte modeSet5[] =  {     8,    13,    25,   102,   255 };	// Must be low to high

// 6 modes (3-7-15-32-55-max)          3%     7%    15%    32%   55%   max 
PROGMEM const byte modeSet6[] =  {     8,    18,    38,    56,   137,   255};

// 7 modes (3-5-10-18-40-65-max)       3%     5%    10%   18%    40%    65%   max
PROGMEM const byte modeSet7[] =  {     8,    13,    25,   46,   102,   166,   255 };	// Must be low to high

// #8:  3 modes (10-25-50)            10%    25%    50%
PROGMEM const byte modeSet8[] =  {    25,    64,   128 };	// Must be low to high

// #9:   3 modes (3-20-max)            3%    20%    max
PROGMEM const byte modeSet9[] =  {     8,    51,    255 };	// Must be low to high

// #10:  3 modes (3-40-max)            3%    40%    max
PROGMEM const byte modeSet10[] =  {    8,   102,    255 };	// Must be low to high

// #11:  3 modes (10-35-max)          10%    35%   max
PROGMEM const byte modeSet11[] =  {   25,    89,   255 };	// Must be low to high

// #12:  4 modes (5-15-55-max)         5%    15%   55%   max
PROGMEM const byte modeSet12[] =  {   13,    38,  137,   255};


PROGMEM const byte modeSetCnts[] = {
	sizeof(modeSet1), sizeof(modeSet2), sizeof(modeSet3), sizeof(modeSet4), sizeof(modeSet5), sizeof(modeSet6),
   sizeof(modeSet7), sizeof(modeSet8),	sizeof(modeSet9), sizeof(modeSet10), sizeof(modeSet11), sizeof(modeSet12)};

const byte *(modeTable1Chan[]) =  {
	modeSet1, modeSet2, modeSet3, modeSet4,  modeSet5,  modeSet6,
	modeSet7, modeSet8, modeSet9, modeSet10, modeSet11, modeSet12};

// Index 0 value must be zero for OFF state (up to 8 modes max, including moonlight)
byte byMainModes[10];			// main LED output
#define byFETModes byMainModes	// map references for FET modes to the single one channel modes

volatile byte currOutLvl;		// set to current: byMainModes[mode]

// Common Output Settings:
#define MAX_BRIGHTNESS 255
#define MAX_7135 25				// about 10%
#define BLINK_BRIGHTNESS 20	// output to use for blinks on battery check mode, about 7.8%
#define CLICK_BRIGHTNESS 15	// about 6%
#define OFF_OUTPUT 0

#endif


#endif /* MODESETS_H_ */