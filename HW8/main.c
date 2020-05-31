#include "font.h"
#include "i2c_master_noint.h"
#include "ssd1306.h"
#include "rtcc.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/attribs.h>  // __ISR macro

// DEVCFG0
#pragma config DEBUG = OFF          // disable debugging
#pragma config JTAGEN = OFF         // disable jtag
#pragma config ICESEL = ICS_PGx1    // use PGED1 and PGEC1
#pragma config PWP = OFF            // disable flash write protect
#pragma config BWP = OFF            // disable boot write protect
#pragma config CP = OFF             // disable code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL       // use primary oscillator with pll
#pragma config FSOSCEN = ON         // enable secondary oscillator
#pragma config IESO = OFF           // disable switching clocks
#pragma config POSCMOD = HS         // high speed crystal mode
#pragma config OSCIOFNC = OFF       // disable clock output
#pragma config FPBDIV = DIV_1       // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD       // disable clock switch and FSCM
#pragma config WDTPS = PS1048576    // use largest wdt
#pragma config WINDIS = OFF         // use non-window mode wdt
#pragma config FWDTEN = OFF         // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

int time;
int date;
void setClock();

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
    ANSELBbits.ANSB15 = 0; // make B15 digital I/O
    TRISBbits.TRISB15 = 1; // Configure B15 as an input
    ANSELBbits.ANSB12 = 0; // make B12 digital I/O
    TRISBbits.TRISB12 = 1; // Configure B12 as an input
    
    i2c_master_setup(); // enable I2C communication
    ssd1306_setup();    // enable use of LCD screen via I2C
    
    setClock();
    
    rtcc_setup(time, date);
    
    __builtin_enable_interrupts();
    
    rtccTime t;
    char message[50];
    char day[12];
    int counter=0;
    
    while(1) {
        
        ssd1306_clear();
        t = readRTCC();
        sprintf(message,"%d%d:%d%d:%d%d",t.hr10,t.hr01,t.min10,t.min01,t.sec10,t.sec01);
        drawString(message,8,16,1);
        dayOfTheWeek(t.wk, day);
        sprintf(message,"%s, %d%d/%d%d/20%d%d",day,t.mn10,t.mn01,t.dy10,t.dy01,t.yr10,t.yr01);
        drawString(message,8,24,1);
        sprintf(message,"%d updates",counter);
        drawString(message,0,0,1);
        _CP0_SET_COUNT(0);
        while (_CP0_GET_COUNT() < 48000000/2/2) {} // 2 Hz
        ssd1306_update();
        counter++;
        
    }
}

void setClock() {
    char day[12];
    char hr=0,min=0,sec=0,mn=1,dy=1,wk=0;
    char message[50];
    while (PORTBbits.RB12!=0) { // while this button is not pushed
        ssd1306_clear();
        sprintf(message,"Month: %d",mn);
        drawString(message,0,0,1);
        ssd1306_update();
        if (PORTBbits.RB15==0) { // if increment button is pushed
            mn++; // then increment 1
        }
        while (PORTBbits.RB15==0) {} // if increment button is still pushed, wait until it isn't
        if (mn>12) {
            mn=1;
        }
    }
    while (PORTBbits.RB12==0) {} // while continue button is still pushed just wait
    while (PORTBbits.RB12!=0) { // while this button is not pushed
       ssd1306_clear();
       sprintf(message,"Day: %d",dy);
       drawString(message,0,0,1);
       ssd1306_update();
       if (PORTBbits.RB15==0) { // if increment button is pushed
           dy++; // then increment 1
       }
       while (PORTBbits.RB15!=1) {} // if increment button is still pushed, wait until it isn't
       if (dy>31) {
           dy=1;
       }
    }
    while (PORTBbits.RB12==0) {} // while continue button is still pushed just wait
    while (PORTBbits.RB12!=0) { // while this button is not pushed
       ssd1306_clear();
       dayOfTheWeek(wk,day);
       sprintf(message,"Weekday: %s",day);
       drawString(message,0,0,1);
       ssd1306_update();
       if (PORTBbits.RB15==0) { // if increment button is pushed
           wk++; // then increment 1
       }
       while (PORTBbits.RB15!=1) {} // if increment button is still pushed, wait until it isn't
       if (wk>6) {
           wk=0;
       }
    }
    while (PORTBbits.RB12==0) {} // while continue button is still pushed just wait
    while (PORTBbits.RB12!=0) { // while this button is not pushed
       ssd1306_clear();
       sprintf(message,"Hour: %d",hr);
       drawString(message,0,0,1);
       ssd1306_update();
       if (PORTBbits.RB15==0) { // if increment button is pushed
           hr++; // then increment 1
       }
       while (PORTBbits.RB15!=1) {} // if increment button is still pushed, wait until it isn't
       if (hr>24) {
           hr=0;
       }
    }
    while (PORTBbits.RB12==0) {} // while continue button is still pushed just wait
    while (PORTBbits.RB12!=0) { // while this button is not pushed
       ssd1306_clear();
       sprintf(message,"Minute: %d",min);
       drawString(message,0,0,1);
       ssd1306_update();
       if (PORTBbits.RB15==0) { // if increment button is pushed
           min++; // then increment 1
       }
       while (PORTBbits.RB15!=1) {} // if increment button is still pushed, wait until it isn't
       if (min>59) {
           min=0;
       }
    }
    while (PORTBbits.RB12==0) {} // while continue button is still pushed just wait
    while (PORTBbits.RB12!=0) { // while this button is not pushed
       ssd1306_clear();
       sprintf(message,"Second: %d",sec);
       drawString(message,0,0,1);
       ssd1306_update();
       if (PORTBbits.RB15==0) { // if increment button is pushed
           sec++; // then increment 1
       }
       while (PORTBbits.RB15!=1) {} // if increment button is still pushed, wait until it isn't
       if (sec>59) {
           sec=0;
       }
    }
    ssd1306_clear();
    sprintf(message,"%02d%02d%02d00",hr,min,sec);
    time = (int)strtol(message, NULL, 16);
    sprintf(message,"20%02d%02d%02d",mn,dy,wk);
    date = (int)strtol(message, NULL, 16);
}
