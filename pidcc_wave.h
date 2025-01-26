/* DCC Transmitter - A software that generates the DCC signal for a booster.
 *
 * Copyright 2025, Pascal Martin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * ------------------------------------------------------------------------
 *
 * pidcc_wave.h - A module that generates the wave form for each DCC packet.
 */
const char *pidcc_wave_initialize (int gpioa, int gpiob);
const char *pidcc_wave_send (const unsigned char *data, int length);
int pidcc_wave_microseconds (void);
int pidcc_wave_busy (void);
const char *pidcc_wave_idle (void);
void pidcc_wave_release (void);

