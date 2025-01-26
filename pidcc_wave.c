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
 * pidcc_wave.c - A module that generates the wave form for each DCC packet.
 *
 * This module is responsible for generating the DCC signal: it takes care
 * of the preamble, start bits data bits, error detection byte, stop bit and
 * transmission repeats.
 *
 * The functions below typically return 0 on success, or a pointer to an error
 * description string on failure.
 *
 * const char *pidcc_wave_initialize (int gpioa, int gpiob, int debug);
 *
 *    Initialize the I/O library, if needed, and select the two GPIO pins
 *    to use. This function can be called multiple times, for example to change
 *    which GPIO to use.
 *
 *    Note that the second GPIO will output the reverse signal compared to
 *    the first GPIO. That second GPIO is optional and can be set to 0
 *    if not needed by the hardware. If gpiob is 0, only gpioa will be used.
 *
 *    Return 0 on success, other value on failure.
 *
 * const char *pidcc_wave_send (int address,
 *                              const unsigned char *data, int length);
 *
 *    Format and send a DCC packet. The DCC decoder address is part of the
 *    data: this module does not interpret the format of the DCC data.
 *
 *    Return 0 on success, other value on failure.
 *
 * int pidcc_wave_microseconds (void);
 *
 *    Return the time it will take to send the latest packet.
 *
 * int pidcc_wave_state (void);
 *
 *    Return PIDCC_STARTING when a transmission was requested but has not
 *    started yet, PIDCC_TRANSMITTING when transmitting a packet and
 *    PIDCC_IDLE when there is nothing to transmit.
 *
 * void pidcc_wave_release (void);
 *
 *    Release all current resources.
 *
 * TBD: create background wave firt time needed, reuse and never delete it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <pigpio.h> // Raspberry Pi OS only, not on regular Debian.

#include "pidcc_wave.h"

static int DccWaveGpioA;
static int DccWaveGpioB;

static gpioPulse_t DccBit0[3];
static gpioPulse_t DccBit1[3];

static gpioPulse_t DccPreamble[31];

// Enough room for 15 preamble bits, 6 start bits, 6 data bytes, 1 stop bit
// and interpackets idle period (5 msec).
#define DCCMAXWAVE (2*(15+6+(8*6)+1)+51)

typedef struct {
   int count;
   int retry;
   int totalTime;
   gpioPulse_t pulses[DCCMAXWAVE];
} DccPacket;

DccPacket DccPendingPacket;

static int DccPendingWave = -1;
static int DccBackgroundWave = -1;

static int DccTransmitStarting = 0;

static int PigioInitialized = 0;

static int PidccWaveDebug = 1; // Until initialized..

static void pidcc_wave_debug (const char *text) {

   if (!PidccWaveDebug) return;

   struct timeval now;
   gettimeofday (&now, 0);
   long long sec = (long long)(now.tv_sec);
   int usec = (int)(now.tv_usec);
   printf ("$ %lld.%06d %s\n", sec, usec, text);
}

static void pidcc_wave_prepare (gpioPulse_t *pulse, int delay) {

   pulse[0].gpioOn = (1 << DccWaveGpioA);
   pulse[0].gpioOff = DccWaveGpioB ? (1 << DccWaveGpioB) : 0;
   pulse[0].usDelay = delay;

   pulse[1].gpioOn = pulse[0].gpioOff;
   pulse[1].gpioOff = pulse[0].gpioOn;
   pulse[1].usDelay = delay;

   pulse[2].usDelay = 0; // End of wave.
}

static const char *pidcc_wave_background (void) {

  int result;

  if (DccBackgroundWave < 0) {

     if (gpioWaveAddNew()) {
        return "gpioWaveAddNew() failed";
     }

     result = gpioWaveAddGeneric(2, DccBit0);
     if (result < 0) {
        return "gpioWaveAddGeneric(background) failed";
     }

     DccBackgroundWave = gpioWaveCreate();
     if (DccBackgroundWave < 0) {
        return "gpioWaveCreate(background) failed";
     }
  }

  result = gpioWaveTxSend (DccBackgroundWave, PI_WAVE_MODE_REPEAT_SYNC);
  if (result < 0) {
     return "gpioWaveTxSend(background) failed";
  }
  return 0;
}

const char *pidcc_wave_initialize (int gpioa, int gpiob, int debug) {

   PidccWaveDebug = debug;

   if (! PigioInitialized) {
      if (gpioInitialise() < 0) {
         return "pigio initialization failed";
      }
      PigioInitialized = 1;
   }

   if (gpioSetMode(gpioa, PI_OUTPUT)) {
      return "gpioSetMode(gpioa) failed";
   }
   DccWaveGpioA = gpioa;

   if (gpiob) {
      if (gpioSetMode(gpiob, PI_OUTPUT)) {
         return "gpioSetMode(gpiob) failed";
      }
   }
   DccWaveGpioB = gpiob;

   pidcc_wave_prepare (DccBit0, 100);
   pidcc_wave_prepare (DccBit1, 58);

   int i;
   for (i = 0; i < 30; i += 2) {
      DccPreamble[i] = DccBit1[0];
      DccPreamble[i+1] = DccBit1[1];
   }
   DccPreamble[30].usDelay = 0; // End of preamble sequence.

   return pidcc_wave_background ();
}

