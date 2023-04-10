#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

// Writes to the I2C register.
// Pulled from Dr. Brian's example code.
void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
    unsigned char buff[2];
    buff[0] = regAddr;
    buff[1] = value;
    int result = write(i2cFileDesc, buff, 2);
    if(result != 2){
        perror("I2C: Unable to write i2c register.");
        exit(1);
    }
}

// Opens the I2C bus so that we can write to the register
int openI2CBus(char* bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);

    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if(result < 0){
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }

    return i2cFileDesc;

}

unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr)
{
    // To read a register, must first write the address
    int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));
    if (res != sizeof(regAddr)) {
        perror("I2C: Unable to write to i2c register.");
        exit(1);
    }
    // Now read the value and return it
    char value = 0;
    res = read(i2cFileDesc, &value, sizeof(value));
    if (res != sizeof(value)) {
        perror("I2C: Unable to read from i2c register");
        exit(1);
    }
    return value;
}
