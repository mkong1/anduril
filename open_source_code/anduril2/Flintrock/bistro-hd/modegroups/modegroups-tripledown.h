#ifndef __MODEGROUPS_H
#define __MODEGROUPS_H
/*
* Define modegroups for bistro triple down
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
#define RAMP_SIZE  40
#define TURBO     RAMP_SIZE      // Convenience code for turbo mode

// 7135
#define RAMP_PWM2   3,4,5,7,10,14,19,25,33,43,54,67,83,101,121,144,170,198,230,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0
#define RAMP_PWM1  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,18,28,40,52,65,78,92,107,124,143,163,184,207,230,255,255,255,255,0
#define RAMP_PWM3    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,33,103,177,255
#define ONE7135 20
#define ALL7135s 36

// uncomment to ramp up/down to a mode instead of jumping directly
// #define SOFT_START          // Cause a flash during mode change with some drivers


// output to use for blinks on battery check (and other modes)
#define BLINK_BRIGHTNESS    RAMP_SIZE/4
// ms per normal-speed blink

// Hidden modes are *before* the lowest (moon) mode, and should be specified
// in reverse order.  So, to go backward from moon to turbo to strobe to
// battcheck, use BATTCHECK,STROBE,TURBO .


//** Enable "strobes" ****//
#define USE_BATTCHECK             // use battery check mode
//#define USE_BIKING_STROBE         // Single flash biking strobe mode
//#define USE_FULL_BIKING_STROBE     // Stutter bike strobe, uncomment to enable
//#define USE_POLICE_STROBE         // Dual mode alternating strobe
//#define USE_RANDOM_STROBE
//#define USE_SOS
//#define USE_STROBE_8HZ
//#define USE_STROBE_10HZ
//#define USE_STROBE_16HZ
//#define USE_STROBE_OLD_MOVIE
//#define USE_STROBE_CREEPY    // Creepy strobe mode, or really cool if you have a bunch of friends around
#define USE_RAMP               //Ramping "strobe"


#ifndef HIDDENMODES // This will already be set as empty if no reverse clicks are enabled.
#define HIDDENMODES         BATTCHECK,RAMP,TURBO

//#define HIDDENMODES         BATTCHECK,RAMP,TURBO
#endif

PROGMEM const uint8_t hiddenmodes[] = { HIDDENMODES };


// mode groups are now zero terminated rather than fixed length.  Presently 8 modes max but this can change now.
// going too near the fast press limit could be annoying though as a full cycle requires pausing to avoid the menu.
//



#define NUM_MODEGROUPS 9  // don't count muggle mode
// Define maximum modes per group
// Increasing this is not expensive, essentially free in program size.
// The memory usage depends only on total of actual used modes (plus the single trailing zero).
#define MAX_MODES 9 // includes moon mode, defined modes per group should be one less. The 0 terminator doesn't count though.
// Each group must end in one and ONLY one zero, and can include no other 0.
PROGMEM const uint8_t modegroups[] = {
	//1,  2,  3,  BATTCHECK,  RAMP,  0,  0,  0,
	40,  0,
	6, 40,  0,
	6, 23, 40,  0,
	6, 17, 28, 40,  0,
	6, 14, 23, 31, 40,  0,
	6, 12, 19, 26, 33, 40,  0,  
	ONE7135, TURBO, RAMP,0,  // 7: special group A
	RAMP,BATTCHECK,6,ONE7135,TURBO,0,  // 8: special group B
	RAMP, 6, 12, 19, 29, 40,  0,  // 9: special group C
	6, 19, 32,  0                  // muggle mode
};
#endif