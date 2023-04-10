#ifndef I2C_H
#define I2C_H

/*
    TAKEN FROM ASSIGNMENT 2

    Used to display different digits on the 14-Segment display
    using I2C
*/

//Cleans up and resets the displays
void SegDisplay_cleanup(void);

//Initializes the display by exporting the pins, etc.
void SegDisplay_Init(void);

//Changes the number to be displayed
void SegDisplay_setNum(int input);

#endif