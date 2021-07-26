/*
 *    Name: ITest001
 * Purpose: Integration Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all right reserved
 *          Distributed under MIT license
 */

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <string.h>
#include "zvector.h"

// Setup tests:
char *testGrp = "001";
uint8_t testID = 1;

void clear_str(char *str, uint32_t numchars)
{
    memset(str, 0, numchars);
}

void run_cmd(char *cmd_line, char *buffer)
{
    FILE *pipe;
    int len; 

    pipe = popen(cmd_line, "r");

    if (NULL == pipe) {
        perror("pipe");
        exit(1);
    } 

    fgets(buffer, sizeof(buffer), pipe);

    len = strlen(buffer);
    buffer[len-1] = '\0'; 

    pclose(pipe); 
}

int main()
{
    printf("=== ITest%s ===\n", testGrp);
    printf("Testing Thread_safe features:\n");

    printf("Test %s_%d: Check is CPPCheck is installed on this system:\n", testGrp, testID);
 
    char buffer[10240];
    clear_str(buffer, 10240);

    run_cmd("which cppcheck 2>&1 | grep -Poi \"no cppcheck in\" | wc -l", buffer);
 
    char *result=(buffer[0] == '0') ? "yes" : "no";

    printf("done.\n");
    testID++;

    if ( buffer[0] == '1' )
    {
        printf("Sckipping CPPCheck test because I couldn't find it on your system: %s\n", result);
        return 0;
    }
    else
    {
        printf("Proceeding with  CPPCheck test because I found it on your system: %s\n", result);
    }

    printf("Test %s_%d: Run CPPCheck analysis against the Library sources:\n", testGrp, testID);

    clear_str(buffer, 10240);
    run_cmd("./tests/cpp_check.sh", buffer);
    
    printf("done.\n");
    testID++;

    printf("CPPCheck Test results:\n%s\n", buffer);

    printf("================\n");

    return 0;
}
