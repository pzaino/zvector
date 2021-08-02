/*
 *    Name: ZVector
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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Include vector.h header
#include "zvector.h"

// Include non-ANSI Libraries
// only if the user has requested
// special extensions:
#if ( THREAD_SAFE == 1 )
#   if MUTEX_TYPE == 1
#       include <pthread.h>
#   elif MUTEX_TYPE == 2
#       include <windows.h>
#       include <psapi.h>
#   endif
#endif

#ifndef ZVECT_MEMX_METHOD
#define ZVECT_MEMX_METHOD 1
#endif

#if defined(Arch32)
#define ADDR_TYPE1 uint32_t
#define ADDR_TYPE2 uint16_t
#else
#define ADDR_TYPE1 uint64_t
#define ADDR_TYPE2 uint32_t
#endif

// Useful macros
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define UNUSED(x) (void)x

// Define the vector data structure:
struct _vector
{
         zvect_index prev_size;         // Used when clearing a vector.
         zvect_index size;              // Current Array size.
         zvect_index init_capacity;     // Initial Capacity (this is set at cre
                                        // ation time).
         zvect_index capacity;          // Max capacity allocated.
              size_t data_size;         // User DataType size.
            uint32_t flags;             // If this flag is set to true then
                                        // every time the vector is extended
                                        // or shrunk, left over values will be
                                        // properly erased.
                void (*SfWpFunc)(const void *item, size_t size);     
                                        // Pointer to a CUSTOM Safe Wipe 
                                        // function (optional) needed only for 
                                        // safe wiping special structures.
#if ( THREAD_SAFE == 1 )
#   if MUTEX_TYPE == 0
                void *lock;             // Vector's mutex for thread safe micro-transactions 
    volatile uint8_t lock_type;         // This field contains the lock used for this Vector.
#   elif MUTEX_TYPE == 1
     pthread_mutex_t *lock;             // Vector's mutex for thread safe micro-transactions
    volatile uint8_t lock_type;         // This field contains the lock used for this Vector.
#   elif MUTEX_TYPE == 2
    CRITICAL_SECTION *lock;             // Vector's mutex for thread safe micro-transactions
    volatile uint8_t lock_type;         // This field contains the lock used for this Vector.
#   endif
#endif
                void **data;            // Vector's storage.
};
// __attribute__((aligned(__WORDSIZE)));

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

static inline void item_safewipe(vector v, const void *item)
{
    if ((item != NULL) && ( (ADDR_TYPE1)item >= 0x100000 ))
    {
        if (!v->SfWpFunc)
        {
            memset((void *)item, 0, v->data_size);
        }
        else
        {
            (*(v->SfWpFunc))(item, v->data_size);
        }
    }
}

#if ( ZVECT_MEMX_METHOD == 0 )
static inline 
#endif
void *vect_memcpy(void *dst, const void *src, size_t size)
{
#if ( ZVECT_MEMX_METHOD == 0 )
    // Using regular memcpy:
    return memcpy(dst, src, size);
#elif ( ZVECT_MEMX_METHOD == 1 )
    // Using improved memcpy:
    size_t i;
    if ( size > 0 )
    {
        if (((uintptr_t)dst % sizeof(ADDR_TYPE1) == 0) &&
            ((uintptr_t)src % sizeof(ADDR_TYPE1) == 0) &&
            (size % sizeof(ADDR_TYPE1) == 0))
        {
            ADDR_TYPE1 * pExDst = (ADDR_TYPE1 *) dst;
            ADDR_TYPE1 const * pExSrc = (ADDR_TYPE1 const *) src;

            for (i = 0; i < size/sizeof(ADDR_TYPE1); i++) {
                // The following should be compiled as: (-O2 on x86_64)
                //         mov     rdi, QWORD PTR [rsi+rcx]
                //         mov     QWORD PTR [rax+rcx], rdi
                *pExDst++ = *pExSrc++;
            }
        }
        else if (((uintptr_t)dst % sizeof(ADDR_TYPE2) == 0) &&
                 ((uintptr_t)src % sizeof(ADDR_TYPE2) == 0) &&
                 (size % sizeof(ADDR_TYPE2) == 0))
        {
            ADDR_TYPE2 * pExDst = (ADDR_TYPE2 *) dst;
            ADDR_TYPE2 const * pExSrc = (ADDR_TYPE2 const *) src;

            for (i = 0; i < size/sizeof(ADDR_TYPE2); i++) {
                // The following should be compiled as: (-O2 on x86_64)
                //         mov     rdi, WORD PTR [rsi+rcx]
                //         mov     WORD PTR [rax+rcx], rdi
                *pExDst++ = *pExSrc++;
            }
        }
        else
        {
            char * pChDst = (char *) dst;
            char const * pChSrc = (char const *) src;
            for (i = 0; i < size; i++)
            {
                // The following should be compiled as: (-O2 on x86_64)
                //         movzx   edi, BYTE PTR [rsi+rcx]
                //         mov     BYTE PTR [rax+rcx], dil
                *pChDst++ = *pChSrc++;
            }
        }
    }
    return dst;
#endif
}

static inline void *vect_memmove(void *dst, const void *src, size_t size)
{
    return memmove(dst, src, size);
}

#if ( THREAD_SAFE == 1 )
#   if MUTEX_TYPE == 0
#   define THREAD_SAFE 0
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
        throw_error("Not enough memory to allocate the vector!");
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

#if ( THREAD_SAFE == 1 )
// The following two functions are generic locking functions
static inline void check_mutex_lock(vector v, volatile uint8_t lock_type)
{
    // If the request comes from ZVector itself and there are no existing
    // higher priority locks already in place, then lock the vector. 
    // For example: NO previous user's request or ZVector complex feature
    // requested a lock already.
    if ( lock_type == 1 && v->lock_type == 2 )
    {
        // In this particoular case we shall not lock
        // mutex_lock(v->lock);
        // v->lock_type = lock_type;
    }
    else
    {
        // Otherwise lock as you would do regularly:
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
// vector vect_create(size_t init_capacity, size_t data_size, uint32_t flags)
vector vect_create(size_t init_capacity, size_t data_size, uint32_t flags)
{
    // Create the vector first:
    vector v = (vector)malloc(sizeof(struct _vector));
    if (v == NULL)
        throw_error("Not enough memory to allocate the vector!");

    // Initialize the vector:
    v->prev_size = 0;
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
    v->flags = flags;
    v->SfWpFunc = NULL;
    v->data = NULL;

#   if ( THREAD_SAFE == 1 )
    mutex_alloc(&(v->lock));
#   endif

    // Allocate memory for the vector storage area
    v->data = (void **)malloc(sizeof(void *) * v->capacity);
    if (v->data == NULL)
        throw_error("Not enough memory to allocate the vector storage area!");

    // Return the vector to the user:
    return v;
}

void vect_destroy(vector v)
{
    // Check if the vector exists:
    vect_check(v);

#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    if (v->flags & ZV_SAFE_WIPE)
    {
        // Safely clear up the old array (security measure)
        zvect_index i;
        for (i = 0; i < v->size; i++)
            item_safewipe(v, v->data[i]);
    }

    // Destroy it:
    if (!v->data)
        free(v->data);
#   if ( THREAD_SAFE == 1 )
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
#if ( THREAD_SAFE == 1 )
inline void vect_lock(vector v)
{
    check_mutex_lock(v, 2);  
}

inline void vect_unlock(vector v)
{
    check_mutex_unlock(v, 2);
}
#endif
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Capacity management functions:

// This function double the CAPACITY of a vector.
static void vect_double_capacity(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    // Get actual capacity and double it
    zvect_index new_capacity = v->capacity * 2;
    void **new_data = (void **)malloc(sizeof(void *) * new_capacity);
    if (new_data == NULL)
        throw_error("Not enough memory to extend the vector capacity!");

    // Copy array of pointers to items into the new (larger) list:
    vect_memcpy(new_data, v->data, sizeof(void *) * (v->size));
    /* zvect_index j;
    for (j = 0; j < v->size; j++)
    {
        // Just move the pointers to the elements!
        // vect_memcpy(v->data[j - 1], v->data[j], sizeof(void *));
        new_data[j] = v->data[j];
    }*/

    // Apply changes and release memory
    free(v->data);
    v->data = new_data;
    v->capacity = new_capacity;
}

