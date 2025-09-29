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
 * pidcc.c - The main module for the Raspberry Pi DCC signal generator.
 *
 * This module is responsible for interpreting user (remote) commands and
 * manage queuing commands to the DCC line.
 *
 * The user commands and their syntax are:
 *
 *    ping <pin+> [<pin->]      Specify the GPIO pins to be used.
 *    send <byte> ...           Send the specified data packet.
 *    debug [0|1]               Enable/disable debug mode (default: enable)
 *    silent [0|1]              Enable/disable silent mode (default: enable)
 *
 * When the program starts, debug and silent modes are disabled.
 *
 * When debug mode is enabled, debug messages are produced.
 *
 * When silent mode is enable, some errors are ignored (e.g. queue full).
 *
 * The format of status message is as follow:
 *
 *    ('#' | '*' | '!' | '$') ' ' <timestamp> ' ' <text message>
 *
 * First character '#': the transmiter is idle.
 * First character '*': the transmiter is busy.
 * First character '!': this is an error message.
 * First character '$': this is a debug message.
 *
 * The timestamp is in format <seconds> '.' <milliseconds>
 *
 * The text portion is meant to be shown to an end user.
 *
 * In its current form, pidcc takes commands from standard input and sends
 * status messages to standard output.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

#include <pigpio.h> // Raspberry Pi OS only, not on regular Debian.

#include "pidcc_wave.h"

int DccCommandChannel = 0;

#define DCCMAXDATALENGTH 16
typedef struct {
   int length;
   unsigned char data[DCCMAXDATALENGTH];
} DccCommand;

static int DccQueueProducer = 0;
static int DccQueueConsumer = 0;
static DccCommand DccQueue[128];

static int Debug = 0;
static int Silent = 0;

static int pidcc_next (int cursor) {
   if (++cursor >= 128) return 0;
   return cursor;
}

static void pidcc_status (char category, const char *text) {
   struct timeval now;
   gettimeofday (&now, 0);
   long long sec = (long long)(now.tv_sec);
   int usec = (int)(now.tv_usec);
   printf ("%c %lld.%06d %s\n", category, sec, usec, text);
}

static void pidcc_error (const char *text) {
   pidcc_status ('!', text);
}

static void pidcc_idle (const char *text) {
   if (!text) text = "idle";
   pidcc_status ('#', text);
}

static void pidcc_busy (const char *text) {
   if (!text) text = "busy";
   if (pidcc_next (DccQueueProducer) == DccQueueConsumer)
      pidcc_status ('*', text); // Queue full, stop accepting commands
   else
      pidcc_status ('%', text); // Busy but still accepting commands.
}

static void pidcc_debug (const char *text) {
   if (! Debug) return;
   pidcc_status ('$', text);
}

static const char *pidcc_enqueue (const unsigned char *data, int length) {

   if (length > DCCMAXDATALENGTH) return "data too long";

   int cursor = DccQueueProducer;
   DccQueueProducer = pidcc_next (DccQueueProducer);
   if (DccQueueProducer == DccQueueConsumer) {
      // Queue is full: backtrack and forget this command.
      DccQueueProducer = cursor;
      return "transmitter queue full";
   }

   memcpy (DccQueue[cursor].data, data, length);
   DccQueue[cursor].length = length;
   return 0;
}

static int pidcc_dequeue (unsigned char **data) {

   if (DccQueueProducer == DccQueueConsumer) return 0; // Queue is empty.

   int cursor = DccQueueConsumer;
   DccQueueConsumer = pidcc_next (DccQueueConsumer);

   if (!data) return 0; // Queue purge.

   *data = DccQueue[cursor].data;
   return DccQueue[cursor].length;
}

static void pidcc_execute (char *command) {

   int count;
   char *words[100];

   if (command[0] == 0) return; // ignore empty commands.

   words[0] = command;
   count = 1;

   int i;
   for (i = 0; command[i] > 0; ++i) {
      if (command[i] == ' ') {
         words[count++] = command + i + 1;
         command[i] = 0;
         if (count >= 100) break;
      }
   }

   if (!strcasecmp (words[0], "send")) {
      if (count < 2) {
         pidcc_error ("missing packet data");
         return;
      }
      int length = 0;
      unsigned char data[DCCMAXDATALENGTH];
      for (i = 1; i < count; ++i) {
         if (length >= DCCMAXDATALENGTH) {
            pidcc_error ("packet data too long");
            return;
         }
         data[length++] = strtol (words[i], 0, 0);
      }
      const char *error = pidcc_enqueue (data, length);
      if (error) {
         if (!Silent) pidcc_error (error);
      } else {
          pidcc_busy ("command queued");
      }
      return;
   }

   if (!strcasecmp (words[0], "pin")) {
      if (count < 2) {
         pidcc_error ("missing pin");
         return;
      }
      int gpioa = atoi (words[1]);
      int gpiob = 0;
      if (count > 2) gpiob = atoi (words[2]);
      const char *error = pidcc_wave_initialize (gpioa, gpiob, Debug);
      if (error) pidcc_error (error);
      return;
   }

   if (!strcasecmp (words[0], "debug")) {
      if (count < 2) Debug = 1;
      else Debug = atoi (words[1]);
      return;
   }

   if (!strcasecmp (words[0], "silent")) {
      if (count < 2) Silent = 1;
      else Silent = atoi (words[1]);
      return;
   }

   pidcc_error ("unknown command");
}

