#ifndef TK_CALIBRATION_H
#define TK_CALIBRATION_H
/*
 * Attiny calibration header.
 * This allows using a single set of hardcoded values across multiple projects.
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


/********************** Voltage ADC calibration **************************/
//  First values are from Dale's wight+1 driver measurements - these
// value also work well for the BLF Q8 SRK drivers
//
// See battcheck/readings.txt for reference values.

#ifdef USING_1000K   // GT-buck
// The ADC values we expect for specific voltages: 2.2v to 4.4v. This is
//  for using R1=1000K, R2=47K, and direct connection from Batt+
//  to R1, no diode in-between
#define ADC_44     184
#define ADC_43     180
#define ADC_42     176	// 100%
#define ADC_41     171
#define ADC_40     167	//  75%
#define ADC_39     163
#define ADC_38     159	//  50%
#define ADC_37     155
#define ADC_36     150
#define ADC_35     146	//  25%
#define ADC_34     142
#define ADC_33     138
#define ADC_32     134
#define ADC_31     130
#define ADC_30     125	//   0%
#define ADC_29     121
#define ADC_28     117
#define ADC_27     113
#define ADC_26     109
#define ADC_25     104
#define ADC_24     100
#define ADC_23     96
#define ADC_22     92
#endif

#ifdef USING_360K
// The ADC values we expect for specific voltages: 4.4V to 8.8V (2.2v to 4.4v). This is
//  for using R1=360K/36K, R2=47K/4.7K, and direct connection from Batt+
//  to R1, no diode in-between, with an LDO and a 2S battery configuration.
#define ADC_44     236
#define ADC_43     230
#define ADC_42     225	// 100%
#define ADC_41     220
#define ADC_40     214	//  75%
#define ADC_39     209
#define ADC_38     203	//  50%
#define ADC_37     198
#define ADC_36     193
#define ADC_35     187	//  25%
#define ADC_34     182
#define ADC_33     177
#define ADC_32     171
#define ADC_31     166
#define ADC_30     161	//   0%
#define ADC_29     155
#define ADC_28     150
#define ADC_27     145
#define ADC_26     139
#define ADC_25     134
#define ADC_24     129
#define ADC_23     123
#define ADC_22     118
#endif

//#define ADC_100p   ADC_42  // the ADC value for 100% full (resting)
//#define ADC_75p    ADC_40  // the ADC value for 75% full (resting)
//#define ADC_50p    ADC_38  // the ADC value for 50% full (resting)
//#define ADC_25p    ADC_35  // the ADC value for 25% full (resting)
//#define ADC_0p     ADC_30  // the ADC value for 0% full (resting)

#define ADC_LOW    ADC_30  // When do we start ramping down
#define ADC_CRIT   ADC_28  // When do we shut the light off

#endif  // TK_CALIBRATION_H
