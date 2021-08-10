/*
 *    Name: UTest001
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#if (  defined(_MSC_VER) )
 // Silly stuff that needs to be added for Microsoft compilers
 // which are still at the MS-DOS age apparently...
#define ZVECTORH "../src/zvector.h"
#else
#define ZVECTORH "zvector.h"
#endif
#include ZVECTORH

int main()
{
    // Setup tests:
    char *testGrp = "001";
    uint8_t testID = 1;

    printf("=== UTest%s ===\n", testGrp);
    printf("Testing basic vector functionalities\n");

    fflush(stdout);

    printf("Test %s_%d: Create a vector of 10 elements and using int for the vector data:\n", testGrp, testID);
    vector v;
    v = vect_create(10, sizeof(int), ZV_NONE);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Insert 10000 elements and check if they are stored correctly:\n", testGrp, testID);
    int i;
    for (i = 0; i < 1000000; i++)
    {
        // Let's add a new value in the vector:
        vect_add(v, &i);
        // Let's check if the vector size has grown correctly:
        assert(vect_size(v) == (zvect_index)i + 1);
        // Let's retrieve the value from the vector correctly:
        // For beginners: this is how in C we convert back a void * into the original dtata_type
        int value = *((int *)vect_get_at(v, i));
        // Let's test if the value we have retrieved is correct:
        assert(value == i);
    }
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: check if the size of the vector is now 10000:\n", testGrp, testID);
    assert(vect_size(v) == 1000000);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Add elements in the middle of the vector:\n", testGrp, testID);
    i=555555;
    vect_add_at(v, &i, 100);
    assert(*((int *)vect_get_at(v, 100)) == i);
    assert(*((int *)vect_get_at(v, 101)) == 100);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Remove an element from the middle of the vector:\n", testGrp, testID);
    printf("At index 5001 now we have: %d\n", *((int *)vect_get_at(v, 5001)));
    vect_delete_at(v, 5001);
    int value = *((int *)vect_get_at(v, 5001));
    printf("At index 5001 now we have: %d\n", value);
    fflush(stdout);
    assert( value == 5001);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Remove vector elements one by one:\n", testGrp, testID);

    while (!vect_is_empty(v))
        vect_delete(v);

    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: check if vector is empty:\n", testGrp, testID);
    assert(vect_is_empty(v));
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Check if vector size is now 0 (zero):\n", testGrp, testID);
    assert(vect_size(v) == 0);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: destroy the vector:\n", testGrp, testID);
    vect_destroy(v);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("================\n\n");

    return 0;
}
