#include "i2c_master_noint.h"
#include "ssd1306.h"
#include "font.h"
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
    
    i2c_master_setup(); // setup I2C stuff
    ssd1306_setup();    // setup LCD screen
    
    setPin(IODIRA,0x00);       // make all MCP A pins outputs
    setPin(IODIRB,0xFF);       // make all MCP B pins inputs

    __builtin_enable_interrupts();
    
    char message[50];
    int answer = 0;
    int counter = 0;
    int dt = 0;
    while (1) {
        _CP0_SET_COUNT(0);
        sprintf(message,"How much wood could Nick");
        drawString(message,0,0,1);
        
        sprintf(message,"Marchuk chuck if Nick");
        drawString(message,0,8,1);
        
        sprintf(message,"Marchuk could chuck wood?");
        drawString(message,0,16,1);
        
        sprintf(message,"Answer:%d",answer);
        drawString(message,0,25,1);

        ssd1306_update();
        
        dt = _CP0_GET_COUNT();
        
        sprintf(message,"FPS=%0.2f",(float) 24000000/dt);
        drawString(message,80,25,1);
        answer++;
        counter++;
        if (counter==10) {
            counter=0;
            LATAbits.LATA4 = !LATAbits.LATA4;
        }
    }
}