// This function halves the CAPACITY of a vector.
static void vect_half_capacity(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    // Check if new capacity is smaller than initial capacity
    if (v->capacity <= v->init_capacity)
        return;

    // Get actual Capacity and halve it
    zvect_index new_capacity = v->capacity / 2;
    zvect_index new_size = min(v->size, new_capacity);
    void **new_data = (void **)malloc(sizeof(void *) * new_capacity);
    if (new_data == NULL)
        throw_error("Not enough memory to resize the vector!");

    // Copy old vector's storage pointers list into new one:
    vect_memcpy(new_data, v->data, sizeof(void *) * new_size);
    /* zvect_index j;
    for (j = 0; j < v->size; j++)
    {
        // Just move the pointers to the elements!
        // vect_memcpy(v->data[j - 1], v->data[j], sizeof(void *));
        new_data[j] = v->data[j];
    }*/

    if (v->flags & ZV_SAFE_WIPE)
    {
        // Secure Erase the portion of the storage that
        // has not been touched:
        zvect_index i2;
        for (i2 = (v->capacity - 1); i2 > new_capacity; i2--)
            item_safewipe(v, v->data[i2]);
    }

    // Apply changes and release memory:
    free(v->data);
    v->data = new_data;
    v->capacity = new_capacity;
    v->size = new_size;
}