static const char *pidcc_wave_append (DccPacket *packet,
                                      const gpioPulse_t *pulse) {

   int i;
   for (i = packet->count; i < DCCMAXWAVE; ++i) {
      if (pulse->usDelay == 0) break; // end of pulse data.
      packet->pulses[i] = *(pulse++);
   }
   if (i > DCCMAXWAVE) return "DCC packet too long";
   packet->count = i;
   return 0;
}

static const char *pidcc_wave_appendByte (DccPacket *packet,
                                          unsigned char byte) {

   int i;
   const char *error;
   for (i = 0x80; i > 0; i >>= 1) {
      if (byte & i)
         error = pidcc_wave_append (packet, DccBit1);
      else
         error = pidcc_wave_append (packet, DccBit0);
      if (error) return error;
   }
   return 0;
}

static const char *pidcc_wave_format (DccPacket *packet,
                                      const unsigned char *data, int length) {

  packet->count = 0;
  packet->retry = 0; // For now, in case of error.
  unsigned char detect = 0;

  const char *error = pidcc_wave_append (packet, DccPreamble);
  if (error) return error;

  int i;
  for (i = 0; i < length; ++i) {
     error = pidcc_wave_append (packet, DccBit0); // Start bit.
     if (error) return error;
     error = pidcc_wave_appendByte (packet, data[i]);
     if (error) return error;
     detect ^= data[i];
  }

  error = pidcc_wave_append (packet, DccBit0);
  if (error) return error;
  error = pidcc_wave_appendByte (packet, detect);
  if (error) return error;

  error = pidcc_wave_append (packet, DccBit1); // Stop bit.
  if (error) return error;

  // A subsequent DCC packet must not be sent within 5 msec after the previous
  // packet. To ensure this, follow the packet with a 5 msec long bit "0"
  // stream ("0" lasts 200 usec, there fore we need 25 of them).
  //
  // Apparently a DCC generator must keep the power line in AC mode by sending
  // a continuous bit "0" signal. The stretched bit "0" was intended to force
  // a DC power level for compatibility with analog systems (now deprecated).
  //
  for (i = 0; i < 25; ++i) {
     error = pidcc_wave_append (packet, DccBit0);
     if (error) return error;
  }

  packet->retry = 2; // Plan to repeat a few times, as per the DCC standard.
  return 0;
}

const char *pidcc_wave_transmit (void) {

  if (gpioWaveAddNew()) {
     return "gpioWaveAddNew() failed";
  }

  int result = gpioWaveAddGeneric(DccPendingPacket.count,
                                  DccPendingPacket.pulses);
  if (result < 0) {
     return "gpioWaveAddGeneric() failed";
  }

  DccPendingWave = gpioWaveCreate();
  if (DccPendingWave < 0) {
     return "gpioWaveCreate() failed";
  }
  DccPendingPacket.totalTime = gpioWaveGetMicros();

  result = gpioWaveTxSend (DccPendingWave, PI_WAVE_MODE_ONE_SHOT_SYNC);
  if (result < 0) {
     return "gpioWaveTxSend() failed";
  }
  DccTransmitStarting = 1;
  return 0;
}

const char *pidcc_wave_send (const unsigned char *data, int length) {

   if (!PigioInitialized) return "Not initialized yet";;

   if (DccPendingWave >= 0) return "busy";

   pidcc_wave_debug ("pidcc_wave_send(): new transmission");
   const char *error = pidcc_wave_format (&DccPendingPacket, data, length);
   if (error) return error;

/*
printf ("New DCC frame:\n");
int i;
for (i = 0; i < DccPendingPacket.count; ++i) {
   printf ("   %c%d usec\n", (i&1) ? '-' : '_', DccPendingPacket.pulses[i].usDelay);
}
*/
   return pidcc_wave_transmit ();
}

int pidcc_wave_microseconds (void) {
   if (DccPendingWave < 0)  return 100000;
   return DccPendingPacket.totalTime + 200; // One background cycle after.
}

int pidcc_wave_state (void) {

   if (!PigioInitialized) return PIDCC_IDLE;

   if (DccPendingWave < 0) {
      pidcc_wave_debug ("pidcc_wave_state(): idle");
      return PIDCC_IDLE;
   }

   if (DccTransmitStarting) {
      if (gpioWaveTxAt () == DccBackgroundWave) {
         pidcc_wave_debug ("pidcc_wave_state(): starting a transmit");
         return PIDCC_STARTING;
      }
      // The transmission has started: restart the background wave right
      // after the transmission completed.
      pidcc_wave_debug ("pidcc_wave_state(): transmission has started");
      pidcc_wave_background ();
      DccTransmitStarting = 0;
   }

   if (gpioWaveTxAt () == DccPendingWave) {
      pidcc_wave_debug ("pidcc_wave_state(): still transmitting");
      return PIDCC_TRANSMITTING; // Not complete yet.
   }

   // At this point, there is a pending wave but transmission is complete.
   gpioWaveDelete (DccPendingWave);
   DccPendingWave = -1;

   if (DccPendingPacket.retry > 0) {
      pidcc_wave_debug ("pidcc_wave_state(): repeat transmission");
      DccPendingPacket.retry -= 1;
      pidcc_wave_transmit();
      return PIDCC_STARTING;
   }

   // At this point, there is really nothing more to transmit.
   pidcc_wave_debug ("pidcc_wave_state(): became idle");
   if (!gpioWaveTxBusy ()) pidcc_wave_background (); // We missed something..
   return PIDCC_IDLE;
}

void pidcc_wave_release (void) {

  gpioTerminate ();
}

