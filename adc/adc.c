/*
 * Analog input to port D4
 * 8 digit 7-segment LED display with MAX7219 controller chip
 * DIN <-> C6 (MOSI)
 * CS  <-> D2 (slave select)
 * CLK <-> C5 (clock)
 */

#include <stdint.h>
#include "stm8.h"

/* Simple busy loop delay */
void delay(unsigned long count) {
    while (count--)
        nop();
}

void init_adc() {
    ADC_CSR = 0;
    ADC_CR1 = 0;
    ADC_CR2 = 0;
    ADC_CR3 = 0;

/*
    ADC_DRH = 0;
    ADC_DRL = 0;
    ADC_TDRH = 0;
    ADC_TDRL = 0;
    ADC_HTRH = 0;
    ADC_HTRL = 0;
    ADC_LTRH = 0;
    ADC_LTRL = 0;
    ADC_AWSRH = 0;
    ADC_AWSRL = 0;
    ADC_AWCRH = 0;
    ADC_AWCRL = 0;
*/
    ADC_CSR = 2; // Select channel 2 (AIN2=PC4)

    ADC_CR1 |= ADC_CR1_ADON; // ADON
    ADC_CR2 &= ~ADC_CR2_ALIGN; // Align left

    delay(1000); // Give little time to be ready for first conversion
}

uint16_t analog_read() {
    ADC_CR1 &= ~ADC_CR1_CONT; // Single conversion mode
    ADC_CR1 |= ADC_CR1_ADON; // Start conversion
    do { nop(); } while ((ADC_CSR >> 7) == 0);
    ADC_CSR &= ~ADC_CSR_EOC; // Clear "End of conversion"-flag
    return (ADC_DRH << 2) | (ADC_DRL >> 6);  // Left aligned
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

void display_number(uint32_t number) {
    uint8_t pos=1;
    if (number == 0)
        output_max(pos++, 0);
    while (number > 0) {
        uint8_t digit = number % 10;
        output_max(pos++, digit);
        number /= 10;
    }
    // clear rest of digits
    while (pos <= 8) {
        output_max(pos++, 0xf);
    }
}

int main(void)
{
    /* Set clock to full speed (16 Mhz) */
    CLK_CKDIVR = 0;

    init_adc();
    setup_spi();
    init_max7219();

    while(1) {
        display_number(analog_read());
        delay(100000L);
    }
}
