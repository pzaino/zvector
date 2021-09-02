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
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include vector.h header
#include "zvector.h"

// Declare Vector status flags:
enum {
	ZVS_NONE = 0,              // Set or Reset vector's status register.
	ZVS_CUST_WIPE_ON = 1 << 0, // Sets the bit to indicate a custom secure wipe
				   // function has been set.
	ZVS_CUST_WIPE_OFF = 0 << 0 // Reset ZVS_CUST_WIPE bit to off.
};

#if (OS_TYPE == 1)
#	if (!defined(macOS))
/*  Improve PThreads on Linux.
 *  macOS seems to be handling pthreads
 *  sligthely differently than Linux so
 * avoid using the same trick on macOS.
 */
#	ifndef _POSIX_C_SOURCE
#		define _POSIX_C_SOURCE 200112L
#	endif // _POSIX_C_SOURCE
#	define __USE_UNIX98
#	endif // macOS
#endif // OS_TYPE

// Include non-ANSI Libraries
// only if the user has requested
// special extensions:
#if (OS_TYPE == 1)
#	if ( !defined(macOS) )
#		include <malloc.h>
#	endif
#	include <xmmintrin.h>
#endif // OS_TYPE
#if (ZVECT_THREAD_SAFE == 1)
#	if MUTEX_TYPE == 1
#		include <pthread.h>
#	elif MUTEX_TYPE == 2
#		include <psapi.h>
#		include <windows.h>
#	endif // MUTEX_TYPE
#endif // ZVECT_THREAD_SAFE

#ifndef ZVECT_MEMX_METHOD
#	define ZVECT_MEMX_METHOD 1
#endif

#if defined(Arch32)
#	define ADDR_TYPE1 uint32_t
#	define ADDR_TYPE2 uint32_t
#	define ADDR_TYPE3 uint16_t
#else
#	define ADDR_TYPE1 uint64_t
#	define ADDR_TYPE2 uint64_t
#	define ADDR_TYPE3 uint32_t
#endif // Arch32

// Useful macros
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define UNUSED(x) (void)x

// Define the vector data structure:
struct _vector {
	zvect_index init_capacity;	// - Initial Capacity (this is set at
					//   creation time).
	zvect_index cap_left;		// - Max capacity allocated on the left.
	zvect_index cap_right;		// - Max capacity allocated on the right.
	zvect_index begin;		// - First vector's Element Pointer
	zvect_index prev_end;		// - Used when clearing a vector.
	zvect_index end;		// - Current Array size. size - 1 gives
					//   us the pointer to the last element
					//   in the vector.
	int64_t alloc_dir;		// - This variable tracks where we have allocated
					//   memory for this vector ( < 0 = left, 0 = no allocations yet, > 0 right)
	size_t data_size;		// - User DataType size.
	uint32_t flags;			// - If this flag set is used to
					//   represent ALL Vector's properties.
					//   It contains bits that set Secure
					//   Wipe, Auto Shrink, Pass Items By
					//   Ref etc.
	uint32_t status;		// - Internal vector Status Flags
#if (ZVECT_THREAD_SAFE == 1)
#	if MUTEX_TYPE == 0
	volatile uint8_t lock_type;	// - This field contains the lock used
					//   for this Vector.
	void *lock;			// - Vector's mutex for thread safe
					//   micro-transactions or user locks.
#	elif MUTEX_TYPE == 1
	volatile uint8_t lock_type;	// - This field contains the lock used
					//   for this Vector.
	pthread_mutex_t lock;		// - Vector's mutex for thread safe
					//   micro-transactions or user locks.
#	elif MUTEX_TYPE == 2
	volatile uint8_t lock_type;	// - This field contains the lock used
					//   for this Vector.
	CRITICAL_SECTION lock;		// - Vector's mutex for thread safe
					//   micro-transactions or user locks.
#	endif // MUTEX_TYPE
#endif  // ZVECT_THREAD_SAFE
#ifdef ZVECT_DMF_EXTENSIONS
	zvect_index balance;		// - Used by the Adaptive Binary Search
					//   to improve performance.
	zvect_index bottom;	 	// - Used to optimise Adaptive Binary
					//   Search.
#endif  // ZVECT_DMF_EXTENSIONS
	void (*SfWpFunc)(const void *item, size_t size);
					// - Pointer to a CUSTOM Safe Wipe
					//   function (optional) needed only
					//   for Secure Wiping special
					//   structures.
	void **data ZVECT_DATAALIGN;
					// - Vector's storage.
} ZVECT_DATAALIGN;

// Initialisation state:
static uint32_t init_state = 0;

/***********************
 ** Support Functions **
 ***********************/

static void throw_error(const char *error_message) {
#if OS_TYPE == 1
	fprintf(stderr, "Error: %s\n", error_message);
	abort();
#else
	printf("Error: %s\n", error_message);
	exit(-1);
#endif // OS_TYPE
}

static inline void vect_check(vector x) {
	if (x == NULL)
		throw_error("Vector not defined!");
}

static inline zvect_index _vect_capacity(vector v) {
	return v->cap_left + v->cap_right;
}

static inline zvect_index _vect_size(vector v) {
	return ( v->end - v->begin );
}

static inline void item_safewipe(vector v, const void *item) {
	if (item != NULL) {
		if (!(v->status & ZVS_CUST_WIPE_ON)) {
			memset((void *)item, 0, v->data_size);
		} else {
			(*(v->SfWpFunc))(item, v->data_size);
		}
	}
}

static void _free_items(vector v, zvect_index first, zvect_index offset) {
	if (_vect_size(v) == 0)
		return;

	register zvect_index j;
	for (j = (first + offset); j >= first; j--) {
		if (v->data[v->begin + j] != NULL) {
			if (v->flags & ZV_SEC_WIPE)
				item_safewipe(v, v->data[v->begin + j]);
			if (!(v->flags & ZV_BYREF)) {
				free(v->data[v->begin + j]);
				v->data[v->begin + j] = NULL;
			}
		}
		if (j == first)
			break;	// this is required if we are using
				// uint and the first element is elemnt
				// 0, because on GCC a uint will fail
				// then check in the for ( j >= first )
				// in this particoular case!
	}
}

