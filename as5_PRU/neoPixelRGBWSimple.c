/*
    NeoPixel RGBW demo program for 8 LED unit such as:
        https://www.adafruit.com/product/2868
    NOTE: This is RGBW, not RGB!

    Install process
    - Put the NeoPixel into a breadboard
    - Connect the NeoPixel with 3 wires:
        - Connect NeoPixel "GND" and "DIN" (data in) to the 3-pin "LEDS" header on Zen
            Zen Cape's LEDS header:
                Pin 1: DIN (Data): left most pin; beside USB-micro connection, connects to P8.11
                Pin 2: GND (Ground): middle pin
                Pin 3: Unused (it's "5V external power", which is not powered normally on the BBG)
        - Connect NeoPixel "5VDC" to P9.7 or P9.8
            Suggest using the header-extender to make it easier to make a good connection.
        - OK to wire directly to BBG: no level shifter required.
    - Software Setup
        - On Host
            make      # on parent folder to copy to NFS
        - On Target: 
            config-pin P8.11 pruout
            make
            make install_PRU0
    - All lights should light up on the LED strip

    Based on code from the PRU Cookbook by Mark A. Yoder:
        https://beagleboard.org/static/prucookbook/#_setting_neopixels_to_different_colors
*/

#include <stdint.h>
#include <math.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "../as5_Linux/include/sharedDataStruct.h"

#define STR_LEN         8       // # LEDs in our string
#define oneCyclesOn     700/5   // Stay on 700ns
#define oneCyclesOff    800/5
#define zeroCyclesOn    350/5
#define zeroCyclesOff   600/5
#define resetCycles     60000/5 // Must be at least 50u, use 60u
#define cyclesPerMs     200000

#define OFF     0x00000000
#define GREEN   0x01000000
#define RED     0x00010000
#define BLUE    0x00000100
#define WHITE   0x01010100

#define GREEN_BRIGHT    0x10000000
#define RED_BRIGHT      0x00100000
#define BLUE_BRIGHT     0x00001000


#define distanceToX pSharedMemStruct->xAccel - pSharedMemStruct->xRand
#define distanceToY pSharedMemStruct->yAccel - pSharedMemStruct->yRand

// GPIO P8_15 = Joystick Right on the Zen
// = pr1_pru0_pru_r31_15    (R31 for input) (15 for bit 15)
// `config-pin p8_15 pruin`  to make us able to read the pin!
#define JOYSTICK_RIGHT_MASK (1 << 15)
// `config-pin p8_16 pruin`  to make us able to read the pin!
#define JOYSTICK_DOWN_MASK  (1 << 14)

// P8_11 for output (on R30), PRU0
#define DATA_PIN 15       // Bit number to output on

volatile register uint32_t __R30; // Output GPIO register
volatile register uint32_t __R31; // Input GPIO register

// Shared Memory Configuration
// -----------------------------------------------------------
#define THIS_PRU_DRAM       0x00000         // Address of DRAM
#define OFFSET              0x200           // Skip 0x100 for Stack, 0x100 for Heap (from makefile)
#define THIS_PRU_DRAM_USABLE (THIS_PRU_DRAM + OFFSET)

// This works for both PRU0 and PRU1 as both map their own memory to 0x0000000
volatile sharedMemStruct_t *pSharedMemStruct = (volatile void *)THIS_PRU_DRAM_USABLE;

void turnOnPin(void){
    __R30 |= 0x1<<DATA_PIN;      // Set the GPIO pin to 1
    __delay_cycles(oneCyclesOn-1);
    __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
    __delay_cycles(oneCyclesOff-2);
}

void turnOffPin(void){
    __R30 |= 0x1<<DATA_PIN;      // Set the GPIO pin to 1
    __delay_cycles(zeroCyclesOn-1);
    __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
    __delay_cycles(zeroCyclesOff-2);
}

void setColor(uint32_t* colours, uint32_t color, int index){
    colours[index] = color;
}

void turnOffAll(void){
    for(int i = 0; i < STR_LEN; i++){
        for(int i = 31; i >= 0; i--) {
            if(OFF & ((uint32_t)0x1 << i)) {
                turnOnPin();
            } 
            else {
                turnOffPin();
            }
        }
    }
}

void setAll(uint32_t* colours, uint32_t color){
    for(int i = 0; i < STR_LEN; i++){
        setColor(colours, color, i);
    }
}

