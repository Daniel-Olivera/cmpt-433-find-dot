#ifndef I2C_H
#define I2C_H

//Writes to an I2C register
void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);

//Opens the I2C bus for usage
int openI2CBus(char* bus, int address);

//Reads an I2C register
unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr);

#endif
