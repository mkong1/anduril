#ifndef config_bistro_H
#define config_bistro_H
/*
 *  Firmware configuration header. 
 *
 * Bistro configuration file for TA driver 2017, (C) Flintrock (FR).
 * USES voltage divider and OTC.
 * 
 * To work with tk-bistro and other compatible software.
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


/*
 * =========================================================================
 * Settings to modify per driver
 */

/////////////// Choose a layout////////////////

//#define FET_7135_LAYOUT  //FET+1
#define TRIPLEDOWN_LAYOUT  //TA triple
//#define NANJG105D_LAYOUT      // biscotti/convoy/nanjg105D
//#define BLFA6_LAYOUT      // BLFA6, FET+1, OTC and star3
//#define NANJG_LAYOUT  // specify an I/O pin layout
//#define QUADRUPLEDOWN_LAYOUT                     // Yes, that right :)
//#define CUSTOM_LAYOUT  // go define it for however you want to wire your driver.
//**** You can now easily customize layouts in fr-tk-attiny.h :

// choose the file that defines your modegruops (so we can easily keep more)
#define MODEGROUPS_H "modegroups/modegroups-TA-triple-v1.3plus.h"
//#define MODEGROUPS_H "modegroups/modegroups-biscotti.h"
//#define MODEGROUPS_H "modegroups/modegroups-BLFA6.h"

/*************************  Misc utility configs*************************************/
//#define VOLTAGE_CAL  // builds a driver that just blinks ADC reading, set the VOLTAGE mode below.
//#define STRIPPED // use for debugging, strips many features to make more space for debug compiler options or testing functions.
                   // customize it below
//#define OTSM_debug //may require STRIPPED to fit. //Will blink out number of wakes (see SLEEP_TIME_EXPONENT) on short clicks, before entering mode.

/************************************OFF-TIME CONFIGS**********************************/

#define OFFTIM3      // Use short/med/long off-time presses instead of just short/long


#define USE_OTC // use off time cap. 

// Off-time sleep mode by -FR, thanks to flashy Mike and Mike C for early feasibility test with similar methods.
//#define USE_OTSM  // USE OTSM.  Pin must be defined in the layout too.

//#define OTSM_USES_OTC // use OTC cap for extra power on OTSM (sets it output high to charge up)
#define POWERSAVE // Also works without OTSM to reduce moon-mode drain. 
             // Squeeze out a bit more off-time by saving power during
             // shutoff detection (so at all times).  Implements ms resolution (could be less) idle sleeps in place of delay.
             // Seems to add at 0.5s of sleep at 3.1V 30uF cap, starting with only 0.75 that matters.  
			 // But at higher voltages, it's still only 0.5s additional, not multiplicative.
             // Also adds about 12 bytes that could be used for about two more mode groups instead :P


// Traditionally Cap values defined in tk-calibration
// But wake times in OTSM are not a calibration, just a configuration, so they go here.
// Times are in decimal seconds.  short (med) wakes are <, not = to wake_time_short(med)
#define SLEEP_TIME_EXPONENT 4   // OTSM clicky sleep will be increments of 16ms*2^STE  
                               // so 0 is 16, 1 is 32, 2 is 64, 3 is 128, 4 is 0.25s, 5 is 0.5s etc.
// To allow long click times use 4. For 1/8s resolution, use 3.
#define wake_time_short 0.5   // 0.5s : Short press is up to 0.5 s
#define wake_time_med   1.25   // this is limited by cap performance, but setting it high won't hurt 
                              // Since anything beyond the cap capacity will be read as long anyway.
							  // It's also the long-press off-threshold for e-switch operation.

/*****************************END OTSM CONGIGS*********************************/

//#define USE_ESWITCH  // pin must be defined in the layout too.
//#define USE_ESWITCH_LOCKOUT_TOGGLE

/**********************VOLTAGE CONFIG****************************************/

#define VOLTAGE_MON         // Comment out to disable LVP
//You should leave one (but only one) of the next two uncommented.  
//#define READ_VOLTAGE_FROM_VCC  // inverted "internal" Vcc voltage monitoring
                               // Works well for 1S lights without worrying about resistor values.
#define READ_VOLTAGE_FROM_DIVIDER  // classic voltage reading
//#define REFERENCE_DIVIDER_READS_TO_VCC // default is 1.1V, but this is needed for divider reading with OTSM on the voltage pin.
                                         // This should normally be used with an LDO.  For 1S (non-LDO or 5.0VLDO) just avoid the problem with READ_VOLTAGE_FROM_VCC.

/***** Choose a battery indicator style (if enabled in modegroups)*******/
//#define BATTCHECK_4bars  // up to 4 blinks
//#define BATTCHECK_8bars  // up to 8 blinks
#define BATTCHECK_VpT  // Volts + tenths

/******thermal protection:  ***/
#define TEMPERATURE_MON          // You can set starting temperature in the "maxtemp" setting in config options first boot options.
  #define USE_TEMP_CAL    // include a TEMP_CAL mode in the menu.  
  #define TEMP_STEP_DOWN //Requires TEMPERATURE_MON, Use step-down and tap-up instead of oscillate
    #define MINIMUM_TURBO_TIME  10 //Turbo will never run less than this long. Requires TEMP_STEP_DOWN

/*******Mode features***********/
#define USE_MUGGLE_MODE  // compile in use of muggle mode
#define USE_REVERSE_MODES  // compile in use of reverse modes
#define USE_MOON  // compile in moon mode control

// Options for first bootup/default:

#define USE_FIRSTBOOT //Enables reset menu option, only costs a couple of bytes.

#define INIT_MODEGROUP      11       // which mode group will be default, mode groups below start at zero, select the mode group you want and subtract one from the number to get it by defualt here
#define INIT_ENABLE_MOON    1       // Should we add moon to the set of modes?
#define INIT_REVERSE_MODES  0       // flip the mode order?
#define INIT_MEMORY         0       // mode memory, or not
#define INIT_OFFTIM3        1       // enable medium-press by default?
#define INIT_MUGGLE_MODE    0       // simple mode designed for mugglesotsm
#define INIT_LOCKSWITCH     0       // 0 => E-swtich enabled, 1 => locked.
#define INIT_MAXTEMP       88       // maximum temperature

#define BLINK_SPEED         750




#ifdef STRIPPED  // define what you want to remove in stripped mode
#undef TEMPERATURE_MON
#undef VOLTAGE_MON
#define NO_STROBES  // don't use strobes (not automatically stripped from modes though, probably will produce BLINK_BRIGHTNESS)
#undef USE_BATTCHECK
#endif


/**********************************************************************************
**********************END OF CONFIGURATION*****************************************
***********************************************************************************/


#endif