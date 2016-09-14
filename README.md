# stm8-sdcc-examples
Some example code for some STM8 microcontrollers (STM8S103 and STM8S003) using SDCC in Linux.

This repository contains example code, code snippets and not-quite-libraries
for some components and devices connected to a STM8-microcontroller board.

This code does not use any STMicroelectronics code, i.e. "Standard peripheral library",
or anybody else's headers.
Instead, stm8.h header file definitions are made from scratch by reading the datasheet.

## Hardware used
These examples assume following hardware configuration:
- STM8 development board based on STM8S103F3.

  I am using the really cheap Chinese STM8S103F3P6 based minimal development
  board available from Ebay/Aliexpress ("Bluepill").
  Some Makefiles and c-files contain \#ifdef:s to support STM8S003F3 MCU too.
- ST-LINK/V2 programmer

## Software environment
- Linux
- SDCC compiler (version > 3.4)
- stm8flash for flashing the board. https://github.com/vdudouyt/stm8flash

## Links
- Tutorial that pretty much describes the hardware/software setup used here:
http://www.cnx-software.com/2015/04/13/how-to-program-stm8s-1-board-in-linux/

## Examples
- blink: Simple "Hello World!" of MCUs. Basic GPIO output control (Blink internal LED)
- uart: Simple "Hello World" UART output
- timer-interrupt: Basic use of a timer (TIM2) and related interrupt
- spi-out-max7219: Simple SPI output to MAX7219 controlled 8 digit 7-segment display
- ds18b20: Temperature sensor on 1-Wire
