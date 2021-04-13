#include "font.h"
#include "i2c_master_noint.h"
#include "ssd1306.h"
#include "adc.h"
#include "ws2812b.h"
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

int adc_read_avg(int pin, int delay);
float get_position();

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
    
    i2c_master_setup(); // enable I2C communication
    ssd1306_setup();    // enable use of LCD screen via I2C
    adc_setup();
    ctmu_setup();
    ws2812b_setup();
    
    int numLEDs = 4;
    wsColor color_array[numLEDs];

    __builtin_enable_interrupts();
    
    char message[50];
    
    int HBcounter=0;
    float pos;

    int InpA;
    int InpB;
    
    float b;
    
    while(1) {
        
        if(0) {
            color_array[0] = HSBtoRGB(0,1,.1);
            color_array[1] = HSBtoRGB(90,1,.1);
            color_array[2] = HSBtoRGB(180,1,.1);
            color_array[3] = HSBtoRGB(270,1,.1);
            ws2812b_setColor(color_array, numLEDs);
            sprintf(message,"Input A: %d ",adc_read_avg(5,48000000/2/800000));
            drawString(message,0,0,1);
            sprintf(message,"Input B: %d ",adc_read_avg(4,48000000/2/800000));
            drawString(message,0,8,1);
            ssd1306_update();
        } else {
            InpA = adc_read_avg(5,48000000/2/975000);
            InpB = adc_read_avg(4,48000000/2/975000);
            pos = get_position();
            sprintf(message,"Position: %0.2f ",pos);
            drawString(message,0,0,1);
            sprintf(message,"Input A: %d ",InpA);
            drawString(message,0,10,1);
            sprintf(message,"Input B: %d ",InpB);
            drawString(message,0,20,1);
            
            if ((InpA < 600) || (InpB < 600)) { // threshold for if the foil is touched or not     
                
                // setting brightness of LEDs based on the position we're touching
                if (pos>0.0) {
                    b = pos / 45.0; // scale brightness with position
                    color_array[0] = HSBtoRGB(0,1,0.5);
                    color_array[1] = HSBtoRGB(0,1,b/2);
                    color_array[2] = HSBtoRGB(180,1,0);
                    color_array[3] = HSBtoRGB(180,1,0);       
                } else if (pos<0.0) {
                    b = -pos / 45.0;
                    color_array[0] = HSBtoRGB(0,1,0);
                    color_array[1] = HSBtoRGB(0,1,0);
                    color_array[2] = HSBtoRGB(180,1,b/2);
                    color_array[3] = HSBtoRGB(180,1,0.5); 
                }
            } else { // if foil is not touched turn off the LEDs
                color_array[0] = HSBtoRGB(0,1,0);
                color_array[1] = HSBtoRGB(0,1,0);
                color_array[2] = HSBtoRGB(180,1,0);
                color_array[3] = HSBtoRGB(180,1,0);
            }
            ws2812b_setColor(color_array, numLEDs);
            ssd1306_update();
        }
        switch (HBcounter) {
            case 5:
                LATAbits.LATA4 = !LATAbits.LATA4;
                HBcounter=0;
                
        }
        HBcounter++;
    }
}

int adc_read_avg(int pin, int delay) {
    int i;
    int sum=0;
    for (i=0;i<10;i++) {
        sum = sum + ctmu_read(pin,delay);
    }
    int avg = (sum/10);
    return avg; // return the average adc value over 15 reads
}

float get_position() {
    float A_base = 955.0;
    float B_base = 975.0;
    float delta_A = A_base - (float)(adc_read_avg(5,48000000/2/975000));
    float delta_B = B_base - (float)(adc_read_avg(4,48000000/2/975000));
    
    float A_pos = (delta_A*100)/(delta_A + delta_B);
    float B_pos = ((1-delta_B)*100)/(delta_A + delta_B);
    float pos = (A_pos + B_pos)/2;
    return pos;
}