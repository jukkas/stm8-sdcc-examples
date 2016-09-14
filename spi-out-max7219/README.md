# SPI with 8 digit 7-segment LED display module

This example demonstrates simple SPI output to an 8 digit 7-segment LED display
with MAX7219 controller chip.

It counts up digits every 1 second.

## Hardware
- 8 digit 7-segment LED display module with MAX7219 controller chip.
- wiring: module <-> STM8:  
  - DIN <-> C6 (MOSI)
  - CS  <-> D2 (slave select)
  - CLK <-> C5 (clock)
