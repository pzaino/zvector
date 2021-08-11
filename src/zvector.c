/*
 *    Name: ZVector
 * Purpose: Library to use Dynamic Arrays (Vectors) in C Language
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
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
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// Include vector.h header
#include "zvector.h"

#if ( OS_TYPE == 1 )
#   if ( !defined(macOS))
/*  Improve PThreads on Linux.
 *  macOS seems to be handling pthreads
 *  sligthely differently than Linux so
 * avoid using the same trick on macOS.
 */
#       define _POSIX_C_SOURCE 200112L
#       define __USE_UNIX98
#   endif
#endif

// Include non-ANSI Libraries
// only if the user has requested
// special extensions:
#if ( OS_TYPE == 1 )
#   include <xmmintrin.h>
#endif
#if ( ZVECT_THREAD_SAFE == 1 )
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
#define ADDR_TYPE2 uint32_t
#define ADDR_TYPE3 uint16_t
#else
#define ADDR_TYPE1 uint64_t 
#define ADDR_TYPE2 uint64_t
#define ADDR_TYPE3 uint32_t
#endif

// Useful macros
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define UNUSED(x) (void)x

// Define the vector data structure:
struct _vector
{
         zvect_index prev_size;         // - Used when clearing a vector.
         zvect_index size;              // - Current Array size. size - 1 gives 
                                        //   us the pointer to the last element 
                                        //   in the vector.
         zvect_index fep;               // - First vector's Element Pointer
         zvect_index init_capacity;     // - Initial Capacity (this is set at 
                                        //   creation time).
         zvect_index capacity;          // - Max capacity allocated.
              size_t data_size;         // - User DataType size.
            uint32_t flags;             // - If this flag set is used to
                                        //   represent ALL Vector's properties.
                                        //   It contains bits that set Secure
                                        //   Wipe, Auto Shrink, Pass Items By 
                                        //   Ref etc.
#if ( ZVECT_THREAD_SAFE == 1 )
#   if MUTEX_TYPE == 0
    volatile uint8_t lock_type;         // - This field contains the lock used
                                        //   for this Vector.
                void *lock;             // - Vector's mutex for thread safe 
                                        //   micro-transactions or user locks.
#   elif MUTEX_TYPE == 1
    volatile uint8_t lock_type;         // - This field contains the lock used 
                                        //   for this Vector.
     pthread_mutex_t *lock;             // - Vector's mutex for thread safe 
                                        //   micro-transactions or user locks.
#   elif MUTEX_TYPE == 2
    volatile uint8_t lock_type;         // - This field contains the lock used 
                                        //   for this Vector.
    CRITICAL_SECTION *lock;             // - Vector's mutex for thread safe 
                                        //   micro-transactions or user locks.
#   endif
#endif
                void (*SfWpFunc)(const void *item, size_t size);     
                                        // - Pointer to a CUSTOM Safe Wipe 
                                        //   function (optional) needed only 
                                        //   for Secure Wiping special 
                                        //   structures.
                uint32_t  ep_filed;     // - This is a safety field to separate
                                        //   the storage from the rest of the 
                                        //   vector structure, to protect the
                                        //   descriptors from acidental writes
                                        //   from memcpy/memmove with hw acell.
                void **data ZVECT_DATAALIGN;
                                        // - Vector's storage.
}ZVECT_DATAALIGN;

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
    // && ( (ADDR_TYPE2)item >= 0x100000 )
    if ((item != NULL))
    {
        if ( v->SfWpFunc == NULL )
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
void *vect_memcpy(void * __restrict dst, const void * __restrict src, size_t size)
{
#if ( ZVECT_MEMX_METHOD == 0 )
    // Using regular memcpy 
    // If you are using ZVector on Linux/macOS/BSD/Windows
    // your should stick to this one!
    return memcpy(dst, src, size);
#elif ( ZVECT_MEMX_METHOD == 1 )
    // Using improved memcpy (where improved means for 
    // embedded systems only!):
    size_t i;
    if ( size > 0 )
    {
        if (((uintptr_t)dst % sizeof(ADDR_TYPE1) == 0) &&
            ((uintptr_t)src % sizeof(ADDR_TYPE1) == 0) &&
            (size % sizeof(ADDR_TYPE1) == 0))
        {
            ADDR_TYPE1 * pExDst = (ADDR_TYPE1 *) dst;
            ADDR_TYPE1 const * pExSrc = (ADDR_TYPE1 const *) src;
            size_t end = size/sizeof(ADDR_TYPE1);
            for (i = 0; i < end; i++) {
                // The following should be compiled as: (-O2 on x86_64)
                //         mov     rdi, QWORD PTR [rsi+rcx]
                //         mov     QWORD PTR [rax+rcx], rdi
                *pExDst++ = *pExSrc++;
            }
        }
        else 
            return memcpy(dst, src, size);
    }
    return dst;
#endif
}

static inline void *vect_memmove(void * __restrict dst, const void * __restrict src, size_t size)
{
    return memmove(dst, src, size);
}

#if ( ZVECT_THREAD_SAFE == 1 )
#   if MUTEX_TYPE == 0
#   define ZVECT_THREAD_SAFE 0
#   elif MUTEX_TYPE == 1
static volatile bool lock_enabled = true;

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
    pthread_mutex_t *set_type = (pthread_mutex_t *)lock;
#if ( !defined(macOS))
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(set_type, &Attr);
#else
    pthread_mutex_init(set_type, NULL);
#endif
    *lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (lock == NULL)
        throw_error("Not enough memory to allocate the vector!");
}

