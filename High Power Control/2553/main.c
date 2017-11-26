/* Author: Ben Jukus
 * Lab 6: High Power Square Wave
 * Based on my: Timer A Blink 2553
*/
#include <msp430.h>

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  //PM5CTL0 &= ~LOCKLPM5;                     // Disable the GPIO power-on default high-impedance mode
  P1DIR |= BIT0;                            // P1.0 LED
  TA0CCTL0 = CCIE;                          // CCR0 interrupt enabled
  TA0CCR0 = 3200;                           // Approximately 10 Hz, Aclk is like 32 kHz
  TA0CTL = TASSEL_1 + MC_1 + TACLR;         // ACLK, upmode, clear TAR

  __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, enable interrupts
  __no_operation();                         // For debugger
}

// Timer0 A0 interrupt service routine
#pragma vector = TIMER0_A0_VECTOR           //Timer counts
__interrupt void TA0_ISR(void)

{
  P1OUT ^= BIT0;                            // Toggle LED 1.0
}
