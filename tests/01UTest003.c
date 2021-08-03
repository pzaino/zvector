/*
 *    Name: UTest003
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "zvector.h"

#define MAX_ITEMS 10000

void multiply_elements(void *element)
{
    // Let's convert element into the right
    // C type, however we use a pointer to int
    // so that we can modify element content
    // directly without the need to copy it and
    // return it.
    int *number = (int *)element;
    *number *= 10;
}

void add_a_int(vector v)
{
    int myint = 100;
    vect_add(v, &myint);
}

int main()
{
    // Setup tests:
    char *testGrp = "003";
    uint8_t testID = 1;

    printf("=== UTest%s ===\n", testGrp);
    printf("Testing vect_apply functions\n");

    fflush(stdout);

    printf("Test %s_%d: Create a vector of 2 elements and using int for the vector data:\n", testGrp, testID);
    vector v;
    v = vect_create(2, sizeof(int), ZV_SAFE_WIPE);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Insert %d elements and check if they are stored correctly:\n", testGrp, testID, MAX_ITEMS);
    int i;
    for (i = 0; i < MAX_ITEMS; i++)
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

#ifdef ZVECT_SFMD_EXTENSIONS
    // We have SFMD extensions enabled so let's use them for this test!
    printf("Test %s_%d: Apply function 'multiply_elements' to the entire vector and verify if it's correct:\n", testGrp, testID);

    vect_apply(v, multiply_elements);

#endif // ZVECT_SFMD_EXTENSIONS
#ifndef ZVECT_SFMD_EXTENSIONS
    // We DO NOT have SFMD extensions enabled so let's use a regular loop!
    printf("Test %s_%d: Multiplying each vector's item (one-by-one):\n", testGrp, testID);

    for (i = 0; i < MAX_ITEMS; i++)
    {
        // Let's increment each vector element one by one:
        multiply_elements(vect_get_at((vector)v, i));
    }

#endif // ZVECT_SFMD_EXTENSIONS

    // Verify Items in the Vector:
    for (i = 0; i < MAX_ITEMS; i++)
    {
        // Let's retrieve the value from the vector correctly:
        // For beginners: this is how in C we convert back a void * into the original dtata_type
        int value = *((int *)vect_get_at(v, i));
        // Let's test if the value we have retrieved is correct:
        assert(value == (i * 10) );
    }
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Dellete a range of elements in the vector:\n", testGrp, testID);
    vect_delete_range(v, 10, 20);
    printf("done.\n");
    testID++;

    fflush(stdout);

    printf("Test %s_%d: Clear vector:\n", testGrp, testID);
    vect_clear(v);
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