static inline void mutex_destroy(pthread_mutex_t *lock)
{
    pthread_mutex_destroy(lock);
    if ( lock != NULL )
        free(lock);
}
#   elif MUTEX_TYPE == 2
static volatile bool lock_enabled = true;

static inline void mutex_lock(CRITICAL_SECTION *lock)
{
    EnterCriticalSection(lock);
}

static inline void mutex_unlock(CRITICAL_SECTION *lock)
{
    LeaveCriticalSection(lock);
}

static inline void mutex_alloc(CRITICAL_SECTION **lock)
{
    InitializeCriticalSection((CRITICAL_SECTION *)lock);
}

static inline void mutex_destroy(CRITICAL_SECTION *lock)
{
    DeleteCriticalSection(lock);
}
#   endif
#endif

#if ( ZVECT_THREAD_SAFE == 1 )
// The following two functions are generic locking functions

/*
 * ZVector uses the concept of Priorities for locking.
 * A user lock has the higher priority while ZVector itself
 * uses two different levels of priorities (both lower than 
 * the user lock priority). 
 * level 1 is the lower priority and it's used just by the
 *         primitives in ZVector.
 * level 2 is the priority used by the ZVEctor functions that 
 *         uses ZVEctor primitives. 
 * level 3 is the priority of the User's locks.
 */
static inline void check_mutex_lock(vector v, volatile uint8_t lock_type)
{
    if ( lock_enabled )
    {
        if ( lock_type >= v->lock_type )
        {
            mutex_lock(v->lock);
            v->lock_type = lock_type;
        }
    }
}

static inline void check_mutex_unlock(vector v, volatile uint8_t lock_type)
{
    if ( lock_enabled )
    {
        if ( lock_type == v->lock_type )
        {
            v->lock_type = 0;
            mutex_unlock(v->lock);
        }
    }
}
#endif

/******************************
 **    ZVector Primitives    **
 ******************************/

/*---------------------------------------------------------------------------*/
// Vector Capacity management functions:

// This function double the CAPACITY of a vector.
static void vect_increase_capacity(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    // Get actual capacity and double it
    zvect_index new_capacity = v->capacity * 2;
    void **new_data = (void **)realloc(v->data, sizeof(void *) * new_capacity);
    if (new_data == NULL)
        throw_error("Not enough memory to extend the vector capacity!");

    // Apply changes and release memory
    v->data = new_data;
    v->capacity = new_capacity;
}

// This function halves the CAPACITY of a vector.
static void vect_decrease_capacity(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    // Check if new capacity is smaller than initial capacity
    if (v->capacity <= v->init_capacity)
        return;

    // Get actual Capacity and halve it
    zvect_index new_capacity = v->capacity / 2;
    if (new_capacity < v->init_capacity)
        new_capacity = v->init_capacity;
    new_capacity = max(v->size, new_capacity);
    zvect_index new_size = min(v->size, new_capacity);

    void **new_data = (void **)realloc(v->data, sizeof(void *) * new_capacity);
    if (new_data == NULL)
        throw_error("Not enough memory to resize the vector!");

    // Apply changes and release memory:
    v->data = new_data;
    v->capacity = new_capacity;
    v->size = new_size;
}

