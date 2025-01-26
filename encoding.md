# DCC Encoding and Packet Format

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

Preamble: sequence of minimum 12 "1" bits. Command stations must send 14 "1" bits.

Packet start bit: 1 "0" bit following the preamble.

Address byte, following the start bit. Most significant bit first.
* 00000000: reserved (broadcast), 
* 11111110: reserved, 
* 11111111: reserved (idle packet), 

Data start bit: 1 "0" bit before a data byte.

Data byte: most significant bit first.

(the data start bit and data byte sequence may repeat.)

Packet end bit: 1 "1" bit, following the last data byte.

Minimal time between two packets: 5msec (5000usec).

## Baseline Packets

### Locomotive speed and direction command

* Address byte: 0AAAAAAA
* Data byte 1:  01DCSSSS
                    D: 1= forward, 0= reverse
                    C: additional speed bit (less significant bit)
                    S: speed bits
                       0000 Stop
                       0001 Emergency stop
                       0010 step 1 (C=0), step 2 (C=1)
                       ..
                       1111 step 27 (C=0), step 28 (C=1)
* Data byte 2:  EEEEEEEE   Error detection. XOR of address and data byte 1.

### Reset

      Address byte: 00000000
      Data byte 1:  00000000
      Data byte 2:  00000000

### Idle

      Address byte: 11111111
      Data byte 1:  00000000
      Data byte 2:  11111111


### Broadcast Stop

      Address byte: 00000000
      Data byte 1:  01DC000S (See 3.1)
      Data byte 2:  EEEEEEEE (See 3.1)

## Extended Packet Format

   (These packets can be 3 to 6 bytes long, including the address byte.)

### Address Ranges

* 00000000: broadcast

* 00000001 to 01111111: 7 bit addresses (1 to 127)

* 10000000 to 10111111: 9 and 11 bit addresses

* 11000000 to 11100111: 14 bit addresses (2 byte address)

* 11101000 to 11111100: reserved

* 11111101 to 11111110: advanced extended packet format

* 11111111: idle packet.

### Multi-Function Command

* Address byte: AAAAAAAA (or 00000000 for broadcast)
* Data byte 1:  instruction bytes
* ...
* Last data byte:  EEEEEEEE