#if (ZVECT_MEMX_METHOD == 0)
static inline
#endif // ZVECT_MEMX_METHOD
void * vect_memcpy(void *__restrict dst, const void *__restrict src, size_t size) {
#if (ZVECT_MEMX_METHOD == 0)
	// Using regular memcpy
	// If you are using ZVector on Linux/macOS/BSD/Windows
	// your should stick to this one!
	return memcpy(dst, src, size);
#elif (ZVECT_MEMX_METHOD == 1)
	// Using improved memcpy (where improved means for
	// embedded systems only!):
	register size_t i;
	if (size > 0) {
		if (((uintptr_t)dst % sizeof(ADDR_TYPE1) == 0) &&
		    ((uintptr_t)src % sizeof(ADDR_TYPE1) == 0) &&
		    (size % sizeof(ADDR_TYPE1) == 0)) {
			ADDR_TYPE1 *pExDst = (ADDR_TYPE1 *)dst;
			ADDR_TYPE1 const *pExSrc = (ADDR_TYPE1 const *)src;
			size_t end = size / sizeof(ADDR_TYPE1);
			for (i = 0; i < end; i++) {
				// The following should be compiled as: (-O2 on x86_64)
				//         mov     rdi, QWORD PTR [rsi+rcx]
				//         mov     QWORD PTR [rax+rcx], rdi
				*pExDst++ = *pExSrc++;
			}
		} else {
			return memcpy(dst, src, size);
		}
	}
	return dst;
#endif // ZVECT_MEMX_METHOD
}

static inline void *vect_memmove(void *__restrict dst,
                                 const void *__restrict src, size_t size) {
	return memmove(dst, src, size);
}

#if (ZVECT_THREAD_SAFE == 1)
static volatile bool lock_enabled = true;
#	if MUTEX_TYPE == 0
#		define ZVECT_THREAD_SAFE 0
#	elif MUTEX_TYPE == 1
static inline void mutex_lock(pthread_mutex_t *lock) {
	pthread_mutex_lock(lock);
}

static inline void mutex_unlock(pthread_mutex_t *lock) {
	pthread_mutex_unlock(lock);
}

static inline void mutex_alloc(pthread_mutex_t *lock) {
#	if (!defined(macOS))
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(lock, &Attr);
#	else
	pthread_mutex_init(lock, NULL);
#	endif // macOS
}

static inline void mutex_destroy(pthread_mutex_t *lock) {
	pthread_mutex_unlock(lock);
	pthread_mutex_destroy(lock);
}
#	elif MUTEX_TYPE == 2
static volatile bool lock_enabled = true;

static inline void mutex_lock(CRITICAL_SECTION *lock) {
	EnterCriticalSection(lock);
}

static inline void mutex_unlock(CRITICAL_SECTION *lock) {
	LeaveCriticalSection(lock);
}

static inline void mutex_alloc(CRITICAL_SECTION *lock) {
	// InitializeCriticalSection(lock);
	InitializeCriticalSectionAndSpinCount(lock, 32);
}

static inline void mutex_destroy(CRITICAL_SECTION *lock) {
	DeleteCriticalSection(lock);
}
#	endif // MUTEX_TYPE
#endif // ZVECT_THREAD_SAFE

#if (ZVECT_THREAD_SAFE == 1)
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
static inline void check_mutex_lock(vector v, volatile uint8_t lock_type) {
	if (lock_enabled) {
		if (lock_type >= v->lock_type) {
			mutex_lock(&(v->lock));
			v->lock_type = lock_type;
		}
	}
}

static inline void check_mutex_unlock(vector v, volatile uint8_t lock_type) {
	if (lock_enabled) {
		if (lock_type == v->lock_type) {
			v->lock_type = 0;
			mutex_unlock(&(v->lock));
		}
	}
}
#endif // ZVECT_THREAD_SAFE

/******************************
 **    ZVector Primitives    **
 ******************************/

void init_zvect(void) {
#if (OS_TYPE == 1)
#	if ( !defined(macOS) )
	// mallopt(M_MXFAST, 80*sizeof(size_t)/4);
#	endif
#endif  // OS_TYPE == 1

	// We are done initialising ZVector so set the following
	// to one, so this function will not longer be called:
	init_state = 1;
}

/*---------------------------------------------------------------------------*/
// Vector Size and Capacity management functions:

// This function double the CAPACITY of a vector.
static void _vect_increase_capacity(vector v, zvect_index direction) {
	zvect_index new_capacity = 0;

	void **new_data = NULL;
	zvect_index nb = 0, ne = 0;
	if (!direction)
	{
		// Increase capacity on the left side of the vector:

		// Get actual left capacity and double it
		//if ( v->cap_left == 0 )
		//	v->cap_left = v->init_capacity / 2;

		new_capacity = v->cap_left * 2;

		new_data = (void **)malloc(sizeof(void *) * (new_capacity + v->cap_right));
		if (new_data == NULL)
			throw_error("Not enough memory to extend the vector capacity!");

		nb = v->cap_left;
		ne = ( nb + (v->end - v->begin) );
		vect_memcpy(new_data + nb, v->data + v->begin, sizeof(void *) * (v->end - v->begin) );
	} else {
		// Increase capacity on the right side of the vector:

		// Get actual left capacity and double it
		//if ( v->cap_right == 0 )
		//	v->cap_right = v->init_capacity / 2;

		new_capacity = v->cap_right * 2;

		new_data = (void **)realloc(v->data, sizeof(void *) * (v->cap_left + new_capacity));
		if (new_data == NULL)
			throw_error("Not enough memory to extend the vector capacity!");
	}

	// Apply changes and release memory
	if (!direction)
		free(v->data);
	v->data = new_data;
	if (direction == 0)
	{
		v->cap_left = new_capacity;
		v->end = ne;
		v->begin = nb;
	} else {
		v->cap_right = new_capacity;
	}
}

// This function halves the CAPACITY of a vector.
static void _vect_decrease_capacity(vector v, zvect_index direction) {
	// Check if new capacity is smaller than initial capacity
	if ( _vect_capacity(v) <= v->init_capacity)
		return;

	zvect_index new_capacity = 0;

	void **new_data = NULL;
	zvect_index nb = 0, ne = 0;
	if (!direction)
	{
		// Decreasing on the left:
		new_capacity = v->cap_left / 2;
		if (new_capacity < ( v->init_capacity / 2 ))
			new_capacity = v->init_capacity / 2;

		new_capacity = max( (_vect_size(v) / 2), new_capacity);

		new_data = (void **)malloc(sizeof(void *) * (new_capacity + v->cap_right));
		if (new_data == NULL)
			throw_error("Not enough memory to resize the vector!");

		nb = ( new_capacity / 2 );
		ne = ( nb + (v->end - v->begin) );
		vect_memcpy(new_data + nb, v->data + v->begin, sizeof(void *) * (v->end - v->begin) );
	} else {
		// Decreasing on the right:
		new_capacity = v->cap_right / 2;
		if (new_capacity < ( v->init_capacity / 2 ))
			new_capacity = v->init_capacity / 2;

		new_capacity = max( (_vect_size(v) / 2), new_capacity);

		new_data = (void **)realloc(v->data, sizeof(void *) * (v->cap_left + new_capacity));
		if (new_data == NULL)
			throw_error("Not enough memory available to resize the vector!");
	}

	// Apply changes and release memory:
	if (!direction)
		free(v->data);
	v->data = new_data;
	if (direction == 0)
	{
		v->cap_left = new_capacity;
		v->end = ne;
		v->begin = nb;
	} else {
		v->cap_right = new_capacity;
	}
}

