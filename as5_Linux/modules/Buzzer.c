#include "../include/Buzzer.h"
#include "../include/Tools.h"
#include "../include/sharedMem-Linux.h"
#include "../include/sharedDataStruct.h"
#include <stdio.h>
#include <pthread.h>

#define BUZZER_ENABLE_FILE "/dev/bone/pwm/0/a/enable"
#define BUZZER_PERIOD_FILE "/dev/bone/pwm/0/a/period"
#define DUTY_CYCLE_FILE "/dev/bone/pwm/0/a/duty_cycle"

#define B5      "1012384"
#define B5_DUTY "506192"

#define E6      "758431"
#define E6_DUTY "379215"

#define E2      "12134891"
#define E2_DUTY "6067445"

static pthread_t buzzerThread;
volatile void *pPruBase;
volatile sharedMemStruct_t *pSharedPru0;

static int Buzzer_writeToFile(char* filePath, char input[])
{
    FILE* fd = fopen(filePath, "w");

    if(fd == NULL){
        printf("ERROR opening file: %s\n", filePath);
        return -1;
    }

    size_t charWritten = fprintf(fd, input);
    if(charWritten < 0){
        printf("ERROR writing %s to file: %s\n", input, filePath);
        return -1;
    }

    fclose(fd);
    return 0;
}

void Buzzer_playHit()
{
    // First part of the sound
    Buzzer_writeToFile(DUTY_CYCLE_FILE, "0");
    Buzzer_writeToFile(BUZZER_PERIOD_FILE, B5);

    Buzzer_writeToFile(DUTY_CYCLE_FILE, B5_DUTY);
    Buzzer_writeToFile(BUZZER_ENABLE_FILE, "1");
    sleepForMs(80);
    Buzzer_writeToFile(BUZZER_ENABLE_FILE, "0");

    // Second part of the sound
    Buzzer_writeToFile(DUTY_CYCLE_FILE, "0");
    Buzzer_writeToFile(BUZZER_PERIOD_FILE, E6);

    Buzzer_writeToFile(DUTY_CYCLE_FILE, E6_DUTY);
    Buzzer_writeToFile(BUZZER_ENABLE_FILE, "1");
    sleepForMs(600);
    Buzzer_writeToFile(BUZZER_ENABLE_FILE, "0");
}

void Buzzer_playMiss()
{
    for(int i = 0; i < 2; i++){
        Buzzer_writeToFile(DUTY_CYCLE_FILE, "0");
        Buzzer_writeToFile(BUZZER_PERIOD_FILE, E2);

        Buzzer_writeToFile(DUTY_CYCLE_FILE, E2_DUTY);
        Buzzer_writeToFile(BUZZER_ENABLE_FILE, "1");
        sleepForMs(100);
        Buzzer_writeToFile(BUZZER_ENABLE_FILE, "0");
    }
}

void* Buzzer_thread(void* nothing)
{
    bool stopping = false;
    int oldHits = 0;

    while(!stopping){
        if(pSharedPru0->isDownPressed){
            // Avoiding using another flag variable to detect a hit
            if(pSharedPru0->hits > oldHits){
                oldHits = pSharedPru0->hits;
                Buzzer_playHit();
            } else{
                Buzzer_playMiss();
            }
        }
        // Kill the thread if the user presses right
        if(pSharedPru0->isRightPressed){
            stopping = true;
        }
        // Update 10 times a second. Also without this delay, 
        // sounds can interleave or play more than intended
        sleepForMs(100);
    }
    return NULL;
}

void Buzzer_init(void)
{
    runCommand("config-pin p9_22 pwm");
    
    // Get access to the shared memory so the buzzer can get info from the PRU
    pPruBase = getPruMmapAddr();
    pSharedPru0 = PRU0_MEM_FROM_BASE(pPruBase);

    pthread_create(&buzzerThread, NULL, Buzzer_thread, NULL);
    Buzzer_writeToFile(DUTY_CYCLE_FILE, "0");
}

void Buzzer_cleanup(void)
{
    // Turn off the buzzer before exiting.
    Buzzer_writeToFile(BUZZER_ENABLE_FILE, "0");
    pthread_join(buzzerThread, NULL);
}