// Thi sfunction shrinks the CAPACITY of a vector
// not its size. To reduce the size of a vector we
// need to remove items from it.
void _vect_shrink(vector v)
{
    // Check if the vector exists:
    vect_check(v);

    // Determine the correct shrunk size:
    zvect_index new_capacity;
    if (v->size < v->init_capacity)
        new_capacity = v->init_capacity;
    else
        new_capacity = v->size + 1;

    // shrink the vector:
    void** new_data = (void**)realloc(v->data, sizeof(void*) * new_capacity);
    if ( new_data == NULL )
        throw_error("No memory available to shrink the vector!");

    // Apply changes:
    v->data = new_data;
    v->capacity = new_capacity;
}

/*---------------------------------------------------------------------------*/

/***********************
 **    ZVector API    **
 ***********************/

/*---------------------------------------------------------------------------*/
// Vector Creation and Destruction:

vector vect_create(size_t init_capacity, size_t item_size, uint32_t properties)
{
    // Create the vector first:
    vector v = (vector)malloc(sizeof(struct _vector));
    if (v == NULL)
        throw_error("Not enough memory to allocate the vector!");

    // Initialize the vector:
    v->prev_size = 0;
    v->size = 0;
    if (item_size == 0)
        v->data_size = ZVECT_DEFAULT_DATA_SIZE;
    else
        v->data_size = item_size;

    if (init_capacity == 0)
        v->capacity = ZVECT_INITIAL_CAPACITY;
    else
        v->capacity = init_capacity;

    v->fep = v->capacity / 2;

    v->init_capacity = v->capacity;
    v->flags = properties;
    v->SfWpFunc = NULL;
    
    v->data = NULL;

#   if ( ZVECT_THREAD_SAFE == 1 )
    v->lock = NULL;
    v->lock_type = 0;
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

#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    // Clear the vector:
    if (v->size > 0)
    {
        // Secure Wipe the vector (or just free) depending on vector properties:
        zvect_index i = v->size; // if v->size is 200, then the first i below will be 199
        while ( i-- )
        {
            if ((v->flags & ZV_SAFE_WIPE) && (v->data[i] != NULL))
                item_safewipe(v, v->data[i]);
            if ((!( v->flags & ZV_BYREF )) && (v->data[i] != NULL))
                free(v->data[i]);
        }
    }

    // Reset interested descriptors:
    v->prev_size = v->size;
    v->size = 0;

    // Shrink Vector's capacity:
    _vect_shrink(v);

    v->prev_size = 0;
    v->init_capacity = 0;
    v->capacity = 0;

    // Destroy it:
    if ( v->SfWpFunc != NULL )
        free(v->SfWpFunc);

    if ( v->data != NULL )
        free(v->data);

#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
    mutex_destroy(v->lock);
#   endif

    // All done and freed, so we can safely
    // free the vector itself:
    free(v);
}

