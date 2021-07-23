#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "vector.h"

void *multiply_elements(void *element)
{
    int number = *((int *)element);
    number *= 10;
    element = &number;
    return element;
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

    printf("Test %s_%d: Create a vector of 2 elements and using int for the vector data:\n", testGrp, testID);
    vector v;
    v = vect_create(2, sizeof(int), true);
    printf("done.\n");
    testID++;

    printf("Test %s_%d: Insert 10000 elements and check if they are stored correctly:\n", testGrp, testID);
    int i;
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

    printf("Test %s_%d: Apply function 'multiply_elements' to the entire vector and verify if it's correct:\n", testGrp, testID);
    vect_apply(v, multiply_elements);
    printf("done.\n");
    testID++;

    printf("Test %s_%d: Clear vector:\n", testGrp, testID);
    vect_clear(v);
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
