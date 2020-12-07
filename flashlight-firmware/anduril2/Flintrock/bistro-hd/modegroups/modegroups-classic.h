#ifndef __MODEGROUPS_H
#define __MODEGROUPS_H
/*
* Define modegroups for bistro classic
*
* Copyright (C) 2015 Selene Scriven
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
#define RAMP_SIZE  64
#define TURBO     RAMP_SIZE      // Convenience code for turbo mode

// 7135
#define ONE7135 29
#define RAMP_PWM2  3,3,4,5,6,8,10,12,15,19,23,28,33,40,47,55,63,73,84,95,108,122,137,153,171,190,210,232,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
//FET
#define RAMP_PWM1   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,5,8,11,14,18,22,26,30,34,39,44,49,54,59,65,71,77,84,91,98,105,113,121,129,137,146,155,164,174,184,194,205,216,255


// uncomment to ramp up/down to a mode instead of jumping directly
// #define SOFT_START          // Cause a flash during mode change with some drivers


// output to use for blinks on battery check (and other modes)
#define BLINK_BRIGHTNESS    17
// ms per normal-speed blink


//** Enable "strobes" ****//
#define USE_BATTCHECK             // use battery check mode
#define USE_BIKING_STROBE         // Single flash biking strobe mode
  #define USE_FULL_BIKING_STROBE     // stutter enhancement to biking strobe, requires USE_BIKING_STROBE
//#define USE_FULL_BIKING_STROBE     // Stutter bike strobe, uncomment to enable
#define USE_POLICE_STROBE         // Dual mode alternating strobe
//#define USE_RANDOM_STROBE
//#define USE_SOS
//#define USE_STROBE_8HZ
//#define USE_STROBE_10HZ
//#define USE_STROBE_16HZ
//#define USE_STROBE_OLD_MOVIE
//#define USE_STROBE_CREEPY    // Creepy strobe mode, or really cool if you have a bunch of friends around
//#define USE_RAMP               //Ramping "strobe"


// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,TURBO .

#ifndef HIDDENMODES // This will already be set as empty if no reverse clicks are enabled.
#define HIDDENMODES         BIKING_STROBE,BATTCHECK,POLICE_STROBE,TURBO
//#define HIDDENMODES         BATTCHECK,RAMP,TURBO
#endif

PROGMEM const uint8_t hiddenmodes[] = { HIDDENMODES };

#define NUM_MODEGROUPS 9  // don't count muggle mode
// mode groups are now zero terminated rather than fixed length.  Presently 8 modes max but this can change now.
// going too near the fast press limit could be annoying though as a full cycle requires pausing to avoid the menu.
//
// Define maximum modes per group
// Increasing this is not expensive, essentially free in program size.
// The memory usage depends only on total of actual used modes (plus the single trailing zero).
#define MAX_MODES 9 // includes moon mode, defined modes per group should be one less. The 0 terminator doesn't count though.
// Each group must end in one and ONLY one zero, and can include no other 0.
PROGMEM const uint8_t modegroups[] = {
	64,  0,
	11, 64,  0,
	11, 35, 64,  0,
	11, 26, 46, 64,  0,
	11, 23, 36, 50, 64,  0,
	11, 20, 31, 41, 53, 64,  0,
	29, 64,POLICE_STROBE,0,  // 7: special group A
	BIKING_STROBE,BATTCHECK,11,29,64,0,  // 8: special group B
	9, 18, 29, 46, 64,  0,  // 9: special group C
	11, 29, 50,  0                  // muggle mode
};

#endif