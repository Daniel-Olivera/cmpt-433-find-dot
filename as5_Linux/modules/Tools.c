// Utility functions such as sleep and runCommand

#include "../include/Tools.h"
#include <stdio.h>
#include <stdlib.h>

void sleepForMs (long long delayInMs)
{
    const long long NS_PER_MS = 1000000;
    const long long NS_PER_SECOND = 1000000000;

    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;

    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}

void runCommand(char* command)
{
    FILE *pipe = popen(command, "r");

    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
    }

    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0){
        perror("Unable to execute command: ");
        printf("    %s\n", command);
        printf("    exit code: %d\n", exitCode);
    }
}

int getRandomInRange(int min, int max, int seed){
    srand(seed);
    return rand() % (max + 1 - min) + min;
}