// Sends the colours array to the neopixel
void updateLEDS(uint32_t* colours){
    for(int j = 0; j < STR_LEN; j++) {
            for(int i=31; i>=0; i--){
                if(colours[j] & ((uint32_t)0x1 << i)){
                    turnOnPin();
                }
                else{
                    turnOffPin();
                }
            }
        }  
}

// Sets the 'middle' light as the bright version, and the 2 surrounding ones
// to the same color but a bit dimmer
void setLightDistance(uint32_t* colours, uint32_t color, int index)
{
    setAll(colours, OFF);
    setColor(colours, (OFF | color) << 4, index);
    if((index - 1) >= 0){
        setColor(colours, (OFF | color), index-1);
    }
    if((index + 1) < STR_LEN){
        setColor(colours, (OFF | color), index+1);
    }
}

void playHitAnimation(uint32_t* colours)
{
    for(int i = 0; i < 3; i++){
        setAll(colours, WHITE);
        updateLEDS(colours);
        __delay_cycles(cyclesPerMs*100);
        setAll(colours, OFF);
        updateLEDS(colours);
        __delay_cycles(cyclesPerMs*100);
    }
}

void playMissAnimation(uint32_t* colours)
{
    setAll(colours, RED);
    updateLEDS(colours);
    __delay_cycles(cyclesPerMs*100);
    setAll(colours, OFF);
    updateLEDS(colours);
    __delay_cycles(cyclesPerMs*100);
    setAll(colours, RED);
    updateLEDS(colours);
    __delay_cycles(cyclesPerMs*100);
    setAll(colours, OFF);
    updateLEDS(colours);
    __delay_cycles(cyclesPerMs*100);
}


void main(void)
{
    // COLOURS
    // - 1st element in array is 1st (bottom) on LED strip; last element is last on strip (top)
    // - Bits: {Green/8 bits} {Red/8 bits} {Blue/8 bits} {White/8 bits}
    uint32_t colours[STR_LEN] = {
        0x00000000, // bottom
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000, 
        0x00000000,
        0x00000000, // top
    };  

    // Clear SYSCFG[STANDBY_INIT] to enable OCP master port
    CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

    __delay_cycles(resetCycles);

    pSharedMemStruct->isDownPressed = false;
    pSharedMemStruct->isRightPressed = false;

    turnOffAll();

    __delay_cycles(resetCycles);
    uint32_t leftOrRight;
    bool onTarget = false;

    // int k = STR_LEN;

    while(true){

        pSharedMemStruct->isDownPressed = (__R31 & JOYSTICK_DOWN_MASK) == 0;
        
        if((__R31 & JOYSTICK_RIGHT_MASK) == 0){
            pSharedMemStruct->isRightPressed = true;
            turnOffAll();
            break;
        }
        // Check X axis tilt and set colour accordingly
        if(distanceToX >= 130){
            leftOrRight = RED;
        }
        if(distanceToX <= -130){
            leftOrRight = GREEN;
        }
        if(distanceToX > -100 && distanceToX < 100){
            leftOrRight = BLUE;
        }
        // --------------------------------------------

        // Check Y axis and set the lights according to distance from point
        if(distanceToY >= 400){
            setLightDistance(colours, leftOrRight, 7);
        }
        if(distanceToY >= 250 && distanceToY < 400){
            setLightDistance(colours, leftOrRight, 6);
        }
        if(distanceToY >= 120 && distanceToY < 230){
            setLightDistance(colours, leftOrRight, 5);
        }
        if(distanceToY <= -120 && distanceToY > -230){
            setLightDistance(colours, leftOrRight, 2);
        }
        if(distanceToY <= -250 && distanceToY > -400){
            setLightDistance(colours, leftOrRight, 1);
        }
        if(distanceToY <= -400){
            setLightDistance(colours, leftOrRight, 0);
        }
        if(distanceToY > -100 && distanceToY < 100){
            setAll(colours, OFF | leftOrRight);
        }
        // --------------------------------------------

        onTarget = distanceToY > -100 && distanceToY < 100 && leftOrRight == BLUE;

        if(onTarget & pSharedMemStruct->isDownPressed){
            pSharedMemStruct->hits++;
            playHitAnimation(colours);
        } else if(!onTarget & pSharedMemStruct->isDownPressed){
            playMissAnimation(colours);
        }

        updateLEDS(colours);
        
        // Updates 20 times / second
        __delay_cycles(cyclesPerMs*50);
    }

    // Send Reset
    __R30 &= ~(0x1<<DATA_PIN);   // Clear the GPIO pin
    __delay_cycles(resetCycles);

    __halt();
}
