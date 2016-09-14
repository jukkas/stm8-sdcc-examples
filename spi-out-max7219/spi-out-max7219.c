/*
 * 8 digit 7-segment LED display with MAX7219 controller chip
 * DIN <-> C6 (MOSI)
 * CS  <-> D2 (slave select)
 * CLK <-> C5 (clock)
 */

#include <stdint.h>
#include "stm8.h"

/* Build in LED is in pin B5 (STM8S103 board) or D3 (STM8S003F3 board) */
#ifdef STM8S103
#define LED_PORT    PB
#define LED_PIN     PIN5
#else
#define LED_PORT    PD
#define LED_PIN     PIN3
#endif

/* TIM1 Update/Overflow interrupt handling routine */
void TIM1_update(void) __interrupt(TIM1_OVR_UIF_IRQ) {
    // Blink internal LED. Port B (or D) output data register. Flip pin 5 (or 3)
    PORT(LED_PORT, ODR) ^= LED_PIN;

    // Clear Timer Status Register 1 Update Interrupt Flag (UIF) (bit 0)
    TIM1_SR1 &= ~TIM_SR1_UIF;
}


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
    uint32_t counter = 0;

    /* Set clock to full speed (16 Mhz) */
    CLK_CKDIVR = 0;

    // Setup internal LED
    PORT(LED_PORT, DDR)  |= LED_PIN;
    PORT(LED_PORT, CR1)  |= LED_PIN;

    setup_spi();
    init_max7219();

    /* TIM1 setup: Generate interrupt every 1000ms */
    TIM1_CR1 = 0;
    // Just for fun, let's count down (TIM1 can do that)
    TIM1_CR1 |= TIM_CR1_DIR;
    // Prescaler: divide clock with 16000 (0x3E7F + 1) (to 1ms)
    TIM1_PSCRH = 0x0E;
    TIM1_PSCRL = 0x7F;
    // Auto-reload registers. Count to 1000 (0x03E8)
    TIM1_ARRH = 0x13;
    TIM1_ARRL = 0xE8;
    // TIM1_IER (Interrupt Enable Register), Update interrupt (UIE) (bit 0)
    TIM1_IER |= TIM_IER_UIE;
    // TIM1_CR1 – Timer Control Register 1, Counter ENable bit (CEN) (bit 0)
    TIM1_CR1 |= TIM_CR1_CEN;

    /* Loop infinitely waiting for an interrupt */
    while(1) {
        wfi();
        display_number(counter++);
        if (counter > 99999999) // In case we are running this for years :-)
            counter = 0;
    }
}
