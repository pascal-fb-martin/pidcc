# DCC Packet Format

This document is a summary of the relevant NMRA S-9 documentations for the operation mode. It omits some DCC packets not relevant to PiDCC. It is believed to be up-to-date as of August, 1 2026.

Programming mode is covered in "programming.md".

## DCC Bit Encoding

Bit "1": two equal and opposite phases, same duration t1 +/- t1d

* t1 = 55 - 58 - 61 usec.
* t1d = 3 usec

Bit "0": two equal and opposite phases, same duration t0, total <= t0tot

* t0 = 95 - 100 - 9900 usec.
* t0tot = 12000 usec
* t0d = 3 usec

Stretched bit "0": first phase t0, second phase up to max t0

Voltage:
* low < -4V and high > 4V.
* Transition 2,5V/usec (max 9.5 usec from -12V to 12V)

## Packet format

The client applications must format the address and data bytes for each packet to transmit. There are two exceptions:

- PiDCC generates the error detection byte. The packets provided by clients must not include this byte.
- PiDCC independently generates and transmits the idle packet.

Preamble: sequence of minimum 12 "1" bits. Command stations must send 14 "1" bits.

Packet start bit: 1 "0" bit following the preamble.

Address byte, following the start bit. Most significant bit first. One byte addresses use the range 1..127. Extended addresses require a second address byte, see Address Ranges section below.

* 00000000: reserved (broadcast), 
* 11111110: reserved, 
* 11111111: reserved (idle packet), 

Data start bit: 1 "0" bit before a data byte.

Data byte: most significant bit first.

(the data start bit and data byte sequence may repeat.)

Packet end bit: 1 "1" bit, following the last data byte.

Minimal time between two packets: 5msec (5000usec).

## Address Ranges

* 00000000: broadcast

* 00000001 to 01111111: 7 bit addresses (1 to 127)

* 10000000 to 10111111: 9 and 11 bit addresses (accessory decoders)

* 11000000 to 11100111: 14 bit addresses (locomotive 2 byte address)

* 11101000 to 11111100: reserved

* 11111101 to 11111110: advanced extended packet format

* 11111111: idle packet.

## Baseline Packets

The PiDCC application only knows the idle packet. DCC packets are described here for the benefit of client application developpers.

In this document, "EEEEEEEE" means the error detection byte This byte is a XOR of each byte in the packet, starting at the first address byte. PiDCC generates the error detection byte.

### Locomotive speed and direction

See NMRA S 9.2, section B.

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1:  01DCSSSS
                    D: 1= forward, 0= reverse
                    C: additional speed bit (least significant bit)
                    S: speed bits
                       0000 Stop
                       0001 Emergency stop
                       0010 step 1 (C=0), step 2 (C=1)
                       ..
                       1111 step 27 (C=0), step 28 (C=1)
* Data byte 2:  EEEEEEEE

If CV29 bit 1 is 0, then bit C above controls FL (function light) instead.

### Reset

See NMRA S 9.2, section B.

* Address byte: 00000000
* Data byte 1:  00000000
* Data byte 2:  00000000 (error detection)

### Idle

See NMRA S 9.2, section B.

* Address byte: 11111111
* Data byte 1:  00000000
* Data byte 2:  11111111 (error detection)

### Broadcast Stop

See NMRA S 9.2, section B. See the Locomotive speed and direction above for more details.

* Address byte: 00000000
* Data byte 1:  01DC000S
* Data byte 2:  EEEEEEEE

## Extended Packet Format

   (These packets can be 3 to 6 bytes long, including the address byte.)

### Decoder Reset

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 00000000
* Data byte 2:  EEEEEEEE

### Set Advanced addressing

This command enables (F = 1) or disables (F = 0) the locomotive 14 bits address mode.

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 0000101F
* Data byte 2:  EEEEEEEE

### Consist Control

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 0001TTTT 0AAAAAAA
* Data byte 2:  EEEEEEEE

The address in data byte 1 is the consist address (1 to 127). If that address is 0, the consist is deactivated, otherwise it is activated.

Supported values for TTTT are:

* 0010: unit in normal direction (forward).
* 0011: unit in opposite direction (reverse)

### 128 Speed Step Control

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 00111111
* Data byte 2: DSSSSSSS
* Data byte 3:  EEEEEEEE

SSSSSSS is the step number, D = 1 is forward, D = 0 is reverse. D = 0 and SSSSSSS = 0000000 means stop, D = 0 and SSSSSSS = 0000001 means emergency stop. Steps 1 to 126 start at SSSSSSS = 0000010 (step 1).

This command must be acknowledged.

### Analog Function Group

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 00111101
* Data byte 2: VVVVVVVV
* Data byte 3: DDDDDDDD
* Data byte 4:  EEEEEEEE

VVVVVVVV is the analog function code (up to 255 different functions) and DDDDDDDD is the data to be applied to the selected function. The VVVVVVVV values are assigned by NMRA:

* 000000001: volume control

This command must be acknowledged.

### Function Group One

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 100DDDDD
* Data byte 2:  EEEEEEEE

Bit 4 of DDDDD is FL (light), bit 0 is F1, bit 3 is F4.

It is apparently possible to combine function group one and multiple function group two commands in a single packet.

### Function Group Two

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 101SDDDD
* Data byte 2:  EEEEEEEE

DDDD is F5 to F8 if S = 1, or F9 to F12 if S = 0.

It is apparently possible to combine function group one and multiple function group two commands in a single packet.

## Configuration Variables

These packets can be used either in operation mode or in programming mode.

### Configuration Variable Access (Short Form)

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 1111GGGG
* Data byte 2, 2 & 3 or 2 & 3 & 4: DDDDDDDD [DDDDDDDD [DDDDDDDD]
* Data byte 3, 4 or 5: EEEEEEEE

GGGG:

* 0010: set CV23 (1 DDDDDDDD byte).
* 0011: set CV24 (1 DDDDDDDD byte).
* 0100: set long address in CV17, CV18 (set CV29 bit 5). Two identical packets required.
* 0101: set indexed CV CV31, CV32. Two identical packets required.
* 0110: set long consist address in CV19, CV20. Two identical packets required.

This packet is used for paired CV values, when the two CVs must be set at the same time.

### Configuration Variable Access (Long Form)

* Address byte(s): 0AAAAAAA or 11AAAAAA AAAAAAAA
* Data byte 1: 1110GGVV
* Data byte 2: VVVVVVVV
* Data byte 3: DDDDDDDD
* Data byte 4: EEEEEEEE

GG is the command: verify (01), write (11) or bit manipulation (10).

VVVVVVVVVV is the CV address (0000000000 is CV1).

Verify causes the decoder to compare the data with its own configuration and acknowledge if equal.

Write cause the entire DDDDDDDD to be written to the specified CV after two identical packets have been received (as a validity check).

Bit manipulation interprets DDDDDDDD as 111FDBBB, where F = 1 is write, F = 0 is verify, D contains the bit value and BBB represents the bit position. This only takes effect after the decoder has received two identical packets (as a validity check).

