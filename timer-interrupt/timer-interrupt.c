/*
 * Blink a LED every second using timer TIM2 and its Update/Overflow interrupt
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

/* TIM2 Update/Overflow interrupt handling routine */
void TIM2_update(void) __interrupt(TIM2_OVR_UIF_IRQ) {
    // Blink internal LED. Port B (or D) output data register. Flip pin 5 (or 3)
    PORT(LED_PORT, ODR) ^= LED_PIN;

    // Clear Timer 2 Status Register 1 Update Interrupt Flag (UIF) (bit 0)
    TIM2_SR1 &= ~TIM_SR1_UIF;
}

int main(void)
{
    /* Set clock to full speed (16 Mhz) */
    CLK_CKDIVR = 0;

    /* GPIO of LED pin setup */
    // Set pin data direction as output
    PORT(LED_PORT, DDR)  |= LED_PIN; // i.e. PB_DDR |= (1 << 5);
    // Set pin as "Push-pull"
    PORT(LED_PORT, CR1)  |= LED_PIN; // i.e. PB_CR1 |= (1 << 5);

    /* TIM2 setup */
    // Prescaler register
    TIM2_PSCR = 14; // 2^14==16384, 16MHz/16384==976.5625 Hz
    // set Counter Auto-Reload Registers - TIM2_ARR=977 == 0x03D1, about once per second
    TIM2_ARRH = 0x03;
    TIM2_ARRL = 0xd1;
    // TIM2_IER (Interrupt Enable Register), Update interrupt (UIE) (bit 0)
    TIM2_IER |= TIM_IER_UIE;
    // TIM2_CR1 – Timer 2 Control Register 1, Counter ENable bit (CEN) (bit 0)
    TIM2_CR1 |= TIM_CR1_CEN;

    /* Loop infinitely waiting for an interrupt */
        while(1) {
        wfi();
    }
}
