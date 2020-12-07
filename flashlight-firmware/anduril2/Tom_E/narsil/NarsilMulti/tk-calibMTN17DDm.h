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

// Only uncomment one of the def's below:
#define USING_220K	// for using the 220K resistor
//#define USING_191K	// for using the 191K resistor

/********************** Voltage ADC calibration **************************/
// First values are the best guess for RMM's MTN17DDm driver with 220K R1 and 47K R2,
//   1st commented out values are from Dale's wight+1 driver measurements,
//   2nd commented out values are values measured using RMM's FET+7135
// See battcheck/readings.txt for reference values.

#ifdef USING_220K

// The ADC values we expect for specific voltages: 2.2v to 4.4v. This is
//  for using R1=220K/22K, R2=47K/4.7K, and with a diode in-between Batt+
//  and R1
#define ADC_44     174 // 184	// 205
#define ADC_43     170 // 180	// 201
#define ADC_42     166 // 175	// 196  100%
#define ADC_41     162 // 171	// 191
#define ADC_40     158 // 167	// 187   75%
#define ADC_39     153// 163	// 182
#define ADC_38     149 // 159	// 177   50%
#define ADC_37     145 // 154	// 172
#define ADC_36     140 // 150	// 168
#define ADC_35     135 // 146	// 163   25%
#define ADC_34     131 // 141	// 158
#define ADC_33     127 // 136	// 153
#define ADC_32     123 // 132	// 149
#define ADC_31     119 // 128	// 144
#define ADC_30     116 // 124	// 139   0%
#define ADC_29     111 // 120	// 134
#define ADC_28     107 // 117	// 130
#define ADC_27     103 // 112	// 125  ---
#define ADC_26      99 // 108	// 120
#define ADC_25      95 // 104	// 116
#define ADC_24      91 // 100	// 111
#define ADC_23      87 //  96	// 106
#define ADC_22      83 //  92	// 101

#elif USING_191K
// 191K R1 - the ADC values we expect for specific voltages: 2.0v to 4.4v
#define ADC_44     192 // 184	// 205
#define ADC_43     188 // 180	// 201
#define ADC_42     184 // 175	// 196  100%
#define ADC_41     180 // 171	// 191
#define ADC_40     175 // 167	// 187   75%
#define ADC_39     171 // 163	// 182
#define ADC_38     167 // 159	// 177   50%
#define ADC_37     163 // 154	// 172
#define ADC_36     159 // 150	// 168
#define ADC_35     154 // 146	// 163   25%
#define ADC_34     150 // 141	// 158
#define ADC_33     146 // 136	// 153
#define ADC_32     141 // 132	// 149
#define ADC_31     136 // 128	// 144
#define ADC_30     132 // 124	// 139   0%
#define ADC_29     128 // 120	// 134
#define ADC_28     123 // 117	// 130
#define ADC_27     119 // 112	// 125  ---
#define ADC_26     116 // 108	// 120
#define ADC_25     111 // 104	// 116
#define ADC_24     107 // 100	// 111
#define ADC_23     103 //  96	// 106
#define ADC_22      99 //  92	// 101
#endif


//#define ADC_100p   ADC_42  // the ADC value for 100% full (resting)
//#define ADC_75p    ADC_40  // the ADC value for 75% full (resting)
//#define ADC_50p    ADC_38  // the ADC value for 50% full (resting)
//#define ADC_25p    ADC_35  // the ADC value for 25% full (resting)
//#define ADC_0p     ADC_30  // the ADC value for 0% full (resting)

#define ADC_LOW    ADC_32  // When do we start ramping down
#define ADC_CRIT   ADC_30  // When do we shut the light off

#endif  // TK_CALIBRATION_H
