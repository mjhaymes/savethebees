#ifndef I2C_MASTER_NOINT_H__
#define I2C_MASTER_NOINT_H__
// Header file for i2c_master_noint.c
// helps implement use I2C1 as a master without using interrupts

#include <xc.h>

#define WRITE 0b01000000
#define READ 0b01000001
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

void i2c_master_setup(void);
void i2c_master_start(void);
void i2c_master_restart(void);
void i2c_master_send(unsigned char byte);
unsigned char i2c_master_recv(void);
void i2c_master_ack(int val);
void i2c_master_stop(void);
void setRegister(unsigned char reg, unsigned char value);
unsigned char readRegister(unsigned char reg);

#endif