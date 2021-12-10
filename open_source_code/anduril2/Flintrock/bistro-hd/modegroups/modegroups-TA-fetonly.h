#ifndef __MODEGROUPS_H
#define __MODEGROUPS_H
/*
* 
* These are the TA version 1.3 modes for bistro triple, but
* with ramps re-designed by FR specifically for FET only.
* and minor tweaks to the modegroups.

* Plus original 5-mode number 14 is added back in as mode 24, 
*  and mode 25, TA's party mode, and a 4 mode no turbo: (26)
*  4, ONE7135, 10 ALL7135, by FR but inspired by TA. (25)
*
* Original Copyright (C) 2015 Selene Scriven
* Much added by TA and Flintrock.
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



// ../../bin/level_calc.py 64 1 10 1300 y 3 0.23 140
// 20 ramp instead of 40 to save space for more mode groups
#define RAMP_SIZE  20

// ../../bin/level_calc.py 3 20 7135 3 0.25 140 7135 3 1.5 840 FET 1 10 3000
// (with some manual tweaks to exactly hit 1x7135 and Nx7135 in the middle, also adjustments to some lower modes to better space them)
#define ONE7135 8
//  This no longer really means all 7135's.  It's just a place-holder for a sensibly bright non-turbo mode.
#define ALL7135s 14
//PWM channels only used if defined here AND enabled in FR-tk-attiny.h

// Ramps designed specifically for 16A Q8  with ALL7135s corresponding to 5.6A, 
//    with curves bent to underlay the TA triple modegroups.  -FR
// from mode 3 through 8 current ratios are 1.664 per step, 1.587 from 8 to 14, and 1.191 from there to 20.
//  1 7135:
//#define RAMP_PWM2  3,10,20,33,55,92,153,255,255,255,255,255,255,255,255,255,255,255,255,0

// FET:
// In fet only the super low modes just don't work.  Moon (level 1) should just be disabled
//  2 is not used in TA modes,  and 3 and 4 are never used in the same modegroup.
//  So this actually works and keeps the same level spacing as TA triple ramps.
#define RAMP_PWM3 1, 1, 1, 1, 2, 3, 4, 6, 9, 14, 22, 35, 56, 89, 106, 127, 151, 180, 214, 255

// FET:
// Just for testing 4th channel, unused in triple layout:
//#define RAMP_PWM4   4,7,20,35,55,100,160,255,255,255,255,255,255,255,255,255,255,255,255,255

//for testing, with fet only
//#define RAMP_PWM2   4,7,20,35,45,65,85,105,115,135,145,165,185,205,215,225,235,245,250,255


//#define TURBO     RAMP_SIZE      // Convenience code for turbo mode
#define TURBO     20      // Convenience code for turbo mode

#define BLINK_BRIGHTNESS    RAMP_SIZE/4

// Set this to cool running but moderately bright mode  to step down to after overheating or turbo timeout
//#define TURBO_STEPDOWN      RAMP_SIZE/2 

#define TURBO_STEPDOWN      ONE7135

//** Enable "strobes" ****//
#define USE_BATTCHECK             // use battery check mode
#define USE_BIKING_STROBE         // Single flash biking strobe mode
#define USE_FULL_BIKING_STROBE     // Stutter bike strobe, uncomment to enable
//#define USE_POLICE_STROBE         // Dual mode alternating strobe
//#define USE_RANDOM_STROBE
//#define USE_SOS
//#define USE_STROBE_8HZ
#define USE_STROBE_10HZ
#define USE_STROBE_16HZ
#define USE_STROBE_OLD_MOVIE
#define USE_STROBE_CREEPY    // Creepy strobe mode, or really cool if you have a bunch of friends around
//#define USE_RAMP               //Ramping "strobe"


// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,TURBO .

#ifndef HIDDENMODES // This will already be set as empty if no reverse clicks are enabled.
  #define HIDDENMODES         STROBE_CREEPY,STROBE_OLD_MOVIE,STROBE_16HZ,STROBE_10HZ,BIKING_STROBE,BATTCHECK,TURBO
//#define HIDDENMODES         BATTCHECK,RAMP,TURBO
#endif

PROGMEM const uint8_t hiddenmodes[] = { HIDDENMODES };

