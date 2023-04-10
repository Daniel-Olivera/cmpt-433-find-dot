#include "../include/Tools.h"
#include "../include/I2C.h"
#include "../include/sharedDataStruct.h"
#include "../include/sharedMem-Linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <pthread.h>
#include <stdbool.h>

#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"

#define I2C_DEVICE_ADDRESS 0x1C

#define CTRL_REG1 0x2A

static bool stopping = false;
static pthread_t accelThreadId;
static int i2cFileDesc;

volatile void *Accel_pruBase;
volatile sharedMemStruct_t *Accel_sharedMem;

char accelValues[7];
int xAccl;
int yAccl;
int zAccl;

//prototypes
static void getAccelerometerValues(int i2cFileDesc, unsigned char regAddr);
static void* accelThread(void* arg);
static void checkAccel(void);

// Initialize the accelerometer
void Accel_init(void)
{
    runCommand("config-pin P9_18 i2c");
    runCommand("config-pin P9_17 i2c");

    Accel_pruBase = getPruMmapAddr();
    Accel_sharedMem = PRU0_MEM_FROM_BASE(Accel_pruBase);

    //open the bus
    i2cFileDesc = openI2CBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    //set the control register for the accelerometer to ACTIVE
    writeI2cReg(i2cFileDesc, CTRL_REG1, 0x01);

    pthread_create(&accelThreadId, NULL, accelThread, NULL);
}

void Accel_cleanup(void)
{
    stopping = true;
    close(i2cFileDesc);
	pthread_join(accelThreadId, NULL);
}

static void getAccelerometerValues(int fileDesc, unsigned char regAddr)
{
    // To read a register, must first write the address
    int res = write(fileDesc, &regAddr, sizeof(regAddr));
    if (res != sizeof(regAddr)) {
        perror("I2C: Unable to write to i2c register.");
        exit(1);
    }
    // Now read the value and return it
    int size = 7;
    res = read(fileDesc, &accelValues, sizeof(char)*size);
    if (res != sizeof(accelValues)) {
        perror("I2C: Unable to read from i2c register");
        exit(1);
    }

    //Two's complement conversion copied from:
    //http://www.getmicros.net/mma8452q-accelerometer-and-raspberry-pi-example-in-c.php#codesyntax_1
    xAccl = ((accelValues[1] * 256) + accelValues[2]) / 16;
    if(xAccl > 2047)
    {
        xAccl -= 4096;
    }
 
    yAccl = ((accelValues[3] * 256) + accelValues[4]) / 16;
    if(yAccl > 2047)
    {
        yAccl -= 4096;
    }
 
    zAccl = ((accelValues[5] * 256) + accelValues[6]) / 16;
    if(zAccl > 2047)
    {
        zAccl -= 4096;
    }
}   

static void* accelThread(void* arg)
{
    sleepForMs(350);
    while(!stopping){
        checkAccel();
        sleepForMs(10);
    }
    return NULL;
}

static void checkAccel(void)
{
    getAccelerometerValues(i2cFileDesc, 0x01);
    
    // Store values in shared memory for the PRU to read
    Accel_sharedMem->xAccel = xAccl;
    Accel_sharedMem->yAccel = yAccl;
    
}
