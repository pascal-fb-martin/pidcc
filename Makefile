# piDCC - A Raspberry PI service to send DCC packets.
#
# Copyright 2024, Pascal Martin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

HAPP=pidcc
HROOT=/usr/local
SHARE=$(HROOT)/share/house

# Application build. --------------------------------------------

OBJS= pidcc_wave.o \
      pidcc.o
LIBOJS=

all: tstgpio pidcc

clean:
	rm -f *.o *.a pidcc tstgpio

rebuild: clean all

%.o: %.c
	gcc -c -Wall -pthread -g -O -o $@ $<

pidcc: $(OBJS)
	gcc -g -pthread -O -o pidcc $(OBJS) -lpigpio -lrt

tstgpio: tstgpio.c
	gcc -g -Wall -pthread -o tstgpio tstgpio.c -lpigpio -lrt

# Distribution agnostic file installation -----------------------

install:
	mkdir -p $(HROOT)/bin
	rm -f $(HROOT)/bin/pidcc $(HROOT)/bin/tstgpio
	cp pidcc tstgpio $(HROOT)/bin
	chown root:root $(HROOT)/bin/pidcc $(HROOT)/bin/tstgpio
	chmod 755 $(HROOT)/bin/pidcc $(HROOT)/bin/tstgpio
	chmod a+s $(HROOT)/bin/pidcc $(HROOT)/bin/tstgpio

uninstall:
	rm -f $(HROOT)/bin/pidcc $(HROOT)/bin/tstgpio

