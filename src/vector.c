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

// Include standard C libs headers
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Include vector.h header
#include "vector.h"

// Useful macros
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define check_vect(x)                           \
    if (x == NULL)                              \
    {                                           \
        fprintf(stderr, "Vector not defined!"); \
        abort();                                \
    }

// Define the vector data structure:
struct _vector
{
    uint32_t size;          // Current Array size
    uint32_t init_capacity; // Initial Capacity (this is set at creation time)
    uint32_t capacity;      // Max capacity allocated
    size_t data_size;       // User DataType size
    void **array;           // vector
    bool wipe;              // If this flag is set to true then
                            // every time the vector is extended
                            // or shrunk, left over values will be
                            // properly erased.
} __attribute__((aligned(__WORDSIZE)));

/*************
 ** Vector API:
 *************/

/*------------------------------------------------------------------------------*/
// Vector Creation and Destruction:

vector vect_create(size_t init_capacity, size_t data_size, bool wipe_flag)
{
    // Create the vector first:
    vector v = (vector)malloc(sizeof(struct _vector));
    if (v == NULL)
    {
        fprintf(stderr, "Not enough memory to allocate the vector!");
        abort();
    }

    // Initialize the vector:
    v->size = 0;
    v->data_size = data_size;
    if (init_capacity == 0)
    {
        v->capacity = INITIAL_CAPACITY;
    }
    else
    {
        v->capacity = init_capacity;
    }
    v->init_capacity = v->capacity;
    v->wipe = wipe_flag;

    // Allocate memory for the vector body
    v->array = (void **)malloc(sizeof(void *) * v->capacity);
    if (v->array == NULL)
    {
        fprintf(stderr, "Not enough memory to allocate the vector data!");
        abort();
    }

    // Return the vector to the user:
    return v;
}