// Thi sfunction shrinks the CAPACITY of a vector
// not its size. To reduce the size of a vector we
// need to remove items from it.
void vect_shrink(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    if (vect_is_empty(v))
        throw_error("Empty vector can't be shrank!");

    zvect_index new_capacity;
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    if (v->size < v->init_capacity)
    {
        new_capacity = v->init_capacity;
    }
    else
        new_capacity = v->size + 1;

    v->capacity = new_capacity;
    v->data = (void **)realloc(v->data, sizeof(void *) * v->capacity);
    if (v->data == NULL)
    {
#   if ( THREAD_SAFE == 1 )
        check_mutex_unlock(v, 1);
#   endif
        throw_error("No memory available to shrink the vector!");
    }
#   if ( THREAD_SAFE == 1 )
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

#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    v->prev_size = v->size;
    v->size = v->init_capacity;

    while (v->capacity > v->init_capacity)
    {
        vect_half_capacity(v);
    }

    if (v->flags & ZV_SAFE_WIPE)
    {
        // Secure Erase the portion of the storage that
        // has not been touched:
        zvect_index i2 = (v->init_capacity - 1);
        while ( i2 )
        {
            item_safewipe(v, v->data[i2]);
            i2--;
        }
    }

    v->size = 0;

#   if ( THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
}

void vect_set_wipefunct(vector v, void (*f1)(const void *, size_t))
{
    v->SfWpFunc = (void *)malloc(sizeof(void *));
    if (v->SfWpFunc == NULL)
        throw_error("No memory available to set safe wipe function!\n");
    
    // Set Safe Wipe function:
    v->SfWpFunc = f1;
    //vect_memcpy(v->SfWpFunc, f1, sizeof(void *));
}

// inline implementation for all add(s):
static inline void _vect_add_at(vector v, const void *value, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if the provided index is out of bounds:
    if ( i > v->size )
        throw_error("Index out of bounds!");

#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    // Check if we need to expand the vector:
    if (v->size >= v->capacity)
        vect_double_capacity(v);

    // Allocate memory for the new item:
    v->data[v->size] = (void *)malloc(v->data_size);

    // "Shift" right the array of one position to make space for the new item:
    if ((i < v->size) && (v->size > 0))
    {
        zvect_index j;
        for (j = v->size; j > i; j--)
            vect_memcpy(v->data[j], v->data[j - 1], sizeof(void *));
    }

    // Finally add new value in at the index
    vect_memcpy(v->data[i], value, v->data_size);
    // Increment vector size
    v->prev_size=v->size;
    v->size++;
#   if ( THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
}

inline void vect_push(vector v, const void *value)
{
    // Add an item at the END (top) of the vector
    _vect_add_at(v, value, v->size);   
}

void vect_add(vector v, const void *value)
{
    // Add an item at the END of the vector
    _vect_add_at(v, value, v->size);
}

void vect_add_at(vector v, const void *value, zvect_index i)
{
    // Add an item at position "i" of the vector
    _vect_add_at(v, value, i);
}

