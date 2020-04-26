#include <xc.h>
#include <sys/attribs.h>  // __ISR macro

#define MCP23017 0b01000000
#define IODIRA 0x00 
#define IODIRB 0x01 
#define IPOLA 0x02 
#define IPOLB 0x03 
#define GPINTENA 0x04 
#define GPINTENB 0x05 
#define GPPUA 0x0C 
#define GPPUB 0x0D 
#define GPIOA 0x12 
#define GPIOB 0x13 
#define OLATA 0x14 
#define OLATB 0x15

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

void i2c_master_setup(void);
void i2c_master_start(void);
void i2c_master_restart(void);
void i2c_master_send(unsigned char byte);
unsigned char i2c_master_recv(void);
void i2c_master_ack(int val);
void i2c_master_stop(void);
void setPin(unsigned char address, unsigned char reg, unsigned char value);
unsigned char readPin(unsigned char address, unsigned char reg);

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
    
    setPin(MCP23017,IODIRA,0x00);       // make all A pins outputs
    setPin(MCP23017,IODIRB,0xFF);       // make all B pins inputs

    __builtin_enable_interrupts();

    while (1) {
        _CP0_SET_COUNT(0);
        while (_CP0_GET_COUNT()<48000000/24) {
            ;
        }
        LATAbits.LATA4 = !LATAbits.LATA4;
        if ((readPin(MCP23017,GPIOB)&0b00000001) == 1) { // if GPB0 is high (button not pressed)
            setPin(MCP23017,OLATA,0b00000000);  // make pin GPA7 output low
        } else if ((readPin(MCP23017,GPIOB)&0b00000001) == 0) { // if GPB0 is low (button pressed)
            setPin(MCP23017,OLATA,0b10000000);  // make pin GPA7 output high
        }
    }
}

void i2c_master_setup(void) {
    // using a large BRG to see it on the nScope, make it smaller after verifying that code works
    // TPGD is typically 104ns (min. 52ns / max. 312ns)
    I2C1BRG = 1000; // I2CBRG = [1/(2*Fsck) - TPGD]*Pblck - 2 (TPGD is the Pulse Gobbler Delay)
    I2C1CONbits.ON = 1; // turn on the I2C1 module
}

void i2c_master_start(void) {
    I2C1CONbits.SEN = 1; // send the start bit
    while (I2C1CONbits.SEN) {
        ;
    } // wait for the start bit to be sent
}

void i2c_master_restart(void) {
    I2C1CONbits.RSEN = 1; // send a restart 
    while (I2C1CONbits.RSEN) {
        ;
    } // wait for the restart to clear
}

void i2c_master_send(unsigned char byte) { // send a byte to slave
    I2C1TRN = byte; // if an address, bit 0 = 0 for write, 1 for read
    while (I2C1STATbits.TRSTAT) {
        ;
    } // wait for the transmission to finish
    if (I2C1STATbits.ACKSTAT) { // if this is high, slave has not acknowledged
        // ("I2C1 Master: failed to receive ACK\r\n");
        LATAbits.LATA4 = 0; // turn off the heartbeat LED
        while(1){ 
            ; // do nothing
        }
    }
}

unsigned char i2c_master_recv(void) { // receive a byte from the slave
    I2C1CONbits.RCEN = 1; // start receiving data
    while (!I2C1STATbits.RBF) {
        ;
    } // wait to receive the data
    return I2C1RCV; // read and return the data
}

void i2c_master_ack(int val) { // sends ACK = 0 (slave should send another byte)
    // or NACK = 1 (no more bytes requested from slave)
    I2C1CONbits.ACKDT = val; // store ACK/NACK in ACKDT
    I2C1CONbits.ACKEN = 1; // send ACKDT
    while (I2C1CONbits.ACKEN) {
        ;
    } // wait for ACK/NACK to be sent
}

void i2c_master_stop(void) { // send a STOP:
    I2C1CONbits.PEN = 1; // comm is complete and master relinquishes bus
    while (I2C1CONbits.PEN) {
        ;
    } // wait for STOP to complete
}

void setPin(unsigned char address, unsigned char reg, unsigned char value) {
    // note that address should be = 0b01000000 to indicate we are writing to the MCP
    i2c_master_start();         // start bit
    i2c_master_send(address);   // write to the MCP
    i2c_master_send(reg);       // write to a MCP register
    i2c_master_send(value);     // set the value of that register
    i2c_master_stop();          // stop 
}

unsigned char readPin(unsigned char address, unsigned char reg) {
    // note that address should be = 0b01000001 to indicate we are reading from the MCP
    i2c_master_start();         // start bit
    i2c_master_send(address);   // write to MCP
    i2c_master_send(reg);       // write to register
    i2c_master_restart();       // restart
    address = 0b01000001;       // make bit 0 = 1 for reading from MCP
    i2c_master_send(address);   // read from MCP
    unsigned char val = i2c_master_recv();          // get received value
    i2c_master_ack(1);          // acknowledge we read the byte
    i2c_master_stop();          // stop
    return val;
}