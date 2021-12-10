#ifndef FR_CALIBRATION_H
#define FR_CALIBRATION_H
/*
 * Attiny calibration header.
 * This allows using a single set of hardcoded values across multiple projects.
 *
 * 2017 written by Flintrock (C)
 *
 * Not quite compatible with original version: Copyright (C) 2015 Selene Scriven
 * But includes original tabulation as a backup option (but still not compatible)
 *
 * Rewritten by Flintrock to use parameterized table generation.
 * Allows simplified calibration, even possibly slightly less accurate.
 * Also added Vcc calibration, which sues a very different curve.
 *  
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

/********************** Voltage ADC calibration **************************/
#if !defined(READ_VOLTAGE_FROM_VCC) //Flintrock 2017 (C)

// allow override in main config
#if !defined(REFERENCE_DIVIDER_READS_TO_VCC)

//*****Calibration for traditional voltage-divider, NON LDO **********/////////
//
  #define v_correction  0.0 //in volts for minor calibration adjustment.  
                            // Higher value will increase reading. (earlier cut-off)
							// Might be better to adjust full scale proportionally though.

  #define ADC_full_scale  192.3  // Actual ADC reading at 4.2V.  
                             // Theoretical R1=22k, R2=4.7k: 171.4  (test build measured 174, very close)
							 //             R1=19.1k, R2=4.7k: 192.3
							 // 
                             // Increasing this will decrease reported voltage => earlier cut-off
                             // If full scale voltage on divider is Vd, this is 255*Vd/V_ref  
							 //  so if divider produces 1.0V at 4.2 and V_ref is 1.1 it's  255*1.0/1.1 
							 //
							 // To calculate divider voltage take max input voltage (ex: 4.2) and multiply by R2/(R1+R2).
							 //
							 // Values may vary though since attiny voltage ref has large variance.  Use bistro-battcheck-divider-attinyXX.hex to measure it.
							 // Now should only need the reading with a full battery.  No longer requires creating full calibration table.


//*********LDO OTSM calibrataion***********//
//
#else  // next version is the value used for LDO OTSM configs where the divider voltage is read relative to Vcc
  #define v_correction  0.0 //in volts for minor calibration adjustment.
  #define ADC_full_scale  255  //  If it's setup right this should be pegged at full battery.
                               // This value CAN go above 255 if 4.2V/cell is slightly off scale on your light.
							   // Of course the actual reading will still peg because while this calibration factor can
							   // go over 255, the ADC value will not.
							   // That's ok though because as soon as the adc comes down to 254, the 
							   //calibration will then be correct, so maybe 2.54 corresponds to 4.10V for example. 
							   // It just means your voltage reads peg at about 4.1V
							   // Better than losing range on the bottom end.
#endif


							
//get_voltage lookup rounds the voltage up.  
//Round up ADC reading here by 0.05V to compensate.  Note: all the floating point math here is done at compile if used as intended.
#define ADC_val(x) (uint8_t)( ((double)x/10.0E0+(double)0.05E0-(double)v_correction)*(double)(ADC_full_scale)/(double)(4.2E0))

#else 
//*************Calibration for Vcc voltage reading voltage-divider*****************//

//Flintrock 2017 (C)
// Try 0.3 and 1.1  or 0.2 and 1.15.  

  #define diode_V  0.18  // Adjust this to shift calibration a little
                       // higher values increase the battery reading (earlier cuttoff)
					   // negative and decimal values are allowed.
					   // The hope is adjusting this can get close enough.
					   // Adjust it by an amount equal to your voltage reading error or LVP error.

  #define reference_V 1.14 // in volts. also impacts calibration, but
                           // brining this higher broadens the reading spread.
						   // So best to focus on spread when adjusting this
						   // and then adjust diode_V for offset.  About 0.03 here corresponds to 
						   // a change in spread of around 0.1V out of 1V maybe.
						   // Example if you're reading too high at 2.9V and too low at 3.9V, increase this. 
						   
  // There is a bistro-battcheck-Vcc-attinyXX.hex that provides adc values, but it's not much use
  //  with the calibration method above, or not in a simple way yet.
  //  Making use of that would require a two-point calibration, and making a one-point adjustment to diode_V is probably/hopefully good enough anyway.
						   
