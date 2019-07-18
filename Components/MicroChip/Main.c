#include <xc.h>
#include <stdint.h>
#include <p33EP512GM710.h>
#include <pps.h>
#include <uart.h>

#pragma config JTAGEN = OFF         // JTAG Enable bit (JTAG is disabled)

// FPOR
#pragma config BOREN = ON           // Brown-out Reset (BOR) Detection Enable bit (BOR is enabled)
#pragma config ALTI2C1 = OFF        // Alternate I2C1 pins (I2C1 mapped to SDA1/SCL1 pins)
#pragma config ALTI2C2 = OFF        // Alternate I2C2 pins (I2C2 mapped to SDA2/SCL2 pins)
#pragma config WDTWIN = WIN25       // Watchdog Window Select bits (WDT Window is 25% of WDT period)
#pragma config ICS = PGD2           // ICD Communication Channel Select bits (Communicate on PGEC1 and PGED1)

// FWDT
#pragma config WDTPOST = PS32768    // Watchdog Timer Postscaler bits (1:32,768)
#pragma config WDTPRE = PR128       // Watchdog Timer Prescaler bit (1:128)
#pragma config PLLKEN = ON          // PLL Lock Enable bit (Clock switch to PLL source will wait until the PLL lock signal is valid.)
#pragma config WINDIS = OFF         // Watchdog Timer Window Enable bit (Watchdog Timer in Non-Window mode)
#pragma config FWDTEN = OFF         // Watchdog Timer Enable bit (Watchdog timer enabled/disabled by user software)

// FOSC
#pragma config POSCMD = XT          // Primary Oscillator Mode Select bits (XT Crystal Oscillator Mode)
#pragma config OSCIOFNC = OFF       // OSC2 Pin Function bit (OSC2 is clock output)
#pragma config IOL1WAY = OFF        // Peripheral pin select configuration (Allow multiple reconfigurations)
#pragma config FCKSM = CSECMD       // Clock Switching Mode bits (Clock switching is enabled,Fail-safe Clock Monitor is disabled)

// FOSCSEL
#pragma config FNOSC = FRC          // Oscillator Source Selection (Internal Fast RC (FRC))
#pragma config PWMLOCK = OFF        // PWM Lock Enable bit is off
#pragma config IESO = OFF           // Two-speed Oscillator Start-up Enable bit (Start up with user-selected oscillator source)

// FGS
#pragma config GWRP = OFF           // General Segment Write-Protect bit (General Segment may be written)
#pragma config GCP = OFF            // General Segment Code-Protect bit (General Segment Code protect is Disabled)


// Define the LEDs of the Development Board
#define LED6 LATCbits.LATC9
#define LED5 LATDbits.LATD6
#define LED4 LATDbits.LATD5
#define LED3 LATCbits.LATC8
#define LED2 LATDbits.LATD13
#define LED1 LATDbits.LATD12

unsigned int data1, data2, data3, data4;
unsigned char U2data, frontbit;
unsigned char flag1,flag2;

unsigned int U1MODEvalue;                 //Holds the value of uart config reg
unsigned int U1STAvalue;                      //Holds the information regarding uart TX & RX interrupt modes


/******************************************************************************/
/******************* Interruption service routines ****************************/
/******************************************************************************/
void __attribute__ ( (interrupt, no_auto_psv) ) _T1Interrupt( void )
{
    IFS0bits.T1IF = 0;
    flag1 = 1;
}

void __attribute__ ( (interrupt, no_auto_psv) ) _U1RXInterrupt( void )
{
    while(!U1STAbits.URXDA);
    data1 = U1RXREG;
    flag2 = 1;
    U1STAbits.OERR = 0;
    IFS0bits.U1RXIF = 0;
}


