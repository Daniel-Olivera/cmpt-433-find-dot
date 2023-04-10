//Copied over from assignment 1
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

// Sleeps the thread for a given amount of Milliseconds
void sleepForMs (long long delayInMs);

// Runs a given command in the shell
void runCommand(char* command);

// Returns a random number between the specified values, given a seed
// A seed param was needed to get different values for x and y points in find dot
int getRandomInRange(int min, int max, int seed);
