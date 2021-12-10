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

//-------------------------------------------------------------------------------------------
//  The tables below represent the max value for reporting of that associated voltage value.
// For example, for 3.0V reporting in the 1000K table, the max value is 125, but the range
// for 3.0V is 122 to 125.
// The tables are all calculated values, the equations used: 
//
//  ADC_FACTOR = R2/ (R2+R1) * 256/1.1, Val = (V - D1) * ADC_FACTOR
//
//-------------------------------------------------------------------------------------------


#ifdef USING_1000K   // GT-buck
// The ADC values we expect for specific voltages: 2.2v to 4.4v per cell. This is
//  for 4S cells (17.6V max) using R1=1000K, R2=47K, D1=0, and direct connection from Batt+
//  to R1, no diode in-between
//
#define ADC_22     94
#define ADC_23     98
#define ADC_24     102
#define ADC_25     107
#define ADC_26     111
#define ADC_27     115
#define ADC_28     119	//   0%
#define ADC_29     123
#define ADC_30     127
#define ADC_31     132
#define ADC_32     136
#define ADC_33     140
#define ADC_34     144
#define ADC_35     148	//  25% (3.5V)
#define ADC_36     153
#define ADC_37     157	//  50% (3.72V)
#define ADC_38     161
#define ADC_39     165
#define ADC_40     169	//  75% (3.95V)
#define ADC_41     173
#define ADC_42     178	// 100%
#define ADC_43     182
#define ADC_44     188
#endif

#ifdef USING_360K
// The ADC values we expect for specific voltages: 4.4V to 8.8V (2.2v to 4.4v per cell). This is
//  for using R1=360K/36K, R2=47K/4.7K, and direct connection from Batt+
//  to R1, no diode in-between, with an LDO and a 2S battery configuration.

// MAX value for the voltage range:
#define ADC_22     121
#define ADC_23     126
#define ADC_24     132
#define ADC_25     137
#define ADC_26     142
#define ADC_27     148
#define ADC_28     153	//   0%
#define ADC_29     159
#define ADC_30     164
#define ADC_31     169
#define ADC_32     175
#define ADC_33     180
#define ADC_34     185
#define ADC_35     191	//  25% (3.5V)
#define ADC_36     196
#define ADC_37     202	//  50% (3.72V)
#define ADC_38     207
#define ADC_39     212
#define ADC_40     218	//  75% (3.95V)
#define ADC_41     223
#define ADC_42     228	// 100%
#define ADC_43     234
#define ADC_44     239
#endif

#ifdef USING_220K
// The ADC values we expect for specific voltages: 2.2v to 4.4v. This is
//  for using R1=220K/22K, R2=47K/4.7K, and direct connection from Batt+
//  to R1, no diode in-between
#define ADC_22     92
#define ADC_23     96
#define ADC_24     100
#define ADC_25     104
#define ADC_26     109
#define ADC_27     113
#define ADC_28     117	//   0%
#define ADC_29     121
#define ADC_30     125
#define ADC_31     129
#define ADC_32     133
#define ADC_33     137
#define ADC_34     141
#define ADC_35     145	//  25% (3.5V)
#define ADC_36     150
#define ADC_37     154	//  50% (3.72V)
#define ADC_38     158
#define ADC_39     162
#define ADC_40     166	//  75% (3.95V)
#define ADC_41     170
#define ADC_42     174	// 100%
#define ADC_43     178
#define ADC_44     182
#endif

#define ADC_LOW    ADC_30  // When do we start ramping down
#define ADC_CRIT   ADC_28  // When do we shut the light off

#endif  // TK_CALIBRATION_H
