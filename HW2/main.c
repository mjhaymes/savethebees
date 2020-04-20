#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<stdio.h>
#include<math.h>         // imported for use of sin()
#include "spi.h"

#define PI 3.14159265 // used for sine function

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0; // A4 is an output
    LATAbits.LATA4 = 0; // A4 is initially off
    TRISBbits.TRISB4 = 1; // B4 is an input
    
    initSPI();

    __builtin_enable_interrupts();

    unsigned int v;
    unsigned int c;
    unsigned int p;
    
    ////////// SETTING UP TRIANGLE WAVE'S DAC VOLTAGE CODE ARRAY //////////
    
    int i; // initiate triangle wave array counter
    int triangle[100]; // 100 discrete voltage values per second for 1 triangle wave period (1Hz)
                       // infinite while loop loops every 1/100th of a second via core timer (see below)
    float temp[100];
    
    triangle[0] = 0; // initial voltage = 0
    temp[0] = 0.0;
    
    for (i=1; i<100; i++) { 
        if (i<=50) { // increasing (1st half of triangle wave)
            
            temp[i] = temp[i-1]+81.92; // 0 to 3.3V in 50 steps --> 3.3[V]/50[steps] = 0.066[V/step]
                                       // (0.066V/step * 4096[DAC code increments])/3.3V = 81.92[DAC code increments/step]
            
            // make DAC code an integer so it can be converted to a 12-bit number (0-4096)
            triangle[i] = (int) temp[i]; 
            
        } else { // decreasing (2nd half of triangle wave)
            temp[i] = temp[i-1]-81.92; 
            triangle[i] = (int) temp[i];
        }
    }
    
    ////////// SETTING UP TRIANGLE WAVE'S DAC VOLTAGE CODE ARRAY //////////
    
    int j; // initiate sine wave array counter
    int sine[50]; //50 discrete voltage values *per second* for 1 sine wave period (2Hz)
                  //*infinite while loop loops every 1/100th of a second via core timer (see below)*
    
    sine[0] = 2048; // initial DAC value - sine wave starts in between 0 and 3.3V
    
    for (i=1; i<50; i++) {
        // using wave function for an amplitude of 1.65V (2048 DAC)
        // frequency of 2Hz --> 2*PI*2Hz = 4*PI)
        // t = 1/100th of a second * step (infinite while loop loops every 1/100th of a second via core timer; see below)
        // shift function up by 2048 DAC so that it oscillates b/t 0 and 3.3V
        // make each value an integer so that it can be used as a 12-bit number
        sine[i] = (int) (2048*sin(4*PI*(0.01*i)) + 2048); 
    }
    
    i = 0; // set triangle wave counter to 0
    j = 0; // set sine wave counter to 0
    while (1) {
        
        // writing the triangle wave voltages to channel A
        
        v = triangle[i]; // discrete voltage value from triangle array
        c = 0b0; // (channel) bit indicates channel A
        
        //setting up 16-bit number
        p = c<<15|(0b111<<12)|v; // c: bit 15
                                 // 0b111: bits 14-12 (0b111 corresponds to: |buffered|gain of 1|active mode operation| )
                                 // v: bits 11-0 (the 12 bit voltage we store in wave array)
        
        // write 2 bytes over SPI1
        LATAbits.LATA0 = 0; // bring CS low
        spi_io(p>>8); // write the 1st byte
        spi_io(p); // write 2nd byte (1st byte truncated)
        LATAbits.LATA0 = 1; // bring CS high
        
        // writing the sine wave voltages to channel B
        
        v = sine[j]; // discrete voltage value from sine array
        c = 0b1; // bit indicates channel B
        
        // setting up 16-bit number
        p = c<<15|(0b111<<12)|v;
        
        // write 2 bytes over SPI1
        LATAbits.LATA0 = 0; // bring CS low
        spi_io(p>>8); // write the 1st byte
        spi_io(p); // write 2nd byte
        LATAbits.LATA0 = 1; // bring CS high
        
        // increment array counters
        i++;
        j++;
        
        if (i==100) { // reset triangle wave counter so it loops through size 100 array
            i = 0; 
        }
        if (j==50) { // reset sine wave counter so it loops through size 50 array
            j = 0; 
        }
        
        _CP0_SET_COUNT(0);
        while (_CP0_GET_COUNT()<48000000/200) {
            ; // wait 1/100th of a second until loop
        }
    }
}
