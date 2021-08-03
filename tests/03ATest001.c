/*
 *    Name: ITest001
 * Purpose: Integration Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all right reserved
 *          Distributed under MIT license
 */

#if ( __GNUC__ <  6 )
#define _BSD_SOURCE
#endif
#if ( __GNUC__ >  5 )
#define _DEFAULT_SOURCE
#endif

#define UNUSED(x)			(void)x

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

    if( fgets(buffer, sizeof(buffer), pipe) != NULL) {
        // we are populating buffer now...
    }

    len = strlen(buffer);
    buffer[len-1] = '\0'; 

    pclose(pipe); 
}

int main()
{
    printf("=== ITest%s ===\n", testGrp);
    printf("CPPCheck Source code Analisys:\n");

    fflush(stdout);

    printf("Test %s_%d: Check is CPPCheck is installed on this system:\n", testGrp, testID);
 
    char buffer[10240];
    clear_str(buffer, 10240);

    run_cmd("which cppcheck 2>&1 | grep -Poi \"no cppcheck in\" | wc -l", buffer);
 
    char *result=(buffer[0] == '0') ? "yes" : "no";

    if ( buffer[0] == '1' )
    {
        printf("Sckipping CPPCheck test because I couldn't find it on your system: %s\n", result);
        return 0;
    }
    else
    {
        printf("Proceeding with  CPPCheck test because I found it on your system: %s\n", result);
    }

    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Run CPPCheck analysis against the Library sources:\n", testGrp, testID);
    fflush(stdout);

    clear_str(buffer, 10240);
    int rval;
    rval = system("$(pwd)/tests/cpp_check.sh");
    
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("================\n\n");

    return 0;
    UNUSED(rval);
}
