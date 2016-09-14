/*
 * DS18B20 input to port D4
 * 8 digit 7-segment LED display with MAX7219 controller chip
 * DIN <-> C6 (MOSI)
 * CS  <-> D2 (slave select)
 * CLK <-> C5 (clock)
 */

#include <stdint.h>
#include "stm8.h"

/* 1-Wire (DS18B20 data) pin */
#define OW_PORT PA
#define OW_PIN  PIN3


/* Simple busy loop delay */
void delay(unsigned long count) {
    while (count--)
        nop();
}

/* For LED-display */
void setup_spi() {
    // SPI port setup: MISO is pullup in, MOSI & SCK are push-pull out
    PC_DDR |= PIN5 | PIN6; // clock and MOSI
    PC_CR1 |= PIN5 | PIN6 | PIN7;

    // CS/SS (PD2) as output
    PD_DDR |= PIN2;
    PD_CR1 |= PIN2;
    PD_ODR |= PIN2; // CS high

    // SPI registers: First reset everything
    SPI_CR1 = 0;
    SPI_CR2 = 0;

    // SPI_CR1 LSBFIRST=0 (MSB is transmitted first)
    SPI_CR1 &= ~SPI_CR1_LSBFIRST;
    // Baud Rate Control: 0b111 = fmaster / 256 (62,500 baud)
    SPI_CR1 |= SPI_CR1_BR(0b111);
    // SPI_CR1 CPOL=0 (Clock Phase, The first clock transition is the first data capture edge)
    SPI_CR1 &= ~SPI_CR1_CPOL;
    // SPI_CR1 CPHA=0 (Clock Polarity, SCK to 0 when idle)
    SPI_CR1 &= ~SPI_CR1_CPHA;

    SPI_CR2 |= SPI_CR2_SSM; // bit 1 SSM=1 Software slave management, enabled
    SPI_CR2 |= SPI_CR2_SSI; // bit 0 SSI=1 Internal slave select, Master mode
    SPI_CR1 |= SPI_CR1_MSTR;  // CR1 bit 2 MSTR = 1, Master configuration.
}

uint8_t SPIOut(uint8_t data) {
    SPI_CR1 |= SPI_CR1_MSTR;;  // MSTR = 1, Master device.
    SPI_CR1 |= SPI_CR1_SPE; // SPE, SPI Enable, Peripheral enabled
    SPI_DR = data;
    while (SPI_SR & SPI_SR_BSY); // SPI is busy in communication or Tx buffer is not empty
    SPI_CR1 &= ~SPI_CR1_SPE; // Disable SPI
    data = SPI_DR;
    return data; // Not yet used.
}

void output_max(uint8_t address, uint8_t data) {
    PD_ODR &= ~PIN2; // CS low
    SPIOut(address);
    SPIOut(data);
    PD_ODR |= PIN2; // CS high
}

void init_max7219() {
    uint8_t i;
    output_max(0x0f, 0x00); //display test register - test mode off
    output_max(0x0c, 0x01); //shutdown register - normal operation
    output_max(0x0b, 0x07); //scan limit register - display digits 0 thru 7
    output_max(0x0a, 0x01); //intensity register (1 = 3/32 on.  0xf is max)
    output_max(0x09, 0xff); //decode mode register - CodeB decode all digits
    // Blank all digits
    for (i=1; i <= 8; i++) {
        output_max(i, 0xf);
    }
}

void display_number_dot(uint32_t number, uint8_t dot_pos, uint8_t is_negative) {
    uint8_t pos=1;
    if (number == 0)
        output_max(pos++, 0);
    while (number > 0) {
        uint8_t digit = number % 10;
        if (pos == dot_pos) {
            digit = digit | 0x80;
        }
        output_max(pos++, digit);
        number /= 10;
    }
    if (is_negative) {
        output_max(pos++, 0xa);
    }
    // clear rest of digits
    while (pos <= 8) {
        output_max(pos++, 0xf);
    }
}

