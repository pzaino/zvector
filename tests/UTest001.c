/*
 *    Name: UTest001
 * Purpose: Unit Testing ZVector Library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all right reserved
 *          Distributed under MIT license
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "zvector.h"

int main()
{
    // Setup tests:
    char *testGrp = "001";
    uint8_t testID = 1;

    printf("=== UTest%s ===\n", testGrp);

    printf("Test %s_%d: Create a vector of 10 elements and using int for the vector data:\n", testGrp, testID);
    vector v;
    v = vect_create(10, sizeof(int), false);
    printf("done.\n");
    testID++;

    printf("Test %s_%d: Insert 10000 elements and check if they are stored correctly:\n", testGrp, testID);
    uint32_t i;
    for (i = 0; i < 1000000; i++)
    {
        // Let's add a new value in the vector:
        vect_add(v, &i);
        // Let's check if the vector size has grown correctly:
        assert(vect_size(v) == i + 1);
        // Let's retrieve the value from the vector correctly:
        // For beginners: this is how in C we convert back a void * into the original dtata_type
        int value = *((int *)vect_get_at(v, i));
        // Let's test if the value we have retrieved is correct:
        assert(value == i);
    }
    printf("done.\n");
    testID++;

    printf("Test %s_%d: check if the size of the vector is now 10000:\n", testGrp, testID);
    assert(vect_size(v) == 1000000);
    printf("done.\n");
    testID++;

    printf("Test %s_%d: Remove vector elements one by one:\n", testGrp, testID);
    while (!vect_is_empty(v))
    {
        vect_remove_at(v, vect_size(v) - 1);
    }
    printf("done.\n");
    testID++;

    printf("Test %s_%d: check if vector is empty:\n", testGrp, testID);
    assert(vect_is_empty(v));
    printf("done.\n");
    testID++;

    printf("Test %s_%d: Check if vector size is now 0 (zero):\n", testGrp, testID);
    assert(vect_size(v) == 0);
    printf("done.\n");
    testID++;

    printf("Test %s_%d: destroy the vector:\n", testGrp, testID);
    vect_destroy(v);
    printf("done.\n");
    testID++;

    printf("================\n");

    return 0;
}
