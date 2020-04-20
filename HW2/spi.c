#include<xc.h>           // processor SFR definitions
#include "spi.h" 

// initialize SPI1
void initSPI() {
    // Pin B14 has to be SCK1
    // Turn off analog pins
    ANSELA = 0;
    // Make A0 an output pin for CS
    TRISAbits.TRISA0 = 0;
    LATAbits.LATA0 = 1; // configured to be high
    // Make A1 SDO1
    RPA1Rbits.RPA1R = 0b0011;
    // Make B5 SDI1
    SDI1Rbits.SDI1R = 0b0001;

    // setup SPI1
    SPI1CON = 0; // turn off the spi module and reset it
    SPI1BUF; // clear the rx buffer by reading from it
    SPI1BRG = 1; // 1000 for 12kHz, 1 for 12MHz; // baud rate to 10 MHz [SPI1BRG = (48000000/(2*desired))-1]
    SPI1STATbits.SPIROV = 0; // clear the overflow bit
    SPI1CONbits.CKE = 1; // data changes when clock goes from hi to lo (since CKP is 0)
    SPI1CONbits.MSTEN = 1; // master operation
    SPI1CONbits.ON = 1; // turn on spi   
}

// send a byte via SPI and return the response
unsigned char spi_io(unsigned char o) {
    SPI1BUF = o;
    while (!SPI1STATbits.SPIRBF) {
        ; // wait to recieve the byte
    }
    return SPI1BUF;
}