#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "vector.h"

int main()
{
    printf("=== UTest001 ===\n");

    printf("1st Test: Create a vector of 10 elements and using int for the vector data:\n");
    vector v;
    v = vect_create(10, sizeof(int), false);
    printf("done.\n");

    printf("2nd Test: Insert 10000 elements and check if they are stored correctly:\n");
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

    printf("3rd Test: check if the size of the vector is now 10000:\n");
    assert(vect_size(v) == 1000000);
    printf("done.\n");

    printf("4th Test: Remove vector elements one by one:\n");
    while (!vect_is_empty(v))
    {
        vect_remove_at(v, vect_size(v) - 1);
    }
    printf("done.\n");

    printf("5th Test: check if vector is empty:\n");
    assert(vect_is_empty(v));
    printf("done.\n");

    printf("6th Test: Check if vector size is now 0 (zero):\n");
    assert(vect_size(v) == 0);
    printf("done.\n");

    printf("7th Test: destroy the vector:\n");
    vect_destroy(v);
    printf("done.\n");

    return 0;
}