/******************************************************************************/
/**************************** Configuration ***********************************/
/******************************************************************************/
void Configuration(void)
{
    // Configure Watchdog timer
    RCONbits.SWDTEN = 0;                        // Disable Dog Watch Timer

    // Configure Oscillator
    // Use equations: Fosc= Fin*M/(N1*N2), Fcy=Fosc/2. To operate at 70MIPS use Fin = 8MHz (external XT), M = 70, N1 = 2, N2 = 2
    PLLFBD = 68;                                // M = (PLLBD + 2) = 70
    CLKDIVbits.PLLPOST = 1;                     // N2 = 2*(PLLPOST + 1) = 2
    CLKDIVbits.PLLPRE = 0;                        // N1 = (PLLPRE + 1) = 2
    __builtin_write_OSCCONH( 0x03 );            // Initiate Clock Switch to Primary
    __builtin_write_OSCCONL( OSCCON || 0x01 );  // Start clock switching
    while( OSCCONbits.COSC != 0b011 );
    while (OSCCONbits.LOCK != 1);

    // Configure Timer 1
    T1CON = 0;          // Timer reset
    T1CONbits.TCKPS = 0b11;
    IFS0bits.T1IF = 0;  // Reset Timer1 interrupt flag
    IPC0bits.T1IP = 1;  // Timer1 Interrupt priority level=4
    IEC0bits.T1IE = 1;  // Enable Timer1 interrupt
    PR1 = 0xFFFF;       // Timer1 period register

    // Configure PWM Clock
    PTCONbits.PTEN = 0;             // Keep PWM module off for now
    PTCONbits.PTSIDL = 0;
    PTCONbits.SESTAT = 0;
    PTCONbits.EIPU = 1;
    PTCON2bits.PCLKDIV = 0b001;
    PTPER = 6999;
    PWMKEY = 0;
    PWMCON2bits.FLTIEN = 0;         // Disable Fault Interrupt
    PWMCON2bits.CLIEN = 0;          // Disable Current-Limit Interrupt
    PWMCON2bits.TRGIEN = 0;         // Disable Trigger Interrupt
    PWMCON2bits.MDCS = 0;           // Duty cycle is determined by PDCx and SDCx instead of the Master Duty cycle
    PWMCON2bits.DTC = 0b10;         // Dead time function is disabled
    PWMCON2bits.MTBS = 0;           // Use primary master time base
    PWMCON2bits.CAM = 0;            // Edge aligned mode selected
    PWMCON2bits.XPRES = 0;          // External pins do not affect PWM time base
    PWMCON2bits.IUE = 1;            // Immediate updates
    IOCON2bits.PENH = 1;            // PWM2H is PWM pin
    IOCON2bits.PENL = 1;            // PWM2L is PWM pin
    IOCON2bits.POLH = 0;            // Polarity is active high
    IOCON2bits.POLL = 0;            // Polarity is active high
    IOCON2bits.PMOD = 0b11;         // Independent mode
    IOCON2bits.OVRENH = 0;
    IOCON2bits.OVRENL = 0;
    FCLCON2bits.IFLTMOD = 1;
    FCLCON2bits.CLMOD = 0;
    FCLCON2bits.FLTMOD = 0b11;
    PDC2 = 3500;
    SDC2 = 5250;

    // Configure USB Communication
    CloseUART1();
    //U1BRG = 4374;               // Baud rate = 100k
//    U1MODEbits.UARTEN = 0;      // Keep UART2 disabled for now
//    U1MODEbits.USIDL = 0;       // Continue operation in idle mode
//    U1MODEbits.IREN = 0;        // Disable irDA encoder/decoder
//    U1MODEbits.RTSMD = 1;       // Simplex Mode
//    U1MODEbits.UEN = 0b00;      // Flow control pins are controlled by PORT latches
//    U1MODEbits.ABAUD = 0;       // Disable Baud rate measurement
//    U1MODEbits.URXINV = 0;      // Idle state is one
//    U1MODEbits.BRGH = 0;        // BRG in low speed mode
//    U1MODEbits.PDSEL = 0b00;    // 8-bit no parity
//    U1MODEbits.STSEL = 0;       // One stop bit
//    U1MODEbits.LPBACK = 0;      // No loopback
//    U1STAbits.UTXINV = 0;       // U2TX idle state is 1
//    U1STAbits.UTXBRK = 0;       // Sends Sync Break is disabled
//    U1STAbits.UTXEN = 0;        // Transmit is disabled for now
//    U1STAbits.ADDEN = 0;        // Address character detect is disables
//    U1STAbits.OERR = 0;
//    U1STAbits.URXISEL = 0b00;

    // Configure the ports
    TRISC = 0x0000;
    TRISD = 0x0000;
    TRISBbits.TRISB5 = 1;
    TRISBbits.TRISB6 = 1;
    TRISBbits.TRISB12 = 0;      // PWM pin is output
    TRISBbits.TRISB13 = 0;      // PWM pin is output
    TRISAbits.TRISA8 = 1;       // U2RX is input
    TRISBbits.TRISB4 = 0;       // U2TX is output
    LATBbits.LATB4 = 1;

    // Configure Peripheral Ports
    // 1. UART to USB
    PPSInput(IN_FN_PPS_U1RX, IN_PIN_PPS_RPI24);
    //RPINR18bits.U1RXR = 0b0011000;          // Connect RPI24 to U1RX
    RPOR1bits.RP36R = 0b000001;             // Connect RP36 to U1TX
}

/******************************************************************************/
/**************************** Initialization **********************************/
/******************************************************************************/
void Initialization(void)
{
    LED4 = 1;
    
    INTCON2bits.GIE = 1;                    // Global Interruptions are enabled

    data1 = 0;

    TMR1 = 0;
    T1CONbits.TON = 1;

    PTCONbits.PTEN = 0;         // Enable bit for PWM module

//    U1STAbits.OERR = 0;         // Clear overrun errors
//    U1STAbits.FERR = 0;         // Clear overrun errors
    IEC0bits.U1RXIE = 1;        // Turn on receive interruption
    IFS0bits.U1RXIF = 0;        // clear the interrupt flag
    IPC2bits.U1RXIP = 7;        // Maximum priority
//    U1MODEbits.UARTEN = 1;      // Enable UART2

    U1MODEvalue = UART_EN & UART_IDLE_CON &                      // Configure UART1 module to transmit
              UART_IrDA_DISABLE & UART_BRGH_FOUR  &          // 8 bit data with one stopbit, no parity
              UART_EN_WAKE & UART_DIS_LOOPBACK &
              UART_DIS_ABAUD & UART_NO_PAR_8BIT &
              UART_1STOPBIT & UART_UXRX_IDLE_ONE;

    U1STAvalue  = UART_INT_RX_CHAR & UART_TX_ENABLE &
              UART_ADR_DETECT_DIS &                           //   UART_INT_TX &
              UART_IrDA_POL_INV_ZERO & UART_RX_INT_EN &
              UART_RX_OVERRUN_CLEAR;

    OpenUART1(U1MODEvalue, U1STAvalue, 8);

}




/******************************************************************************/
/******************************* Main *****************************************/
/******************************************************************************/
int main( void )
{
    Configuration();
    Initialization();

    while(1)
    {
        if (flag1)
        {
            LED6 = LED6-1;
            flag1 = 0;
        }
        if (flag2)
        {
            U1TXREG = data1;
            while(!U1STAbits.TRMT);
            LED4 = LED4-1;
            flag2 = 0;
        }
    }
}