void vect_add_front(vector v, const void *value)
{
    // Add an item at the FRONT of the vector
    _vect_add_at(v, value, 0);
}

void vect_add_ordered(vector v, const void *value, void (*f1)())
{
    /* TODO(pzaino): Implement a vect_add function that stores items in 
     *               an order fashion in the given vector. 
     */
    UNUSED(v);
    UNUSED(value);
    UNUSED(f1);
}

// inline implementation for all get(s):
static inline void *_vect_get_at(vector v, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if passed index is out of bounds:
    if ( i >= v->size )
        throw_error("Index out of bounds!");

    // Return found element:
    return v->data[i];
}

void *vect_get(vector v)
{
    return _vect_get_at(v, v->size - 1);
}

void *vect_get_at(vector v, zvect_index i)
{
    return _vect_get_at(v, i);
}

void *vect_get_front(vector v)
{
    return _vect_get_at(v, 0);
}

// inlin eimplementation for all put:
static inline void _vect_put_at(vector v, const void *value, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if the index passed is out of bounds:
    if ( i >= v->size )
        throw_error("Index out of bounds!");

#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    // Add value at the specified index:
    vect_memcpy(v->data[i], value, v->data_size);
#   if ( THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
}

void vect_put(vector v, const void *value)
{
    _vect_put_at(v, value, v->size - 1); 
}

void vect_put_at(vector v, const void *value, zvect_index i)
{
    _vect_put_at(v, value, i);
}

void vect_put_front(vector v, const void *value)
{
    _vect_put_at(v, value, 0);
}

// This is the inline implementation for all the remove and pop
static inline void *_vect_remove_at(vector v, zvect_index i)
{
    // check if the vector exists:
    vect_check(v);

    // Check if the index is out of bounds:
    if ( i >= v->size )
        throw_error("Index out of bounds!");

    // If the vector is empty just return null
    if ( v->size == 0 )
        return NULL;

    // Get the value we are about to remove:
    void *rval = (void *)malloc(v->data_size);
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    vect_memcpy(rval, v->data[i], v->data_size);

#   if ( FULL_REENTRANT == 1 )
    void **new_data = (void **)malloc(sizeof(void *) * v->capacity);
    if (new_data == NULL)
        throw_error("Not enough memory to resize the vector!");

    uint32_t array_changed = 0;
#   endif

    // "shift" left the array of one position:
    if ( (i < (v->size - 1)) && (v->size > 0))
    {
#   if ( FULL_REENTRANT == 1 )
        array_changed = 1;
        vect_memcpy(new_data, v->data + 1, sizeof(void *) * (v->capacity));
#   else
        // We can't use the vect_memcpy when not in full reentrant code
        // because it's not safe to use it on the same src and dst.
        zvect_index j;
        for (j = i + 1; j <= v->size; j++)
        {
            // Just move the pointers to the elements!
            // vect_memcpy(v->data[j - 1], v->data[j], sizeof(void *));
            v->data[j - 1] = v->data[j];
        }
#   endif
    }

#if ( FULL_REENTRANT == 1 )
    if (array_changed == 1)
    {
        free(v->data);
        v->data = new_data;
    }
#endif
    // Reduce vector size:
    v->prev_size=v->size;
    v->size--;

    // Check if we need to shrink the vector:
    if (4 * v->size < v->capacity)
        vect_half_capacity(v);

#   if ( THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif

    return rval;
}

inline void *vect_pop(vector v)
{
    return _vect_remove_at(v, v->size - 1);
}

void *vect_remove(vector v)
{
    return _vect_remove_at(v, v->size - 1);
}

void *vect_remove_at(vector v, zvect_index i)
{
    return _vect_remove_at(v, i);
}

void *vect_remove_front(vector v)
{
    return _vect_remove_at(v, 0);
}

