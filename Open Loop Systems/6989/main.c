/*Author: Ben Jukus
 * Worked with Josh Ford
 * Based on code from Jessica Wozniak & Ryan Hare
*/

#include <msp430.h>

#define ADC12 BIT5              //ADC12 set to BIT5
#define LED1 BIT0               //LED1 set to BIT0
#define RXD BIT4                //RXD set to BIT3
#define TXD BIT3                //TXD set to BIT3
#define OUTPUT BIT4             //Pin 1.4 is the TB0CCR1 output pin.

void TimerAInit(void);
void TimerBInit(void);
void ADC12Init(void);
void UARTInit(void);
void GPIOInit(void);
void PinInit(void);

unsigned volatile int in = 0;
volatile float tempC = 0;
volatile float tempF = 0;
volatile float voltage = 0;

int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
    PM5CTL0 &= ~LOCKLPM5;                     //Disable HIGH Z mode

    PinInit();                    //PinInit Function call
    GPIOInit();                   //GPIOInit Function call
    TimerAInit();                 //TimerAInit Function call
    TimerBInit();                 //TimerBInit Function call
    UARTInit();                   //UARTInit Function call


    while (REFCTL0 & REFGENBUSY);            // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_0 + REFON;            // Enable internal 1.2V reference

    ADC12Init();                             //ADC10 Function call

    while (!(REFCTL0 & REFGENRDY));          // Wait for reference generator
    __enable_interrupt();                    //Enable interrupts.

    while (1)
    {

        __bis_SR_register(LPM0 + GIE); // Enter LPM0, interrupts enabled
        __no_operation();              // For debugger
    }
}
//ADC ISR
#pragma vector=EUSCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch (__even_in_range(UCA0IV, USCI_UART_UCTXCPTIFG))
    {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
        while (!(UCA0IFG&UCTXIFG));     //Is TX ready?

        TB0CCR1 = 255 - UCA0RXBUF;      //duty cycle of FAN
        __no_operation();
        break;

    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;

    default:
        break;
    }
}

void PinInit(void)
{
    //For pin 1.4, P1DIR = 1, P1SEL0 = 1, P1SEL1 = 0.
    P1DIR |= OUTPUT; //Pin 1.4
    P1SEL1 &= ~OUTPUT;
    P1SEL0 |= OUTPUT;
}

void TimerBInit(void)
{
    TB0CCTL1 = OUTMOD_3;      //set/reset mode
    TB0CCR1 = 254;            //Set initial CCR1
    TB0CCR0 = 255 - 1;        //Set CCR0 for a ~1kHz clock.
    TB0CTL = TBSSEL_2 + MC_1; //Enable Timer B0 with SMCLK and up mode.
}

void UARTInit(void)
{

    CSCTL0_H = CSKEY_H;                         // Unlock CS registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;               // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;       // Set all dividers
    CSCTL0_H = 0;                               // Lock CS registers

    P2SEL0 &= ~(BIT0 | BIT1);                   //P2.0 => RXD
    P2SEL1 |= BIT0 + BIT1;                      //P2.1 => TXD

                                                  // Configure USCI_A0 for UART mode
    UCA0CTLW0 = UCSWRST;                        // Put eUSCI in reset
    UCA0CTLW0 |= UCSSEL__SMCLK;                 // CLK = SMCLK
    UCA0BRW = 52;                               // 8000000/16/9600
    UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;     //modulation
    UCA0CTLW0 &= ~UCSWRST;                      // Initialize eUSCI
    UCA0IE |= UCRXIE;                           // Enable USCI_A0 RX interrupt
}
void ADC12Init(void)
{
    ADC12CTL0 = ADC12SHT0_2 + ADC12ON;      // Set sample time
    ADC12CTL1 = ADC12SHP;                   // Enable sample timer
    ADC12CTL2 |= ADC12RES_2;                // 12-bit conversion results
    ADC12MCTL0 = ADC12INCH_5 | ADC12VRSEL_1;// Vref+ , Input channel A5
    ADC12IER0 |= ADC12IE0;                  // Enable ADC conv complete interrupt
    P1OUT = BIT0;

}
void GPIOInit()
{
    P1OUT &= ~BIT0;                         // Clear LED to start
    P1DIR |= BIT0;                          // P1.0 output
    P1SEL1 |= ADC12;                        // Configure P1.5 for ADC
    P1SEL0 |= ADC12;

}
void TimerAInit(void) {
    TA0CCTL0 = CCIE;                       //Timer A interrupt enable
    TA0CCTL1 = OUTMOD_3;                   //Set/ Reset mode
    TA0CCR1 = 256;                         //Duty cycle CCR1
    TA0CCR0 = 4096 - 1;                    //Set period
    TA0CTL = TASSEL_1 + MC_1 + ID_3;       //ACLK, UPMODE, DIV 4
}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
 ADC12CTL0 |= ADC12SC | ADC12ENC;       //ADC12 start conversion, ADC12 enable
 //voltage = in * 0.00029;           //converts ADC to voltage
 //tempC = voltage / 0.01;           //converts voltage to Temp C
 //tempF = ((9 * tempC) / 5) + 32;   //Temp C to Temp F
 //UCA0TXBUF = tempC;                //send tempF to TX
}
//ADC ISR
#pragma vector=ADC12_B_VECTOR
__interrupt void ADC12ISR(void)
{
    in = ADC12MEM0;
    voltage = in * 0.00029;           //converts ADC to voltage
    tempC = voltage / 0.01;           //converts voltage to Temp C
    tempF = ((9 * tempC) / 5) + 32;   //Temp C to Temp F
    while (!(UCA0IFG&UCTXIFG));
    UCA0TXBUF = tempF;                //send tempF to TX
}