static void pidcc_input (void) {

   static int  CommandCursor = 0;
   static char Command[1024];

   int length = read (DccCommandChannel,
                       Command+CommandCursor, sizeof(Command)-CommandCursor-1);

   if (length <= 0) {
      CommandCursor = 0; // Erase everything on error.
      return;
   }
   CommandCursor += length;
   Command[CommandCursor] = 0; // Force string terminator.

   char *start = Command;
   for (;;) {
      if (*start == 0) {
         CommandCursor = 0;
         return;
      }
      char *eol = strchr (start, '\n');
      if (!eol) {
         if (start == Command) return;
         size_t leftover = CommandCursor - (start - Command);
         if (leftover > sizeof(Command)) {
            CommandCursor = 0; // Erase everything in inconsistant situations.
            return;
         }
         memmove (Command, start, leftover);
         CommandCursor = leftover;
         return; // Incomplete command.
      }
      *eol = 0;
      pidcc_execute (start);
      start = eol + 1;
      while ((*start > 0) && (*start <= ' ')) start += 1; // Skip \r if any.
   }
}

static void pidcc_eventLoop (void) {

   int busy = 0; // Detect changes of state.
   struct timeval deadline = {0, 0};
   struct timeval timeout;

   timeout.tv_sec = 1;
   timeout.tv_usec = 0;

   for (;;) {
      fd_set read;

      FD_ZERO(&read);
      FD_SET(DccCommandChannel, &read);

      switch (pidcc_wave_state()) {

      case PIDCC_STARTING:

         if (!busy) pidcc_busy (0);
         busy = 1;
         timeout.tv_sec = 0;
         timeout.tv_usec = 1000;
         break;

      case PIDCC_TRANSMITTING:

         if (!busy) pidcc_busy (0);
         busy = 1;
         timeout.tv_sec = 0;
         timeout.tv_usec = 10000;
         break;

      case PIDCC_IDLE:

         timeout.tv_sec = 1; // Default, unless a new packet is transmitted.
         timeout.tv_usec = 0;

         deadline.tv_usec = 0;

         unsigned char *data;
         int length = pidcc_dequeue (&data);
         if (length > 0) {
            const char *error = pidcc_wave_send (data, length);
            if (error) {
               pidcc_error (error);
               deadline.tv_sec = 0;
               deadline.tv_usec = 0;
            } else {
               gettimeofday (&deadline, 0);
               deadline.tv_usec += pidcc_wave_microseconds ();
               while (deadline.tv_usec > 1000000) {
                  deadline.tv_usec -= 1000000;
                  deadline.tv_sec += 1;
               }
               pidcc_busy ("transmitting..");
               timeout.tv_sec = 0;
               timeout.tv_usec = 1000;
            }
            busy = 1;
         } else if (busy) {
            pidcc_idle (0);
            busy = 0;
         }
      }

      if (Debug) {
         char text[1024];
         if (deadline.tv_usec) {
            snprintf (text, sizeof(text), "waiting for %lld.%06d seconds, transmission ends at %lld.%06d...",
                      (long long)(timeout.tv_sec), (int)(timeout.tv_usec),
                      (long long)(deadline.tv_sec), (int)(deadline.tv_usec));
         } else {
            snprintf (text, sizeof(text), "waiting for %lld.%06d seconds...",
                      (long long)(timeout.tv_sec), (int)(timeout.tv_usec));
         }
         pidcc_debug (text);
      }
      int status = select (DccCommandChannel+1, &read, 0, 0, &timeout);
      pidcc_debug ("waking up");
      if (status > 0) {
         if (FD_ISSET(DccCommandChannel, &read)) {
            pidcc_debug ("received input");
            pidcc_input ();
         }
      }
   }
}


int main (int argc, const char **argv) {

   // TBD: decode argument, setup communications.

   nice (-20);
   pidcc_eventLoop ();
}

