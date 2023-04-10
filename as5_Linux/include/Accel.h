#ifndef ACCEL_H
#define ACCEL_H

/*
    TAKEN FROM ASSIGNMENT 3

    Configures the pins for i2c and sets the register for the
    Accelerator to ACTIVE
    Initializes the Accelerator and starts the thread
    to listen for values
*/

void Accel_init(void);
void Accel_cleanup(void);

#endif
