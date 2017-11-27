/*Author: Ben Jukus
 * Worked with Josh Ford in Lab
 * Referenced code from Jessica Wozniak & Ryan Hare
*/

#include <msp430.h>

unsigned volatile int ADCin = 0;
volatile float tempC = 0;
volatile float tempF = 0;
volatile float volts = 0;

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                               // Stop WDT
    PM5CTL0 &= ~LOCKLPM5;                                   //Disable HIGH Z mode
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//PWM
    //Sets Pin 1.4 as GPIO which outputs the PWM which goes to the fan.
    P1DIR |= BIT4; //Pin 1.4
    P1SEL1 &= ~BIT4;
    P1SEL0 |=  BIT4;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Input to the ADC
    P1OUT  &= ~BIT0;                                        // Clear LED to start
    P1DIR  |= BIT0;                                         // P1.0 output
    P1SEL1 |= BIT5;                                         // Configure P1.5 for ADC
    P1SEL0 |= BIT5;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Timer A
    TA0CCTL0 = CCIE;                                        //Timer A interrupt enable
    TA0CCTL1 = OUTMOD_3;                                    //Set/ Reset mode
    TA0CCR1 = 256;                                          //Duty cycle CCR1
    TA0CCR0 = 4096 - 1;                                     //Set period
    TA0CTL = TASSEL_1 + MC_1 + ID_3;                        //ACLK, UPMODE, DIV 4

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Timer B
//Timer B is just here to PWM
    TB0CCTL1 = OUTMOD_3;                                    //set/reset mode
    TB0CCR1 = 254;                                          //Set initial CCR1
    TB0CCR0 = 255 - 1;                                      //Set CCR0 for a ~1kHz clock.
    TB0CTL = TBSSEL_2 + MC_1;                               //Enable Timer B0 with SMCLK and up mode.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
    ADC12CTL0 = ADC12SHT0_2 + ADC12ON;                      // Set sample time
    ADC12CTL1 = ADC12SHP;                                   // Enable sample timer
    ADC12CTL2 |= ADC12RES_2;                                // 12-bit conversion results
    ADC12MCTL0 = ADC12INCH_5 | ADC12VRSEL_1;                // Vref+ , Input channel A5
    ADC12IER0 |= ADC12IE0;                                  // Enable ADC conv complete interrupt
    P1OUT = BIT0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    while (REFCTL0 & REFGENBUSY);                           // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_0 + REFON;                           // Enable ADC internal reference and set to 1.2V


    while (!(REFCTL0 & REFGENRDY));                         // Wait for reference generator
    __enable_interrupt();                                   //Enable all interrupts.

    while (1)
    {
        __bis_SR_register(LPM0 + GIE);                      // Go into LPM0 + general interrupt enable
        __no_operation();                                   // For debugger
    }
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

        TB0CCR1 = 255 - UCA0RXBUF;                          //PWM for the Fan
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
 ADC12CTL0 |= (ADC12SC | ADC12ENC);                         //ADC12 start conversion, ADC12 enable
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ADC ISR
#pragma vector=ADC12_B_VECTOR
__interrupt void ADC12ISR(void)
{
    ADCin = ADC12MEM0;
    volts = ADCin * 0.000244;                               //converts ADC to voltage
    tempC = volts / 0.01;                                   //converts voltage to Temp C
    tempF = ((9 * tempC) / 5) + 32;                         //Convert from Centigrade to Fahrenheit
    while (!(UCA0IFG&UCTXIFG));                             //While the Transmit Flag is clear send the Temperature in Fahrenheit
    UCA0TXBUF = tempF;
}
