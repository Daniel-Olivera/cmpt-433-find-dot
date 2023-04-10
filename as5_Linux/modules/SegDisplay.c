#include "../include/Tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>

#define GPIO_DIR "/sys/class/gpio/gpio"
#define SEG_LEFT_VAL "/sys/class/gpio/gpio61/value"
#define SEG_RIGHT_VAL "/sys/class/gpio/gpio44/value"
#define SEG_LEFT_DIRECTION "/sys/class/gpio/gpio61/direction"
#define SEG_RIGHT_DIRECTION "/sys/class/gpio/gpio44/direction"

#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"

#define ON "1"
#define OFF "0"

#define I2C_DEVICE_ADDRESS 0x20

#define REG_DIRA 0x00
#define REG_DIRB 0x01
#define REG_OUTA 0x14
#define REG_OUTB 0x15

typedef struct SegDisplayDigit{
    unsigned char top_half_val;
    unsigned char bot_half_val;
} SegDisplayDigit;

//The index in the array will correspond to number to display
SegDisplayDigit displayDigits[] = 
{
    [0].top_half_val = 0x86, [0].bot_half_val = 0xa1, // values for 0
    [1].top_half_val = 0x12, [1].bot_half_val = 0x80, //            1
    [2].top_half_val = 0x0e, [2].bot_half_val = 0x31, //            2
    [3].top_half_val = 0x06, [3].bot_half_val = 0xb0, //            3
    [4].top_half_val = 0x8a, [4].bot_half_val = 0x90, //            4
    [5].top_half_val = 0x8c, [5].bot_half_val = 0xb0, //            5
    [6].top_half_val = 0x8c, [6].bot_half_val = 0xb1, //            6
    [7].top_half_val = 0x14, [7].bot_half_val = 0x04, //            7
    [8].top_half_val = 0x8e, [8].bot_half_val = 0xb1, //            8
    [9].top_half_val = 0x8e, [9].bot_half_val = 0x90, //            9
};

static pthread_t displayThread;
static int numToShow = 0;
static bool shutdown_app = false;

//prototypes
static int openI2CBus(char* bus, int address);
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value);
static FILE* openFile(char* fileName);
static void writeToFile(char* fileName, char input[]);
static void exportGpioPin(char value[]);
static void setDisplayValue(int value);
static void setLeftDigitOnOrOff(char mode[]);
static void setRightDigitOnOrOff(char mode[]);

// Opens the I2C bus so that we can write to the register
static int openI2CBus(char* bus, int address)
{
    int i2cFileDesc = open(bus, O_RDWR);

    int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
    if(result < 0){
        perror("I2C: Unable to set I2C device to slave address.");
        exit(1);
    }

    return i2cFileDesc;

}

// Sets the display value on the 14-seg display
static void setDisplayValue(int value)
{
    int i2cFileDesc = openI2CBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    writeI2cReg(i2cFileDesc, REG_OUTB, displayDigits[value].top_half_val);
    writeI2cReg(i2cFileDesc, REG_OUTA, displayDigits[value].bot_half_val);

    close(i2cFileDesc);
}

static void setLeftDigitOnOrOff(char mode[])
{
    writeToFile(SEG_LEFT_VAL, mode);
}

static void setRightDigitOnOrOff(char mode[])
{
    writeToFile(SEG_RIGHT_VAL, mode);
}

// Writes to the I2C register.
// Pulled from Dr. Brian's example code.
static void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
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

// Opens a desired file to write to
static FILE* openFile(char* fileName)
{
    FILE* valueFile = fopen(fileName, "w");

    if(valueFile == NULL){
        perror("I2C: Unable to open value file.");
        exit(1);
    }

    return valueFile;
}

// Write desired input to a desired file
static void writeToFile(char* fileName, char input[])
{
    FILE* fd = openFile(fileName);

    int charWritten = fprintf(fd, input);
    if(charWritten <= 0){
        perror("I2C: Error writing data to.");
        exit(1);
    }

    fclose(fd);
}

static void exportGpioPin(char value[])
{
    // Try opening the directory for the desired pin
    char fileName[256]; 
    char gpioDirPath[] = "/sys/class/gpio/gpio%s";
    sprintf(fileName, gpioDirPath, value);
    DIR* dir = opendir(fileName);

    if(!dir){ // pin not exported yet
    char command[256];
    char commandTemplate[] = "echo %s > /sys/class/gpio/export";
    sprintf(command, commandTemplate, value);
        runCommand(command);
    }

    closedir(dir);
}

static void * showNum(void* nothing)
{
    while(!shutdown_app){
        int firstDigit = numToShow / 10;
        int secondDigit = numToShow % 10;

        if (numToShow > 99){
            firstDigit = 9;
            secondDigit = 9;
        }

        setLeftDigitOnOrOff(OFF);
        setRightDigitOnOrOff(OFF);

        setDisplayValue(firstDigit);

        setLeftDigitOnOrOff(ON);

        sleepForMs(5);

        setLeftDigitOnOrOff(OFF);
        setRightDigitOnOrOff(OFF);

        setDisplayValue(secondDigit);
        
        setRightDigitOnOrOff(ON);
        sleepForMs(5);
    }
    
    setLeftDigitOnOrOff(OFF);
    setRightDigitOnOrOff(OFF);
    return NULL;
}

void SegDisplay_setNum(int input)
{
    numToShow = input;
}

// Initialize the display
void SegDisplay_Init(void)
{
    //exports the pins for the 14-seg display
    //iff not already exported
    exportGpioPin("61");
    exportGpioPin("44");

    //set the directions to output and turn them both on
    writeToFile(SEG_LEFT_DIRECTION, "out");
    writeToFile(SEG_RIGHT_DIRECTION, "out");
    writeToFile(SEG_LEFT_VAL, ON);
    writeToFile(SEG_RIGHT_VAL, ON);

    //configure the pins for i2c mode
    runCommand("config-pin P9_18 i2c");
    runCommand("config-pin P9_17 i2c");

    //open the bus
    int i2cFileDesc = openI2CBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);

    //set direction of the ports on the I2C extender to be output
    writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);

    close(i2cFileDesc);

    pthread_create(&displayThread, NULL, showNum, NULL);
}

void SegDisplay_cleanup(void)
{
    shutdown_app = true;
    setLeftDigitOnOrOff(OFF);
    setRightDigitOnOrOff(OFF);
    pthread_join(displayThread, NULL);
}