// Thi sfunction shrinks the CAPACITY of a vector
// not its size. To reduce the size of a vector we
// need to remove items from it.
void _vect_shrink(vector v) {
	// Check if the vector exists:
	vect_check(v);

	if (v->init_capacity < 2)
		v->init_capacity = 2;

	if (_vect_capacity(v) == v->init_capacity)
		return;

	if ( _vect_capacity(v) <= _vect_size(v))
		return;

	// Determine the correct shrunk size:
	zvect_index new_capacity;
	if (_vect_size(v) < v->init_capacity)
		new_capacity = v->init_capacity;
	else
		new_capacity = _vect_size(v) + 2;

	// shrink the vector:
	/*
	void **new_data = (void **)realloc(v->data, sizeof(void *) * new_capacity);
	if (new_data == NULL)
		throw_error("Not enough memory available to shrink the vector!");
	*/

	void **new_data = (void **)malloc(sizeof(void *) * new_capacity);
	if (new_data == NULL)
		throw_error("Not enough memory available to shrink the vector!");

	zvect_index ne = 0, nb = 0;
	nb = ( new_capacity / 2 );
	ne = ( nb + (v->end - v->begin) );
	vect_memcpy(new_data + nb, v->data + v->begin, sizeof(void *) * (v->end - v->begin) );

	// Apply changes:
	free(v->data);
	v->data = new_data;
	v->end = ne;
	v->begin = nb;
	v->cap_left = new_capacity / 2;
	v->cap_right = new_capacity / 2;
}

