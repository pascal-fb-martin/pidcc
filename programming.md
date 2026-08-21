# DCC Decoder Programming

Programming a decoder can be done in one of two modes:

- Service mode programming: the locomotive is isolated on a programming track and all commands can use the broadcast address or have no locomotive address.

- Operation mode programming: the locomotive is on a regular layout and all commands use the locomotive's DCC address.

This document describes service mode programming. See "decoding.md" for operation mode programming.

CV can be accessed in several ways:

- Direct mode: the full CV number is sent in the command. Larger packets but only one command is required.

- Paged mode: A firsdt command sets the page's base address, a second command can address up to 4 CV using an offset from the base.

- Physical register mode: can only access CV 01, 02, 03, 04 and 29. This is an older version of the DCC standard.

Service mode commands require a long preamble (20 bits).

## Basic Commands

> [!NOTE]
> Packets shown below omit the error correction byte.

### Reset packet:

```
<adr> 00000000
```

### Hard reset packet:

```
<adr> 00000001
```

### Factory Reset:

```
01111111 00001000
```

This packet must be preceded by a long preamble. A power cycle is required, as the CV reset occurs only on power on.

### Set short addressing:

```
<adr> 00001010
```

### Set extended addressing:

```
<adr> 00001011
```

### Activate consist (forward, or normal, orientation):

```
<adr> 00010010 0CCCCCCC
```
(CCCCCCC is the consist address.)

### Activate consist (reverse orientation):

```
<adr> 00010011 0CCCCCCC
```

CCCCCCC is the consist address.

### Cancel consist:

```
<adr> 00010010 00000000, or
<adr> 00010011 00000000
```

### CV write, long form

This packet is described in S-9.2.1, and is used for programming on the main line.

```
<adr> 111011VV VVVVVVVV DDDDDDDD
```

VVVVVVVVVV is the CV address, DDDDDDDD is the CV data.

### CV write bit, long form

This packet is described in S-9.2.1, and is used for programming on the main line.

```
<adr> 111010VV VVVVVVVV 1111DBBB
```

VVVVVVVVVV is the CV address, D is the bit data, BBB is the bit position.

### Extended Program on the main (XPOM)

This is a Direct mode multi CV write, 24 bits CV address, defined in S-9.2.1, and is used for programming on the main line.

```
<adr> 11101100 VVVVVVVV VVVVVVVV VVVVVVVV DDDDDDDD[0-4]
```

Note that the first byte is the same as for the previous packet when the CV address is lower than 256. The decoder is assumed to distinguish the two based on the length of the packet (fixed 3 bytes payload compared to 4+ bytes).

### Service mode direct single CV write (per S-9.2.3):

```
011111AA AAAAAAAA DDDDDDDD
```

AAAAAAAAAA is the CV address, DDDDDDDD is the value to write to the CV.

Note that there is no locomotive address. This is the packet to use in service mode (see sequences below). This is preceded by a long preamble (20 bits).

### Service mode direct single CV bit write (per S-9.2.3):

```
011110AA AAAAAAAA 1111DBBB
```

AAAAAAAAAA is the CV address, D is the bit value and BBB is the bit position.

Note that there is no locomotive address. This is the packet to use in service mode (see sequences below). This is preceded by a long preamble (20 bits).

### Service mode physical addressing

```
01111RRR DDDDDDDD
```

RRR indicates which set of CV is accessed:

- 000: CV1 (short address)
- 001: CV2 (start voltage)
- 010: CV3 (acceleration)
- 011: CV4 (deceleration)
- 100: CV29
- 101: page register
- 110: CV7 (version, read only)
- 111: CV8 (manufacturer ID, or factory reset when written to 8)

Note that the distinction from direct mode is the length of the packet.

## Programming sequences

The address-only mode (and alternative to direct mode for CV #1) only exist for compatibility with legacy (as in very old) decoders. It will be ignored here

The preferred programming mode is direct addressing mode.

### Service mode, Direct address mode

- Optional power cycle (e.g. set the H-Bridge bits to the same value)
- 3+ reset packets.
- 5+ writes to a single CV
- 6+ identical writes to the CV, or reset packets (this is for recovery time)
- Optional power off.

### Service mode, Physical addressing

- Optional power cycle (e.g. set the H-Bridge bits to the same value)
- 3+ reset packets.
- 5+ page preset packets
- 6+ reset packets (recovery)
- Optional power cycle.
- 3+ reset packets
- 5+ write packets.
- 6+ identical write packets or reset packets (recovery). Must be 10+ for register1.

(This is basically the combination of two programming cycles, one to preset the CV page, and the other one to write the CV(s).

### Service mode, Paged addressing

- Optional power cycle (e.g. set the H-Bridge bits to the same value)
- 3+ reset packets.
- 5+ write to page register packets
- 6+ reset packets (recovery)
- 3+ reset packet (back to programming mode)
- 5+ write packets.
- 6+ identical write packets or reset packets (recovery)