/********************** OneWire/DS18B20 routines ***************************/
void delay_us(uint16_t i) {
    if (i < 9) { // FIXME: Really ugly
        nop();
        return;
    }
    TIM2_CNTRH = 0;
    TIM2_CNTRL = 0;
    TIM2_EGR = 0x01; // Update Generation
    while(1) {
        volatile uint16_t counter = (((TIM2_CNTRH) << 8) | TIM2_CNTRL);
        if (i-6 < counter)
            return;
    }
}

#define OW_INPUT_MODE()     PORT(OW_PORT,DDR) &= ~OW_PIN
#define OW_OUTPUT_MODE()    PORT(OW_PORT,DDR) |= OW_PIN
#define OW_LOW()            PORT(OW_PORT,ODR) &= ~OW_PIN
#define OW_HIGH()           PORT(OW_PORT,ODR) |= OW_PIN
#define OW_READ()           (PORT(OW_PORT,IDR) & OW_PIN)

void ow_pull_low(unsigned int us) {
    OW_OUTPUT_MODE();
    OW_LOW();
    delay_us(us);
    OW_INPUT_MODE();
}

void ow_write_byte(uint8_t out) {
    uint8_t i;
    for (i=0; i < 8; i++) {
        if ( out & ((uint8_t)1<<i) ) {
            // write 1
            ow_pull_low(1);
            delay_us(60);
        } else {
            // write 0
            ow_pull_low(60);
            delay_us(1);
        }
    }
}

uint8_t ow_read_byte() {
    uint8_t val = 0;
    uint8_t i;
    for (i=0; i < 8; i++) {
        ow_pull_low(1);
        delay_us(5);
        if (OW_READ()) {
            val |= ((uint8_t)1<<i);
        }
        delay_us(55);
    }
    return val;
}

unsigned int ow_init() {

    uint8_t input;

    ow_pull_low(480);
    delay_us(60);

    input = !OW_READ();
    delay_us(420);

    return input;
}

unsigned int ow_convert_temperature() {
    int cycles = 1; // For debugging purposes

    ow_write_byte(0x44); // Convert Temperature

    while (1) {
        ow_pull_low(1);
        delay_us(5);
        if (OW_READ()) {
            return cycles;
        }
        delay_us(55);
        cycles++;
    }
}

void display_ds_temperature(uint8_t high, uint8_t low) {
    uint8_t is_negative = 0;
    uint16_t decimals = 0; // 4 decimals (e.g. decimals 625 means 0.0625)
    uint16_t i;

    uint16_t temp = ((int16_t)high << 8) | low;
    if (temp & 0x8000) {
        is_negative = 1;
        temp = (~temp) + 1;
    }
    low = temp & 0x0f;
    temp = temp >> 4;

    // low[3:0] mean values 0.5,0.25,0.125 and 0.0625
    for (i=625; i <= 5000; i=i*2) {
        if (low & 0x01) {
            decimals += i;
        }
        low = low >> 1;
    }

    // Display temperature rounded to one decimal
    display_number_dot((temp*1000 + ((decimals+5)/10) + 50)/100, 2, is_negative);
}

void read_ds18b20() {
    uint8_t i;
    uint8_t scratchpad[9];

    if (ow_init()) {
        ow_write_byte(0xcc); // Skip ROM
        ow_convert_temperature();

        ow_init();
        ow_write_byte(0xcc); // Skip ROM
        ow_write_byte(0xbe); // Read Scratchpad
        for (i=0; i<9; i++) {
            scratchpad[i] = ow_read_byte();
        }

        display_ds_temperature(scratchpad[1], scratchpad[0]);

    } else {
        /* DS18B20 was not detected */
        output_max(0x8, 0xa);
    }
}

/***************************************************************************/



int main(void)
{
    /* Set clock to full speed (16 Mhz) */
    CLK_CKDIVR = 0;

    // Timer setup (for delay_us)
    TIM2_PSCR = 0x4; // Prescaler: to 1MHz
    TIM2_CR1 |= TIM_CR1_CEN; // Start timer

    setup_spi();
    init_max7219();

    while(1) {
        read_ds18b20();
        delay(10000L);
    }
}
