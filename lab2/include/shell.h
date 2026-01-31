#ifndef __SHELLF_H__
#define __SHELLF_H__
#include <stdint.h>

typedef struct Shell{
    int32_t pid;
    char* command;
}shell_t;

// Function declarations for shell commands
void processCommand(shell_t* shell); // Already declared
void runAShell(int32_t pid); // Already declared

#endif