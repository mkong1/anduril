/*
 * momentary-mode.h: Momentary mode for Anduril.
 *
 * Copyright (C) 2017 Selene Scriven
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
 */

#ifndef MOMENTARY_MODE_H
#define MOMENTARY_MODE_H

// momentary / signalling mode
uint8_t momentary_state(Event event, uint16_t arg);
uint8_t momentary_mode = 0;  // 0 = ramping, 1 = strobe
uint8_t momentary_active = 0;  // boolean, true if active *right now*


#endif
