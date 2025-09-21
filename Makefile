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

prefix=/usr/local

HAPP=pidcc
HCAT=Train
SHARE=$(prefix)/share/house

HMAN=/var/lib/house/note/content/manuals/$(HCAT)
HMANCACHE=/var/lib/house/note/cache/manuals/$(HCAT)

INSTALL=/usr/bin/install

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
# This program does not run as a service, so this does not use
# the House install generic target.

purge-doc:
	if [ -d $(DESTDIR)$(HMANCACHE) ] ; then rm -rf $(DESTDIR)$(HMANCACHE)/* ; fi

install-doc: purge-doc
	$(INSTALL) -m 0755 -d $(DESTDIR)$(HMAN)
	$(INSTALL) -m 0644 -T README.md $(DESTDIR)$(HMAN)/$(HAPP).md

install: install-doc
	$(INSTALL) -m 0755 -d $(DESTDIR)$(prefix)/bin
	rm -f $(prefix)/bin/pidcc $(DESTDIR)$(prefix)/bin/tstgpio
	$(INSTALL) -m 6755 -s pidcc tstgpio $(DESTDIR)$(prefix)/bin

uninstall: purge-doc
	rm -f $(DESTDIR)$(prefix)/bin/pidcc $(DESTDIR)$(prefix)/bin/tstgpio
	rm -f $(DESTDIR)$(HMAN)/$(HAPP).md

