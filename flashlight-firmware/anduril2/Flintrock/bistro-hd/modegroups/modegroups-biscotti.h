#ifndef __MODEGROUPS_H
#define __MODEGROUPS_H
/*
* Define modegroups for bistro
*
* Copyright (C) 2015 Selene Scriven
* Much work added by Texas Ace, and some minor format change by Flintrock.
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
#define RAMP_SIZE  7
#define TURBO     RAMP_SIZE      // Convenience code for turbo mode

#define ONE7135 8
#define ALL7135s 14

//PWM channels only used if defined here AND enabled in FR-tk-attiny25.h

// FET
#define RAMP_PWM2   1,10,42,75,122,133,255


// uncomment to ramp up/down to a mode instead of jumping directly
// #define SOFT_START          // Cause a flash during mode change with some drivers


// output to use for blinks on battery check (and other modes)
#define BLINK_BRIGHTNESS    RAMP_SIZE/4
// ms per normal-speed blink

//** Enable "strobes" ****//
#define USE_BATTCHECK             // use battery check mode
#define USE_BIKING_STROBE         // Single flash biking strobe mode
//  #define USE_FULL_BIKING_STROBE     // stutter enhancement to biking strobe, requires USE_BIKING_STROBE
//#define USE_POLICE_STROBE         // Dual mode alternating strobe
//#define USE_RANDOM_STROBE
//#define USE_SOS
//#define USE_STROBE_8HZ
#define USE_STROBE_10HZ
//#define USE_STROBE_16HZ
//#define USE_STROBE_OLD_MOVIE
//#define USE_STROBE_CREEPY    // Creepy strobe mode, or really cool if you have a bunch of friends around
//#define USE_RAMP               //Ramping "strobe"


// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,TURBO .

#ifndef HIDDENMODES // This will already be set as empty if no reverse clicks are enabled.
#define HIDDENMODES         BIKING_STROBE,BATTCHECK,STROBE_10HZ,TURBO
//#define HIDDENMODES         BATTCHECK,RAMP,TURBO
#endif

PROGMEM const uint8_t hiddenmodes[] = { HIDDENMODES };

// default values calculated by group_calc.py
// ONE7135 = 8, ALL7135s = 14, TURBO = 20
// Modes approx mA and lumen values for single XP-L LEDe
// 1 = 5.5ma = .5lm, 3 = 19ma = 4lm, 4 = 37mA = 12lm, 5 = 66ma = 24lm,
// 6 = 135ma = ~60lm, 8 (one7135) = 355ma = ~150lm
// Above this it will depend on how many 7135's are installed
// With 7x 7135 10 = 640ma, 12 = 1A = ~335lm, 14 (ALL7135s) = 2.55A = ~830lm
// Above this you are using the FET and it will vary wildy depending on the build, generally you only need turbo after ALL7135s unless it is a triple build
#define NUM_MODEGROUPS 2  // don't count muggle mode
// mode groups are now zero terminated rather than fixed length.  Presently 8 modes max but this can change now.
// going too near the fast press limit could be annoying though as a full cycle requires pausing to avoid the menu.
//
// Define maximum modes per group
// Increasing this is not expensive, essentially free in program size.
// The memory usage depends only on total of actual used modes (plus the single trailing zero).
#define MAX_MODES 9 // includes moon mode, defined modes per group should be one less. The 0 terminator doesn't count though.
// Each group must end in one and ONLY one zero, and can include no other 0.
PROGMEM const uint8_t modegroups[] = {
	1,  2,  3,  5,  7,  STROBE_10HZ, BIKING_STROBE, BATTCHECK,0,
	1,  2,  3,  5,  7, 0,
	7,  5,  3,  2,  1,  0,
	2,  4,  7,  STROBE_10HZ, BIKING_STROBE, BATTCHECK, SOS,  0,
	2,  4,  7,  0, 
	7,  4,  2,  0, 
	1,  2,  3,  6,  STROBE_10HZ, BIKING_STROBE, BATTCHECK, SOS,0,
	1,  2,  3,  6,  0, 
	6,  3,  2,  1,  0, 
	2,  3,  5,  7,  0,  
	7,  4, STROBE_10HZ,  0,
	7,  0,
};

#endif