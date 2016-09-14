# DS18B20 1-Wire temperature sensor

This example implements simple temperature display using DS18B20.
This code supports only one device on 1-wire.  DS18B20 must be powered by 3.3V
(i.e. not using Parasitic Power Mode). About 4.7k resistor must be connected
between data pin (A3) and Vcc.
Setup builds on spi-out-max7219 setup with 8 digit 7-segment LED display.

## Hardware
- DS18B20 connected to pin A3.
- 4.7kohm resistor b/w A3 and 3.3V. 
- 8 digit 7-segment LED display module with MAX7219 controller chip.
- Display wiring: module <-> STM8:  
  - DIN <-> C6 (MOSI)
  - CS  <-> D2 (slave select)
  - CLK <-> C5 (clock)