// This is the inline implementation for all the delete 
static inline void _vect_delete_at(vector v, zvect_index start, zvect_index offset)
{
    // check if the vector exists:
    vect_check(v);

    // Check if the index is out of bounds:
    if ( ( start + offset ) >= v->size )
        throw_error("Index out of bounds!");

    // If the vector is empty just return null
    if ( v->size == 0 )
        return;

    zvect_index j;
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    // "shift" left the data of one position:
    if ( ((start + offset) < (v->size - 1)) && (v->size > 0))
    {
        for ( j = (start + offset) + 1; j < v->size; j++)
            vect_memcpy(v->data[(j - offset) - 1], v->data[j], sizeof(void *));
        if ( v->flags & ZV_SAFE_WIPE )
        {
            zvect_index j2;
            for ( j2 = (v->size - 1); j2 >= ((v->size - 1) - offset); j2--)
                item_safewipe(v, v->data[j2]);
        }
    }

    // Reduce vector size:
    v->prev_size=v->size;
    v->size = v->size - (offset + 1);

    // Check if we need to shrink the vector:
    if ((4 * v->size) < v->capacity)
        vect_half_capacity(v);

#   if ( THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
}

void vect_delete(vector v)
{
    _vect_delete_at(v, v->size - 1, 0);
}

void vect_delete_at(vector v, zvect_index i)
{
    _vect_delete_at(v, i, 0);
}

void vect_delete_range(vector v, zvect_index first_element, zvect_index last_element)
{
    last_element = ( last_element - first_element );
    _vect_delete_at(v, first_element, last_element);
}

void vect_delete_front(vector v)
{
    _vect_delete_at(v, 0, 0);
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
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    temp = v->data[i2];
    v->data[i2] = v->data[i1];
    v->data[i1] = temp;
#   if ( THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
    // We are done, let's clean up memory
    free(temp);
}

void vect_rotate_left(vector v, zvect_index i)
{
    // TODO(pzaino): Implement an inline rotation to the left
    UNUSED(v);
    UNUSED(i);

}

void vect_rotate_right(vector v, zvect_index i)
{
    // TODO(pzaino): Implement an inline rotation to the right
    UNUSED(v);
    UNUSED(i);
}

#endif  // ZVECT_DMF_EXTENSIONS

#ifdef ZVECT_SFMD_EXTENSIONS
// Single Function Call Multiple Data operations extensions:

void vect_apply(vector v, void (*f)(void *))
{
    // check if the vector exists:
    vect_check(v);

    zvect_index i;
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    for (i = 0; i < v->size; i++)
        (*f)(v->data[i]);
#   if ( THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
}

void vect_apply_if(vector v1, vector v2, void (*f1)(void *), bool (*f2)(void *, void *))
{
    // check if the vector exists:
    vect_check(v1);
    vect_check(v2);

    if (v1->size > v2->size)
        throw_error("Vector 2 size too small, can't apply 'if' function for all items in vector 1!");

    zvect_index i;
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v1, 1);
#   endif
    for (i = 0; i < v1->size; i++)
        if ((*f2)(v1->data[i],v2->data[i]))
            (*f1)(v1->data[i]);
#   if ( THREAD_SAFE == 1 )
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
        throw_error("Vectors data size mismatch!");

    // Let's check if the indexes provided are correct for
    // v2:
    if (start + max_elements > v2->size )
        throw_error("Index out of bounds!");

    // If the user specified 0 max_elements then
    // copy the entire vector from start position 
    // till the last item in the vector 2:
    if (max_elements == 0)
        max_elements = ( v2->size - 1 ) - start;

    zvect_index i;
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v1, 3);
#   endif
    for (i = start; i <= max_elements; i++)
        vect_add(v1, v2->data[i]);
#   if ( THREAD_SAFE == 1 )
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
        throw_error("Vectors data size mismatch!");

    // Let's check if the indexes provided are correct for
    // v2:
    if (start + max_elements > v2->data_size )
        throw_error("Index out of bounds!");

    // If the user specified 0 max_elements then
    // move the entire vector from start position 
    // till the last item in the vector 2:
    if (max_elements == 0)
        max_elements = ( v2->size - 1 ) - start;

    zvect_index i;
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v1, 3);
#   endif
    for (i = start; i <= max_elements; i++)
    {
        vect_add(v1, v2->data[i]);
        vect_remove_at(v2, i);
    }
#   if ( THREAD_SAFE == 1 )
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
        throw_error("Vectors data size mismatch!");

    zvect_index i;
#   if ( THREAD_SAFE == 1 )
    check_mutex_lock(v1, 3);
#   endif
    for (i = 0; i < v2->size; i++)
        vect_add(v1, v2->data[i]);
#   if ( THREAD_SAFE == 1 )
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
