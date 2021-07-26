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
 *          Gnome Team (GArray demo)
 *          Dimitros Michail (Dynamic Array in C presentation)
 *          
 */

// Include standard C libs headers
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// Include vector.h header
#include "zvector.h"

#ifdef THREAD_SAFE
#   if MUTEX_TYPE == 1
#       include <pthread.h>
#   endif
#endif

// Useful macros
#define min(x, y) (((x) < (y)) ? (x) : (y))

// Define the vector data structure:
struct _vector
{
    uint32_t size;              // Current Array size
    uint32_t init_capacity;     // Initial Capacity (this is set at creation time)
    uint32_t capacity;          // Max capacity allocated
    size_t data_size;           // User DataType size
    void **array;               // Vector's storage
    bool wipe;                  // If this flag is set to true then
                                // every time the vector is extended
                                // or shrunk, left over values will be
                                // properly erased.
#ifdef THREAD_SAFE
#   if MUTEX_TYPE == 0
    void *lock;                 // Vector's mutex for thread safe micro-transactions 
    volatile uint8_t lock_type; // This field contains the lock used for this Vector.
#   elif MUTEX_TYPE == 1
    pthread_mutex_t *lock;      // Vector's mutex for thread safe micro-transactions
    volatile uint8_t lock_type; // This field contains the lock used for this Vector.
#   elif MUTEX_TYPE == 2
    CRITICAL_SECTION *lock;     // Vector's mutex for thread safe micro-transactions
    volatile uint8_t lock_type; // This field contains the lock used for this Vector.
#   endif
#endif
} __attribute__((aligned(__WORDSIZE)));

/***********************
 ** Support Functions **
 ***********************/ 

static void throw_error(const char *error_message)
{
#if OS_TYPE == 1
    fprintf(stderr, "Error: %s\n", error_message);
    abort();
#else
    printf("Error: %s\n", error_message);
    exit(-1);
#endif
}

static inline void vect_check(vector x)
{
    if ( x == NULL ) 
        throw_error ("Vector not defined!");
}

#ifdef THREAD_SAFE
#   if MUTEX_TYPE == 0
static inline void mutex_lock(void *lock)
{
}

static inline void mutex_unlock(void *lock)
{
}

static inline void mutex_alloc(void **lock)
{
}

static inline void mutex_destroy(void *lock)
{
}

#   elif MUTEX_TYPE == 1
static inline void mutex_lock(pthread_mutex_t *lock)
{
    pthread_mutex_lock(lock);
}

static inline void mutex_unlock(pthread_mutex_t *lock)
{
    pthread_mutex_unlock(lock);
}

static inline void mutex_alloc(pthread_mutex_t **lock)
{
    *lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (lock == NULL)
    {
        throw_error("Not enough memory to allocate the vector!");
    }
}

static inline void mutex_destroy(pthread_mutex_t *lock)
{
    pthread_mutex_destroy(lock);
}
#   elif MUTEX_TYPE == 2
static inline void mutex_lock(CRITICAL_SECTION *lock)
{
    EnterCriticalSection(lock);
}

static inline void mutex_unlock(CRITICAL_SECTION *lock)
{
    LeaveCriticalSection(v->lock);
}

static inline void mutex_alloc(CRITICAL_SECTION **lock)
{
    InitializeCriticalSection(&lock);
}

static inline void mutex_destroy(CRITICAL_SECTION *lock)
{
    DeleteCriticalSection(lock);
}
#   endif
#endif

#ifdef THREAD_SAFE
// The following two functions are generic locking functions
static inline void check_mutex_lock(vector v, volatile uint8_t lock_type)
{
    if ( v->lock_type == 0 )
    {
        mutex_lock(v->lock);
        v->lock_type = lock_type;
    }
}

static inline void check_mutex_unlock(vector v, volatile uint8_t lock_type)
{
    if ( v->lock_type == lock_type )
    {
        v->lock_type = 0;
        mutex_unlock(v->lock);
    }
}
#endif

/***********************
 **    ZVector API    **
 ***********************/

/*---------------------------------------------------------------------------*/
// Vector Creation and Destruction:

