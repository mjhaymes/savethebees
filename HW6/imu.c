#include "imu.h"
#include "i2c_master_noint.h"

void imu_setup(){
    unsigned char who = 0;

    who = readRegister(IMU_ADDR,IMU_WHOAMI);
    
    if (who != 0b01101001){
        while(1){}
    }

    setRegister(IMU_ADDR,IMU_CTRL1_XL,0b10000010); // 1.66kHz output data rate, +/-2g sensitivity, 100Hz filter
    
    setRegister(IMU_ADDR,IMU_CTRL2_G,0b10001000);  // 1.66kHz output data rate, 1000 dps
    
    setRegister(IMU_ADDR,IMU_CTRL3_C,0b00000100);  // read from multiple registers in a row without specifying every register location

}

void imu_read(unsigned char address, unsigned char reg, signed short * data, int len){
    int counter;
    signed char byte_data[len*2];
    
    // read multiple from the imu, each data takes 2 reads so you need len*2 chars
    i2c_master_start();                             // start bit
    i2c_master_send(address);                       // write to slave device
    i2c_master_send(reg);                           // write to slave device register
    i2c_master_restart();                           // restart
    i2c_master_send(address|1);                     // read from slave device
    
    for (counter=0; counter<(len*2); counter++) {
        byte_data[counter] = i2c_master_recv();
        if (counter < ((len*2)-1) ) {i2c_master_ack(0);}   // keep reading
        else {i2c_master_ack(1);}                   // done reading after the last loop
    } 
    
    for (counter=1; counter<=len; counter++) {
        data[counter-1] = (byte_data[2*counter - 1]<<8)|byte_data[2*counter-2];
    } 
}