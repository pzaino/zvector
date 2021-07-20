#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "vector.h"

int main() {
    printf("=== UTest001 ===\n");

    printf("1st Test: Create a vector of 10 elements and using int for the vector data:\n");
    vector v;
    v = vector_create(10, sizeof(int));
    printf("done.\n");

    printf("2nd Test: Insert 10000 elements and check if they are stored correctly:\n");
    for(int i = 0; i < 1000000; i++) {
        vector_add(v, &i);
        assert(vector_size(v) == i+1);
        assert(vector_get(v, (index_int)i) == &i);
    }
    printf("done.\n");

    printf("3rd Test: check if the size of the vector is now 10000:\n");
    assert(vector_size(v) == 1000000);
    printf("done.\n");

    printf("4th Test: Remove vector elements one by one:\n");
    while(!vector_is_empty(v)) {
        vector_remove_at(v, vector_size(v)-1);
    }
    printf("done.\n");

    printf("5th Test: check if vector is empty:\n");
    assert(vector_is_empty(v));
    printf("done.\n");

    printf("6th Test: Check if vector size is now 0 (zero):\n");
    assert(vector_size(v) == 0);
    printf("done.\n");

    printf("7th Test: destroy the vector:\n");
    vector_destroy(v);
    printf("done.\n");

    return 0;
}