void vect_shrink(vector v)
{    
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    _vect_shrink(v);

#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
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
#if ( ZVECT_THREAD_SAFE == 1 )
void vect_lock_enable(void)
{
    lock_enabled = true;
}

void vect_lock_disable(void)
{
   lock_enabled = false; 
}

inline void vect_lock(vector v)
{
    check_mutex_lock(v, 3);  
}

inline void vect_unlock(vector v)
{
    check_mutex_unlock(v, 3);
}
#endif
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Data Storage functions:

void vect_clear(vector v)
{
    // check if the vector exists:
    vect_check(v);

#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    // Clear the vector:
    if (v->size > 0)
    {
        // Secure Wipe the vector (or just free) depending on vector properties:
        zvect_index i = v->size; // if v->size is 200, then the first i below will be 199
        while ( i-- )
        {
            if ((v->flags & ZV_SAFE_WIPE) && (v->data[i] != NULL))
                item_safewipe(v, v->data[i]);
            if ((!( v->flags & ZV_BYREF )) && (v->data[i] != NULL))
                free(v->data[i]);
        }
    }

    // Reset interested descriptors:
    v->prev_size = v->size;
    v->size = 0;

    // Shrink Vector's capacity:
    _vect_shrink(v);

    // Done.
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
}

void vect_set_wipefunct(vector v, void (*f1)(const void *, size_t))
{
    v->SfWpFunc = (void *)malloc(sizeof(void *));
    if (v->SfWpFunc == NULL)
        throw_error("No memory available to set safe wipe function!\n");

    // Set custom Safe Wipe function:
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

#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    // Check if we need to expand the vector:
    if (v->size >= v->capacity)
        vect_increase_capacity(v);

    // Allocate memory for the new item:
    if ( v->flags & ZV_BYREF )
        v->data[v->size] = (void *)malloc(sizeof(void *));
    else
        v->data[v->size] = (void *)malloc(v->data_size);
    if (v->data[v->size] == NULL )
        throw_error("Not enough memory to add new item in the vector!");

#   if ( ZVECT_FULL_REENTRANT == 1 )
    // If we are in FULL_REENTRANT MODE prepare for potential
    // array copy:
    uint32_t array_changed = 0;
    void **new_data = NULL;
    if ((i < v->size) && (v->size > 0))
    {
        new_data = (void **)malloc(sizeof(void *) * v->capacity);
        if (new_data == NULL)
            throw_error("Not enough memory to resize the vector!");
    } 
    else
        UNUSED(new_data);
#   endif

    // "Shift" right the array of one position to make space for the new item:
    if ((i < v->size) && (v->size > 0))
    {
#   if ( ZVECT_FULL_REENTRANT == 1 )
        array_changed = 1;
        if ( i > 0 )
            vect_memcpy(new_data, v->data, sizeof(void *) * i );
        vect_memcpy(new_data + (i + 1), v->data + i, sizeof(void *) * ( v->size - i ));
        /* zvect_index j;
        if ( i > 0 )
            for (j=0; j <= i; j++)
                new_data[j] = v->data[j];
        for (j=i + 1; j <= v->size; j++)
            new_data[j - 1] = v->data[j];
        */
#   else
        // We can't use the vect_memcpy when not in full reentrant code
        // because it's not safe to use it on the same src and dst.
        vect_memmove(v->data + (i + 1), v->data + i, sizeof(void *) * ( v->size - i ));
#   endif
    }

    // Finally add new value in at the index
#   if ( ZVECT_FULL_REENTRANT == 1 )
    if ( array_changed == 1 )
    {
        if (i < v->size )
        {
            if ( v->flags & ZV_BYREF )
                new_data[i] = (void *)malloc(sizeof(void *));
            else
                new_data[i] = (void *)malloc(v->data_size);
            if (new_data[i] == NULL )
                throw_error("Not enough memory to add new item in the vector!");
        }
        if ( v->flags & ZV_BYREF )
            new_data[i] = (void *)value;
        else
            vect_memcpy(new_data[i], value, v->data_size);
    } else {
        if ( v->flags & ZV_BYREF )
            v->data[i] = (void *)value;
        else
            vect_memcpy(v->data[i], value, v->data_size);
    }
#   else
    if ( i < v->size )
    {
        // We moved chunks of memory so we need to 
        // allocate new memory for the item in position i:
        if ( v->flags & ZV_BYREF )
            v->data[i] = (void *)malloc(sizeof(void *));
        else
            v->data[i] = (void *)malloc(v->data_size);
        if (v->data[i] == NULL )
            throw_error("Not enough memory to add new item in the vector!");
    }
    if ( v->flags & ZV_BYREF )
        v->data[i] = (void *)value;
    else
        vect_memcpy(v->data[i], value, v->data_size);
#   endif

    // Apply changes:
#   if ( ZVECT_FULL_REENTRANT == 1 )
    if (array_changed == 1)
    {
        free(v->data);
        v->data = new_data;
    }
#   endif

    // Increment vector size
    v->prev_size=v->size;
    v->size++;

#   if ( ZVECT_THREAD_SAFE == 1 )
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

#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    // Add value at the specified index:
    vect_memcpy(v->data[i], value, v->data_size);
#   if ( ZVECT_THREAD_SAFE == 1 )
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

    // Reorganise the vector (if needed)
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    // Get the value we are about to remove:
    void *rval;
    if ( v->flags & ZV_BYREF )
    {
        rval = (void *)malloc(sizeof(void *));
        rval = v->data[i];
    } else {
        rval = (void *)malloc(v->data_size);
        vect_memcpy(rval, v->data[i], v->data_size );
        if ( v->flags & ZV_SAFE_WIPE )
            item_safewipe(v, v->data[i]);
    }

#   if ( ZVECT_FULL_REENTRANT == 1 )
    void **new_data = (void **)malloc(sizeof(void *) * v->capacity);
    if (new_data == NULL)
        throw_error("Not enough memory to resize the vector!");

    uint32_t array_changed = 0;
#   endif

    // "shift" left the array of one position:
    if ( (i < (v->size - 1)) && (v->size > 0))
    {
#   if ( ZVECT_FULL_REENTRANT == 1 )
        array_changed = 1;
        if ( i > 0 )
            vect_memcpy(new_data, v->data, sizeof(void *) * i );
        vect_memcpy(new_data + i, v->data + (i + 1), sizeof(void *) * ( v->size - i ));
        /* zvect_index j;
        if ( i > 0 )
            for (j=0; j <= i; j++)
                new_data[j] = v->data[j];
        for (j=i + 1; j <= v->size; j++)
            new_data[j - 1] = v->data[j];
        */
#   else
        // We can't use the vect_memcpy when not in full reentrant code
        // because it's not safe to use it on the same src and dst.
        /* zvect_index j;
        for (j = i + 1; j <= v->size; j++)
        {
            // Just move the pointers to the elements!
            // vect_memcpy(v->data[j - 1], v->data[j], sizeof(void *));
            v->data[j - 1] = v->data[j];
        } */
        vect_memmove(v->data + i, v->data + ( i + 1 ), sizeof(void *) * ( v->size - i ));
#   endif
    }

#if ( ZVECT_FULL_REENTRANT == 1 )
    if (array_changed == 1)
    {
        free(v->data);
        v->data = new_data;
    }
#endif
    
    // Reduce vector size:
    //if ( !(v->flags & ZV_BYREF)  && !(v->data[v->size - 1] == NULL))
    //    free(v->data[v->size - 1]);
    v->prev_size=v->size;
    v->size--;

    // Check if we need to shrink the vector:
    if (4 * v->size < v->capacity)
        vect_decrease_capacity(v);

#   if ( ZVECT_THREAD_SAFE == 1 )
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

#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif

    // "shift" left the data of one position:
    if ( ((start + offset) < (v->size - 1)) && (v->size > 0))
    {
        zvect_index j2;
        if ( start + offset > 0)
            for ( j2 = (start + offset); j2 >= start; j2--)
            {
                if ( v->flags & ZV_SAFE_WIPE )
                    item_safewipe(v, v->data[j2]);
                if ( (!(v->flags & ZV_BYREF)) && (v->data[j2] != NULL) )
                    free(v->data[j2]);
            }
        vect_memmove(v->data + start, v->data + ((start + offset) + 1), sizeof(void *) * ( v->size - (start + offset) ));
    }

    // Reduce vector size:
    if (!(v->flags & ZV_BYREF))
    {
        zvect_index j;
        for ( j = v->size; j >= (v->size - offset); j-- )
            if ( v->data[j] != NULL ) 
                free(v->data[j]);
    }
    v->prev_size=v->size;
    v->size = v->size - (offset + 1);

    // Check if we need to shrink the vector:
    if ((4 * v->size) < v->capacity)
        vect_decrease_capacity(v);

#   if ( ZVECT_THREAD_SAFE == 1 )
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
    // void *temp = (void *)malloc(sizeof(void *));
    // if ( temp == NULL )
    //    throw_error("Not enough memory to swap elements!");

    // Let's swap items:
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    void *temp  = v->data[i2];
    v->data[i2] = v->data[i1];
    v->data[i1] = temp;
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_unlock(v, 1);
#   endif
    // We are done, let's clean up memory
    // free(temp);
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
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v, 1);
#   endif
    for (i = 0; i < v->size; i++)
        (*f)(v->data[i]);
#   if ( ZVECT_THREAD_SAFE == 1 )
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
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v1, 1);
#   endif
    for (i = 0; i < v1->size; i++)
        if ((*f2)(v1->data[i],v2->data[i]))
            (*f1)(v1->data[i]);
#   if ( ZVECT_THREAD_SAFE == 1 )
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
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v1, 2);
#   endif
    for (i = start; i <= max_elements; i++)
        vect_add(v1, v2->data[i]);
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_unlock(v1, 2);
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
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v1, 2);
#   endif
    for (i = start; i <= max_elements; i++)
    {
        vect_add(v1, v2->data[i]);
        vect_remove_at(v2, i);
    }
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_unlock(v1, 2);
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
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_lock(v1, 2);
#   endif
    for (i = 0; i < v2->size; i++)
        vect_add(v1, v2->data[i]);
#   if ( ZVECT_THREAD_SAFE == 1 )
    check_mutex_unlock(v1, 2);
#   endif 
    // Because we are merging two vectors in one
    // after merged v2 to v1 there is no need for
    // v2 to still exists, so let's destroy it to
    // free memory correctly:
    vect_destroy(v2);
}
#endif

/*---------------------------------------------------------------------------*/