vector vect_create(size_t init_capacity, size_t data_size, bool wipe_flag)
{
    // Create the vector first:
    vector v = (vector)malloc(sizeof(struct _vector));
    if (v == NULL)
    {
        throw_error("Not enough memory to allocate the vector!");
    }

    // Initialize the vector:
    v->size = 0;
    if (data_size == 0)
    {
        v->data_size = ZVECT_DEFAULT_DATA_SIZE;
    }
    else
        v->data_size = data_size;
    if (init_capacity == 0)
    {
        v->capacity = ZVECT_INITIAL_CAPACITY;
    }
    else
    {
        v->capacity = init_capacity;
    }
    v->init_capacity = v->capacity;
    v->wipe = wipe_flag;

#   ifdef THREAD_SAFE
    mutex_alloc(&(v->lock));
#   endif

    // Allocate memory for the vector storage area
    v->array = (void **)malloc(sizeof(void *) * v->capacity);
    if (v->array == NULL)
    {
        throw_error("Not enough memory to allocate the vector storage area!");
    }

    // Return the vector to the user:
    return v;
}

void vect_destroy(vector v)
{
    // Check if the vector exists:
    vect_check(v);

#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif

    if (v->wipe)
    {
        zvect_index i;
        for (i = 0; i < v->size; i++)
        {
            // Safely clear up the old array (security measure)
            memset(v->array[i], 0, v->data_size);
        }
    }

    // Destroy it:
    free(v->array);
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
    mutex_destroy(v->lock);
#   endif
    free(v);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Structural Information report:

bool vect_is_empty(vector v)
{
    // Check if the vector exists
    vect_check(v);

    return v->size == 0;
}

zvect_index vect_size(vector v)
{
    // Check if the vector exists
    vect_check(v);

    return v->size;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Thread Safe user functions:
#ifdef THREAD_SAFE
void vect_lock(vector v)
{
    check_mutex_lock(v, 2);  
}

void vect_unlock(vector v)
{
    check_mutex_unlock(v, 2);
}
#endif
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Capacity management functions:

static void vect_double_capacity(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    // Get actual capacity and double it
    zvect_index new_capacity = 2 * v->capacity;

    // Create a new array to work on (for reentrancy):
    void **new_array = (void **)malloc(sizeof(void *) * new_capacity);
    if (new_array == NULL)
    {
        throw_error("Not enough memory to extend the vector capacity!");
    }

    zvect_index i;
    for (i = 0; i < v->size; i++)
    {
        new_array[i] = v->array[i];
    }

    free(v->array);
    v->array = new_array;
    v->capacity = new_capacity;
}

static void vect_half_capacity(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    // Check if new capacity is smaller than initial capacity
    if (v->capacity <= v->init_capacity)
    {
        return;
    }

    // Get actual Capacity and halve it
    zvect_index new_capacity = v->capacity / 2;
    void **new_array = (void **)malloc(sizeof(void *) * new_capacity);
    if (new_array == NULL)
    {
        throw_error("Not enough memory to resize the vector!");
    }

    // Rearraange the vector data:
    zvect_index i;
    // Store old capacity, we'll need it for safe erase if enabled
    zvect_index old_size = v->size;

    for (i = 0; i < min(v->size, new_capacity); i++)
    {
        new_array[i] = v->array[i];
    }

    if (v->wipe)
    {
        // Secure Erase the portion of the old storage that
        // is going to be released in a bit:
        zvect_index i2;
        for (i2 = i + 1; i2 < old_size; i2++)
        {
            memset(v->array[i2], 0, v->data_size);
        }
    }
    // Free old array:
    free(v->array);

    // Update vector:
    v->array = new_array;
    v->capacity = new_capacity;
    v->size = min(v->size, new_capacity);
}

void vect_shrink(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    if (vect_is_empty(v))
    {
        throw_error("Empty vector can't be shrank!");
    }

    zvect_index size = 0;
#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    if (v->size < v->init_capacity)
    {
        size = v->init_capacity;
    }
    else
        size = v->size;

    // Secure wipe unused data
    if (v->wipe)
    {
        // Store old capacity, we'll need it for safe erase if enabled
        zvect_index old_size = v->size;
        zvect_index delta = old_size - size;

        // Secure Erase the portion of the old storage that
        // is going to be released in a bit:
        zvect_index i2;
        for (i2 = delta + 1; i2 < old_size; i2++)
        {
            memset(v->array[i2], 0, v->data_size);
        }
    }

    v->capacity = size + 1;
    v->array = (void **)realloc(v->array, sizeof(void *) * v->capacity);
    if (v->array == NULL)
    {
#   ifdef THREAD_SAFE
        check_mutex_unlock(v, 1);
#   endif
        throw_error("No memory available to shrink the vector!");
    }
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Data Storage functions:

void vect_clear(vector v)
{
    // check if the vector exists:
    vect_check(v);

#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    v->size = 0;
    while (v->capacity > v->init_capacity)
    {
        vect_half_capacity(v);
    }

    if (v->wipe)
    {
        // Secure Erase the portion of the storage that
        // has not been touched:
        zvect_index i2;
        for (i2 = 0; i2 < v->size; i2++)
        {
            memset(v->array[i2], 0, v->data_size);
        }
    }
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif
}

// The following function will add an element to the vector
// and before doing so it will move the existsting elements
// around to make space for the new element:
void vect_add_at(vector v, const void *value, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if the provided index is out of bounds:
    if (i < 0 || i > v->size)
    {
        throw_error("Index out of bounds!");
    }

#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    // Check if we need to expand the vector:
    if (v->size >= v->capacity)
    {
        vect_double_capacity(v);
    }

    // Allocate memory for the new item:
    v->array[v->size] = (void *)malloc(v->data_size);

    // Move vector elements around were we are adding the new one:
    if ((i < v->size) && (v->size > 0))
    {
        zvect_index j;
        for (j = v->size; j > i; j--)
        {
            memcpy(v->array[j], v->array[j - 1], v->data_size);
        }
    }

    // Finally add new value in at the index
    memcpy(v->array[i], value, v->data_size);
    // Increment vector size
    v->size++;
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif
}

void vect_push(vector v, const void *value)
{
     // Add an item at the END of the vector
    vect_add_at(v, value, v->size);   
}

void vect_add(vector v, const void *value)
{
    // Add an item at the END of the vector
    vect_add_at(v, value, v->size);
}

void vect_add_front(vector v, const void *value)
{
    // Add an item at the FRONT of the vector
    vect_add_at(v, value, 0);
}

void *vect_get_at(vector v, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if passed index is out of bounds:
    if (i < 0 || i >= v->size)
    {
        throw_error("Index out of bounds!");
    }

    // Return found element:
    return v->array[i];
}

void *vect_get(vector v)
{
    // check if the vector exists:
    vect_check(v);

    // Return found element:
    return v->array[v->size - 1];
}

void *vect_get_front(vector v)
{
    // check if the vector exists:
    vect_check(v);

    // Return found element:
    return v->array[0];
}

void vect_put_at(vector v, const void *value, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if the index passed is out of bounds:
    if (i < 0 || i >= v->size)
    {
        throw_error("Index out of bounds!");
    }
#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    // Add value at the specified index:
    memcpy(v->array[i], value, v->data_size);
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif
}

void vect_put(vector v, const void *value)
{
    // check if the vector exists:
    vect_check(v);

    // Add value at the specified index:
#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    memcpy(v->array[v->size], value, v->data_size);
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif   
}

void vect_put_front(vector v, const void *value)
{
    // check if the vector exists:
    vect_check(v);

    // Add value at the specified index:
#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    memcpy(v->array[0], value, v->data_size);
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif 
}

void *vect_remove_at(vector v, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if the index is out of bounds:
    if (i < 0 || i >= v->size)
    {
        throw_error("Index out of bounds!");
    }

    // Get the value we are about to remove:
    void *rval = (void *)malloc(sizeof(v->data_size));
    zvect_index j;
#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    memcpy(rval, v->array[i], v->data_size);

    // Reorganise the vector:
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
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif

    return rval;
}

void *vect_pop(vector v)
{
    return vect_remove_at(v, v->size - 1);
}

void *vect_remove(vector v)
{
    return vect_remove_at(v, v->size - 1);
}

void *vect_remove_front(vector v)
{
    return vect_remove_at(v, 0);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Data Manipoulation functions
#ifdef ZVECT_DMF_EXTENSIONS

void vect_swap(vector v, zvect_index i1, zvect_index i2)
{
    // check if the vector exists:
    vect_check(v);

    // Let's allocate some meory for the temporary pointer:
    void *temp = (void *)malloc(sizeof(void *));

    // Let's swap items:
#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    temp = v->array[i2];
    v->array[i2] = v->array[i1];
    v->array[i1] = temp;
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif
    // We are done, let's clean up memory
    free(temp);
}

#endif  // ZVECT_DMF_EXTENSIONS

#ifdef ZVECT_SFMD_EXTENSIONS
// Single Function Call Multiple Data operations extensions:

void vect_apply(vector v, void (*f)(void *))
{
    // check if the vector exists:
    vect_check(v);

    zvect_index i;
#   ifdef THREAD_SAFE
    check_mutex_lock(v, 1);
#   endif
    for (i = 0; i < v->size; i++)
    {
        (*f)(v->array[i]);
    }
#   ifdef THREAD_SAFE
    check_mutex_unlock(v, 1);
#   endif
}

void vect_apply_if(vector v1, vector v2, void (*f1)(void *), bool (*f2)(void *, void *))
{
    // check if the vector exists:
    vect_check(v1);
    vect_check(v2);

    if (v1->size > v2->size)
    {
        throw_error("Vector 2 size too small, can't apply 'if' function for all items in vector 1!");
    }

    zvect_index i;
#   ifdef THREAD_SAFE
    check_mutex_lock(v1, 1);
#   endif
    for (i = 0; i < v1->size; i++)
    {
        if ((*f2)(v1->array[i],v2->array[i]))
            (*f1)(v1->array[i]);
    }
#   ifdef THREAD_SAFE
    check_mutex_unlock(v1, 1);
#   endif
}

void vect_copy(vector v1, vector v2, zvect_index start, 
               zvect_index max_elements)
{
    // check if the vector v1 exists:
    vect_check(v1);

    // check if the vector v2 exists:
    vect_check(v2);

    // We can only copy vectors with the same data_size!
    if ( v1->data_size != v2->data_size )
    {
        throw_error("Vectors data size mismatch!");
    }

    // Let's check if the indexes provided are correct for
    // v2:
    if (start + max_elements > v2->size )
    {
        throw_error("Index out of bounds!");
    }

    // If the user specified 0 max_elements then
    // copy the entire vector from start position 
    // till the last item in the vector 2:
    if (max_elements == 0)
    {
        max_elements = ( v2->size - 1 ) - start;
    }

    zvect_index i;
#   ifdef THREAD_SAFE
    check_mutex_lock(v1, 3);
#   endif
    for (i = start; i <= max_elements; i++)
    {
        vect_add(v1, v2->array[i]);
    }
#   ifdef THREAD_SAFE
    check_mutex_unlock(v1, 3);
#   endif
}

void vect_move(vector v1, vector v2, zvect_index start, 
               zvect_index max_elements)
{
    // check if the vector v1 exists:
    vect_check(v1);

    // check if the vector v2 exists:
    vect_check(v2);

    // We can only copy vectors with the same data_size!
    if ( v1->data_size != v2->data_size )
    {
        throw_error("Vectors data size mismatch!");
    }

    // Let's check if the indexes provided are correct for
    // v2:
    if (start + max_elements > v2->data_size )
    {
        throw_error("Index out of bounds!");
    }

    // If the user specified 0 max_elements then
    // move the entire vector from start position 
    // till the last item in the vector 2:
    if (max_elements == 0)
    {
        max_elements = ( v2->size - 1 ) - start;
    }

    zvect_index i;
#   ifdef THREAD_SAFE
    check_mutex_lock(v1, 3);
#   endif
    for (i = start; i <= max_elements; i++)
    {
        vect_add(v1, v2->array[i]);
        vect_remove_at(v2, i);
    }
#   ifdef THREAD_SAFE
    check_mutex_unlock(v1, 3);
#   endif 
}

void vect_merge(vector v1, vector v2)
{
    // check if the vector v1 exists:
    vect_check(v1);

    // check if the vector v2 exists:
    vect_check(v2);

    // We can only copy vectors with the same data_size!
    if ( v1->data_size != v2->data_size )
    {
        throw_error("Vectors data size mismatch!");
    }

    zvect_index i;
#   ifdef THREAD_SAFE
    check_mutex_lock(v1, 3);
#   endif
    for (i = 0; i < v2->size; i++)
    {
        vect_add(v1, v2->array[i]);
        vect_remove_at(v2, i);
    }
#   ifdef THREAD_SAFE
    check_mutex_unlock(v1, 3);
#   endif 
    // Because we are merging two vectors in one
    // after merged v2 to v1 there is no need for
    // v2 to still exists, so let's destroy it to
    // free memory correctly:
    vect_destroy(v2);
}
#endif

/*---------------------------------------------------------------------------*/