# PiDcc
A DCC transmitter for the Raspberry Pi.

## Overview

DCC is a standard for sending commands to locomotives and accessories on a model train layout. The commands are sent through the rails, as a pulse modulation of the electrical power.

This program is designed to transmit DCC commands on the specified GPIO pins. When a DCC data packet is received from the standard input, PiDcc generates the proper modulated signal and then outputs it to the specified GPIO pins.

PiDcc can handle up to two GPIO pins. When two pins are provided, PiDcc generates an inverted signal to the second pin. This matches how boosters made with DC motor drivers work.

PiDcc transmits every DCC message three times, with the minimal separation as specified in the DCC standard. This reduces the risk of data loss due to transiant noise.

PiDcc implements a transmission queue: the client application may send a burst of DCC messages. PiDcc will send each message back to back, one after the other. This queue has a limited size (up to 128 commands). This is meant to allow the client application some control on the timing between messages, especially for the DCC programming mode.

The software is based on the PiGPIO library, which requires root access. Therefore the `pidcc` program is installed with the setuid bit.

The purpose of PiDcc is to isolate a client application from the PiGPIO library:
- The client application does not require the setuid bit and does not need to run as root.
- The client application does not have to be built as multithread.

The client application must launch `pidcc` in the background and control it through a pipe.

> [!NOTE]
> PiGPIO provides its own application, `pigpiod`, which allows multiple client (non root) applications to share access to the GPIO pins. The interface and client libraries provided by PiGPIO are still multithread. The PiDcc application is specific to the DCC standard, not general purpose: the client application does not need to be aware of the DCC signal modulation rules. PiDcc uses a simple (simplistic?) and documented protocol, and the client application does not depend on any specific library. A PiDcc client application is not required to be built in multithread mode, and can use any programming language it sees fit (see the test scripts). The avoidance of multithread mode is intentional, as the client applications that PiDcc is intended for are single threaded. On the downside, PiDcc does not support sharing access between multiple applications (yet..).

## Restrictions

PiDcc does not implement, and does not support, any of the feedback mechanisms described in the DCC standard.

## Installation

* Install git, make gcc.
* Clone this repository.
* make rebuild
* sudo make install

The pidcc program is installed in /usr/local/bin.

> [!NOTE]
> The PiGPIO library is normally installed by default on all Raspberry Pi OS variants. If any package is missing, install packages pigpio and libpigpio-dev

## Commands

The PiDcc program accepts the following commands on its standard input:

```
pin GPIOA [GPIOB]
```
Initialize the GPIO access. This command must be issued before any `send` commands (see below). GPIOA is the number of the first GPIO to use, GPIOB is the number if the second GPIO pin to use. GPIOB is optional.

```
send BYTE ...
```
Send the specified sequence of bytes as a DCC packet. The first byte must be the DCC address (or the first byte of the DCC address). The pidcc program makes no assumption regarding the format of a DCC packet. Each byte value must be an integer formatted in the usual fashion, including:
- A decimal value if the first digit is from 1 to 9.
- An hexadecimal value if it starts with "0x".

```
debug [0|1]
```
Enable or disable debug output. Without parameter, debug output is enabled. Any debug output line starts with character `$`.

```
silent [0|1]
```
Enable or disable silent mode. Without parameter silent mode is enabled. Silent mode suppresses some (verbose) status output and minor error output.

## Status

The PiDcc program prints status, error and debug messages to its standard output. The syntax on an output line is:

```
('#' | '*' | '!' | '$') ' ' TIMESTAMP ' ' TEXT ...
```
The first character defines the type of the line:

| Character | Description |
| :--- | :--- |
| _'#'_ | The transmitter is idle. |
| _'*'_ | The transmitter is busy. |
| _'!'_ | Error message. |
| _'$'_ | Debug message. |

The TIMESTAMP part provides the time when this message was initiated as fractional seconds with a micro-second accuracy.

The TEXT part provides a human readable description of the event.

