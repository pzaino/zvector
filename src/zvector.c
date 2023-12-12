/*
 *    Name: ZVector
 * Purpose: Library to use Dynamic Arrays (Vectors) in C Language
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved.
 *          Distributed under MIT license
 *
 * Credits: This Library was inspired by the work of quite few,
 *          apologies if I forgot to  mention them all!
 *
 *          Gnome Team (GArray demo)
 *          Dimitros Michail (Dynamic Array in C presentation)
 *
 */

/*
 * Few code standard notes:
 *
 * p_ <- indicates a PRIVATE method or variable. Not reachable outside this
 *       module.
 *
 */

/* Include standard C libs headers */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Include vector.h header */
#include "zvector.h"

#if (OS_TYPE == 1)
#	if (!defined(macOS))
/*  Improve PThreads on Linux.
 *  macOS seems  to be  handling pthreads
 *  in a   slightly  different   way than
 *  Linux, so, avoid using the same trick
 *  on macOS.
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
#	if (CPU_TYPE == x86_64)
//#		include <xmmintrin.h>
#	endif
#endif // OS_TYPE
#if (ZVECT_THREAD_SAFE == 1)
#	if MUTEX_TYPE == 1
#		if ( !defined(macOS) )
#			include <semaphore.h>
#		else
#			include <dispatch/dispatch.h>
#		endif
#		include <pthread.h>
#	elif MUTEX_TYPE == 2
#		include <windows.h>
#		include <psapi.h>
#	endif // MUTEX_TYPE
#endif // ZVECT_THREAD_SAFE

// Local Defines/Macros:

// Declare Vector status flags:
enum {
	ZVS_NONE = 0,			// - Set or Reset vector's status
					//   register.
	ZVS_CUST_WIPE_ON = 1 << 0,	// - Set the bit to indicate a custom
					//    secure wipe
				   	//   function has been set.
	ZVS_USR1_FLAG = 1 << 1,		// - This is a "user" available flag,
					//   a user code  can set it to 1 or
					//   0 for its own need.
					//   Can be useful when signaling
					//   between threads.
	ZVS_USR2_FLAG = 1 << 2

};

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

/*---------------------------------------------------------------------------*/
// Useful macros
// disabled for now: # define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define UNUSED(x) (void)x

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Define the vector data structure:
 *
 * This is ZVector core data structure, it is the structure of a ZVector
 * vector :)
 * Please note: fields order is based on "most used fields" to help a bit
 * with cache
 */
struct ZVECT_PACKING p_vector
{
	zvect_index cap_left;		// - Max capacity allocated on the
					//   left.
	zvect_index cap_right;		// - Max capacity allocated on the
					//   right.
	zvect_index begin;		// - First vector's Element Pointer
	zvect_index end;		// - Current Array size. size - 1 gives
					//   us the pointer to the last element
					//   in the vector.
	uint32_t flags;			// - This flag set is used to store all
					//   Vector's properties.
					//   It contains bits that set Secure
					//   Wipe, Auto Shrink, Pass Items By
					//   Ref etc.
	size_t data_size;		// - User DataType size.
					//   This should be 2 bytes size on a
					//   16-bit system, 4 bytes on a 32
					//   bit, 8 bytes on a 64 bit. But
					//   check your compiler for the
					//   actual size, it's implementation
					//   dependent.
#if (ZVECT_THREAD_SAFE == 1)
#	if MUTEX_TYPE == 0
	void *lock ZVECT_DATAALIGN;	// - Vector's mutex for thread safe
					//   micro-transactions or user locks.
					//   This should be 2 bytes size on a
					//   16 bit machine, 4 bytes on a 32
					//   bit 4 bytes on a 64 bit.
	void *cond;			// - Vector's mutex condition variable
#	elif MUTEX_TYPE == 1
	pthread_mutex_t lock ZVECT_DATAALIGN;
					// - Vector's mutex for thread safe
					//   micro-transactions or user locks.
					//   This should be 24 bytes on a 32bit
					//   machine and 40 bytes on a 64bit.
	 pthread_cond_t cond;		// - Vector's mutex condition variable

#	elif MUTEX_TYPE == 2
	CRITICAL_SECTION lock ZVECT_DATAALIGN;
					// - Vector's mutex for thread safe
					//   micro-transactions or user locks.
					//   Check your WINNT.H to calculate
					//   the size of this one.
	CONDITION_VARIABLE cond;	// - Vector's mutex condition variable
#	endif  // MUTEX_TYPE
#endif  // ZVECT_THREAD_SAFE
	void **data ZVECT_DATAALIGN;	// - Vector's storage.
	zvect_index init_capacity;	// - Initial Capacity (this is set at
					//   creation time).
					//   For the size of zvect_index check
					//   zvector_config.h.
	void (*SfWpFunc)(const void *item, size_t size);
					// - Pointer to a CUSTOM Safe Wipe
					//   function (optional) needed only
					//   for Secure Wiping special
					//   structures.
#ifdef ZVECT_DMF_EXTENSIONS
	zvect_index balance;		// - Used by the Adaptive Binary Search
					//   to improve performance.
	zvect_index bottom;		// - Used to optimise Adaptive Binary
					//   Search.
#endif  // ZVECT_DMF_EXTENSIONS
	volatile uint32_t status;	// - Internal vector Status Flags
#if (ZVECT_THREAD_SAFE == 1)
#	if ( !defined(macOS) )
		   sem_t semaphore;     // - Vector semaphore
#	else
    dispatch_semaphore_t semaphore;	// - Vector semaphore
#	endif
	volatile int32_t lock_type;	// - This field contains the lock type
					//   used for this Vector.
#endif  // ZVECT_THREAD_SAFE
} ZVECT_DATAALIGN;


// Internal representation of a vector
typedef struct p_vector * const ivector;
typedef struct p_vector * const cvector;

#if defined(ZVECT_COOPERATIVE)
// Struct used to store temporary range of the vector:
// when ZVector is used on CMT systems (like RISC OS or some embedded systems)
struct p_cmt_state_range {
    int low;
    int high;
};

// Struc used to store the state of the cooperative apply:
struct p_cmt_state {
    zvect_index currentIndex;
    cmt_state_range *stack;
    int stackSize;
    int stackCapacity;
    bool isInitialized;
};
#endif

// Initialisation state:
static uint32_t p_init_state = 0;

/*---------------------------------------------------------------------------*/

/*****************************************************************************
 **                       ZVector Support Functions                         **
 *****************************************************************************/

/*---------------------------------------------------------------------------*/
// Errors and messages handling:

enum ZVECT_LOGPRIORITY
{
	ZVLP_NONE       = 0,      // No priority
	ZVLP_INFO       = 1 << 0, // This is an informational priority
				  // message.
	ZVLP_LOW        = 1 << 1, //     "      low       "
	ZVLP_MEDIUM     = 1 << 2, //     "      medium    "
	ZVLP_HIGH       = 1 << 3, //     "      high      "
	ZVLP_ERROR      = 1 << 4  // This is an error message.
};

// Set the message priority at which we log it:
#ifdef DEBUG
unsigned int LOG_PRIORITY = (ZVLP_ERROR | ZVLP_HIGH | ZVLP_MEDIUM |
			     ZVLP_LOW | ZVLP_INFO);
#endif
#ifndef DEBUG
unsigned int LOG_PRIORITY = (ZVLP_ERROR | ZVLP_HIGH | ZVLP_MEDIUM);
#endif

// Local declaration of private functions
void p_init_zvect(void);
zvect_retval p_vect_clear(ivector v);

// This is a vprintf wrapper nothing special:
static void log_msg(int const priority, const char * const format, ...)
{
	va_list args;
	va_start(args, format);

	if ( priority & LOG_PRIORITY )
        	vprintf(format, args);

#ifdef DEBUG
	fflush(stdout);
#endif
#ifndef DEBUG
	if (priority & (ZVLP_ERROR | ZVLP_HIGH))
		fflush(stdout);
#endif

	va_end(args);
}

static size_t safe_strlen(const char *str, size_t max_len)
{
    const char *end = (const char *)memchr(str, '\0', max_len);
    if (end == NULL)
        return max_len;
    else
        return end - str;
}

static void *safe_strncpy(const char * const str_src,
                          size_t max_len)
{
        void *str_dst = NULL;
        char tmp_dst[max_len];
        memset(tmp_dst, 0, max_len - 1);
        memcpy(tmp_dst, str_src, max_len - 1);
        tmp_dst[max_len - 1] = 0;

        str_dst = malloc(sizeof(char) * (safe_strlen(tmp_dst, max_len) + 1));
        if ( str_dst == NULL )
        {
                log_msg(ZVLP_ERROR, "Error: %*i, %s\n", 8, -1000, "Out of memory!");
        } else {
                memcpy((char *)str_dst, tmp_dst, safe_strlen(tmp_dst, max_len));
                ((char *)str_dst)[safe_strlen(tmp_dst, max_len) - 1] = 0;
        }
        return str_dst;
}