// Define maximum modes per group
// Increasing this is not expensive, essentially free in program size.
// The memory usage depends only on total of actual used modes (plus the single trailing zero).
#define MAX_MODES 9 // includes moon mode, defined modes per group should be one less. The 0 terminator doesn't count though.
// Each group must end in one and ONLY one zero, and can include no other 0.

#define NUM_MODEGROUPS 26  // don't count muggle mode
// mode groups are now zero terminated rather than fixed length.  Presently 8 modes max but this can change now.
// going too near the fast press limit could be annoying though as a full cycle requires pausing to avoid the menu.
// Mode numbers correspond to ramp positions above, first position is "0" and is reserved for moon

//ALL7135s here just means a hopefully-sustainable but bright mode (depends on the light of course).
PROGMEM const uint8_t modegroups[] = {
	TURBO,  0,                                              // 1: 1 mode group
	ONE7135, TURBO,  0,                                     // 2: 2 mode Group
	ONE7135, ALL7135s, TURBO,  0,                           // 3: You should get the idea by now....
	4,  ONE7135,  ALL7135s,  TURBO,  0,                     // 4: 4 mode group that has turbo in the main modes instead of just in the hidden modes.
	3,  5,  ONE7135,  ALL7135s,  TURBO,  0,                 // 5: Wild guess on # of modes?
	4,  ONE7135,  11,  ALL7135s,  17, TURBO,  0,            // 6: If you guessed right before, you should be able to figure this one out
	3,  5,  ONE7135,  11,  ALL7135s,  16,  TURBO,  0,       // 7: 7 modes for triple builds, the high modes are too close together on single emitter builds
	3,  5,  ONE7135,  11,  ALL7135s,  16,  18,  TURBO, 0,   // 8: 8 modes for triple builds, the high modes are too close together on single emitter builds
	ONE7135,  0,                                            // 9: Basic Non-PWM ~150 lumen low regulated mode
	ALL7135s,  0,                                           // 10: Basic non-PWM ~800 lumen regulated mode
	ONE7135,  ALL7135s,  0,                                 // 11: Only non-PWM modes
	4,  ONE7135,  ALL7135s,  0,                             // 12: Defualt mode group 3 mode regulated, Recomended mode group for single emitter lights, you can accsess turbo via the hidden modes
	3,  5,  ONE7135,  ALL7135s,  0,                         // 13: 4 mode regulated
	4,  ONE7135,  ALL7135s,  17, 0,                         // 14: I kinda think you get it by now
	ONE7135,  ALL7135s,  17, 0,                             // 15: I kinda think you get it by now
	BIKING_STROBE,BATTCHECK,4,ONE7135,ALL7135s,16,18,TURBO, 0,      // 16: Biking group
	ONE7135,  10,  12,  ALL7135s,  15,  16,  18,  TURBO,  0,        // 17: Lots of steady modes biking group for fine tuning heat mangagement. Adjust number of 7135's for finer current control.
	ONE7135, ALL7135s, STROBE_16HZ, TURBO, 0,               // 18: Tactical group
	4,  5,  6,  7,  0,                                      // 19: Super sneaker low modes, .5 - 24 lumens
	4,  TURBO,  0,                                          // 20: Security guard speical, highly requested mode group for guards
	4,  ALL7135s,  TURBO,  0,                               // 21: 3 mode group setup without a 1x 7135 mode, jumps from low ~13 lumens, to the bank of 7135's. It is recomended you only install 2-4 7135's on the bank so that the mode is a good mid range between low and turbo.
	4,  ONE7135,  16,  TURBO,  0,                           // 22: 4 mode group setup for using the driver as a normal FET+1 with no 7135's on the bottom
	4,  ONE7135,  15,  17,  TURBO,  0,                      // 23: 5 mode group setup for using the driver as a normal FET+1 with no 7135's on the bottom
	3,  5, ONE7135,  10,  ALL7135s, 0,                      // 24: original mode 14.
	4, ONE7135, 11, ALL7135s, 0,                             // 25: Added by Flintrock in HD version. 
	STROBE_10HZ,STROBE_16HZ,STROBE_OLD_MOVIE,STROBE_CREEPY,TURBO,ALL7135s,ONE7135,4, 0,   // 26: Disco party mode! Lots o fun for the whole party with all sorts of strobe modes to play with!
	//2,  5, ONE7135,  11,  ALL7135s,  16,  18,  TURBO, 0,    // : Custom mode group so the rest of the groups can stay the same for easy referance later
	4, ONE7135, ALL7135s,  0                               // muggle mode, 
};




