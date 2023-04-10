#include "include/sharedMem-Linux.h"
#include "include/sharedDataStruct.h"
#include "include/Tools.h"
#include "include/Buzzer.h"
#include "include/Accel.h"
#include "include/SegDisplay.h"


int main(void) {

    // Configure all the required pins
    runCommand("config-pin p8_11 pruout");
    runCommand("config-pin p8_15 pruin");
    runCommand("config-pin p8_16 pruin");

    // Get access to shared memory for my uses
    volatile void *pPruBase = getPruMmapAddr();
    volatile sharedMemStruct_t *pSharedPru0 = PRU0_MEM_FROM_BASE(pPruBase);
    bool stopping = false;
    pSharedPru0->hits = 0;
    int oldHits = 0;

    Buzzer_init();
    Accel_init();
    SegDisplay_Init();

    // On my scale, +/-500 = 45 degrees
    pSharedPru0->xRand = getRandomInRange(-500, 500, time(NULL));
    pSharedPru0->yRand = getRandomInRange(-500, 500, rand());

    // Drive it
    while(!stopping) {
        
        if(pSharedPru0->isRightPressed){
            stopping = true;
        }
        // If we hit the target, get a new random point
        if(pSharedPru0->hits > oldHits){
            oldHits = pSharedPru0->hits;
            pSharedPru0->xRand = getRandomInRange(-500, 500, time(NULL));
            pSharedPru0->yRand = getRandomInRange(-500, 500, rand());
        }
        SegDisplay_setNum(pSharedPru0->hits);

        // Timing
        sleepForMs(200);
    }

    // Cleanup
    freePruMmapAddr(pPruBase);
    Buzzer_cleanup();
    Accel_cleanup();
    SegDisplay_cleanup();
}