void vect_shrink(vector v) {
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	_vect_shrink(v);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

/*---------------------------------------------------------------------------*/

/***********************
 **    ZVector API    **
 ***********************/

/*---------------------------------------------------------------------------*/
// Vector Structural Information report:

bool vect_is_empty(vector v) {
	// Check if the vector exists
	vect_check(v);

	return _vect_size(v) == 0;
}

zvect_index vect_size(vector v) {
	// Check if the vector exists
	vect_check(v);

	// return v->end;
	return _vect_size(v);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Creation and Destruction:

vector vect_create(const size_t init_capacity, const size_t item_size,
                   const uint32_t properties) {
	// If ZVector has not been initialised yet, then initialise it
	// when creating the first vector:
	if (init_state == 0)
		init_zvect();

	// Create the vector first:
	vector v = (vector)malloc(sizeof(struct _vector));
	if (v == NULL)
		throw_error("Not enough memory to allocate the vector!");

	// Initialize the vector:
	v->prev_end = 0;
	v->end = 0;
	if (item_size == 0)
		v->data_size = ZVECT_DEFAULT_DATA_SIZE;
	else
		v->data_size = item_size;

	zvect_index capacity = init_capacity;
	if (init_capacity == 0)
	{
		v->cap_left = ZVECT_INITIAL_CAPACITY / 2;
		v->cap_right= ZVECT_INITIAL_CAPACITY / 2;
	} else {

		if (init_capacity <= 4)
			capacity = 4;

		v->cap_left = capacity / 2;
		v->cap_right= capacity / 2;
	}
	v->begin = 1;
	v->end = 1;

	v->init_capacity = v->cap_left + v->cap_right;
	v->flags = properties;
	v->SfWpFunc = NULL;
	v->status = 0;
#ifdef ZVECT_DMF_EXTENSIONS
	v->balance = 0;
	v->bottom = 0;
#endif // ZVECT_DMF_EXTENSIONS

	v->data = NULL;

#if (ZVECT_THREAD_SAFE == 1)
	v->lock_type = 0;
	mutex_alloc(&(v->lock));
#endif

	// Allocate memory for the vector storage area
	v->data = (void **)malloc(sizeof(void *) * _vect_capacity(v));
	if (v->data == NULL)
		throw_error("Not enough memory to allocate the vector storage area!");

	// Return the vector to the user:
	return v;
}

static void _vect_destroy(vector v, uint32_t flags) {
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif
	// Clear the vector:
	if ((_vect_size(v) > 0) && (flags & 1)) {
		// Secure Wipe the vector (or just free) depending on vector properties:
		zvect_index i = _vect_size(v); // if _vect_size(v) is 200, then the first i below will be 199
		while (i--) {
			if (v->data[v->begin + i] != NULL) {
				if ((v->flags & ZV_SEC_WIPE))
					item_safewipe(v, v->data[v->begin + i]);

				if (!(v->flags & ZV_BYREF)) {
					free(v->data[v->begin + i]);
					v->data[v->begin + i] = NULL;
				}
			}
		}
	}

	// Reset interested descriptors:
	v->prev_end = _vect_size(v);
	v->end = 0;

	// Shrink Vector's capacity:
	if (_vect_capacity(v) > v->init_capacity)
		_vect_shrink(v);

	v->prev_end = 0;
	v->init_capacity = 0;
	v->cap_left = 0;
	v->cap_right = 0;

	// Destroy it:
	if ((v->status & ZVS_CUST_WIPE_ON)) {
		free(v->SfWpFunc);
		v->SfWpFunc = NULL;
	}

	if (v->data != NULL) {
		free(v->data);
		v->data = NULL;
	}

	// Clear vector status flags:
	v->status = 0;

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
	mutex_destroy(&(v->lock));
#endif

	// All done and freed, so we can safely
	// free the vector itself:
	free(v);
	v = NULL;
}

void vect_destroy(vector v) {
	// Check if the vector exists:
	vect_check(v);

	// Call _vect_destroy with flags set to 1
	// to destroy data according to the vector
	// properties:
	_vect_destroy(v, 1);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Thread Safe user functions:

#if (ZVECT_THREAD_SAFE == 1)
void vect_lock_enable(void) {
	lock_enabled = true;
}

void vect_lock_disable(void) {
	lock_enabled = false;
}

inline void vect_lock(vector v) {
	check_mutex_lock(v, 3);
}

inline void vect_unlock(vector v) {
	check_mutex_unlock(v, 3);
}
#endif

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Data Storage functions:

void vect_clear(vector v) {
	// check if the vector exists:
	vect_check(v);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	// Clear the vector:
	if (!vect_is_empty(v))
		_free_items(v, 0, (_vect_size(v) - 1));

	// Reset interested descriptors:
	v->prev_end = _vect_size(v);
	v->begin = 1;
	v->end = 1;

	// Shrink Vector's capacity:
	_vect_shrink(v);

	// Done.
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_set_wipefunct(vector v, void (*f1)(const void *, size_t)) {
	v->SfWpFunc = (void *)malloc(sizeof(void *));
	if (v->SfWpFunc == NULL)
		throw_error("No memory available to set safe wipe function!\n");

	// Set custom Safe Wipe function:
	v->SfWpFunc = f1;
	// vect_memcpy(v->SfWpFunc, f1, sizeof(void *));
	v->status |= ZVS_CUST_WIPE_ON;
}

// inline implementation for all add(s):
static inline void _vect_add_at(vector v, const void *value,
                                const zvect_index i) {
	// check if the vector exists:
	vect_check(v);

	zvect_index vsize = _vect_size(v);

	// Check if the provided index is out of bounds:
	if (i > vsize)
		throw_error("Index out of bounds!");

#if (ZVECT_FULL_REENTRANT == 1)
	// If we are in FULL_REENTRANT MODE prepare for potential
	// array copy:
	void **new_data = NULL;
	if (i < vsize) {
		new_data = (void **)malloc(sizeof(void *) * _vect_capacity(v));
		if (new_data == NULL)
			throw_error("Not enough memory to resize the vector!");
	}
#endif

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	// Check if we need to expand the vector:
	//if ( vsize >= _vect_capacity(v) )
	//	_vect_increase_capacity(v, i);

	if (!i) {
		// Check if we need to expand on the left side:
		if ( v->begin == 0 || v->cap_left == 1 )
			_vect_increase_capacity(v, 0);
	} else {
		// Check if we need to expand on thr right side:
		if ( v->end >= v->cap_right )
			_vect_increase_capacity(v, 1);
	}

	// Allocate memory for the new item:
	zvect_index base = v->begin;
	if (!i) {
		// Prepare left side of the vector:
		base--;
		if (!(v->flags & ZV_BYREF)) {
			v->data[base] = (void *)malloc(v->data_size);
			if (v->data[base] == NULL)
				throw_error("Not enough memory to add new item in the vector!");
		}
	} else if (i == vsize) {
		// Prepare right side of the vector:
		if (!(v->flags & ZV_BYREF)) {
			v->data[base + vsize] = (void *)malloc(v->data_size);
			if (v->data[base + vsize] == NULL)
				throw_error("Not enough memory to add new item in the vector!");
		}
	}

	// "Shift" right the array of one position to make space for the new item:
	int16_t array_changed = 0;
	if ((i < vsize) && (i != 0)) {
		array_changed = 1;
#if (ZVECT_FULL_REENTRANT == 1)
		// Algorithm to try to copy an array of pointers as fast as possible:
		if (i > 0)
			vect_memcpy(new_data + base, v->data + base, sizeof(void *) * i);
		vect_memcpy(new_data + base + (i + 1), v->data + base + i,
			    sizeof(void *) * (_vect_size(v) - i));
#else
		// We can't use the vect_memcpy when not in full reentrant code
		// because it's not safe to use it on the same src and dst.
		vect_memmove(v->data + base + (i + 1), v->data + base + i,
			     sizeof(void *) * (vsize - i));
#endif  // (ZVECT_FULL_REENTRANT == 1)
	}

	// Add new value in (at the index i):
#if (ZVECT_FULL_REENTRANT == 1)
	if (array_changed) {
		if (v->flags & ZV_BYREF) {
			new_data[base + i] = (void *)value;
		} else {
			new_data[base + i] = (void *)malloc(v->data_size);
			if (new_data[base + i] == NULL)
			throw_error("Not enough memory to add new item in the vector!");
			vect_memcpy(new_data[base + i], value, v->data_size);
		}
	} else {
		if (v->flags & ZV_BYREF)
			v->data[base + i] = (void *)value;
		else
			vect_memcpy(v->data[base + i], value, v->data_size);
	}
#else
	if (array_changed && !(v->flags & ZV_BYREF)) {
		// We moved chunks of memory so we need to
		// allocate new memory for the item at position i:
		v->data[base + i] = (void *)malloc(v->data_size);
		if (v->data[base + i] == NULL)
			throw_error("Not enough memory to add new item in the vector!");
	}
	if (v->flags & ZV_BYREF)
		v->data[base + i] = (void *)value;
	else
		vect_memcpy(v->data[base + i], value, v->data_size);
#endif  // (ZVECT_FULL_REENTRANT == 1)

	// Apply changes:
#if (ZVECT_FULL_REENTRANT == 1)
	if (array_changed) {
		free(v->data);
		v->data = new_data;
	}
#endif
	// Increment vector size
	v->prev_end = vsize;
	if (!i) {
		v->begin = base;
	} else {
		v->end++;
	}

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
	return;

#if (ZVECT_FULL_REENTRANT == 1)
	UNUSED(new_data);
#endif
}

inline void vect_push(vector v, const void *value) {
	// Add an item at the END (top) of the vector
	_vect_add_at(v, value, _vect_size(v));
}

void vect_add(vector v, const void *value) {
	// Add an item at the END of the vector
	_vect_add_at(v, value, _vect_size(v));
}

void vect_add_at(vector v, const void *value, const zvect_index i) {
	// Add an item at position "i" of the vector
	_vect_add_at(v, value, i);
}

void vect_add_front(vector v, const void *value) {
	// Add an item at the FRONT of the vector
	_vect_add_at(v, value, 0);
}

// inline implementation for all get(s):
static inline void *_vect_get_at(vector v, const zvect_index i) {
	// check if the vector exists:
	vect_check(v);

	// Check if passed index is out of bounds:
	if (i >= _vect_size(v))
		throw_error("Index out of bounds!");

	// Return found element:
	return v->data[v->begin + i];
}

void *vect_get(vector v) {
	return _vect_get_at(v, _vect_size(v) - 1);
}

void *vect_get_at(vector v, const zvect_index i) {
	return _vect_get_at(v, i);
}

void *vect_get_front(vector v) {
	return _vect_get_at(v, 0);
}

// inlin eimplementation for all put:
static inline void _vect_put_at(vector v, const void *value,
                                const zvect_index i) {
	// check if the vector exists:
	vect_check(v);

	// Check if the index passed is out of bounds:
	if (i >= _vect_size(v))
		throw_error("Index out of bounds!");

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	// Add value at the specified index, considering
	// if the vector has ZV_BYREF property enabled:
	if ( v->flags & ZV_BYREF ) {
		void *temp = v->data[v->begin + i];
		v->data[v->begin + i] = (void *)value;
		if ( v->flags & ZV_SEC_WIPE )
			memset((void *)temp, 0, v->data_size);
	} else {
		vect_memcpy(v->data[v->begin + i], value, v->data_size);
	}

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_put(vector v, const void *value) {
	_vect_put_at(v, value, _vect_size(v) - 1);
}

void vect_put_at(vector v, const void *value, const zvect_index i) {
	_vect_put_at(v, value, i);
}

void vect_put_front(vector v, const void *value) {
	_vect_put_at(v, value, 0);
}

// This is the inline implementation for all the remove and pop
static inline void *_vect_remove_at(vector v, const zvect_index i) {
	// check if the vector exists:
	vect_check(v);

	zvect_index vsize = _vect_size(v);

	// Check if the index is out of bounds:
	if (i >= vsize)
		throw_error("Index out of bounds!");

	// If the vector is empty just return null
	if (vsize == 0)
		return NULL;

	// Check if the vector got corrupted
	if (v->begin > v->end)
		throw_error("Vector corrupted, left side overwritten the right side!");

	// Start processing the vector:
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

#if (ZVECT_FULL_REENTRANT == 1)
	// Allocate memory for support Data Structure:
	void **new_data = (void **)malloc(sizeof(void *) * _vect_capacity(v));
	if (new_data == NULL)
		throw_error("Not enough memory to resize the vector!");
#endif

	// Get the value we are about to remove:
	// If the vector is set as ZV_BYREF then just copy the pointer to the item
	// If the vector is set as regular then copy the item
	void *rval;
	zvect_index base = v->begin;
	if (v->flags & ZV_BYREF) {
		rval = v->data[base + i];
	} else {
		rval = (void *)malloc(v->data_size);
		vect_memcpy(rval, v->data[base + i], v->data_size);
		// If the vector is set for secure wipe and we copied the item
		// then we need to wipe the old copy:
		if (v->flags & ZV_SEC_WIPE)
			item_safewipe(v, v->data[base + i]);
	}

	// "shift" left the array of one position:
	uint16_t array_changed = 0;
	if ( i != 0 ) {
		if ((i < (vsize - 1)) && (vsize > 0)) {
			array_changed = 1;
			free(v->data[base + i]);
#if (ZVECT_FULL_REENTRANT == 1)
			vect_memcpy(new_data + base, v->data + base, sizeof(void *) * i);
			vect_memcpy(new_data + base + i, v->data + base + (i + 1),
				sizeof(void *) * (vsize - i));
#else
			// We can't use the vect_memcpy when not in full reentrant code
			// because it's not safe to use it on the same src and dst.
			vect_memmove(v->data + base + i, v->data + base + (i + 1),
				sizeof(void *) * (vsize - i));
#endif
		}
	} else {
		if ( base < v->end ) {
			array_changed = 1;
			free(v->data[base]);
		}
	}

  // Reduce vector size:
#if (ZVECT_FULL_REENTRANT == 0)
	if (!(v->flags & ZV_BYREF)) {
		if (!array_changed)
			_free_items(v, vsize - 1, 0);
	}
#else
	// Apply changes
	if (array_changed) {
		free(v->data);
		v->data = new_data;
	}
#endif
	v->prev_end = vsize;
	if ( i != 0 ) {
		if (v->end > v->begin) {
			v->end--;
		} else {
			v->end = v->begin;
		}
	} else {
		if (v->begin < v->end) {
			v->begin++;
		} else {
			v->begin = v->end;
		}
	}
	// Check if we need to shrink vector's capacity:
	if ((4 * vsize) < _vect_capacity(v) )
		_vect_decrease_capacity(v, i);

	// All done, return control:
#if (ZVECT_THREAD_SAFE == 1)
 	check_mutex_unlock(v, 1);
#endif

	return rval;
}

inline void *vect_pop(vector v) {
	return _vect_remove_at(v, _vect_size(v) - 1);
}

void *vect_remove(vector v) {
	return _vect_remove_at(v, _vect_size(v) - 1);
}

void *vect_remove_at(vector v, const zvect_index i) {
  return _vect_remove_at(v, i);
}

void *vect_remove_front(vector v) { return _vect_remove_at(v, 0); }

// This is the inline implementation for all the delete
static inline void _vect_delete_at(vector v, const zvect_index start,
                                   const zvect_index offset) {
	// check if the vector exists:
	vect_check(v);

	zvect_index vsize = _vect_size(v);

	// Check if the index is out of bounds:
	if ((start + offset) >= vsize)
		throw_error("Index out of bounds!");

	// If the vector is empty just return null
	if (vsize == 0)
		return;

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	uint16_t array_changed = 0;

	// "shift" left the data of one position:
	zvect_index tot_items = start + offset;
	if ( tot_items < (vsize - 1) ) {
		array_changed = 1;
		_free_items(v, start, offset);
		if ( tot_items )
			vect_memmove(v->data + (v->begin + start), v->data + (v->begin + (tot_items + 1)),
				sizeof(void *) * (vsize - start));
	}

	// Reduce vector size:
	if (!(v->flags & ZV_BYREF)) {
		if (!array_changed)
			_free_items(v, ((vsize - 1) - offset), offset);
	}
	v->prev_end = vsize;
	if ( start ) {
		if ((v->end - (offset + 1)) > v->begin) {
			v->end -= (offset + 1);
		} else {
			v->end = v->begin;
		}
	} else {
		if ((v->begin + (offset + 1)) < v->end) {
			v->begin += (offset + 1);
		} else {
			v->begin = v->end;
		}
	}

	// Check if we need to shrink the vector:
	if ((4 * vsize) < _vect_capacity(v))
		_vect_decrease_capacity(v, start);
	/* if (!start) {
		// Check if we need to expand on the left side:
		if ( (v->begin * 4) < v->cap_left )
			_vect_decrease_capacity(v, 0);
	} else {
		// Check if we need to expand on thr right side:
		if ( (v->end * 4) < v->cap_right )
			_vect_decrease_capacity(v, 1);
	} */

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_delete(vector v) {
	_vect_delete_at(v, _vect_size(v) - 1, 0);
}

void vect_delete_at(vector v, zvect_index i) {
	_vect_delete_at(v, i, 0);
}

void vect_delete_range(vector v, const zvect_index first_element,
                       const zvect_index last_element) {
	zvect_index end = (last_element - first_element);
	_vect_delete_at(v, first_element, end);
}

void vect_delete_front(vector v) {
	_vect_delete_at(v, 0, 0);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Data Manipoulation functions
#ifdef ZVECT_DMF_EXTENSIONS

void vect_swap(vector v, const zvect_index i1, const zvect_index i2) {
	// check if the vector exists:
	vect_check(v);

	// Check parameters:
	if (i1 > _vect_size(v))
		throw_error("Index out of bounds!");

	if (i2 > _vect_size(v))
		throw_error("Index out of bounds!");

	if (i1 == i2)
		return;

	// Let's swap items:
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	void *temp = v->data[v->begin + i2];
	v->data[v->begin + i2] = v->data[v->begin + i1];
	v->data[v->begin + i1] = temp;

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_swap_range(vector v, const zvect_index s1, const zvect_index e1,
                     const zvect_index s2) {
	// check if the vector exists:
	vect_check(v);

	// Check parameters:
	zvect_index end = e1;
	if (e1 != 0)
		end = e1 - s1;

	if ((s1 + end) > _vect_size(v))
		throw_error("Index out of bounds!");

	if ((s2 + end) > _vect_size(v))
		throw_error("Index out of bounds!");

	if (s2 < (s1 + end))
		throw_error("Index out of bounds!");

	if (s1 == s2)
		return;

	// Let's swap items:
	register zvect_index j, i;
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	for (j = s1; j <= (s1 + end); j++) {
		i = j - s1;
		void *temp = v->data[v->begin + j];
		v->data[v->begin + j] = v->data[v->begin + (s2 + i)];
		v->data[v->begin + (s2 + i)] = temp;
	}

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_rotate_left(vector v, const zvect_index i) {
	// check if the vector exists:
	vect_check(v);

	// Check parameters:
	if (i == 0)
		return;

	if (i > _vect_size(v))
		throw_error("Index out of bounds!");

	if (i == _vect_size(v))
		return;

	// Process the vector
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	if (i == 1) {
		// Rotate left the vector of 1 position:
		void *temp = v->data[0];
		vect_memmove(v->data, v->data + 1, sizeof(void *) * (_vect_size(v) - 1));
		v->data[_vect_size(v) - 1] = temp;
	} else {
		void **new_data = (void **)malloc(sizeof(void *) * _vect_capacity(v));
		if (new_data == NULL)
			throw_error("Not enough memory to rotate the vector!");

		// Rotate left the vector of i positions:
		vect_memcpy(new_data + v->begin, v->data + v->begin, sizeof(void *) * i);
		vect_memmove(v->data + v->begin, v->data + v->begin + i, sizeof(void *) * (_vect_size(v) - i));
		vect_memcpy(v->data + v->begin + (_vect_size(v) - i), new_data + v->begin, sizeof(void *) * i);

		free(new_data);
	}

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_rotate_right(vector v, const zvect_index i) {
	// check if the vector exists:
	vect_check(v);

	// Check parameters:
	if (i == 0)
		return;

	if (i > _vect_size(v))
		throw_error("Index out of bounds!");

	if (i == _vect_size(v))
		return;

	// Process the vector
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	if (i == 1) {
		// Rotate right the vector of 1 position:
		void *temp = v->data[v->begin + _vect_size(v) - 1];
		vect_memmove(v->data + v->begin + 1, v->data + v->begin, sizeof(void *) * (_vect_size(v) - 1));
		v->data[v->begin] = temp;
	} else {
		void **new_data = (void **)malloc(sizeof(void *) * _vect_capacity(v));
		if (new_data == NULL)
			throw_error("Not enough memory to rotate the vector!");

		// Rotate right the vector of i positions:
		vect_memcpy(new_data + v->begin, v->data + v->begin + (_vect_size(v) - i), sizeof(void *) * i);
		vect_memmove(v->data + v->begin + i, v->data + v->begin, sizeof(void *) * (_vect_size(v) - i));
		vect_memcpy(v->data + v->begin, new_data + v->begin, sizeof(void *) * i);

		free(new_data);
	}

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

#ifdef TRADITIONAL_QSORT
static inline zvect_index
_partition(vector v, zvect_index low, zvect_index high,
           int (*compare_func)(const void *, const void *)) {
	if (high >= _vect_size(v))
		high = _vect_size(v) - 1;

	void *pivot = v->data[v->begin + high];
	zvect_index i = (low - 1);

	zvect_index j;
	for (j = low; j <= (high - 1); j++) {
		// v->data[j] <= pivot
		if ((*compare_func)(v->data[v->begin + j], pivot) <= 0)
			vect_swap(v, ++i, j);
	}

	vect_swap(v, i + 1, high);
	return (i + 1);
}

static void _vect_qsort(vector v, zvect_index low, zvect_index high,
                        int (*compare_func)(const void *, const void *)) {
	if (low < high) {
		zvect_index pi = _partition(v, low, high, compare_func);
		_vect_qsort(v, low, pi - 1, compare_func);
		_vect_qsort(v, pi + 1, high, compare_func);
	}
}
#endif // TRADITIONAL_QSORT

#ifndef TRADITIONAL_QSORT
// This is my much faster imlementation of a quicksort algorithm
// it foundamentally use the 3 ways partitioning adapted and improved
// to dela with arrays of pointers together with having a custom
// compare function:
static void _vect_qsort(vector v, zvect_index l, zvect_index r,
                        int (*compare_func)(const void *, const void *)) {
	if (r <= l)
	return;

	register zvect_index k = 0;
	zvect_index i, p, j, q;
	void *ref_val = NULL;

	// l = left (also low)
	if (l > 0)
		i = l - 1;
	else
		i = l;
	// r = right (also high)
	j = r;
	p = i;
	q = r;
	ref_val = v->data[v->begin + r];

	for (;;) {
		while ((*compare_func)(v->data[v->begin + i], ref_val) < 0)
			i++;
		while ((*compare_func)(ref_val, v->data[(--j) + v->begin]) < 0)
			if (j == l)
				break;
		if (i >= j)
			break;
		vect_swap(v, i, j);
		if ((*compare_func)(v->data[v->begin + i], ref_val) == 0) {
			p++;
			vect_swap(v, p, i);
		}
		if ((*compare_func)(ref_val, v->data[v->begin + j]) == 0) {
			q--;
			vect_swap(v, q, j);
		}
	}
	vect_swap(v, i, r);
	j = i - 1;
	i = i + 1;
	for (k = l; k < p; k++, j--)
		vect_swap(v, k, j);
	for (k = r - 1; k > q; k--, i++)
		vect_swap(v, k, i);
	_vect_qsort(v, l, j, compare_func);
	_vect_qsort(v, i, r, compare_func);
}
#endif // ! TRADITIONAL_QSORT

void vect_qsort(vector v, int (*compare_func)(const void *, const void *)) {
	// check if the vector v1 exists:
	vect_check(v);

	// Check parameters:
	if (_vect_size(v) <= 1)
		return;

	if (compare_func == NULL)
		return;

	// Process the vector:
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	_vect_qsort(v, 0, _vect_size(v) - 1, compare_func);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

#ifdef TRADITIONAL_BINARY_SEARCH
static bool _standard_binary_search(vector v, const void *key,
                                    zvect_index *item_index,
                                    int (*f1)(const void *, const void *)) {
	zvect_index bot, mid, top;

	bot = 0;
	top = _vect_size(v) - 1;

	while (bot < top) {
		mid = top - (top - bot) / 2;

		// key < array[mid]
		if ((*f1)(key, v->data[v->begin + mid]) < 0) {
			top = mid - 1;
		} else {
			bot = mid;
		}
	}

	// key == array[top]
	if ((*f1)(key, v->data[v->begin + top]) == 0) {
		*item_index = top;
		return true;
	}

	*item_index = top;
	return false;
}
#endif // TRADITIONAL_BINARY_SEARCH

#ifndef TRADITIONAL_BINARY_SEARCH
// This is my re-implementation of Igor van den Hoven's Adaptive
// Binary Search algorithm. It has few improvements over the
// original design, most notably the use of custom compare
// fuction that makes it suitable also to search through strings
// and other types of vectors.
static bool _adaptive_binary_search(vector v, const void *key,
                                    zvect_index *item_index,
                                    int (*f1)(const void *, const void *)) {
	zvect_index bot, top, mid;

	if ((v->balance >= 32) || (_vect_size(v) <= 64)) {
		bot = 0;
		top = _vect_size(v);
		goto monobound;
	}
	bot = v->bottom;
	top = 32;

	// key >= array[bot]
	if ((*f1)(key, v->data[v->begin + bot]) >= 0) {
		while (1) {
			if ((bot + top) >= _vect_size(v)) {
				top = _vect_size(v) - bot;
				break;
			}
			bot += top;

			// key < array[bot]
			if ((*f1)(key, v->data[v->begin + bot]) < 0) {
				bot -= top;
				break;
			}
			top *= 2;
		}
	} else {
		while (1) {
			if (bot < top) {
				top = bot;
				bot = 0;
				break;
			}
			bot -= top;

			// key >= array[bot]
			if ((*f1)(key, v->data[v->begin + bot]) >= 0)
				break;
			top *= 2;
		}
	}

	monobound:
	while (top > 3) {
		mid = top / 2;
		// key >= array[bot + mid]
		if ((*f1)(key, v->data[v->begin + (bot + mid)]) >= 0)
			bot += mid;
		top -= mid;
	}

	v->balance = v->bottom > bot ? v->bottom - bot : bot - v->bottom;
	v->bottom = bot;

	int test = 0;
	while (top) {
		// key == array[bot + --top]
		test = (*f1)(key, v->data[v->begin + (bot + (--top))]);
		if (test == 0) {
			*item_index = bot + top;
			return true;
		} else if (test > 0) {
			*item_index = bot + (top + 1);
			return false;
		}
	}

	*item_index = bot + top;
	return false;
}
#endif // ! TRADITIONAL_BINARY_SEARCH

bool vect_bsearch(vector v, const void *key,
                  int (*f1)(const void *, const void *),
                  zvect_index *item_index) {
	// check if the vector exists:
	vect_check(v);

	*item_index = 0;

	// Check parameters:
	if (key == NULL)
		return false;

	if (f1 == NULL)
		return false;

	if (_vect_size(v) == 0)
		return false;

	*item_index = 0;
#ifdef TRADITIONAL_BINARY_SEARCH
	if (_standard_binary_search(v, key, item_index, f1)) {
		return true;
	} else {
		*item_index = 0;
		return false;
	}
#endif // TRADITIONAL_BINARY_SEARCH
#ifndef TRADITIONAL_BINARY_SEARCH
	if (_adaptive_binary_search(v, key, item_index, f1)) {
		return true;
	} else {
		*item_index = 0;
		return false;
	}
#endif // ! TRADITIONAL_BINARY_SEARCH
}

/*
 * Althought if the vect_add_* doesn't belong to this group of
 * functions, the vect_add_ordered is an exception because it
 * requires vect_bserach and vect_qsort to be available.
 */
void vect_add_ordered(vector v, const void *value,
                      int (*f1)(const void *, const void *)) {
	// check if the vector exists:
	vect_check(v);

	// Check parameters:
	if (value == NULL)
		return;

	// Few tricks to make it faster:
	if (_vect_size(v) == 0) {
		// If the vector is empty clearly we can just
		// use vect_add and add the value normaly!
		vect_add(v, value);
		return;
	}

	if ((*f1)(value, v->data[v->begin + (_vect_size(v) - 1)]) > 0) {
		// If the compare function returns that
		// the value passed should go after the
		// last value in the vector, just do so!
		vect_add(v, value);
		return;
	}

	// Ok previous checks didn't help us so we need
	// to get "heavy weapons" out and find where in
	// the vector we should add "value":
	zvect_index item_index = 0;

	// Here is another trick:
	// I improved adaptive binary search to ALWAYS
	// return a index (even when it doesn't find a
	// searched item), this works for both: regular
	// searches which will also use the bool to
	// know if we actually found the item in that
	// item_index or not and the vect_add_ordered
	// which will use item_index (which will be the
	// place where value should have been) to insert
	// value as an ordered item :)
	_adaptive_binary_search(v, value, &item_index, f1);

	vect_add_at(v, value, item_index);
}

#endif // ZVECT_DMF_EXTENSIONS

#ifdef ZVECT_SFMD_EXTENSIONS
// Single Function Call Multiple Data operations extensions:

void vect_apply(vector v, void (*f)(void *)) {
	// check if the vector exists:
	vect_check(v);

	// Check parameters:
	if (f == NULL)
		return;

	// Process the vector:
	register zvect_index i;
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	for (i = _vect_size(v); i--;)
		(*f)(v->data[v->begin + i]);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_apply_range(vector v, void (*f)(void *), const zvect_index x,
                      const zvect_index y) {
	// check if the vector exists:
	vect_check(v);

	// Check parameters:
	if (f == NULL)
		return;

	if (x > _vect_size(v))
		throw_error("Index out of bounds!");

	if (y > _vect_size(v))
		throw_error("Index out of bounds!");

	zvect_index start;
	zvect_index end;
	if (x > y) {
		start = x;
		end = y;
	} else {
		start = y;
		end = x;
	}

	// Process the vector:
	register zvect_index i;
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v, 1);
#endif

	for (i = start; i <= end; i++)
		(*f)(v->data[v->begin + i]);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v, 1);
#endif
}

void vect_apply_if(vector v1, vector v2, void (*f1)(void *),
                   bool (*f2)(void *, void *)) {
	// check if the vector exists:
	vect_check(v1);
	vect_check(v2);

	// Check parameters:
	if (_vect_size(v1) > _vect_size(v2))
		throw_error("Vector 2 size too small, can't apply 'if' function for all "
			    "items in vector 1!");

	// Process vectors:
	register zvect_index i;
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v1, 1);
#endif

	for (i = _vect_size(v1); i--;)
		if ((*f2)(v1->data[v1->begin + i], v2->data[v2->begin + i]))
			(*f1)(v1->data[v1->begin + i]);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v1, 1);
#endif
}

void vect_copy(vector v1, vector v2, const zvect_index s2,
               const zvect_index e2) {
	// check if the vector v1 exists:
	vect_check(v1);

	// check if the vector v2 exists:
	vect_check(v2);

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size)
		throw_error("Vectors data size mismatch!");

	// Let's check if the indexes provided are correct for
	// v2:
	if (e2 > _vect_size(v2) || s2 > _vect_size(v2))
		throw_error("Index out of bounds!");

	// If the user specified 0 max_elements then
	// copy the entire vector from start position
	// till the last item in the vector 2:
	zvect_index ee2;
	if (e2 == 0)
		ee2 = (_vect_size(v2) - 1) - s2;
	else
		ee2 = e2;

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v1, 2);
#endif
	// Set the correct capacity for v1 to get the whole v2:
	while ( _vect_capacity(v1) <= (_vect_size(v1) + ee2))
		_vect_increase_capacity(v1, 1);

	// Copy v2 (from s2) in v1 at the end of v1:
	// zvect_index i;
	// for (i = s2; i <= s2+ee2; i++)
	//   vect_add(v1, v2->data[i]);
	vect_memcpy(v1->data + v1->begin + _vect_size(v1), v2->data + v2->begin + s2, sizeof(void *) * ee2);

	// Update v1 size:
	v1->end += ee2;

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v1, 2);
#endif
}

/*
 * vect_insert inserts the specified number of elements
 * from vector v2 (from position s2) in vector v1 (from
 * position s1).
 *
 * example: to insert 10 items from v2 (form item at
 *          position 10) into vector v1 starting at
 *          position 5, use:
 * vect_insert(v1, v2, 10, 10, 5);
 */
void vect_insert(vector v1, vector v2, const zvect_index s2,
                 const zvect_index e2, const zvect_index s1) {
	// check if the vector v1 exists:
	vect_check(v1);

	// check if the vector v2 exists:
	vect_check(v2);

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size)
		throw_error("Vectors data size mismatch!");

	// Let's check if the indexes provided are correct for
	// v2:
	if ((e2 > _vect_size(v2)) || (s2 > _vect_size(v2)))
		throw_error("Index out of bounds!");

	// If the user specified 0 max_elements then
	// copy the entire vector from start position
	// till the last item in the vector 2:
	zvect_index ee2;
	if (e2 == 0)
		ee2 = (_vect_size(v2) - 1) - s2;
	else
		ee2 = e2;

	// Process vectors:
	register zvect_index i, j = 0;
#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v1, 2);
#endif

	// Copy v2 items (from s2) in v1 (from s1):
	for (i = s2; i <= s2 + ee2; i++, j++)
		vect_add_at(v1, v2->data[v2->begin + i], s1 + j);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v1, 2);
#endif
}

/*
 * vect_move movess the specified number of elements
 * from vector v2 (from position s2) in vector v1 (at
 * the end of it).
 *
 * example: to move 10 items from v2 (from item at
 *          position 10) into vector v1, use:
 * vect_move(v1, v2, 10, 10, 5);
 */
void vect_move(vector v1, vector v2, const zvect_index s2,
               const zvect_index e2) {
	// check if the vector v1 exists:
	vect_check(v1);

	// check if the vector v2 exists:
	vect_check(v2);

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size)
		throw_error("Vectors data size mismatch!");

	// Let's check if the indexes provided are correct for
	// v2:
	if ((e2 > v2->data_size) || (s2 > _vect_size(v2)))
		throw_error("Index out of bounds!");

	// If the user specified 0 max_elements then
	// move the entire vector from start position
	// till the last item in the vector 2:
	zvect_index ee2;
	if (e2 == 0)
		ee2 = (_vect_size(v2) - 1) - s2;
	else
		ee2 = e2;

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v1, 2);
#endif

	// Set the correct capacity for v1 to get the whole v2:
	while (_vect_capacity(v1) <= (_vect_size(v1) + ee2))
		_vect_increase_capacity(v1, 1);

	// Copy v2 (from s2) in v1 at the end of v1:
	vect_memcpy(v1->data + v1->begin + _vect_size(v1), v2->data + v2->begin + s2, sizeof(void *) * ee2);

	// Update v1 size:
	v1->end += ee2;

	zvect_index i;
	for (i = s2; i <= s2 + ee2; i++)
		vect_remove_at(v2, i);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v1, 2);
#endif
}

void vect_merge(vector v1, vector v2) {
	// check if the vector v1 exists:
	vect_check(v1);

	// check if the vector v2 exists:
	vect_check(v2);

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size)
		throw_error("Vectors data size mismatch!");

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_lock(v1, 2);
#endif

	// Set the correct capacity for v1 to get the whole v2:
	while (_vect_capacity(v1) <= (_vect_size(v1) + _vect_size(v2)))
		_vect_increase_capacity(v1, 1);

	// Copy the whole v2 in v1 at the end of v1:
	// zvect_index i;
	// for (i = 0; i < _vect_size(v2); i++)
	//    vect_add(v1, v2->data[i]);
	vect_memcpy(v1->data + v1->begin + _vect_size(v1), v2->data + v2->begin, sizeof(void *) * _vect_size(v2));

	// Update v1 size:
	v1->end += _vect_size(v2);

	// Because we are merging two vectors in one
	// after merged v2 to v1 there is no need for
	// v2 to still exists, so let's destroy it to
	// free memory correctly:
	_vect_destroy(v2, 0);

#if (ZVECT_THREAD_SAFE == 1)
	check_mutex_unlock(v1, 2);
#endif
}
#endif

/*---------------------------------------------------------------------------*/