#endif

// some old commented out stuff:
// log curve
//#define RAMP_PWM2  3,3,3,3,3,3,4,4,4,4,4,5,5,5,6,6,7,7,8,9,10,11,12,13,15,16,18,21,23,27,30,34,39,44,50,57,65,74,85,97,111,127,145,166,190,217,248,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM3   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,6,11,17,23,30,39,48,59,72,86,103,121,143,168,197,255
// x**2 curve
//#define RAMP_PWM2  3,5,8,12,17,24,32,41,51,63,75,90,105,121,139,158,178,200,223,247,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM3   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,4,6,9,12,16,19,22,26,30,33,37,41,45,50,54,59,63,68,73,78,84,89,94,100,106,111,117,123,130,136,142,149,156,162,169,176,184,191,198,206,214,221,255
// x**3 curve
// ../../bin/level_calc.py 3 40 7135 3 0.25 140 7135 3 1.5 840 FET 1 10 3000
// (with some manual tweaks to exactly hit 1x7135 and Nx7135 in the middle)
//#define ONE7135 14
//#define ALL7135s 27
//#define RAMP_PWM2  3,4,7,11,18,25,35,55,75,100,133,169,211,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM1 0,0,0,0,0,0,0,0,0,0,0,0,0,0,11,22,35,49,64,82,101,122,144,169,195,224,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM3   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,24,39,55,73,91,111,132,154,177,202,228,255
//X**3 curve


// OLD commented stuff that might be useful:
// testing only, separate channels:
//#define RAMP_PWM2   4,7,20,35,55,100,160,0,0,0,0,0,0,0,0,0,0,0,0,0  // for testing individual channels
//#define RAMP_PWM1   0,0,0,0,0,0,0,0,35,64,95,122,180,0,0,0,0,0,0,0 // for testing individual channels
//#define RAMP_PWM3   0,0,0,0,0,0,0,0,0,0,0,0,0,0,35,69,108,152,201,255
// testing only: First 3 modes show each channel individually
//#define RAMP_PWM2  255,0,0,255,18,27,40,57,77,103,133,169,211,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM1 0,255,0,255,0,0,0,0,0,0,0,0,0,0,11,22,35,49,64,82,101,122,144,169,195,224,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM3   0,0,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,24,39,55,73,91,111,132,154,177,202,228,255
// 1200-lm single LED: ../../bin/level_calc.py 3 40 7135 3 0.25 160 7135 3 1.5 760 FET 1 3 1200
//#define RAMP_PWM2   3,4,5,7,10,14,19,25,33,43,54,67,83,101,121,144,170,198,230,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM1  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,18,28,40,52,65,78,92,107,124,143,163,184,207,230,255,255,255,255,0
//#define RAMP_PWM3    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,33,103,177,255
//#define ONE7135 20
//#define ALL7135s 36
// x**5 curve
//#define RAMP_PWM2  3,3,3,4,4,5,5,6,7,8,10,11,13,15,18,21,24,28,33,38,44,50,57,66,75,85,96,108,122,137,154,172,192,213,237,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//#define RAMP_PWM3   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,3,6,9,13,17,21,25,30,35,41,47,53,60,67,75,83,91,101,111,121,132,144,156,169,183,198,213,255

// uncomment to ramp up/down to a mode instead of jumping directly
// #define SOFT_START          // Cause a flash during mode change with some drivers


// output to use for blinks on battery check (and other modes)

// default values calculated by group_calc.py
// ONE7135 = 8, ALL7135s = 14, TURBO = 20
// Modes approx mA and lumen values for single XP-L LEDe
// 1 = 5.5ma = .5lm, 3 = 19ma = 4lm, 4 = 37mA = 12lm, 5 = 66ma = 24lm,
// 6 = 135ma = ~60lm, 8 (one7135) = 355ma = ~150lm
// Above this it will depend on how many 7135's are installed
// With 7x 7135 10 = 640ma, 12 = 1A = ~335lm, 14 (ALL7135s) = 2.55A = ~830lm
// Above this you are using the FET and it will vary wildy depending on the build, generally you only need turbo after ALL7135s unless it is a triple build

