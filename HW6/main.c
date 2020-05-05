#include "font.h"
#include "i2c_master_noint.h"
#include "ssd1306.h"
#include "imu.h"
#include <stdio.h>
#include <math.h>
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

void xBar(signed short Xaxis_data);
void yBar(signed short Yaxis_data);

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
    
    LATBbits.LATB13 = 0; // turn off the WS2812b's bc they're bright af
    
    i2c_master_setup(); // enable I2C communication
    ssd1306_setup();    // enable use of LCD screen via I2C
    imu_setup();   // initiate accelerometer, gyroscope, and ability to read from multiple registers in a row

    __builtin_enable_interrupts();
    
    char message[50];
   
    signed short IMUdata[7];
    
    int HBcounter=0;

    while(1) {
        switch (HBcounter) {
            case 25:
                LATAbits.LATA4 = !LATAbits.LATA4;
                HBcounter=0;
        }
        
        imu_read(IMU_ADDR,IMU_OUT_TEMP_L,IMUdata,7);
        
        if (0) {
            sprintf(message,"T: %0.1f C",0.0019074068*(IMUdata[0])+25);
            drawString(message,0,0,1);
            sprintf(message,"a[X]: %d   ",IMUdata[4]);
            drawString(message,0,8,1);
            sprintf(message,"a[Y]: %d   ",IMUdata[5]);
            drawString(message,0,16,1);
            sprintf(message,"a[Z]: %d   ",IMUdata[6]);
            drawString(message,0,24,1);
            ssd1306_update();
        } else {
            xBar(-IMUdata[5]);
            yBar(IMUdata[4]);
            ssd1306_update();
        }
        HBcounter++;
    }
}

void yBar(signed short Yaxis_data) {
    int i,y;
    y = (int)((float)(Yaxis_data) / 16384.0 * 16.0 + 16.0);
    if (y<16) {
        for (i=16;i>=y;i--) {ssd1306_drawPixel(64,i,1);}
        for (i=y;i>=0;i--) {ssd1306_drawPixel(64,i,0);}
    } else if (y>16) {
        for (i=16;i<=y;i++) {ssd1306_drawPixel(64,i,1);}
        for (i=y;i<32;i++) {ssd1306_drawPixel(64,i,0);}
    } else {
        for (i=0;i<16;i++) {ssd1306_drawPixel(64,i,0);}
        for (i=32;i>16;i--) {ssd1306_drawPixel(64,i,0);}
    }
}

void xBar(signed short Xaxis_data) {
    int i,x;
    x = (int)((float)(Xaxis_data) / 16384.0 * 64.0 + 64.0);
    if (x<64) {
        for (i=64;i>=x;i--) {ssd1306_drawPixel(i,16,1);}
        for (i=x;i>=0;i--) {ssd1306_drawPixel(i,16,0);}
    } else if (x>64) {
        for (i=64;i<=x;i++) {ssd1306_drawPixel(i,16,1);}
        for (i=x;i<128;i++) {ssd1306_drawPixel(i,16,0);}
    } else {
        for (i=0;i<64;i++) {ssd1306_drawPixel(i,16,0);}
        for (i=127;i>64;i--) {ssd1306_drawPixel(i,16,0);}
    }
}