//Note: all the floating point math here is done at compile time if used as inteded.
  #define ADC_val(x) (uint8_t)(255-255*(double)((double)reference_V/((double)(x)/(10.0E0)-(double)diode_V)))

#endif

#ifdef USE_CAL_TABLE
#if defined(READ_VOLTAGE_FROM_DIVIDER) // use
// OLD calibrations. 
//  (C) Selene Scriven 2015 (with minor modifications)
// These values were measured using RMM's FET+7135.
// See battcheck/readings.txt for reference values.
// the ADC values we expect for specific voltages
// This table it setup for the 19.1K R1 resistor

//#define USE_CAL_TABLE  uncomment to use calibration table instead

#define ADC(46)     213  // extrapolated by eye
#define ADC(45)     209  // extrapolated by eye
#define ADC(44)     205
#define ADC(43)     201
#define ADC(42)     196
#define ADC(41)     191
#define ADC(40)     187
#define ADC(39)     182
#define ADC(38)     177
#define ADC(37)     172
#define ADC(36)     168
#define ADC(35)     163
#define ADC(34)     158
#define ADC(33)     153
#define ADC(32)     149
#define ADC(31)     144
#define ADC(30)     139
#define ADC(29)     134
#define ADC(28)     130
#define ADC(27)     125
#define ADC(26)     120
#define ADC(25)     116
#define ADC(24)     111
#define ADC(23)     106
#define ADC(22)     101
#define ADC(21)     97
#define ADC(20)     92

#else 
No calibration table defined for VCC mode.  Disable USE_CAL_TABLE or choose use divider mode.

// FR the don't work now but they could:
//#define cal_v_offset -0.2 // voltage offset in tenths of a volt negative will lower the reading.

//#define cal_slope ADC(37-ADC(29
//#define cal_offset (uint8_t)(((uint16_t)cal_v_offset*(uint16_t)cal_slope)/8) // The compiler should do the math

#endif
#endif


#define ADC_100p   ADC_val(42)  // the ADC value for 100% full (resting)
#define ADC_75p    ADC_val(40)  // the ADC value for 75% full (resting)
#define ADC_50p    ADC_val(38)  // the ADC value for 50% full (resting)
#define ADC_25p    ADC_val(35)  // the ADC value for 25% full (resting)
#define ADC_0p     ADC_val(30)  // the ADC value for 0% full (resting)
#define ADC_LOW    ADC_val(28)  // When do we start ramping down
#define ADC_CRIT   ADC_val(27)  // When do we shut the light off // THIS ISN'T ACTUALLY USED.  ONLY LOW.


/********************** Offtime capacitor calibration ********************/
// Values are between 1 and 255, and can be measured with offtime-cap.c
// See battcheck/otc-readings.txt for reference values.
// These #defines are the edge boundaries, not the center of the target.
#ifdef OFFTIM3
//UPdated values from V1.2 of TA-bistro, it seems this slows down the clicks a bit.
// The OTC value 0.5s after being disconnected from power
// (anything higher than this is a "short press")
//#define CAP_SHORT           180 // Very long timing
  #define CAP_SHORT           190 // stock value
//#define CAP_SHORT           215 // Texas Avenger faster timing
//#define CAP_SHORT           248 // Texas Avenger fastest timing
// The OTC value 1.5s after being disconnected from power
// Between CAP_MED and CAP_SHORT is a "medium press"
  #define CAP_MED             80 // Very long timing
//#define CAP_MED             94 // Stock value
//#define CAP_MED             150 // Texas Avenger faster timing
//#define CAP_MED             210 // Texas Avenger fastest Timing
// Below CAP_MED is a long press
#else
// The OTC value 1.0s after being disconnected from power
// Anything higher than this is a short press, lower is a long press
//#define CAP_SHORT           100 // Very long timing
  #define CAP_SHORT           115 // Stock value
//#define CAP_SHORT           165 // Texas Avenger faster timing
//#define CAP_SHORT           215 // Texas Avenger fastest Timing
#endif
#endif  // TK_CALIBRATION_H
