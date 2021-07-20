/*
 *    Name: Vector
 * Purpose: Library to use Dynamic Arrays (Vectors) in C Language
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all right reserved
 *          Distributed under MIT license
 *
 * Credits: This Library was inspired by the work of quite few, 
 *          apologies if I forgot to  mention them all!
 *
 *          Dimitros Michail (Dynamic Array in C)
 *          
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "vector.h"

#define INITIAL_CAPACITY 4
#define min(x,y) (((x)<(y))?(x):(y))
#define check_vect(x) if ( x == NULL ) { fprintf(stderr, "Vector not defined!"); abort(); }

struct _vector {
    uint32_t          size; // Current Array size
    uint32_t init_capacity; // Initial Capacity (this is set at creation time)
    uint32_t      capacity; // Max capacity allocated
      size_t     data_size; // User DataType size
      void**         array; // vector
        bool          wipe; // If this flag is set to true then 
                            // every time the vector is extended 
                            // or shrunk, left over values will be
                            // properly erased. 
} __attribute__((aligned(__WORDSIZE)));

vector vector_create(size_t init_capacity, size_t data_size) {
    // Create the vector first:
    vector v = (vector) malloc(sizeof(struct _vector));
    if ( v == NULL ) {
        fprintf(stderr, "Not enough memory to allocate the vector!");
        abort();
    }

    // Initialize the vector:
    v->size = 0;
    v->data_size = data_size;
    if ( init_capacity == 0 ) {
        v->capacity = INITIAL_CAPACITY;
    } else {
        v->capacity = init_capacity;
    }
    v->init_capacity = v->capacity;

    // Allocate memory for the vector body
    v->array = (void *) malloc(sizeof(v->data_size) * v->capacity);
    if ( v->array == NULL ) {
        fprintf(stderr, "Not enough memory to allocate the vector data!");
        abort();
    }

    // Return the vector to the user:
    return v;
}

void vector_destroy(vector v) {
    // Check if the vector exists:
    check_vect(v);

    // Destroy it:
    free(v->array);
    free(v);
}

bool vector_is_empty(vector v) {
    // Check if the vector exists
    check_vect(v);

    return v->size == 0;
}

index_int vector_size(vector v) {
    // Check if the vector exists
    check_vect(v);

    return v->size;
}

static 
void vector_double_capacity(vector v) {
    // Check if the vector exists:
    check_vect(v);

    // Get actual capacity and double it
    index_int new_capacity = 2 * v->capacity;
    void** new_array = (void *) malloc(sizeof(v->data_size) * new_capacity);
    if ( new_array == NULL ) {
        fprintf(stderr, "Not enough memory to extend the vector capacity!");
        abort();
    }

    index_int i;
    for ( i = 0; i < v->size; i++ ) {
        new_array[i] = v->array[i];
        if ( v->wipe ) v->array[i] = (void **) 0; // Safely clear up the old array (security measure)
    }

    free(v->array);
    v->array = new_array;
    v->capacity = new_capacity;
}

static
void vector_half_capacity(vector v) {
    // Check if the vector exists:
    check_vect(v);

    // Check if new capacity is smaller than INITIAL_CAPACITY
    if ( v->capacity <= v->init_capacity ) {
        return;
    }

    // Get actual Capacity and halve it
    index_int new_capacity = v->capacity / 2;
    void** new_array = (void *) malloc(sizeof(v->data_size) * new_capacity);
    if ( new_array == NULL ) {
        fprintf(stderr, "Not enough memory to resize the vector!");
        abort();
    }

    // Rearraange the vector data:
    index_int i;
    for ( i = 0; i < min(v->size, new_capacity); i++ ) {
        new_array[i] = v->array[i];
        if ( v->wipe ) v->array[i] = (void *) 0; // safely clear up the old array (security measure)
    }

    // Free old array:
    free(v->array);

    // Update vector:
    v->array = new_array;
    v->capacity = new_capacity;
    v->size = min(v->size, new_capacity);
}

void vector_clear(vector v) {
    // check if the vector exists:
    check_vect(v);

    // TODO: The following implementation is not secure, so I need to reimplement it in a more secure way
    v->size = 0;
    while ( v->capacity > v->init_capacity ) {
        vector_half_capacity(v);
    }
}

void vector_add(vector v, void* const value) {
    // Check if the vector exists:
    check_vect(v);

    // Check if the addition of an element has to trigger 
    // vector grown:
    if ( v->size >= v->capacity ) {
        vector_double_capacity(v);
    }

    // Add new element
    v->array[v->size++] = (void **)value;
}

void* vector_get(vector v, index_int i) {
    // check if the vector exists:
    check_vect(v);

    // Check if passed index is out of bounds:
    if ( i < 0 || i >= v->size ) {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Return found element:
    return v->array[i];
}

void vector_put(vector v, index_int i, void* value) {
    // check if the vector exists:
    check_vect(v);

    // Check if the index passed is out of bounds:
    if ( i < 0 || i >= v->size ) {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Add value at the specified index:
    memcpy( v->array[i], value, v->data_size );
}

// The following function will add an element to the vector
// and before doing so it will move the existsting elements
// around to make space for the new element:
void vector_add_at(vector v, index_int i, void* value) {
    // check if the vector exists:
    check_vect(v);
 
    // Check if the provided index is out of bounds:
    if ( i < 0 || i >= v->size ) {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Check if we need to expand the vector:
    if ( v->size >= v->capacity ) {
        vector_double_capacity(v);
    }

    // Mode vector elements around were we are adding the new one:
    index_int j;
    for ( i = v->size; j > i; j-- ) {
        memcpy( v->array[i], v->array[j-1], v->data_size );
    }
    // Finally add new value in at the index
    memcpy( v->array[i], value, v->data_size );

    // Increase Vector size:
    v->size++;
}

void* vector_remove_at(vector v, index_int i) {
    // check if the vector exists:
    check_vect(v);

    // Check if the index is out of bounds:
    if ( i < 0 || i >= v->size ) {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Get the value we are about to remove: 
    void* rval = (void *) malloc(sizeof(v->data_size));
    memcpy( rval, v->array[i], v->data_size );

    // Reorganise the vector:
    index_int j;
    for ( j = i+1; j < v->size; j++ ) {
        memcpy( v->array[j-1], v->array[j], v->data_size );
    }
    // Reduce vector size:
    v->size--;

    // Check if we need to shrink the vector:
    if ( 4 * v->size < v->capacity ) {
        vector_half_capacity(v);
    }

    return rval;
}