void vect_destroy(vector v)
{
    // Check if the vector exists:
    check_vect(v);

    if (v->wipe)
    {
        index_int i;
        for (i = 0; i < v->size; i++)
        {
            //v->array[i] = (void **)0; // Safely clear up the old array (security measure)
            memset(v->array[i], 0, v->data_size);
        }
    }

    // Destroy it:
    free(v->array);
    free(v);
}
/*------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
// Vector Structural Information report:

bool vect_is_empty(vector v)
{
    // Check if the vector exists
    check_vect(v);

    return v->size == 0;
}

index_int vect_size(vector v)
{
    // Check if the vector exists
    check_vect(v);

    return v->size;
}

/*------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
// Vector Capacity management functions:

static void vect_double_capacity(vector v)
{
    // Check if the vector exists:
    check_vect(v);

    // Get actual capacity and double it
    index_int new_capacity = 2 * v->capacity;
    void **new_array = (void *)malloc(sizeof(v->data_size) * new_capacity);
    if (new_array == NULL)
    {
        fprintf(stderr, "Not enough memory to extend the vector capacity!");
        abort();
    }

    index_int i;
    for (i = 0; i < v->size; i++)
    {
        new_array[i] = v->array[i];
        if (v->wipe)
        {
            // v->array[i] = (void **)0; // Safely clear up the old array (security measure)
            memset(v->array[i], 0, v->data_size);
        }
    }

    free(v->array);
    v->array = new_array;
    v->capacity = new_capacity;
}

static void vect_half_capacity(vector v)
{
    // Check if the vector exists:
    check_vect(v);

    // Check if new capacity is smaller than INITIAL_CAPACITY
    if (v->capacity <= v->init_capacity)
    {
        return;
    }

    // Get actual Capacity and halve it
    index_int new_capacity = v->capacity / 2;
    void **new_array = (void *)malloc(sizeof(v->data_size) * new_capacity);
    if (new_array == NULL)
    {
        fprintf(stderr, "Not enough memory to resize the vector!");
        abort();
    }

    // Rearraange the vector data:
    index_int i;
    for (i = 0; i < min(v->size, new_capacity); i++)
    {
        new_array[i] = v->array[i];
        if (v->wipe)
        {
            // v->array[i] = (void *)0; // safely clear up the old array (security measure)
            memset(v->array[i], 0, v->data_size);
        }
    }

    // Free old array:
    free(v->array);

    // Update vector:
    v->array = new_array;
    v->capacity = new_capacity;
    v->size = min(v->size, new_capacity);
}

/*------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
// Vector Data Storage functions:

void vect_clear(vector v)
{
    // check if the vector exists:
    check_vect(v);

    // TODO: The following implementation is not secure, so I need to reimplement it in a more secure way
    v->size = 0;
    while (v->capacity > v->init_capacity)
    {
        vect_half_capacity(v);
    }
}

// The following function will add an element to the vector
// and before doing so it will move the existsting elements
// around to make space for the new element:
void vect_add_at(vector v, index_int i, const void *value)
{
    // check if the vector exists:
    check_vect(v);

    // Check if the provided index is out of bounds:
    if (i < 0 || i > v->size)
    {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Check if we need to expand the vector:
    if (v->size >= v->capacity)
    {
        vect_double_capacity(v);
    }

    // Allocat ememory for the new item:
    v->array[v->size] = (void *)malloc(v->data_size);

    // Mode vector elements around were we are adding the new one:
    if (v->size > 0)
    {
        index_int j;
        for (j = v->size; j > i; j--)
        {
            memcpy(v->array[j], v->array[j - 1], v->data_size);
        }
    }

    // Finally add new value in at the index
    memcpy(v->array[i], value, v->data_size);
    // Increment vector size
    v->size++;
}

void vect_add(vector v, const void *value)
{
    // Add an item at the END of the vector
    vect_add_at(v, v->size, value);
}

void vect_add_front(vector v, const void *value)
{
    // Add an item at the FRONT of the vector
    vect_add_at(v, 0, value);
}

void *vect_get_at(vector v, index_int i)
{
    // check if the vector exists:
    check_vect(v);

    // Check if passed index is out of bounds:
    if (i < 0 || i >= v->size)
    {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Return found element:
    return v->array[i];
}

void *vect_get(vector v)
{
    // check if the vector exists:
    check_vect(v);

    // Return found element:
    return v->array[v->size - 1];
}

void *vect_get_front(vector v)
{
    // check if the vector exists:
    check_vect(v);

    // Return found element:
    return v->array[0];
}

void vect_put_at(vector v, index_int i, const void *value)
{
    // check if the vector exists:
    check_vect(v);

    // Check if the index passed is out of bounds:
    if (i < 0 || i >= v->size)
    {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Add value at the specified index:
    memcpy(v->array[i], value, v->data_size);
}

void vect_put(vector v, const void *value)
{
    // check if the vector exists:
    check_vect(v);

    // Add value at the specified index:
    memcpy(v->array[v->size], value, v->data_size);
}

void vect_put_front(vector v, const void *value)
{
    // check if the vector exists:
    check_vect(v);

    // Add value at the specified index:
    memcpy(v->array[0], value, v->data_size);
}

void *vect_remove_at(vector v, index_int i)
{
    // check if the vector exists:
    check_vect(v);

    // Check if the index is out of bounds:
    if (i < 0 || i >= v->size)
    {
        fprintf(stderr, "Index out of bounds!");
        abort();
    }

    // Get the value we are about to remove:
    void *rval = (void *)malloc(sizeof(v->data_size));
    memcpy(rval, v->array[i], v->data_size);

    // Reorganise the vector:
    index_int j;
    for (j = i + 1; j < v->size; j++)
    {
        memcpy(v->array[j - 1], v->array[j], v->data_size);
    }
    // Reduce vector size:
    v->size--;

    // Check if we need to shrink the vector:
    if (4 * v->size < v->capacity)
    {
        vect_half_capacity(v);
    }

    return rval;
}

void *vect_remove(vector v)
{
    return vect_remove_at(v, v->size);
}

void *vect_remove_front(vector v)
{
    return vect_remove_at(v, 0);
}

/*------------------------------------------------------------------------------*/
