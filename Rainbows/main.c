#include "ws2812b.h"
#include "font.h"
#include "i2c_master_noint.h"
#include "ssd1306.h"
#include <stdio.h>
#include <sys/attribs.h>  // __ISR macro

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

void readRGB(wsColor * color_array);

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
    TRISAbits.TRISA4 = 0; // Configure A4 as the "heartbeat" output
    LATAbits.LATA4 = 0; // A4 is initially off
    TRISBbits.TRISB4 = 1; // B4 is an input
    
    i2c_master_setup();
    ssd1306_setup();
    ws2812b_setup();

    __builtin_enable_interrupts();
    
    int numLEDs = 4;
    wsColor color_array[numLEDs];
    
    int i3=0;   // initial hue of LED1
    int i2=300;  // initial hue of LED2
    int i1=600; // initial hue of LED3
    int i0=900; // initial hue of LED4
    
    int dt = 1;
    int LCDcounter = 0;
    int HBcounter = 0;
    while (1) {    
        color_array[0] = HSBtoRGB(i0/10, 1, 1);
        color_array[1] = HSBtoRGB(i1/10, 1, 1);
        color_array[2] = HSBtoRGB(i2/10, 1, 1);
        color_array[3] = HSBtoRGB(i3/10, 1, 1);
        
        i0 = i0+dt;
        i1 = i1+dt;
        i2 = i2+dt;
        i3 = i3+dt;
        
        ws2812b_setColor(color_array, numLEDs);
        
        switch (i0) {
            case 3601:
                i0=0;
        }
        switch (i1) {
            case 3601:
                i1=0;
        }
        switch (i2) {
            case 3601:
                i2=0;
        }
        switch (i3) {
            case 3601:
                i3=0;
        }
        switch (LCDcounter) {
            case 75:
                LCDcounter=0;
                readRGB(color_array);
                ssd1306_update();
        }
        switch (HBcounter) {
            case 250:
                HBcounter=0;
                LATAbits.LATA4 = !LATAbits.LATA4;
        }
        LCDcounter++;
        HBcounter++;
    }
}

void readRGB(wsColor * color_array) {
    int k=0;
    char m[50];
    for (k=0;k<4;k++) {
        sprintf(m,"LED%d:   %d   %d   %d   ",k+1,color_array[k].r,color_array[k].g,color_array[k].b);
        drawString(m,0,k*8,1);
    }
}