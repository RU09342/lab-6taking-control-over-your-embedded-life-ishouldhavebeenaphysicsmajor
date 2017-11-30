/*Author: Ben Jukus
 * Lab 6: Open Loop
 * Worked with Josh Ford and Timothy Gordon in Lab
 * Referenced code from Jessica Wozniak & Ryan Hare
 */
#include <msp430.h>
unsigned volatile int ADCin = 0;
volatile int tempC = 0;
volatile int tempF = 0;
volatile int volts = 0;
int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                               // Stop WDT
    PM5CTL0 &= ~LOCKLPM5;                                   //Disable HIGH Z mode

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Input to the ADC
    P1SEL1 |= BIT3;                                         // Configure P1.3 for ADC input
    P1SEL0 |= BIT3;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Timer A
    TA0CCTL0 |= CCIE;                                       //Timer A interrupt enable
    TA0CCTL1 |= OUTMOD_3;                                   //Set/ Reset mode
    TA0CCR1 = 20;                                           //Duty cycle CCR1
    TA0CCR0 = 205 - 1;                                      //Set period
    TA0CTL |= TASSEL_1 + MC_1 + ID_3;                       //ACLK, UPMODE, DIV 8
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Timer B
    TB0CTL |= TBSSEL_2 + MC_1 + ID_2;                       //SMCLK, UPDOWN, divided by 1, 1MHz
    TB0CCR0 = 1024;                                         //Max TimerB reaches before coming down, so 1kHz  signal
    TB0CCR2 = 5;                                            //Starting duty cycle is set
    TB0CCTL2 |= OUTMOD_7;                                   //Set Timer B to reset/set
    P1SEL0 |= BIT5;                                         //1.5 is used for PWM output
    P1DIR |= BIT5;                                          //Set P1.5 to output
    P1OUT &= ~(BIT5);                                       //set p1.5 to off
//UART
    CSCTL0_H = CSKEY_H;                                     // Unlock CS registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;                           // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;                   // Set all dividers
    CSCTL0_H = 0;                                           // Lock CS registers
    P2SEL0 &= ~(BIT0 | BIT1);                               //P2.0 => RXD
    P2SEL1 |= BIT0 + BIT1;                                  //P2.1 => TXD
                                                            // Configure USCI_A0 for UART mode
    UCA0CTLW0 = UCSWRST;                                    // Put eUSCI in reset
    UCA0CTLW0 |= UCSSEL__SMCLK;                             // CLK = SMCLK
    UCA0BRW = 52;                                           // 8000000/16/9600
    UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;                 //modulation
    UCA0CTLW0 &= ~UCSWRST;                                  // Initialize UCA
    UCA0IE |= UCRXIE;                                       // Enable USCI_A0 RX interrupt
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ADC
    REFCTL0 |= REFON + REFTCOFF + REFVSEL_0;                //Enable reference voltage at 1.2v
    ADC12CTL0 |= ADC12SHT0_2 + ADC12ON;                     // Set sample time
    ADC12CTL1 |= ADC12SHP + ADC12SSEL_1;                  // Enable sample timer
    ADC12CTL2 |= ADC12RES_2;                                // 12-bit conversion results
    ADC12MCTL0 = ADC12INCH_3 | ADC12VRSEL_1;                // Vref+ , Input channel A3
    ADC12IER0 |= ADC12IE0;                                  // Enable ADC conv complete interrupt
    P1OUT |= BIT0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    while (REFCTL0 & REFGENBUSY);                           // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_0 + REFON;                           // Enable ADC internal reference and set to 1.2V
    while (!(REFCTL0 & REFGENRDY));                         // Wait for reference generator
    __enable_interrupt();                                   //Enable all interrupts.
       __bis_SR_register(LPM3 + GIE);                      // Go into LPM0 + general interrupt enable
       __no_operation();                                   // For debugger*/
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UART ISR
#pragma vector=EUSCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch (__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
    {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
        while (!(UCA0IFG&UCTXIFG));                         //Wait while transmit flag is clear
        tempWant = UCA0RXBUF;                               //Sets the temperature we want to whatever we're getting from UART
        __no_operation();                                   //Wait
        break;
    case USCI_UART_UCTXIFG: break;                          //If flag is triggered break
    case USCI_UART_UCSTTIFG: break;                         //If flag is triggered break
    case USCI_UART_UCTXCPTIFG: break;                       //If flag is triggered break
    default:
        break;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Timer A ISR
//Timer A is used to let the ADC think
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    ADC12CTL0 |= (ADC12SC | ADC12ENC);                      //ADC12 start conversion, ADC12 enable
    UCA0TXBUF = tempC;                                      //UART output the temperature in Celsius

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ADC ISR
#pragma vector=ADC12_B_VECTOR
__interrupt void ADC12ISR(void)
{
     ADCin = ADC12MEM0;
     tempC = (ADCin *0.02929);                               //converts ADC to Temp C
     tempF = ((9 * tempC) / 5) + 32;                         //Converts from Centigrade to Fahrenheit
     while (!(UCA0IFG&UCTXIFG));                             //While the Transmit Flag is clear send the Temperature in Fahrenheit    UCA0TXBUF = tempC;

}

