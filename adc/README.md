# ADC

This example demonstrates basic analog to digital conversion.
Setup builds on spi-out-max7219 setup, just adding analog input to pin C4
(e.g. potentiometer, TMP36 temperature sensor etc).

## Hardware
- 8 digit 7-segment LED display module with MAX7219 controller chip.
- wiring: module <-> STM8:  
  - DIN <-> C6 (MOSI)
  - CS  <-> D2 (slave select)
  - CLK <-> C5 (clock)
- Analog input (max 3.3v) to port D4
 
