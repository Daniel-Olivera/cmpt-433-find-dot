#ifndef BUZZER_H
#define BUZZER_H

/*
    Allows for use and control of the buzzer on the Zen Cape
    
    While the module is currently specialized to the find dot game,
    it can easily be expanded for other uses.
*/

// Starts the buzzer thread
void Buzzer_init(void);

// Stops the buzzer thread
void Buzzer_cleanup(void);

#endif