#if (ZVECT_COMPTYPE == 1) || (ZVECT_COMPTYPE == 3)
__attribute__((noreturn))
#endif
static void p_throw_error(const zvect_retval error_code,
			  const char *error_message)
{
	int32_t locally_allocated = 0;
	char *message = NULL;
	size_t msg_len = 0;

	if ( error_message == NULL )
	{
		msg_len = sizeof(char *) * 255;
		locally_allocated=-1;
	} else {
		msg_len = safe_strlen(error_message, 255);
	}

	if ( locally_allocated ) {
		switch (error_code)
		{
			case ZVERR_VECTUNDEF:
				message=(char *)safe_strncpy("Undefined or uninitialized vector.\n\0", msg_len);
				break;
			case ZVERR_IDXOUTOFBOUND:
				message=(char *)safe_strncpy("Index out of bound.\n\0", msg_len);
				break;
			case ZVERR_OUTOFMEM:
				message=(char *)safe_strncpy("Not enough memory to allocate space for the vector.\n\0", msg_len);
				break;
			case ZVERR_VECTCORRUPTED:
				message=(char *)safe_strncpy("Vector corrupted.\n\0", msg_len);
				break;
			case ZVERR_RACECOND:
				message=(char *)safe_strncpy("Race condition detected, cannot continue.\n\0", msg_len);
				break;
			case ZVERR_VECTTOOSMALL:
				message=(char *)safe_strncpy("Destination vector is smaller than source.\n\0", msg_len);
				break;
			case ZVERR_VECTDATASIZE:
				message=(char *)safe_strncpy("This operation requires two (or more vectors) with the same data size.\n\0", msg_len);
				break;
			case ZVERR_VECTEMPTY:
				message=(char *)safe_strncpy("Vector is empty.\n\0", msg_len);
				break;
			case ZVERR_OPNOTALLOWED:
				message=(char *)safe_strncpy("Operation not allowed.\n\0", msg_len);
				break;
			default:
				message=(char *)safe_strncpy("Unknown error.\n\0", msg_len);
				break;
		}
	} else
		message=(char *)safe_strncpy(error_message, msg_len);

	log_msg(ZVLP_ERROR, "Error: %*i, %s\n", 8, error_code, message);
	if (locally_allocated)
	{
		free((void *)message);
		message = NULL;
	}

	exit(error_code);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Memory management:

#if (ZVECT_MEMX_METHOD == 0)
ZVECT_ALWAYSINLINE
static inline
#endif // ZVECT_MEMX_METHOD
void *p_vect_memcpy(const void * dst, const void * src,
                    size_t size)
{
#if (ZVECT_MEMX_METHOD == 0)
	// Using regular memcpy
	// If you are using ZVector on Linux/macOS/BSD/Windows
	// you should stick to this one!
	return memcpy((void *)dst, src, size);
#elif (ZVECT_MEMX_METHOD == 1)
	// Using improved memcpy (where improved means for
	// embedded systems only!):
	if (size > 0) {
		if (((uintptr_t)dst % sizeof(ADDR_TYPE1) == 0) &&
		    ((uintptr_t)src % sizeof(ADDR_TYPE1) == 0) &&
		    (size % sizeof(ADDR_TYPE1) == 0)) {
			ADDR_TYPE1 *pExDst = (ADDR_TYPE1 *)dst;
			ADDR_TYPE1 const *pExSrc = (ADDR_TYPE1 const *)src;
			size_t end = size / sizeof(ADDR_TYPE1);
			for (register size_t i = 0; i < end; i++) {
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

ZVECT_ALWAYSINLINE
static inline void *p_vect_memmove(const void * dst,
                                   const void * src, size_t size)
{
#ifdef DEBUG
	log_msg(ZVLP_INFO, "p_vect_memmove: dst      %*p\n", 14, dst);
	log_msg(ZVLP_INFO, "p_vect_memmove: src      %*p\n", 14, src);
	log_msg(ZVLP_INFO, "p_vect_memmove: size     %*u\n", 14, size);
#endif
	return memmove((void *)dst, src, size);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Thread Safe functions:

#if (ZVECT_THREAD_SAFE == 1)
static volatile bool locking_disabled = false;
#	if MUTEX_TYPE == 0
#		define ZVECT_THREAD_SAFE 0
#	elif MUTEX_TYPE == 1

ZVECT_ALWAYSINLINE
static inline int mutex_lock(pthread_mutex_t *lock)
{
	return pthread_mutex_lock(lock);
}

ZVECT_ALWAYSINLINE
static inline int mutex_trylock(pthread_mutex_t *lock)
{
	return pthread_mutex_trylock(lock);
}

ZVECT_ALWAYSINLINE
static inline int mutex_unlock(pthread_mutex_t *lock)
{
	return pthread_mutex_unlock(lock);
}

static inline void mutex_init(pthread_mutex_t *lock)
{
#	if (!defined(macOS))
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutexattr_setpshared(&Attr, PTHREAD_PROCESS_PRIVATE);
	pthread_mutexattr_setprotocol(&Attr, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(lock, &Attr);
#	else
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutexattr_setpshared(&Attr, PTHREAD_PROCESS_PRIVATE);
	pthread_mutexattr_setprotocol(&Attr, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(lock, &Attr);
#	endif // macOS
}

static inline void mutex_cond_init(pthread_cond_t *cond)
{
#	if (!defined(macOS))
	pthread_condattr_t Attr;
	pthread_condattr_init(&Attr);
	pthread_condattr_setpshared(&Attr, PTHREAD_PROCESS_PRIVATE);
	pthread_cond_init(cond, &Attr);
#	else
	pthread_cond_init(cond, NULL);
#	endif // macOS
}

static inline void mutex_destroy(pthread_mutex_t *lock)
{
	pthread_mutex_destroy(lock);
}

static inline int semaphore_init
#		if !defined(macOS)
				(sem_t *sem, int value)
#		else
				(dispatch_semaphore_t *sem, int value)
#		endif
{
#		if !defined(macOS)
	return sem_init(sem, 0, value);
#		else
	*sem = dispatch_semaphore_create(value);
	return 0;
#		endif
}

static inline int semaphore_destroy
#		if !defined(macOS)
			(sem_t *sem)
#		else
			(dispatch_semaphore_t *sem)
#		endif
{
#		if !defined(macOS)
	return sem_destroy(sem);
#		else
	(void)sem;
	return 0; // "dispatch_semaphore_destroy(sem)" is not needed
#		endif
}

#	elif MUTEX_TYPE == 2

static inline void mutex_lock(CRITICAL_SECTION *lock)
{
	EnterCriticalSection(lock);
}

static inline void mutex_unlock(CRITICAL_SECTION *lock)
{
	LeaveCriticalSection(lock);
}

static inline void mutex_alloc(CRITICAL_SECTION *lock)
{
	// InitializeCriticalSection(lock);
	InitializeCriticalSectionAndSpinCount(lock, 32);
}

static inline void mutex_destroy(CRITICAL_SECTION *lock)
{
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
 * level 1 is the lower priority, and it's used just by the
 *         primitives in ZVector.
 * level 2 is the priority used by the ZVector functions that
 *         uses ZVector primitives.
 * level 3 is the priority of the User's locks.
 */
static inline zvect_retval get_mutex_lock(const vector v,
					  const int32_t lock_type)
{
	if (lock_type >= v->lock_type) {
		mutex_lock(&(v->lock));
		v->lock_type = lock_type;
		return 1;
	}
	return 0;
}

static inline zvect_retval get_mutex_lock2(const vector v1,
					   const vector v2,
					   const int32_t lock_type)
{
	if (lock_type >= v1->lock_type) {
		mutex_lock(&(v1->lock));
		v1->lock_type = lock_type;
	}
	if (lock_type >= v2->lock_type) {
		mutex_lock(&(v2->lock));
		v2->lock_type = lock_type;
	}
	return 1;
}

static inline zvect_retval check_mutex_trylock(const vector v,
					       const int32_t lock_type)
{
	if ((lock_type >= v->lock_type) && (!mutex_trylock(&(v->lock)))) {
		v->lock_type = lock_type;
		return 1;
	}
	return 0;
}

static inline zvect_retval lock_after_signal(const vector v,
					     const int32_t lock_type)
{
	if (lock_type >= v->lock_type) {
		while (!pthread_cond_wait(&(v->cond), &(v->lock))) {
			// wait until we get a signal
		}
		v->lock_type = lock_type;
		return 1;
	}
	return 0;
}

/* This function waits for a signal from another thread
 * and then it locks the vector.
 * It's used by the ZVector primitives.

 * The function takes a variable number of arguments.
 * The first argument is the vector to lock.
 * The second argument is the lock type.
 * The third argument is a function that returns a boolean
 * value. This function is used to check if the condition
 * for the signal is true or not.
 * The fourth argument is a function that is called before
 * the signal is sent.
 * The fifth argument is a function that is called after
 * the signal is sent.
*/

// Function prototype for condition check function
 typedef bool (*condition_func)(const vector*);

// Function prototype for other functions to be called before signal
 typedef void (*before_signal_func)(void);

static inline bool wait_for_signal(ivector v, const int32_t lock_type,
                                   condition_func f1, ...)
{
	if (lock_type >= v->lock_type)
	{
	    mutex_lock(&(v->lock));
            while(!f1(&v)) {
                va_list args;
                va_start(args, f1);
                before_signal_func func;

                // Calling functions f2, f3, ... before the signal is sent
                while ((func = va_arg(args, before_signal_func)) != NULL) {
                    func();
                }

                va_end(args);

                // Wait for the condition to be true
#if (MUTEX_TYPE == 1)
                pthread_cond_wait(&(v->cond), &(v->lock));
#elif (MUTEX_TYPE == 2)
		LeaveCriticalSection(&(v->lock));
		WaitForSingleObject(&(v->cond), INFINITE);
		EnterCriticalSection(&(v->lock));
#endif
            }
#if (MUTEX_TYPE == 1)
            mutex_unlock(&(v->lock));
#elif (MUTEX_TYPE == 2)
	    LeaveCriticalSection(&(v->lock));
#endif
            return true;
        }
    return false;
}

static inline zvect_retval send_signal(cvector v, const int32_t lock_type)
{
	return (lock_type >= v->lock_type) ? pthread_cond_signal(&(v->cond)) : 0;
}

static inline zvect_retval broadcast_signal(cvector v,
					    const int32_t lock_type)
{
	return (lock_type >= v->lock_type) ? pthread_cond_broadcast(&(v->cond)) : 0;
}

static zvect_retval get_mutex_unlock(ivector v,
					    const int32_t lock_type)
{
	if (lock_type == v->lock_type) {
		v->lock_type = 0;
		mutex_unlock(&(v->lock));
		return 1;
	}
	return 0;
}
#endif // ZVECT_THREAD_SAFE
/*---------------------------------------------------------------------------*/

/*****************************************************************************
 **                          ZVector Primitives                             **
 *****************************************************************************/

/*---------------------------------------------------------------------------*/
// Library Initialisation:

void p_init_zvect(void)
{
#if (OS_TYPE == 1)
#	if ( !defined(macOS) )
		//mallopt(M_MXFAST, 196*sizeof(size_t)/4);
#	endif
#endif  // OS_TYPE == 1

	// We are done initialising ZVector so set the following
	// to one, so this function will no longer be called:
	p_init_state = 1;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector's Utilities:

ZVECT_ALWAYSINLINE
static inline zvect_retval p_vect_check(const_vector const x)
{
	return (x == NULL) ? ZVERR_VECTUNDEF : 0;
}

ZVECT_ALWAYSINLINE
static inline zvect_index p_vect_capacity(const_vector const v)
{
	return ( v->cap_left + v->cap_right );
}

ZVECT_ALWAYSINLINE
static inline zvect_index p_vect_size(const_vector const v)
{
	return ( v->end > v->begin ) ? ( v->end - v->begin ) : ( v->begin - v->end );
}

static inline void p_item_safewipe(const_vector const v, void *const item)
{
	if (item != NULL) {
		if (!(v->status & ZVS_CUST_WIPE_ON)) {
			memset(item, 0, v->data_size);
		} else {
			(*(v->SfWpFunc))(item, v->data_size);
		}
	}
}

ZVECT_ALWAYSINLINE
static inline zvect_retval p_usr_status(zvect_index flag_id)
{
	// For now leave the switch 'cause I may add more cases later
	switch (flag_id) {
		case  1: return 0;
		default: return 1;
	}
}

bool vect_check_status(const_vector const v, zvect_index flag_id)
{
	return (v->status >> flag_id) & 1U;
}

bool vect_set_status(ivector v, zvect_index flag_id)
{
	zvect_retval rval = p_usr_status(flag_id);

	if (!rval)
		v->status |= 1 << flag_id;

	return (bool)rval;
}

bool vect_clear_status(ivector v, zvect_index flag_id)
{
	zvect_retval rval = p_usr_status(flag_id);

	if (!rval)
		v->status &= ~(1 << flag_id);

	return (bool)rval;
}

bool vect_toggle_status(ivector v, zvect_index flag_id)
{
	zvect_retval rval = p_usr_status(flag_id);

	if (!rval)
		v->status ^= (1 << flag_id);

	return rval;
}

static void p_free_items(ivector v, zvect_index first, zvect_index offset)
{
	if (p_vect_size(v) == 0)
		return;

	for (register zvect_index j = (first + offset); j >= first; j--) {
		if (v->data[v->begin + j] != NULL) {
			if (v->flags & ZV_SEC_WIPE)
				p_item_safewipe(v, v->data[v->begin + j]);
			if (!(v->flags & ZV_BYREF)) {
				free(v->data[v->begin + j]);
                		v->data[v->begin + j] = NULL;
            		}
		}
		if (j == first)
			break;	// this is required if we are using
				// uint and the first element is element
				// 0, because on GCC an uint will fail
				// then check in the for loop if j >= first
				// in this particular case!
	}
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Size and Capacity management functions:

/*
 * Set vector capacity to a specific new_capacity value
 */
static zvect_retval p_vect_set_capacity(ivector v,
					const zvect_index direction,
					const zvect_index new_capacity_)
{
	zvect_index new_capacity = new_capacity_;

	if ( new_capacity <= v->init_capacity )
		new_capacity = v->init_capacity;

	void **new_data = NULL;
	if (!direction)
	{

		// Set capacity on the left side of the vector to new_capacity:
		new_data = (void **)malloc(sizeof(void *) * (new_capacity + v->cap_right));
		if (new_data == NULL)
			return ZVERR_OUTOFMEM;

		zvect_index nb;
		zvect_index ne;
		nb = v->cap_left;
		ne = ( nb + (v->end - v->begin) );
		if (v->end != v->begin)
			p_vect_memcpy(new_data + nb, v->data + v->begin, sizeof(void *) * (v->end - v->begin) );

		// Reconfigure vector:
		v->cap_left = new_capacity;
		v->end = ne;
		v->begin = nb;
		free(v->data);

	} else {

		// Set capacity on the right side of the vector to new_capacity:
		new_data = (void **)realloc(v->data, sizeof(void *) * (v->cap_left + new_capacity));
		if (new_data == NULL)
			return ZVERR_OUTOFMEM;

		// Reconfigure Vector:
		v->cap_right = new_capacity;
	}

	// Apply changes
	v->data = new_data;

	// done
	return 0;
}

/*
 * This function increase the CAPACITY of a vector.
 */
static zvect_retval p_vect_increase_capacity(ivector v,
					     const zvect_index direction)
{
	zvect_index new_capacity;

	if (!direction)
	{

		// Increase capacity on the left side of the vector:

		// Get actual left capacity and double it
		// Note: "<< 1" is the same as "* 2"
		//       this is an optimisation for old
		//       compilers.
		new_capacity = v->cap_left << 1;

	} else {

		// Increase capacity on the right side of the vector:

		// Get actual left capacity and double it
		// Note: "<< 1" is the same as "* 2"
		//       this is an optimisation for old
		//       compilers.
		new_capacity = v->cap_right << 1;

	}

	zvect_retval rval = p_vect_set_capacity(v, direction, new_capacity);

	// done
	return rval;
}

/*
 * This function decrease the CAPACITY of a vector.
 */
static zvect_retval p_vect_decrease_capacity(ivector v,
					     const zvect_index direction)
{
	// Check if new capacity is smaller than initial capacity
	if ( p_vect_capacity(v) <= v->init_capacity)
		return 0;

	zvect_index new_capacity;

	void **new_data = NULL;
	if (!direction)
	{
		// Decreasing on the left:

		// Note: ">> 1" is the same as "/ 2"
		//       this is an optimisation for old
		//       compilers.
		new_capacity = v->cap_left >> 1;

		if (new_capacity < ( v->init_capacity >> 1 ))
			new_capacity = v->init_capacity >> 1;

		new_capacity = max( (p_vect_size(v) >> 1), new_capacity);

		new_data = (void **)malloc(sizeof(void *) * (new_capacity + v->cap_right));
		if (new_data == NULL)
			return ZVERR_OUTOFMEM;

		zvect_index nb;
        	zvect_index ne;
		nb = ( new_capacity >> 1);
		ne = ( nb + (v->end - v->begin) );
		p_vect_memcpy(new_data + nb, v->data + v->begin, sizeof(void *) * (v->end - v->begin) );

		// Reconfigure Vector:
		v->cap_left = new_capacity;
		v->end = ne;
		v->begin = nb;
		free(v->data);

	} else {

		// Decreasing on the right:
		// Note: ">> 1" is the same as "/ 2"
		//       this is an optimisation for old
		//       compilers.
		new_capacity = v->cap_right >> 1;

		if (new_capacity < ( v->init_capacity >> 1 ))
			new_capacity = v->init_capacity >> 1;

		new_capacity = max( (p_vect_size(v) >> 1), new_capacity);

		new_data = (void **)realloc(v->data, sizeof(void *) * (v->cap_left + new_capacity));
		if (new_data == NULL)
			return ZVERR_OUTOFMEM;

		// Reconfigure vector:
		v->cap_right = new_capacity;
	}

	// Apply changes
	v->data = new_data;

	// done:
	return 0;
}

/*
 * This function shrinks the CAPACITY of a vector
 * not its size. To reduce the size of a vector we
 * need to remove items from it.
 */
static zvect_retval p_vect_shrink(ivector v)
{
	if (v->init_capacity < 2)
		v->init_capacity = 2;

	if (p_vect_capacity(v) == v->init_capacity || p_vect_capacity(v) <= p_vect_size(v))
		return 0;

	// Determine the correct shrunk size:
	zvect_index new_capacity;
	if (p_vect_size(v) < v->init_capacity)
		new_capacity = v->init_capacity;
	else
		new_capacity = p_vect_size(v) + 2;

	// shrink the vector:
	// Given that zvector supports vectors that can grow on the left and on the right
	// I cannot use realloc here.
	void **new_data = (void **)malloc(sizeof(void *) * new_capacity);
	if (new_data == NULL)
		return ZVERR_OUTOFMEM;

	zvect_index ne;
	zvect_index nb;
	// Note: ">> 1" is the same as "/ 2"
	//       this is an optimisation for old
	//       compilers.
	nb = ( new_capacity >> 1 );
	ne = ( nb + (v->end - v->begin) );
	p_vect_memcpy(new_data + nb, v->data + v->begin, sizeof(void *) * (v->end - v->begin) );

	// Apply changes:
	free(v->data);
	v->data = new_data;
	v->end = ne;
	v->begin = nb;
	v->cap_left = new_capacity >> 1;
	v->cap_right = new_capacity >> 1;

	// done:
	return 0;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Creation and destruction primitives:

zvect_retval p_vect_clear(ivector v)
{
	// Clear the vector:
	if (!vect_is_empty(v))
		p_free_items(v, 0, (p_vect_size(v) - 1));

	// Reset interested descriptors:
	v->begin = v->end = 0;

	// Shrink Vector's capacity:
	// p_vect_shrink(v); // commented this out to make vect_clear behave more like the clear method in C++

	return 0;
}

static zvect_retval p_vect_destroy(vector v, uint32_t flags)
{
	// p_destroy is an exception to the rule of handling
	// locking from the public methods. This because
	// p_destroy has to destroy the vector mutex too and
	// so it needs to control the lock as well!
#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
	if (!lock_owner && (!locking_disabled) && !(v->flags & ZV_NOLOCKING))
		return ZVERR_RACECOND;
#endif

	// Clear the vector (if LSB of flags is set to 1):
	if ((p_vect_size(v) > 0) && (flags & 1)) {
		// Clean the vector:
		p_vect_clear(v);

		// Reset interested descriptors:
		v->end = 0;
	}

	// Destroy the vector:
	v->init_capacity = v->cap_left = v->cap_right = 0;

	// Destroy it:
	if (v->status & ZVS_CUST_WIPE_ON)
		v->SfWpFunc = NULL;

	if (v->data != NULL) {
		free(v->data);
		v->data = NULL;
	}

	// Clear vector status flags:
	v->status = v->flags = v->begin = v->end = v->data_size = v->balance = v->bottom = 0;

#if (ZVECT_THREAD_SAFE == 1)
	if ( lock_owner )
		get_mutex_unlock(v, v->lock_type);
	mutex_destroy(&(v->lock));
	semaphore_destroy(&(v->semaphore));
#endif

	// All done and freed, so we can safely
	// free the vector itself:
	free(v);

	return 0;
}

/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
// Vector data storage primitives:

// inline implementation for all put:
static zvect_retval p_vect_put_at(ivector v, const void *value,
                                const zvect_index i) {
	// Check if the index passed is out of bounds:
	zvect_index idx = i;
	if (!(v->flags & ZV_CIRCULAR))
	{
		if (idx >= p_vect_size(v))
			return ZVERR_IDXOUTOFBOUND;
	} else {
		if (idx >= p_vect_size(v))
			idx = i % v->init_capacity;
	}

	// Add value at the specified index, considering
	// if the vector has ZV_BYREF property enabled:
	if ( v->flags & ZV_BYREF ) {
		void *temp = v->data[v->begin + idx];
		v->data[v->begin + idx] = (void *)value;
		if ( v->flags & ZV_SEC_WIPE )
			memset((void *)temp, 0, v->data_size);
	} else {
		p_vect_memcpy(v->data[v->begin + idx], value, v->data_size);
	}

	// done
	return 0;
}

// inline implementation for all add(s):
static inline zvect_retval p_vect_add_at(ivector v, const void *value,
                                	 const zvect_index i) {
	// Check if the item is NULL:
	if (value == NULL)
		return 0;

	zvect_index idx = i;

	// Get vector size:
	zvect_index vsize = p_vect_size(v);

	// Allocate memory for the new item:
	zvect_index base = v->begin;

#if (ZVECT_FULL_REENTRANT == 1)
	// If we are in FULL_REENTRANT MODE prepare for potential
	// array copy:
	void **new_data = NULL;
	new_data = (void **)malloc(sizeof(void *) * p_vect_capacity(v));
	if (new_data == NULL)
		return ZVERR_OUTOFMEM;
	// Algorithm to try to copy an array of pointers as fast as possible:
	//if (idx > 0)
	//	p_vect_memcpy(new_data + base, v->data + base, sizeof(void *) * idx);
	//p_vect_memcpy(new_data + base + (idx + 1), v->data + base +  idx,
	//	      sizeof(void *) * (vsize - idx));
	if (vsize)
		p_vect_memcpy(new_data + base, v->data + base, sizeof(void *) * vsize);
#endif

	// Allocate space at the beginning or end of the vector as required:
	if (!idx) {
		// Prepare left side of the vector:
		if ( base )
			base--;
		if (!(v->flags & ZV_BYREF)) {
#if (ZVECT_FULL_REENTRANT == 1)
			new_data[base] = (void *)malloc(v->data_size);
			if (new_data[base] == NULL) {
				free(new_data);
				new_data = NULL;
				return ZVERR_OUTOFMEM;
			}
#else
			v->data[base] = malloc(v->data_size);
			if (v->data[base] == NULL)
				return ZVERR_OUTOFMEM;
#endif
		}
	} else if (idx == vsize && !(v->flags & ZV_BYREF)) {
		// Prepare right side of the vector:
#if (ZVECT_FULL_REENTRANT == 1)
		new_data[base + vsize] = (void *)malloc(v->data_size);
		if (new_data[base + vsize] == NULL) {
			free(new_data);
			new_data = NULL;
			return ZVERR_OUTOFMEM;
		}
#else
		v->data[base + vsize] = malloc(v->data_size);
		if (v->data[base + vsize] == NULL)
			return ZVERR_OUTOFMEM;
#endif
	}

	// "Shift" right the array of one position to make space for the new item:
	int16_t array_changed = 0;

	if ((idx < vsize) && (idx != 0)) {
		array_changed = 1;
#if (ZVECT_FULL_REENTRANT == 1)
		p_vect_memmove(new_data + base + (idx + 1), new_data + base + idx,
			       sizeof(void *) * (vsize - idx));
#else
		p_vect_memmove(v->data + base + (idx + 1), v->data + base + idx,
			       sizeof(void *) * (vsize - idx));
#endif  // (ZVECT_FULL_REENTRANT == 1)
	}

	// Add new value in (at the index idx):
#if (ZVECT_FULL_REENTRANT == 1)
	if (array_changed && !(v->flags & ZV_BYREF)) {
		new_data[base + idx] = (void *)malloc(v->data_size);
		if (new_data[base + idx] == NULL) {
			free(new_data);
			new_data = NULL;
			return ZVERR_OUTOFMEM;
		}
	}
	if (v->flags & ZV_BYREF)
		new_data[base + idx] = (void *)value;
	else
		p_vect_memcpy(new_data[base + idx], value, v->data_size);
#else
	if (array_changed && !(v->flags & ZV_BYREF)) {
		// We moved chunks of memory, so we need to
		// allocate new memory for the item at position i:
		v->data[base + idx] = malloc(v->data_size);
		if (v->data[base + idx] == NULL)
			return ZVERR_OUTOFMEM;
	}
	if (v->flags & ZV_BYREF)
		v->data[base + idx] = (void *)value;
	else
		p_vect_memcpy(v->data[base + idx], value, v->data_size);
#endif  // (ZVECT_FULL_REENTRANT == 1)

	// Apply changes:
#if (ZVECT_FULL_REENTRANT == 1)
	//if (array_changed) {
		free(v->data);
		v->data = new_data;
	//}
#endif
	// Increment vector size
	if (!idx) {
		if (v->begin == base)
			v->end++;
		else
			v->begin = base;
	} else {
		v->end++;
	}

	// done
	return 0;
}

// This is the inline implementation for all the remove and pop
static inline zvect_retval p_vect_remove_at(ivector v, const zvect_index i, void **item) {
	zvect_index idx = i;

	// Get the vector size:
	zvect_index vsize = p_vect_size(v);

	// Check if the index is out of bounds:
	if (idx >= vsize) {
		if (!(v->flags & ZV_CIRCULAR)) {
			return ZVERR_IDXOUTOFBOUND;
		} else {
			idx = idx % vsize;
		}
	}

	// Check if the vector got corrupted
	if ((v->end != 0) && (v->begin > v->end))
		return ZVERR_VECTCORRUPTED;

	zvect_index base = v->begin;
	zvect_index top = v->end;

	// Start processing the vector:
#if (ZVECT_FULL_REENTRANT == 1)
	// Allocate memory for support Data Structure:
	void **new_data = (void **)malloc(sizeof(void *) * p_vect_capacity(v));
	if (new_data == NULL)
		return ZVERR_OUTOFMEM;
	if (vsize)
		p_vect_memcpy(new_data + base, v->data + base, sizeof(void *) * vsize);
#endif

	// Get the value we are about to remove:
	// If the vector is set as ZV_BYREF, then just copy the pointer to the item
	// If the vector is set as regular, then copy the item
	if (v->flags & ZV_BYREF) {
#if (ZVECT_FULL_REENTRANT == 1)
		*item = new_data[base + idx];
#else
		*item = v->data[base + idx];
#endif
	} else {
		*item = (void **)malloc(v->data_size);
#if (ZVECT_FULL_REENTRANT == 1)
		if ( new_data[base + idx] != NULL )
#else
		if ( v->data[base + idx] != NULL )
#endif
		{
#if (ZVECT_FULL_REENTRANT == 1)
			p_vect_memcpy(*item, new_data[base + idx], v->data_size);
#else
			p_vect_memcpy(*item, v->data[base + idx], v->data_size);
#endif
			// If the vector is set for secure wipe, and we copied the item
			// then we need to wipe the old copy:
			if (v->flags & ZV_SEC_WIPE)
#if (ZVECT_FULL_REENTRANT == 1)
				p_item_safewipe(v, new_data[base + idx]);
#else
				p_item_safewipe(v, v->data[base + idx]);
#endif
		}
	}

	// "shift" left the array of one position:
	uint16_t array_changed;
	array_changed = 0;
	if ( idx != 0 ) {
		if ((idx < (vsize - 1)) && (vsize > 0)) {
			array_changed = 1;
#if (ZVECT_FULL_REENTRANT == 1)
			if (!(v->flags & ZV_BYREF)) {
				free(new_data[base + idx]);
				new_data[base + idx]=NULL;
			}
			/*
			// old code, was faster than new logic, but too keen to problems
			// I may try to reuse it in a better way in the future.
			p_vect_memcpy(new_data + base, v->data + base, sizeof(void *) * idx);
			p_vect_memcpy(new_data + base + idx, v->data + base + (idx + 1),
				sizeof(void *) * (vsize - idx));
			*/
#else
		if (!(v->flags & ZV_BYREF)) {
			free(v->data[base + idx]);
			v->data[base + idx]=NULL;
		}
#endif

			// move data
#if (ZVECT_FULL_REENTRANT == 1)
			p_vect_memmove(new_data + (base + idx), new_data + (base + (idx + 1)),
					sizeof(void *) * (vsize - idx));
			// Clear leftover item pointers:
			memset(new_data[(v->begin + vsize) - 1], 0, 1);
#else
			p_vect_memmove(v->data + (base + idx), v->data +
				       (base + (idx + 1)),
				       sizeof(void *) * (vsize - idx));

			// Clear leftover item pointers:
			memset(v->data[(v->begin + vsize) - 1], 0, 1);
#endif
		}
	} else {
		if ( base < top ) {
			array_changed = 1;
#if (ZVECT_FULL_REENTRANT == 1)
			if (new_data[base] != NULL) {
				if (!(v->flags & ZV_BYREF)) {
					free(new_data[base]);
				}
				new_data[base]=NULL;
			}
#else
			if (v->data[base] != NULL) {
				if (!(v->flags & ZV_BYREF)) {
					free(v->data[base]);
				}
				v->data[base]=NULL;
			}
#endif
		}
	}

	// Reduce vector size:
#if (ZVECT_FULL_REENTRANT == 0)
	if (!(v->flags & ZV_BYREF) && !array_changed)
		p_free_items(v, vsize - 1, 0);
#else
	if (!(v->flags & ZV_BYREF) && !array_changed)
		for (register zvect_index j = (vsize - 1); j >= vsize; j--) {
			if (new_data[base + j] != NULL) {
				if (v->flags & ZV_SEC_WIPE)
					p_item_safewipe(v, new_data[base + j]);
				if (!(v->flags & ZV_BYREF)) {
					free(new_data[base + j]);
					new_data[base + j]=NULL;
				}
			}
			if (j == (vsize - 1))
				break;	// this is required if we are using
					// uint and the first element is element
					// 0, because on GCC an uint will fail
					// then check in the for loop if j >= first
					// in this particular case!
		}

	// Apply changes
	//if (array_changed) {
	free(v->data);
	v->data = new_data;
	//}
#endif
	if (!(v->flags & ZV_CIRCULAR)) {
		if ( idx != 0 ) {
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
		if ((4 * vsize) < p_vect_capacity(v) )
			p_vect_decrease_capacity(v, idx);
	}

	// All done, return control:
	return 0;
}

// This is the inline implementation for all the "delete" methods
static inline zvect_retval p_vect_delete_at(ivector v,
					    const zvect_index start,
					    const zvect_index offset,
					    uint32_t flags)
{

	zvect_index vsize = p_vect_size(v);

	// If the vector is empty just return
	if (vsize == 0)
		return ZVERR_VECTEMPTY;

	// Check if the index is out of bounds:
	if ((start + offset) > vsize)
		return ZVERR_IDXOUTOFBOUND;

	uint16_t array_changed = 0;

	// "shift" left the data of one position:
	zvect_index tot_items = start + offset;
#ifdef DEBUG
	/*
	log_msg(ZVLP_INFO, "p_vect_delete_at: start     %*u\n", 14, start);
	log_msg(ZVLP_INFO, "p_vect_delete_at: offset    %*u\n", 14, offset);
	log_msg(ZVLP_INFO, "p_vect_delete_at: tot_items %*u\n", 14, tot_items);
	log_msg(ZVLP_INFO, "p_vect_delete_at: v->begin  %*u\n", 14, v->begin);
	log_msg(ZVLP_INFO, "p_vect_delete_at: v->end    %*u\n", 14, v->end);
	log_msg(ZVLP_INFO, "p_vect_delete_at: vsize     %*u\n", 14, vsize);
	log_msg(ZVLP_INFO, "p_vect_delete_at: data      %*p\n", 14, v->data);
	*/
#endif
	if ( (vsize > 1) && (start < (vsize - 1)) && (v->data != NULL) ) {
		array_changed = 1;
#ifdef DEBUG
		/* for (zvect_index ptrID = start; ptrID < start + offset; ptrID++)
		{
				log_msg(ZVLP_INFO, "p_vect_delete_at: data ptr  %*p\n", 14, v->data + (v->begin + ptrID));
		}*/
#endif
		// Safe-erase items?
		if ( flags & 1 )
			p_free_items(v, start, offset);

		// Move remaining items pointers up:
		if ( tot_items )
			p_vect_memmove(v->data + (v->begin + start), v->data + (v->begin + tot_items + 1),
				sizeof(void *) * ((vsize - start) - offset));

		// Clear leftover item pointers:
		if ( offset && !(flags & 1))
			memset(v->data + ((v->begin + vsize) - (offset + 1)), 0, offset);
	}

	// Reduce vector size:
	if (!(v->flags & ZV_BYREF) && (flags & 1) && !array_changed)
			p_free_items(v, ((vsize - 1) - offset), offset);

	// Check if we need to increment begin or decrement end
	// depending on the direction of the "delete" (left or right)
	if ( (start != 0) || array_changed ) {
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

	if (v->begin == v->end)
	{
		v->begin = 0;
		v->end = 0;
	}

	// Check if we need to shrink the vector:
	if ((4 * vsize) < p_vect_capacity(v))
		p_vect_decrease_capacity(v, start);

	// All done, return control:
	return 0;
}

/*---------------------------------------------------------------------------*/




/*****************************************************************************
 **                            ZVector API                                  **
 *****************************************************************************/

/*---------------------------------------------------------------------------*/
// Vector Size and Capacity management functions:

/*
 * Public method to request ZVector to
 * shrink a vector.
 */
void vect_shrink(ivector v)
{
#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = get_mutex_lock(v, 1);
#endif

	zvect_retval rval = p_vect_shrink(v);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif
	if (rval)
		p_throw_error(rval, NULL);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Structural Information report:

bool vect_is_empty(const_vector const v)
{
	return !p_vect_check(v) ? (p_vect_size(v) == 0) : (bool)ZVERR_VECTUNDEF;
}

zvect_index vect_size(const_vector const v)
{
	return !p_vect_check(v) ? p_vect_size(v) : 0;
}

zvect_index vect_max_size(const_vector const v)
{
	return (!p_vect_check(v) ? zvect_index_max : 0);
}

void *vect_begin(const_vector const v)
{
	return !p_vect_check(v) ? v->data[v->begin] : NULL;
}

void *vect_end(const_vector const v)
{
	return !p_vect_check(v) ? v->data[v->end] : NULL;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Creation and Destruction:

vector vect_create(const zvect_index init_capacity, const size_t item_size,
                   const uint32_t properties) {
	// If ZVector has not been initialised yet, then initialise it
	// when creating the first vector:
	if (p_init_state == 0)
		p_init_zvect();

	// Create the vector first:
	vector v = (vector)malloc(sizeof(struct p_vector));
	if (v == NULL)
		p_throw_error(ZVERR_OUTOFMEM, NULL);

	// Initialize the vector:
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
	v->begin = v->end = 0;

	v->init_capacity = v->cap_left + v->cap_right;
	v->flags = properties;
	v->SfWpFunc = NULL;
	v->status = 0;
	if (v->flags & ZV_CIRCULAR)
	{
		// If the vector is circular then
		// we need to pre-allocate all the elements
		// the vector will not grow:
		v->end = v->cap_right - 1;
		v->begin = v->cap_left - 1;
	}
#ifdef ZVECT_DMF_EXTENSIONS
	v->balance = v->bottom = 0;
#endif // ZVECT_DMF_EXTENSIONS

	v->data = NULL;

#if (ZVECT_THREAD_SAFE == 1)
	v->lock_type = 0;
	mutex_init(&(v->lock));
	mutex_cond_init(&(v->cond));
	semaphore_init(&(v->semaphore), 0);
#endif

	// Allocate memory for the vector storage area
	v->data = (void **)calloc(p_vect_capacity(v), sizeof(void *));
	if (v->data == NULL)
		p_throw_error(ZVERR_OUTOFMEM, NULL);

	// Return the vector to the user:
	return v;
}

void *vect_destroy(vector v) {
	// Call p_vect_destroy with flags set to 1
	// to destroy data according to the vector
	// properties:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_DESTROY_JOB_DONE;

	rval = p_vect_destroy(v, 1);

VECT_DESTROY_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

	return NULL;
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Thread Safe user functions:

#if (ZVECT_THREAD_SAFE == 1)
void vect_lock_enable(void) {
	locking_disabled = false;
}

void vect_lock_disable(void) {
	locking_disabled = true;
}

static inline zvect_retval p_vect_lock(ivector v) {
	return (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 3);
}

zvect_retval vect_lock(ivector v) {
	return p_vect_lock(v);
}

static inline zvect_retval p_vect_unlock(ivector v) {
	return (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_unlock(v, 3);
}

zvect_retval vect_unlock(ivector v) {
	return p_vect_unlock(v);
}

static inline zvect_retval p_vect_trylock(ivector v) {
	return (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : check_mutex_trylock(v, 3);
}

zvect_retval vect_trylock(ivector v) {
	return p_vect_trylock(v);
}

zvect_retval vect_sem_wait(ivector v) {
#if !defined(macOS)
	return sem_wait(&(v->semaphore));
#else
	return dispatch_semaphore_wait(v->semaphore, DISPATCH_TIME_FOREVER);
#endif
}

zvect_retval vect_sem_post(ivector v) {
#	if !defined(macOS)
	return sem_post(&(v->semaphore));
#	else
	return (zvect_retval)dispatch_semaphore_signal(v->semaphore);
#	endif
}

/*
TODO: Implement this function:

static inline zvect_retval p_vect_wait_for_signal(const vector v) {
    	return (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 1 : wait_for_signal(v, 3);
}

and also:

inline zvect_retval vect_wait_for_signal(const vector v) {
    	return p_vect_wait_for_signal(v);
}
*/

static inline zvect_retval p_vect_lock_after_signal(ivector v) {
    	return (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 1 : lock_after_signal(v, 3);
}

zvect_retval vect_lock_after_signal(ivector v) {
	return p_vect_lock_after_signal(v);
}

zvect_retval vect_send_signal(ivector v) {
	return send_signal(v, 3);
}

zvect_retval vect_broadcast_signal(ivector v) {
	return broadcast_signal(v, 3);
}

#endif

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Data Storage functions:

void vect_clear(ivector v) {
	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_CLEAR_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	rval = p_vect_clear(v);

//DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_CLEAR_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

void vect_set_wipefunct(ivector v, void (*f1)(const void *, size_t)) {
	// Set custom Safe Wipe function:
	v->SfWpFunc = f1;
	v->status |= ZVS_CUST_WIPE_ON;
}

// Add an item at the END (top) of the vector
void vect_push(ivector v, const void *value) {
	zvect_index vsize = 0;
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_PUSH_JOB_DONE;

	// If the vector is circular then use vect_put_at
	// instead:
	if (v->flags & ZV_CIRCULAR) {
		rval = p_vect_put_at(v, value, p_vect_size(v));
		goto VECT_PUSH_JOB_DONE;
	}

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	// The very first time we do a push, if the vector is
	// declared very small, we may need to expand its
	// capacity on the left. This because the very first
	// push will use index 0 and the rule in ZVector is
	// when we use index 0 we may need to expand on the
	// left. So let's check if we do for this vector:
	vsize = p_vect_size(v);

	// Check if we need to expand on thr right side:
	if ( (v->end >= v->cap_right) && ((rval = p_vect_increase_capacity(v, 1)) != 0) )
		goto VECT_PUSH_DONE_PROCESSING;

	rval = p_vect_add_at(v, value, vsize);

VECT_PUSH_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_PUSH_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

}

// Add an item at the END of the vector
void vect_add(ivector v, const void *value) {
	vect_push(v, value);
}

// Add an item at position "i" of the vector
void vect_add_at(ivector v, const void *value, const zvect_index i) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_ADD_AT_JOB_DONE;

	// If the vector is circular then use vect_put_at
	// instead:
	if (v->flags & ZV_CIRCULAR) {
		rval = p_vect_put_at(v, value, i);
		goto VECT_ADD_AT_JOB_DONE;
	}

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	// Check if the provided index is out of bounds:
	if (i > p_vect_size(v))
	{
		rval = ZVERR_IDXOUTOFBOUND;
		goto VECT_ADD_AT_DONE_PROCESSING;
	}

	if (!i) {
		// Check if we need to expand on the left side:
		if ( (v->begin == 0 || v->cap_left <= 1) && ( (rval = p_vect_increase_capacity(v, 0)) != 0 ) )
				goto VECT_ADD_AT_DONE_PROCESSING;
	} else {
		// Check if we need to expand on thr right side:
		if ( (v->end >= v->cap_right ) && ( (rval = p_vect_increase_capacity(v, 1)) != 0 ) )
				goto VECT_ADD_AT_DONE_PROCESSING;
	}
	rval = p_vect_add_at(v, value, i);

VECT_ADD_AT_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_ADD_AT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

// Add an item at the FRONT of the vector
void vect_add_front(ivector v, const void *value) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_ADD_FRONT_JOB_DONE;

	// If the vector is circular then use vect_put_at
	// instead:
	if (v->flags & ZV_CIRCULAR) {
		rval = p_vect_put_at(v, value, 0);
		goto VECT_ADD_FRONT_JOB_DONE;
	}

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	// Check if we need to expand on the left side:
	if ( (v->begin == 0 || v->cap_left <= 1 ) && ( (rval = p_vect_increase_capacity(v, 0)) != 0 ) )
		goto VECT_ADD_FRONT_DONE_PROCESSING;

	rval = p_vect_add_at(v, value, 0);

VECT_ADD_FRONT_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_ADD_FRONT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

// inline implementation for all get(s):
static inline void *p_vect_get_at(const_vector const v, const zvect_index i) {
	// Check if passed index is out of bounds:
	if (i >= p_vect_size(v))
		p_throw_error(ZVERR_IDXOUTOFBOUND, NULL);

	// Return found element:
	return v->data[v->begin + i];
}

void *vect_get(const_vector const v) {
	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (!rval)
		return p_vect_get_at(v, p_vect_size(v) - 1);
	else
		p_throw_error(rval, NULL);

	return NULL;
}

void *vect_get_at(const_vector const v, const zvect_index i) {
	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (!rval)
		return p_vect_get_at(v, i);
	else
		p_throw_error(rval, NULL);

	return NULL;
}

void *vect_get_front(const_vector const v) {
	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (!rval)
		return p_vect_get_at(v, 0);
	else
		p_throw_error(rval, NULL);

	return NULL;
}

void vect_put(ivector v, const void *value) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_PUT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	rval = p_vect_put_at(v, value, p_vect_size(v) - 1);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_PUT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

void vect_put_at(ivector v, const void *value, const zvect_index i) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_PUT_AT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	rval = p_vect_put_at(v, value, i);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_PUT_AT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

void vect_put_front(ivector v, const void *value) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_PUT_FRONT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

		rval = p_vect_put_at(v, value, 0);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_PUT_FRONT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

void *vect_pop(ivector v) {
	void *item = NULL;
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_POP_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	zvect_index vsize = p_vect_size(v);
	if (vsize != 0)
		rval = p_vect_remove_at(v, vsize - 1, &item);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_POP_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

	return item;
}

void *vect_remove(ivector v) {
	void *item = NULL;
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_REM_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	zvect_index vsize = p_vect_size(v);
	if (vsize != 0)
		rval = p_vect_remove_at(v, vsize - 1, &item);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_REM_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

	return item;
}

void *vect_remove_at(ivector v, const zvect_index i) {
	void *item = NULL;
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_REM_AT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	if (p_vect_size(v) != 0)
		rval = p_vect_remove_at(v, i, &item);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_REM_AT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

	return item;
}

void *vect_remove_front(ivector v) {
	void *item = NULL;
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_REM_FRONT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	if (p_vect_size(v) != 0)
		rval = p_vect_remove_at(v, 0, &item);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_REM_FRONT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

	return item;
}

// Delete an item at the END of the vector
void vect_delete(ivector v) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_DEL_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	rval = p_vect_delete_at(v, p_vect_size(v) - 1, 0, 1);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_DEL_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

// Delete an item at position "i" on the vector
void vect_delete_at(ivector v, const zvect_index i) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_DEL_AT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	rval = p_vect_delete_at(v, i, 0, 1);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_DEL_AT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

// Delete a range of items from "first_element" to "last_element" on the vector v
void vect_delete_range(ivector v, const zvect_index first_element,
                       const zvect_index last_element) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_DEL_RANGE_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	zvect_index end = (last_element - first_element);
	rval = p_vect_delete_at(v, first_element, end, 1);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_DEL_RANGE_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

// Delete an item at the BEGINNING of a vector v
void vect_delete_front(ivector v) {
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_DEL_FRONT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	rval = p_vect_delete_at(v, 0, 0, 1);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_DEL_FRONT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
// Vector Data Manipulation functions
#ifdef ZVECT_DMF_EXTENSIONS

void vect_swap(ivector v, const zvect_index i1, const zvect_index i2) {
	// Check parameters:
	if (i1 == i2)
		return;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_SWP_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	// Check parameters:
	zvect_index vsize = p_vect_size(v);
	if (i1 > vsize || i2 > vsize) {
		rval = ZVERR_IDXOUTOFBOUND;
		goto VECT_SWP_DONE_PROCESSING;
	}

	// Let's swap items:
	void *temp = v->data[v->begin + i2];
	v->data[v->begin + i2] = v->data[v->begin + i1];
	v->data[v->begin + i1] = temp;

VECT_SWP_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_SWP_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

void vect_swap_range(ivector v, const zvect_index s1, const zvect_index e1,
                     const zvect_index s2) {
	// Check parameters:
	if (s1 == s2)
		return;

	zvect_index end = e1;
	if (e1 != 0)
		end = e1 - s1;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_SWP_RANGE_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	zvect_index vsize = p_vect_size(v);
	if ((s1 + end) > vsize || (s2 + end) > vsize || s2 < (s1 + end)) {
		rval = ZVERR_IDXOUTOFBOUND;
		goto VECT_SWP_RANGE_DONE_PROCESSING;
	}

	// Let's swap items:
	register zvect_index i;
	for (register zvect_index j = s1; j <= (s1 + end); j++) {
		i = j - s1;
		void *temp = v->data[v->begin + j];
		v->data[v->begin + j] = v->data[v->begin + (s2 + i)];
		v->data[v->begin + (s2 + i)] = temp;
	}

VECT_SWP_RANGE_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_SWP_RANGE_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

void vect_rotate_left(ivector v, zvect_index i) {

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_ROT_LEFT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	// Check parameters:
	zvect_index vsize = p_vect_size(v);
	if (i == 0 || i == vsize)
		goto VECT_ROT_LEFT_DONE_PROCESSING;

	if (i > vsize)
		i = i % vsize;

	// Process the vector
	if (i == 1) {
		// Rotate left the vector of 1 position:
		void *temp = v->data[0];
		p_vect_memmove(v->data, v->data + 1, sizeof(void *) * (vsize - 1));
		v->data[vsize - 1] = temp;
	} else {
		void **new_data = (void **)malloc(sizeof(void *) * p_vect_capacity(v));
		if (new_data == NULL) {
			rval = ZVERR_OUTOFMEM;
			goto VECT_ROT_LEFT_DONE_PROCESSING;
		}
		// Rotate left the vector of "i" positions:
		p_vect_memcpy(new_data + v->begin, v->data + v->begin, sizeof(void *) * i);
		p_vect_memmove(v->data + v->begin, v->data + v->begin + i, sizeof(void *) * (vsize - i));
		p_vect_memcpy(v->data + v->begin + (vsize - i), new_data + v->begin, sizeof(void *) * i);

		free(new_data);
		new_data=NULL;
	}

VECT_ROT_LEFT_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_ROT_LEFT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

void vect_rotate_right(ivector v, zvect_index i) {
	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_ROT_RIGHT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	// Check parameters:
	zvect_index vsize = p_vect_size(v);
	if (i == 0 || i == vsize)
		goto VECT_ROT_RIGHT_DONE_PROCESSING;

	if (i > vsize)
		i = i % vsize;

	// Process the vector
	if (i == 1) {
		// Rotate right the vector of 1 position:
		void *temp = v->data[v->begin + p_vect_size(v) - 1];
		p_vect_memmove(v->data + v->begin + 1, v->data + v->begin, sizeof(void *) * (vsize - 1));
		v->data[v->begin] = temp;
	} else {
		void **new_data = (void **)malloc(sizeof(void *) * p_vect_capacity(v));
		if (new_data == NULL) {
			rval = ZVERR_OUTOFMEM;
			goto VECT_ROT_RIGHT_DONE_PROCESSING;
		}

		// Rotate right the vector of "i" positions:
		p_vect_memcpy(new_data + v->begin, v->data + v->begin + (p_vect_size(v) - i), sizeof(void *) * i);
		p_vect_memmove(v->data + v->begin + i, v->data + v->begin, sizeof(void *) * (vsize - i));
		p_vect_memcpy(v->data + v->begin, new_data + v->begin, sizeof(void *) * i);

		free(new_data);
		new_data=NULL;
	}

VECT_ROT_RIGHT_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_ROT_RIGHT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

#if !defined(ZVECT_COOPERATIVE)
#ifdef TRADITIONAL_QSORT
static inline zvect_index
p_partition(vector v, zvect_index low, zvect_index high,
           int (*compare_func)(const void *, const void *)) {
	if (high >= p_vect_size(v))
		high = p_vect_size(v) - 1;

	void *pivot = v->data[v->begin + high];
	zvect_index i = (low - 1);

	for (register zvect_index j = low; j <= (high - 1); j++) {
		// v->data[j] <= pivot
		if ((*compare_func)(v->data[v->begin + j], pivot) <= 0)
			vect_swap(v, ++i, j);
	}

	vect_swap(v, i + 1, high);
	return (i + 1);
}

static void p_vect_qsort(vector v, zvect_index low, zvect_index high,
                        int (*compare_func)(const void *, const void *)) {
	if (low < high) {
		zvect_index pi = p_partition(v, low, high, compare_func);
		p_vect_qsort(v, low, pi - 1, compare_func);
		p_vect_qsort(v, pi + 1, high, compare_func);
	}
}
#endif // TRADITIONAL_QSORT

#if !defined(TRADITIONAL_QSORT)
// This is my much faster implementation of a quicksort algorithm
// it fundamentally uses the 3 ways partitioning adapted and improved
// to deal with arrays of pointers together with having a custom
// compare function:
static void p_vect_qsort(ivector v, zvect_index l, zvect_index r,
                        int (*compare_func)(const void *, const void *)) {
	if (r <= l)
	    return;

	zvect_index i;
	zvect_index p;
	zvect_index j;
	zvect_index q;
	void const *ref_val = NULL;

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
    	register zvect_index k;
	for (k = l; k < p; k++, j--)
		vect_swap(v, k, j);
	for (k = r - 1; k > q; k--, i++)
		vect_swap(v, k, i);
	p_vect_qsort(v, l, j, compare_func);
	p_vect_qsort(v, i, r, compare_func);
}
#endif // ! TRADITIONAL_QSORT
#endif // ! ZVECT_COOPERATIVE

#if defined(ZVECT_COOPERATIVE)
// Cooperative quicksort algorithm:
void initQSortState(cmt_state const state, int initialCapacity) {
    state->stack = malloc(initialCapacity * sizeof(struct p_cmt_state_range));
    state->stackSize = 0;
    state->isInitialized = 1;
}

void freeQSortState(cmt_state const state) {
    free(state->stack);
    state->stackSize = 0;
    state->stack = NULL;
}

void pushQSortRange(struct p_cmt_state_range range, cmt_state const state) {
    if (state->stackSize >= state->stackCapacity) {
        // Resize the stack if needed
        state->stackCapacity *= 2;
        state->stack = realloc(state->stack, state->stackCapacity * sizeof(struct p_cmt_state_range));
    }
    state->stack[state->stackSize] = range;
    state->stackSize++;
}

struct p_cmt_state_range popQSortRange(cmt_state const state) {
    if (state->stackSize == 0) {
        // Handle error for empty stack or return a default value
        struct p_cmt_state_range empty = {0, 0};
        return empty;
    }
    state->stackSize--;
    return state->stack[state->stackSize];
}

int p_qsort_is_empty(cmt_state const state) {
    return state->stackSize == 0;
}

zvect_index partition(ivector v,
		      zvect_index low, zvect_index high,
		      int (*compare_func)(const void *, const void *))
{
    const void * const pivot = vect_get_at(v, high); // Pivot element
    zvect_index i = (low - 1); // Index of smaller element

    for (zvect_index j = low; j <= high - 1; j++) {
        // If current element is smaller than or equal to pivot
        if (compare_func(vect_get_at(v, j), pivot) <= 0) {
            i++; // Increment index of smaller element
            vect_swap(v, i, j); // Swap elements at i and j
        }
    }
    vect_swap(v, i + 1, high); // Swap the pivot element with the element at i + 1
    return (i + 1);
}


// Cooperative and iterative quicksort algorithm:
void p_vect_cqsort(ivector v, cmt_state state,
	    	   int(*compare_func)(const void *, const void *),
	    	   zvect_index iterationLimit)
{
    zvect_index iterations = 0;
    if (!state->isInitialized) {
	initQSortState(state, p_vect_size(v) / iterationLimit );
    }

    while (!p_qsort_is_empty(state) && iterations < iterationLimit) {
        cmt_state_range range = popQSortRange(state);

        if (range.low < range.high) {
            zvect_index pivot = partition(v, range.low, range.high, compare_func);

            pushQSortRange((struct p_cmt_state_range){range.low, pivot - 1}, state);
            pushQSortRange((struct p_cmt_state_range){pivot + 1, range.high}, state);

            iterations++;
        }
    }

    // If the function exits here, the state is automatically saved
    // and can be used to resume in the next call.
}
#endif

// Public quicksort function:
// This function is a wrapper for the actual quicksort function
// Depending on which macro is defined, it will call the:
// traditional quicksort
// or
// the 3 ways partitioning quicksort
// or
// the cooperative quicksort
void vect_qsort(ivector v,
		int (*compare_func)(const void *, const void *)
#if defined(ZVECT_COOPERATIVE)
		, struct p_cmt_state* const state
		, zvect_index iterationLimit
#endif
		)
{
	// Check parameters:
	if ( compare_func == NULL )
		return;

	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_QSORT_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	zvect_index vsize = p_vect_size(v);
	if (vsize <= 1)
		goto VECT_QSORT_DONE_PROCESSING;

	// Process the vector:
#ifndef ZVECT_COOPERATIVE
	p_vect_qsort(v, 0, vsize - 1, compare_func);
#else
	if (iterationLimit == 0)
	    iterationLimit = vect_size(v);
	if (iterationLimit > vect_size(v))
	    iterationLimit = vect_size(v);
	if (!state->isInitialized) {
	    initQSortState(state, vsize / iterationLimit );
	}
	struct p_cmt_state_range range = {0, vsize - 1};
	pushQSortRange(range, state);
	p_vect_cqsort(v, state, compare_func, iterationLimit);
	freeQSortState(state);
#endif

VECT_QSORT_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_QSORT_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);
}

#ifdef TRADITIONAL_BINARY_SEARCH
static bool p_standard_binary_search(vector v, const void *key,
                                    zvect_index *item_index,
                                    int (*f1)(const void *, const void *));
#else
static bool p_adaptive_binary_search(ivector v, const void *key,
                                    zvect_index *item_index,
                                    int (*f1)(const void *, const void *));
#endif

/*
 * Although if the vect_add_* doesn't belong to this group of
 * functions, the vect_add_ordered is an exception because it
 * requires vect_bserach and vect_qsort to be available.
 */
void vect_add_ordered(ivector v, const void *value,
                      int (*f1)(const void *, const void *)) {
	// Check parameters:
	if (value == NULL)
		return;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_ADD_ORD_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 2);
#endif

	// Few tricks to make it faster:
	zvect_index vsize = p_vect_size(v);
	if (vsize == 0) {
		// If the vector is empty clearly we can just
		// use vect_add and add the value normally!
		vect_add(v, value);
		goto VECT_ADD_ORD_DONE_PROCESSING;
	}

	if ((*f1)(value, v->data[v->begin + (vsize - 1)]) > 0) {
		// If the compare function returns that
		// the value passed should go after the
		// last value in the vector, just do so!
		vect_add(v, value);
		goto VECT_ADD_ORD_DONE_PROCESSING;
	}

	// Ok previous checks didn't help us, so we need
	// to get "heavy weapons" out and find where in
	// the vector we should add "value":
	zvect_index item_index = 0;

	// Here is another trick:
	// I improved adaptive binary search to ALWAYS
	// return an index (even when it doesn't find a
	// searched item), this works for both: regular
	// searches which will also use the bool to
	// know if we actually found the item in that
	// item_index or not and the vect_add_ordered
	// which will use item_index (which will be the
	// place where value should have been) to insert
	// value as an ordered item :)
#ifdef TRADITIONAL_BINARY_SEARCH
	p_standard_binary_search(v, value, &item_index, f1);
#else
	p_adaptive_binary_search(v, value, &item_index, f1);
#endif

	vect_add_at(v, value, item_index);

VECT_ADD_ORD_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 2);
#endif

VECT_ADD_ORD_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}


// Searching Algorithms:

#ifdef TRADITIONAL_BINARY_SEARCH
static bool p_standard_binary_search(vector v, const void *key,
                                    zvect_index *item_index,
                                    int (*f1)(const void *, const void *)) {
	zvect_index bot, top;

	bot = 0;
	top = p_vect_size(v) - 1;

	while (bot < top) {
        zvect_index mid // mid-point
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
// function that makes it suitable also to search through strings
// and other types of vectors.
static bool p_adaptive_binary_search(ivector v, const void *key,
                                    zvect_index *item_index,
                                    int (*f1)(const void *, const void *)) {
	zvect_index bot;
	zvect_index top;
	zvect_index mid;

	if ((v->balance >= 32) || (p_vect_size(v) <= 64)) {
		bot = 0;
		top = p_vect_size(v);
		goto P_ADP_BSEARCH_MONOBOUND;
	}
	bot = v->bottom;
	top = 32;

	// the following evaluation correspond to: key >= array[bot]
	if ((*f1)(key, v->data[v->begin + bot]) >= 0) {
		while (1) {
			if ((bot + top) >= p_vect_size(v)) {
				top = p_vect_size(v) - bot;
				break;
			}
			bot += top;

			// the meaning of the line below is: key < array[bot]
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

			// the meaning of the line below is: key >= array[bot]
			if ((*f1)(key, v->data[v->begin + bot]) >= 0)
				break;
			top *= 2;
		}
	}

P_ADP_BSEARCH_MONOBOUND:
	while (top > 3) {
		mid = top / 2;
		// the meaning of the following statement is: key >= array[bot + mid]
		if ((*f1)(key, v->data[v->begin + (bot + mid)]) >= 0)
			bot += mid;
		top -= mid;
	}

	v->balance = v->bottom > bot ? v->bottom - bot : bot - v->bottom;
	v->bottom = bot;

	while (top) {
		// the meaning of the following statement is: key == array[bot + --top]
		int test = (*f1)(key, v->data[v->begin + (bot + (--top))]);
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
#endif // ADAPTIVE TRADITIONAL_BINARY_SEARCH

bool vect_bsearch(ivector v, const void *key,
                  int (*f1)(const void *, const void *),
                  zvect_index *item_index) {
	*item_index = 0;

	// Check parameters:
	if ((key == NULL) || (f1 == NULL) || (p_vect_size(v) == 0))
		return false;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_BSEARCH_JOB_DONE;

	// TODO: Add mutex locking

	zvect_index vsize = p_vect_size(v);
	// First case (vector is empty, so we can't search):
	if (vsize == 0)
		goto VECT_BSEARCH_JOB_DONE;

	// Second case (vector has only 1 item, so we can't search):
	if (vsize == 1) {
		if ((*f1)(key, v->data[v->begin]) != 0) {
			*item_index = 0;
			return true;
		} else {
			goto VECT_BSEARCH_JOB_DONE;
		}
	}

#ifdef TRADITIONAL_BINARY_SEARCH
	if (p_standard_binary_search(v, key, item_index, f1)) {
		return true;
	} else {
		*item_index = 0;
		return false;
	}
#endif // TRADITIONAL_BINARY_SEARCH
#ifndef TRADITIONAL_BINARY_SEARCH
	if (p_adaptive_binary_search(v, key, item_index, f1)) {
		return true;
	} else {
		*item_index = 0;
		return false;
	}
#endif // ADAPTIVE TRADITIONAL_BINARY_SEARCH

// VECT_BSEARCH_DONE_PROCESSING:
	// TODO: Add mutex unlock

VECT_BSEARCH_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

	return false;
}

// Traditional Linear Search Algorithm,
// useful with non-sorted vectors:
#if !defined(ZVECT_COOPERATIVE)
bool vect_lsearch(ivector v, const void *key,
                  int (*f1)(const void *, const void *),
                  zvect_index *item_index) {
	*item_index = 0;

	// Check parameters:
	if ((key == NULL) || (f1 == NULL) || (p_vect_size(v) == 0))
		return false;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_LSEARCH_JOB_DONE;

	// TODO: Add mutex locking
	zvect_index vsize = p_vect_size(v);

	// First case (vector is empty, so we can't search):
	if (vsize == 0)
		goto VECT_LSEARCH_JOB_DONE;

	// Second case (vector has only 1 item, so we can't search):
	if (vsize == 1) {
		if ((*f1)(key, v->data[v->begin]) != 0) {
			*item_index = 0;
			return true;
		} else {
			goto VECT_LSEARCH_JOB_DONE;
		}
	}

	// Check if we can do unrolled search:
	if ( (vsize & 3) == 0 ) {
		// Nice, we can do some unrolled search
		// to speed up the process:
		for (register zvect_index x=0; x<vsize; x+=4) {
			if ((*f1)(key, v->data[v->begin + x]) != 0) {
				*item_index = x;
				return true;
			}
			if ((*f1)(key, v->data[v->begin + (x+1)]) != 0) {
				*item_index = x + 1;
				return true;
			}
			if ((*f1)(key, v->data[v->begin + (x+2)]) != 0) {
				*item_index = x + 2;
				return true;
			}
			if ((*f1)(key, v->data[v->begin + (x+3)]) != 0) {
				*item_index = x + 3;
				return true;
			}
		}
	} else {
		// We can't do unrolled search, so let's
		// do a normal linear search:
		for (register zvect_index x=0; x<vsize; x++) {
			if ((*f1)(key, v->data[v->begin + x]) != 0) {
				*item_index = x;
				return true;
			}
		}
	}

VECT_LSEARCH_JOB_DONE:
	if (rval)
		p_throw_error(rval, NULL);

	*item_index = 0;
	return false;
}
#endif // TRADITIONAL_LINEAR_SEARCH

#if defined(ZVECT_COOPERATIVE)
// Cooperative and iterative linear search algorithm:
bool vect_lsearch(ivector v, const void *key,
		  int (*f1)(const void *, const void *),
		  zvect_index *item_index,
		  cmt_state const state,
		  zvect_index maxIterations)
{
	// Check parameters:
	if ((key == NULL) || (f1 == NULL) || (p_vect_size(v) == 0))
		return false;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_LSEARCH_JOB_DONE;

	// TODO: Add mutex locking
	zvect_index vsize = p_vect_size(v);

	// Special case (vector has only 1 item, so we can't search):
	if (vsize == 1) {
		if ((*f1)(key, v->data[v->begin]) != 0) {
			*item_index = 0;
			return true;
		} else {
			goto VECT_LSEARCH_JOB_DONE;
		}
	}

	// Check if the state is initialized:
	if (!state->isInitialized) {
		*item_index = 0;
		state->currentIndex = 0;
		state->isInitialized = true;
	}

	// Initialize internal parameters:
	zvect_index iterations = 0;
	zvect_index x = state->currentIndex;
	if (maxIterations == 0)
		maxIterations = vsize;
	if (maxIterations > vsize)
		maxIterations = vsize;
	if (maxIterations > (vsize - x))
		maxIterations = vsize - x;

	// Search in the vector:
	while (x < vsize && iterations < maxIterations) {
		if (f1(key, v->data[v->begin + x]) != 0) {
			*item_index = x;
			return true;
		}
		x++;
		iterations++;

		// Check if we can do loop unrolling
		// within the cooperative search:
		if ((maxIterations - iterations) > 3)
		{
			// Nice, we can do some unrolled search
			if (f1(key, v->data[v->begin + x]) != 0) {
        			*item_index = x;
        			return true;
    			}
    			if (f1(key, v->data[v->begin + x + 1]) != 0) {
        			*item_index = x + 1;
        			return true;
    			}
    			if (f1(key, v->data[v->begin + x + 2]) != 0) {
        			*item_index = x + 2;
        			return true;
    			}
    			if (f1(key, v->data[v->begin + x + 3]) != 0) {
        			*item_index = x + 3;
        			return true;
    			}
    			x += 3; // Skip ahead for unrolled iterations
    			iterations += 3; // Update iteration count
        	}
	}

	state->currentIndex = x;

	// Check if the search is complete
	if (x >= vsize) {
		*item_index = 0;
		return false;
	}

VECT_LSEARCH_JOB_DONE:
	return false; // Indicate more iterations are needed
}

#endif // COOPERATIVE_LINEAR_SEARCH

#endif // ZVECT_DMF_EXTENSIONS

#ifdef ZVECT_SFMD_EXTENSIONS
// Single Function Call Multiple Data operations extensions:

#if !defined(ZVECT_COOPERATIVE)
void vect_apply(ivector v, void (*f)(void *)) {
	// Check Parameters:
	if (f == NULL)
		return;

	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_APPLY_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	// Process the vector:
	// Check if we can do loop unrolling
	if ((p_vect_size(v) & 3) == 0) {
		// Nice, we can do some unrolled apply:
		for (register zvect_index i = 0; i < p_vect_size(v); i+=4)
		{
			(*f)(v->data[v->begin + i]);
			(*f)(v->data[v->begin + (i+1)]);
			(*f)(v->data[v->begin + (i+2)]);
			(*f)(v->data[v->begin + (i+3)]);
		}
	} else {
		for (register zvect_index i = p_vect_size(v); i--;)
			(*f)(v->data[v->begin + i]);
	}

//VECT_APPLY_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_APPLY_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}
#endif

#if defined(ZVECT_COOPERATIVE)
void vect_apply(ivector v,
		void (*f)(void *),
		cmt_state const state,
		zvect_index maxIterations)
{
	// Check Parameters:
	if (f == NULL)
		return;

	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_APPLY_JOB_DONE;

	zvect_index vsize = p_vect_size(v);
	zvect_index iterations = 0;
	if (!state->isInitialized) {
		state->currentIndex = 0;
		state->isInitialized = true;
	}

	if ((vsize & 3) == 0) {
		// Unrolled loop
		while (state->currentIndex < vsize &&
			iterations < maxIterations) {
        		(*f)(v->data[v->begin + state->currentIndex]);
        		(*f)(v->data[v->begin + state->currentIndex + 1]);
        		(*f)(v->data[v->begin + state->currentIndex + 2]);
        		(*f)(v->data[v->begin + state->currentIndex + 3]);
        		state->currentIndex += 4;
        		iterations += 4;
		}
	} else {
		// Standard loop
		while (state->currentIndex < vsize &&
		       iterations < maxIterations) {
        		(*f)(v->data[v->begin + state->currentIndex]);
        		state->currentIndex++;
        		iterations++;
        	}
	}

VECT_APPLY_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL); // Handle error
}


#endif // COOPERATIVE_APPLY

void vect_apply_range(ivector v, void (*f)(void *), const zvect_index x,
		      const zvect_index y)
{
	// Check parameters:
	if ( f == NULL )
		return;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v);
	if (rval)
		goto VECT_APPLY_RNG_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v, 1);
#endif

	if (x > p_vect_size(v) || y > p_vect_size(v))
	{
		rval = ZVERR_IDXOUTOFBOUND;
		goto VECT_APPLY_RNG_DONE_PROCESSING;
	}

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
	for (register zvect_index i = start; i <= end; i++)
		(*f)(v->data[v->begin + i]);

VECT_APPLY_RNG_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v, 1);
#endif

VECT_APPLY_RNG_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}

#if !defined(ZVECT_COOPERATIVE)
void vect_apply_if(ivector v1, ivector v2, void (*f1)(void *),
                   bool (*f2)(void *, void *)) {
	// Check parameters:
	if (f1 == NULL || f2 == NULL)
		return;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_APPLY_IF_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v1->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v1, 1);
#endif

	// Check parameters:
	if (p_vect_size(v1) > p_vect_size(v2)) {
		rval = ZVERR_VECTTOOSMALL;
		goto VECT_APPLY_IF_DONE_PROCESSING;
	}

	// Process vectors:
	for (register zvect_index i = p_vect_size(v1); i--;)
		if ((*f2)(v1->data[v1->begin + i], v2->data[v2->begin + i]))
			(*f1)(v1->data[v1->begin + i]);

VECT_APPLY_IF_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v1, 1);
#endif

VECT_APPLY_IF_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}
#else
void vect_apply_if(ivector v1, ivector v2, void (*f1)(void *),
		   bool (*f2)(void *, void *),
		   cmt_state const state,
		   zvect_index maxIterations) {
	// Check parameters:
	if (f1 == NULL || f2 == NULL)
		return;

	// check if the vector exists:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_APPLY_IF_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v1->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v1, 1);
#endif

	zvect_index vsize = p_vect_size(v1);
	zvect_index iterations = 0;
	if (!state->isInitialized) {
		state->currentIndex = 0;
		state->isInitialized = true;
	}

	if (vsize > p_vect_size(v2)) {
		rval = ZVERR_VECTTOOSMALL;
		goto VECT_APPLY_IF_DONE_PROCESSING;
	}

	if ((vsize & 3) == 0) {
		// Unrolled loop
		while (state->currentIndex < vsize &&
			iterations < maxIterations) {
			if ((*f2)(v1->data[v1->begin + state->currentIndex],
				  v2->data[v2->begin + state->currentIndex]))
				(*f1)(v1->data[v1->begin + state->currentIndex]);
			if ((*f2)(v1->data[v1->begin + state->currentIndex + 1],
				  v2->data[v2->begin + state->currentIndex + 1]))
				(*f1)(v1->data[v1->begin + state->currentIndex + 1]);
			if ((*f2)(v1->data[v1->begin + state->currentIndex + 2],
				  v2->data[v2->begin + state->currentIndex + 2]))
				(*f1)(v1->data[v1->begin + state->currentIndex + 2]);
			if ((*f2)(v1->data[v1->begin + state->currentIndex + 3],
				  v2->data[v2->begin + state->currentIndex + 3]))
				(*f1)(v1->data[v1->begin + state->currentIndex + 3]);
			state->currentIndex += 4;
			iterations += 4;
		}
	} else {
		// Standard loop
		while (state->currentIndex < vsize &&
		       iterations < maxIterations) {
			if ((*f2)(v1->data[v1->begin + state->currentIndex],
				  v2->data[v2->begin + state->currentIndex]))
				(*f1)(v1->data[v1->begin + state->currentIndex]);
			state->currentIndex++;
			iterations++;
		}
	}

VECT_APPLY_IF_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v1, 1);
#endif

VECT_APPLY_IF_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}

#endif // COOPERATIVE_APPLY_IF

void vect_copy(ivector v1, ivector v2, const zvect_index s2,
               const zvect_index e2) {
	// check if the vectors v1 and v2 exist:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_COPY_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v1->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v1, 2);
#endif

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size) {
		rval = ZVERR_VECTDATASIZE;
		goto VECT_COPY_DONE_PROCESSING;
	}

	// Let's check if the indexes provided are correct for
	// v2:
	if (e2 > p_vect_size(v2) || s2 > p_vect_size(v2)) {
		rval = ZVERR_IDXOUTOFBOUND;
		goto VECT_COPY_DONE_PROCESSING;
	}

	// If the user specified 0 max_elements then
	// copy the entire vector from start position
	// till the last item in the vector 2:
	zvect_index ee2;
	if (e2 == 0)
		ee2 = (p_vect_size(v2) - 1) - s2;
	else
		ee2 = e2;

	// Set the correct capacity for v1 to get the whole v2:
	while ( p_vect_capacity(v1) <= (p_vect_size(v1) + ee2))
		p_vect_increase_capacity(v1, 1);

	// Copy v2 (from s2) in v1 at the end of v1:
	p_vect_memcpy(v1->data + v1->begin + p_vect_size(v1), v2->data + v2->begin + s2, sizeof(void *) * ee2);

	// Update v1 size:
	v1->end += ee2;

VECT_COPY_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v1, 2);
#endif

VECT_COPY_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}

/*
 * vect_insert inserts the specified number of elements
 * from vector v2 (from position s2) in vector v1 (from
 * position s1).
 *
 */
void vect_insert(ivector v1, ivector v2, const zvect_index s2,
                 const zvect_index e2, const zvect_index s1) {
	// check if the vectors v1 and v2 exist:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_INSERT_JOB_DONE;
#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner = (locking_disabled || (v1->flags & ZV_NOLOCKING)) ? 0 : get_mutex_lock(v1, 2);
#endif

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size) {
		rval = ZVERR_VECTDATASIZE;
		goto VECT_INSERT_DONE_PROCESSING;
	}

	// Let's check if the indexes provided are correct for
	// v2:
	if ((e2 > p_vect_size(v2)) || (s2 > p_vect_size(v2))) {
		rval = ZVERR_IDXOUTOFBOUND;
		goto VECT_INSERT_DONE_PROCESSING;
	}

	// If the user specified 0 max_elements then
	// copy the entire vector from start position
	// till the last item in the vector 2:
	zvect_index ee2;
	if (e2 == 0)
		ee2 = (p_vect_size(v2) - 1) - s2;
	else
		ee2 = e2;

	// Process vectors:
	if (ee2 > 1)
	{
		// Number of rows to insert is large, so let's use
		// memmove:

		// Set the correct capacity for v1 to get data from v2:
		if (p_vect_capacity(v1) <= (p_vect_size(v1) + ee2))
			p_vect_set_capacity(v1, 1, p_vect_capacity(v1) + ee2);

		const void *rptr = NULL;

		// Reserve appropriate space in the destination vector
		rptr = p_vect_memmove(v1->data + (v1->begin + s1 + ee2), v1->data + (v1->begin + s1), sizeof(void *) * (p_vect_size(v1) - s1));

		if (rptr == NULL)
		{
			rval = ZVERR_VECTCORRUPTED;
			goto VECT_INSERT_DONE_PROCESSING;
		}

		// Copy items from v2 to v1 at location s1:
		rptr = p_vect_memcpy(v1->data + (v1->begin + s1), v2->data + (v2->begin + s2), sizeof(void *) * ee2);

		if (rptr == NULL)
		{
			rval = ZVERR_VECTCORRUPTED;
			goto VECT_INSERT_DONE_PROCESSING;
		}

		// Update v1 size:
		v1->end += ee2;
	} else {
		// Number of rows to insert is small
		// so let's use vect_Add_at:
		register zvect_index j = 0;

		// Copy v2 items (from s2) in v1 (from s1):
		for (register zvect_index i = s2; i <= s2 + ee2; i++, j++)
			vect_add_at(v1, v2->data[v2->begin + i], s1 + j);

		goto VECT_INSERT_DONE_PROCESSING;
	}

	rval = p_vect_delete_at(v2, s2, ee2 - 1, 0);

VECT_INSERT_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner)
		get_mutex_unlock(v1, 2);
#endif

VECT_INSERT_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}

/*
 * p_vect_move moves the specified number of elements
 * from vector v2 (from position s2) in vector v1 (at
 * the end of it).
 *
 */
static inline zvect_retval p_vect_move(ivector v1, ivector v2, const zvect_index s2,
               			     const zvect_index e2) {
	zvect_retval rval = 0;

#ifdef DEBUG
	log_msg(ZVLP_INFO, "p_vect_move: move pointers set from v2 to v1\n");
#endif
	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size)
		return ZVERR_VECTDATASIZE;

	// Let's check if the indexes provided are correct for
	// v2:
	if ((e2 > p_vect_size(v2)) || (s2 > p_vect_size(v2)))
		return ZVERR_IDXOUTOFBOUND;

	// If the user specified 0 max_elements then
	// move the entire vector from start position
	// till the last item in the vector 2:
	zvect_index ee2;
	if (e2 == 0)
		ee2 = (p_vect_size(v2) - 1) - s2;
	else
		ee2 = e2;

#ifdef DEBUG
	log_msg(ZVLP_INFO, "p_vect_move: v2 capacity = %*u, begin = %*u, end = %*u, size = %*u, s2 = %*u, ee2 = %*u\n", 10, p_vect_capacity(v2), 10, v2->begin, 10, v2->end, 10, p_vect_size(v2), 10, s2, 10, ee2);

	log_msg(ZVLP_INFO, "p_vect_move: v1 capacity = %*u, begin = %*u, end = %*u, size = %*u\n", 10, p_vect_capacity(v1), 10, v1->begin, 10, v1->end, 10, p_vect_size(v1));
#endif

	// Set the correct capacity for v1 to get the whole v2:
	if (p_vect_capacity(v1) <= (p_vect_size(v1) + ee2))
		p_vect_set_capacity(v1, 1, p_vect_capacity(v1)+ee2);

#ifdef DEBUG
	log_msg(ZVLP_INFO, "p_vect_move: v1 capacity = %*u, begin = %*u, end = %*u, size = %*u\n", 10, p_vect_capacity(v1), 10, v1->begin, 10, v1->end, 10, p_vect_size(v1));

	log_msg(ZVLP_INFO, "p_vect_move: ready to copy pointers set\n");
#endif
	// Move v2 (from s2) in v1 at the end of v1:
	const void *rptr = NULL;
	if (v1 != v2)
		rptr = p_vect_memcpy(v1->data + (v1->begin + p_vect_size(v1)), v2->data + (v2->begin + s2), sizeof(void *) * ee2);
	else
		rptr = p_vect_memmove(v1->data + (v1->begin + p_vect_size(v1)), v2->data + (v2->begin + s2), sizeof(void *) * ee2);

	if (rptr == NULL)
	{
		rval = ZVERR_VECTCORRUPTED;
		goto P_VECT_MOVE_DONE_PROCESSING;
	}

	// Update v1 size:
	v1->end += ee2;

#ifdef DEBUG
	log_msg(ZVLP_INFO, "p_vect_move: done copy pointers set\n");
#endif

	// Clean up v2 memory slots that no longer belong to v2:
	//rval = p_vect_delete_at(v2, s2, ee2 - 1, 0);

P_VECT_MOVE_DONE_PROCESSING:
#ifdef DEBUG
	log_msg(ZVLP_INFO, "p_vect_move: v1 capacity = %*u, begin = %*u, end = %*u, size = %*u\n", 10, p_vect_capacity(v1), 10, v1->begin, 10, v1->end, 10, p_vect_size(v1));
	log_msg(ZVLP_INFO, "p_vect_move: v2 capacity = %*u, begin = %*u, end = %*u, size = %*u\n", 10, p_vect_capacity(v2), 10, v2->begin, 10, v2->end, 10, p_vect_size(v2));
#endif

	return rval;
}
// END of p_vect_move
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// vect_move moves a portion of a vector (v2) into another (v1):
void vect_move(ivector v1, vector v2, const zvect_index s2,
               const zvect_index e2) {
	// check if the vectors v1 and v2 exist:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_MOVE_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	// vect_move modifies both vectors, so has to lock them both (if needed)
	zvect_retval lock_owner1 = 0;
	zvect_retval lock_owner2 = 0;
	lock_owner1 = lock_owner2 = (locking_disabled) ? 0 : get_mutex_lock2(v2, v1, 1);
#endif
#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_move: --- begin ---\n");
#	if (ZVECT_THREAD_SAFE == 1)
	log_msg(ZVLP_INFO, "vect_move: lock_owner1 for vector v1: %*u\n",10,lock_owner1);
	log_msg(ZVLP_INFO, "vect_move: lock_owner2 for vector v2: %*u\n",10,lock_owner2);
#	endif // ZVECT_THREAD_SAFE
#endif

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size) {
		rval = ZVERR_VECTDATASIZE;
		goto VECT_MOVE_DONE_PROCESSING;
	}

	rval = p_vect_move(v1, v2, s2, e2);

VECT_MOVE_DONE_PROCESSING:
	if (rval == 0)
		rval = p_vect_delete_at(v2, s2, e2 - 1, 0);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner1)
		get_mutex_unlock(v1, 1);

	if (lock_owner2)
		get_mutex_unlock(v2, 1);
#endif
#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_move: --- end ---\n");
#endif

VECT_MOVE_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);
}
/////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
// vect_move_if moves a portion of a vector (v2) into another (v1) if f2 is true:
zvect_retval vect_move_if(ivector v1, vector v2, const zvect_index s2,
               const zvect_index e2, zvect_retval (*f2)(void *, void *)) {
	// check if the vectors v1 and v2 exist:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_MOVE_IF_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	// vect_move modifies both vectors, so has to lock them both (if needed)
	zvect_retval lock_owner1 = 0;
	zvect_retval lock_owner2 = 0;
	lock_owner1 = lock_owner2 = (locking_disabled) ? 0 : get_mutex_lock2(v2, v1, 1);

#endif
#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_move_if: --- begin ---\n");
#	if (ZVECT_THREAD_SAFE == 1)
	log_msg(ZVLP_INFO, "vect_move_if: lock_owner1 for vector v1: %*u\n",10,lock_owner1);
	log_msg(ZVLP_INFO, "vect_move_if: lock_owner2 for vector v2: %*u\n",10,lock_owner2);
#	endif // ZVECT_THREAD_SAFE
#endif

	// We can only move vectors with the same data_size!
	if (v1->data_size != v2->data_size) {
		rval = ZVERR_VECTDATASIZE;
		goto VECT_MOVE_IF_DONE_PROCESSING;
	}

	rval = (*f2)(v1, v2) ? p_vect_move(v1, v2, s2, e2) : 1;

VECT_MOVE_IF_DONE_PROCESSING:
	if (rval == 0)
		rval = p_vect_delete_at(v2, s2, e2 - 1, 0);

#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner1)
		get_mutex_unlock(v1, 1);

	if (lock_owner2)
		get_mutex_unlock(v2, 1);
#endif
#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_move_if: --- end ---\n");
#endif

VECT_MOVE_IF_JOB_DONE:
	if(rval && (rval != 1))
		p_throw_error(rval, NULL);

	return rval;
}
/////////////////////////////////////////////////////////////////

#if (ZVECT_THREAD_SAFE == 1)
/////////////////////////////////////////////////////////////////
// vect_move_on_signal

zvect_retval vect_move_on_signal(ivector v1, vector v2, const zvect_index s2,
               			const zvect_index e2, zvect_retval (*f2)(void *, void *))
{
	// check if the vectors v1 and v2 exist:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_MOVE_ONS_JOB_DONE;

	// vect_move modifies both vectors, so has to lock them both (if needed)
	zvect_retval lock_owner2 = 0;
	zvect_retval lock_owner1 = 0;
	lock_owner1 = lock_owner2 = (locking_disabled) ? 0 : get_mutex_lock2(v2, v1, 1);

#ifdef DEBUG
	log_msg(ZVLP_MEDIUM, "vect_move_on_signal: --- start waiting ---\n");
#endif
	// wait until we get a signal
	while(!(*f2)(v1, v2) && !(v2->status && (bool)ZVS_USR1_FLAG)) {
		pthread_cond_wait(&(v2->cond), &(v2->lock));
	}
	v2->status &= ~ZVS_USR1_FLAG;

#ifdef DEBUG
	log_msg(ZVLP_MEDIUM, "Set status flag: %*i\n", 10, vect_check_status(v2, 1));

	log_msg(ZVLP_MEDIUM, "vect_move_on_signal: --- received signal ---\n");

	log_msg(ZVLP_MEDIUM, "vect_move_on_signal: --- begin ---\n");
#endif
	// Proceed with move items:
	rval = p_vect_move(v1, v2, s2, e2);

#ifdef DEBUG
	log_msg(ZVLP_MEDIUM, "vect_move_on_signal: --- end ---\n");
#endif

	v2->status &= ~ZVS_USR1_FLAG;
#ifdef DEBUG
	log_msg(ZVLP_MEDIUM, "Reset status flag: %*i\n", 10, vect_check_status(v2, 1));
#endif

//VECT_MOVE_ONS_DONE_PROCESSING:
	if (!rval)
		rval = p_vect_delete_at(v2, s2, e2 - 1, 0);
#ifdef DEBUG
	log_msg(ZVLP_MEDIUM, "v1 owner? %*i\n", 10, lock_owner1);
#endif
	if (lock_owner1 != 0)
	{
		get_mutex_unlock(v1, 1);
	}

#ifdef DEBUG
	log_msg(ZVLP_MEDIUM, "v2 owner? %*i\n", 10, lock_owner2);
#endif

	if (lock_owner2)
	{
		get_mutex_unlock(v2, 1);
	}

VECT_MOVE_ONS_JOB_DONE:
	if(rval && (rval != 1))
		p_throw_error(rval, NULL);

	return rval;
}
#endif // (ZVECT_THREAD_SAFE == 1)
/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////
// vect_merge merges a vector (v2) into another (v1)
void *vect_merge(ivector v1, vector v2) {
	// check if the vector v1 exists:
	zvect_retval rval = p_vect_check(v1) | p_vect_check(v2);
	if (rval)
		goto VECT_MERGE_JOB_DONE;

#if (ZVECT_THREAD_SAFE == 1)
	zvect_retval lock_owner2 = 0;
	zvect_retval lock_owner1 = 0;
	lock_owner1 = lock_owner2 = (locking_disabled) ? 0 : get_mutex_lock2(v2, v1, 1);

#endif

#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_merge: --- begin ---\n");
#	if (ZVECT_THREAD_SAFE == 1)
	log_msg(ZVLP_INFO, "vect_merge: lock_owner1 for vector v1: %*u\n",10,lock_owner1);
	log_msg(ZVLP_INFO, "vect_merge: lock_owner2 for vector v2: %*u\n",10,lock_owner2);
#	endif // ZVECT_THREAD_SAFE
#endif // DEBUG

	// We can only copy vectors with the same data_size!
	if (v1->data_size != v2->data_size) {
		rval = ZVERR_VECTDATASIZE;
		goto VECT_MERGE_DONE_PROCESSING;
	}

	// Check if the user is trying to merge a vector to itself:
	if (v1 == v2) {
		rval = ZVERR_OPNOTALLOWED;
		goto VECT_MERGE_DONE_PROCESSING;
	}

#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_merge: v2 capacity = %*u, begin = %*u, end: %*u, size = %*u\n", 10, p_vect_capacity(v2), 10, v2->begin, 10, v2->end, 10, p_vect_size(v2));
	log_msg(ZVLP_INFO, "vect_merge: v1 capacity = %*u, begin = %*u, end: %*u, size = %*u\n", 10, p_vect_capacity(v1), 10, v1->begin, 10, v1->end, 10, p_vect_size(v1));
#endif

	// Set the correct capacity for v1 to get the whole v2:
	if (p_vect_capacity(v1) <= (p_vect_size(v1) + p_vect_size(v2)))
		p_vect_set_capacity(v1, 1, p_vect_capacity(v1) + p_vect_size(v2));

#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_merge: v1 capacity = %*u, begin = %*u, end: %*u, size = %*u\n", 10, p_vect_capacity(v1), 10, v1->begin, 10, v1->end, 10, p_vect_size(v1));
#endif

	// Copy the whole v2 in v1 at the end of v1:
	p_vect_memcpy(v1->data + (v1->begin + p_vect_size(v1)), v2->data + v2->begin, sizeof(void *) * p_vect_size(v2));

	// Update v1 size:
	v1->end += p_vect_size(v2);

#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_merge: v1 capacity = %*u, begin = %*u, end: %*u, size = %*u\n", 10, p_vect_capacity(v1), 10, v1->begin, 10, v1->end, 10, p_vect_size(v1));
#endif

VECT_MERGE_DONE_PROCESSING:
#if (ZVECT_THREAD_SAFE == 1)
	if (lock_owner1)
		get_mutex_unlock(v1, 1);

	if (lock_owner2)
		get_mutex_unlock(v2, 1);
#endif
	if (!rval)
		rval = p_vect_destroy(v2, 0);
	// ^ Because we are merging two vectors in one
	// after merged v2 to v1  there is no need for
	// v2 to still exists, so let's  destroy it to
	// free memory correctly.
#ifdef DEBUG
	log_msg(ZVLP_INFO, "vect_merge: --- end ---\n");
#endif

VECT_MERGE_JOB_DONE:
	if(rval)
		p_throw_error(rval, NULL);

	return NULL;
}
// END of vect_merge

#endif

/*---------------------------------------------------------------------------*/
