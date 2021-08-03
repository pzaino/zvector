/*
 *    Name: ITest002
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#if (__GNUC__ < 6)
#define _BSD_SOURCE
#endif
#if (__GNUC__ > 5)
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <time.h>
#include <string.h>

#include "zvector.h"

#define MAX_ITEMS 20
#define MAX_MSG_SIZE 72

// Setup tests:
char *testGrp = "002";
uint8_t testID = 1;

#if ( ZVECT_THREAD_SAFE == 1 ) && ( OS_TYPE == 1 )

// Initialise Random Gnerator
static int mySeed = 25011984;
int max_strLen = 64;

#include <pthread.h>

pthread_t tid[2]; // Two threads IDs

typedef struct StackItem
{
    uint32_t eventID;
    char msg[MAX_MSG_SIZE];
    uint32_t priority;
} StackItem;

// Generates random strings of chars:
void mk_rndstr(char *rndStr, size_t len)
{
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";

    if (len)
    {
        if (rndStr)
        {
            int l = (int)(sizeof(charset) - 1);
            for (size_t n = 0; n < len; n++)
            {
                int key = rand() % l;
                rndStr[n] = charset[key];
            }
            rndStr[len] = '\0';
        }
    }
}

void clear_str(char *str, size_t len)
{
    memset(str, 0, len);
}

// Threads
void *producer(void *arg)
{
    vector v = (vector)arg;
    int evt_counter = 0;
    // Simulating Producer:
    printf("Test %s_%d: Produce %d events, store them in the queue and check if they are stored correctly:\n", testGrp, testID, MAX_ITEMS);
    uint32_t i;
    for (i = 0; i < MAX_ITEMS; i++)
    {
        StackItem qi;
        qi.eventID = i;
        // qi.msg = malloc(sizeof(char) * max_strLen);
        clear_str(qi.msg, MAX_MSG_SIZE);
        mk_rndstr(qi.msg, max_strLen - 1);
        qi.priority = 0;

        //printf("produced event message: %s\n", qi.msg);

        vect_lock(v);
        // Let's add a new item in the queue:
        vect_add(v, &qi);

        StackItem item = *((StackItem *)vect_get(v));

        vect_unlock(v);

        // Let's test if the value we have retrieved is correct:
        printf("Produced Event %*d: ID (%*d) - Message: %s\n", 2, i, 2, item.eventID, item.msg);
        evt_counter++;
        fflush(stdout);
    }
    printf("Producer done. Produced %d events.\n", evt_counter);
    testID++;

    fflush(stdout);

    return NULL;
}

void *consumer(void *arg)
{
    vector v = (vector)arg;
    int evt_counter = 0;
    // Simulating Consumer:
    printf("Test %s_%d: Consume %d events from the queue in LIFO order:\n", testGrp, testID, MAX_ITEMS);
    uint32_t i;
    for (i = 0; i < MAX_ITEMS; i++)
    {
        while (vect_is_empty(v));
        vect_lock(v);

        // Let's retrieve the value from the vector correctly:
        // For beginners: this is how in C we convert back a void * into the original dtata_type
        StackItem *item = (StackItem *)malloc(sizeof(StackItem *));
        if (!vect_is_empty(v))
            item = (StackItem *)vect_remove(v);

        vect_unlock(v);

        if ( item != NULL )
        {
            // Let's test if the value we have retrieved is correct:
            printf("Consumed Event %*d: ID (%*d) - Message: %s\n", 2, i, 2, item->eventID, item->msg);
            evt_counter++;
            fflush(stdout);
        }
        free(item);
    }

    printf("Consumer done. Consumed %d events.\n", evt_counter);
    testID++;

    fflush(stdout);

    return NULL;
}

int main()
{
    // Setup
    srand((time(NULL) * max_strLen) + (++mySeed));

    printf("=== UTest%s ===\n", testGrp);
    printf("Testing Dynamic STACKS (MULTI thread)\n");

    fflush(stdout);

    printf("Test %s_%d: Create a dynamic stack of 2 initial elements capacity:\n", testGrp, testID);
    vector v;
    v = vect_create(2, sizeof(struct StackItem), ZV_SAFE_WIPE);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Spin 2 threads and use them to manipoulate the stack created above.\n", testGrp, testID);

    int err = 0;
    int i = 0;
    err = pthread_create(&(tid[i]), NULL, &producer, v);
    if (err != 0)
        printf("Can't create thread :[%s]\n", strerror(err));
    i++;

    err = pthread_create(&(tid[i]), NULL, &consumer, v);
    if (err != 0)
        printf("Can't create thread :[%s]\n", strerror(err));
    i++;

    // Let's start the threads:
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    printf("done.\n");
    testID++;

    fflush(stdout);

    // Allow the Producer and Consumer process 
    // To start properly:
    //usleep(100);

    // Now wait until the Queue is empty:
    while(!vect_is_empty(v));

    printf("Test %s_%d: Dellete all left over events (if any):\n", testGrp, testID);
    while (!vect_is_empty(v))
    {
        vect_delete(v);
    }
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Clear stack:\n", testGrp, testID);
    vect_clear(v);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Check if stack size is now 0 (zero):\n", testGrp, testID);
    assert(vect_size(v) == 0);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: destroy the stack:\n", testGrp, testID);
    vect_destroy(v);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("================\n\n");

    return 0;
}

#else

int main()
{
    printf("=== ITest%s ===\n", testGrp);
    printf("Testing Thread_safe features:\n");

    printf("Skipping test because library has been built without THREAD_SAFE enabled or on a platform that does not supports pthread.\n");

    printf("================\n\n");

    return 0;
}

#endif // THREAD_SAFE