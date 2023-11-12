/*
 *    Name: Vector_config
 * Purpose: Base configuration for the ZVector library
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 *
 */

#ifndef SRC_ZVECTOR_CONFIG_H_
#define SRC_ZVECTOR_CONFIG_H_
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

// Include some standard C lib header
#include <stdint.h>
#include <stdbool.h>

// Data alignment configuration
#if ( ZVECT_COMPTYPE == 1 )
#define ZVECT_DATAALIGN __attribute__((aligned))
#define ZVECT_PACKING __attribute__((__packed__))
#define ZVECT_ALWAYSINLINE __attribute__ ((__always_inline__))
#else
#define ZVECT_DATAALIGN
#define ZVECT_PACKING
#define ZVECT_ALWAYSINLINE
#endif

// Default vector Index type
// This is set to unsigned int of 32bit
// size (so all different architectures
// and OS behaves in a similar way)
// If you want a larger index you can
// change it to, for example, uint64_t
typedef uint32_t zvect_index;
#define zvect_index_max 4294967295 // If you change zvect_index type make sure you update this value
				   // it's the maximum number that can be stored in a zvect_index.

// Default vector return type for
// error codes.
// Generally negative numbers identify
// an error.
// 0 identify completed successful
// Positive numbers identify return
// attributes, like 1 is generally true
typedef int32_t zvect_retval;

// Default vector storage size
// This will be used when the user
// specifies 0 (zero) as data type
// size.
#define ZVECT_DEFAULT_DATA_SIZE sizeof(int)

// Default vector capacity
// This will be used when the user
// does NOT specify an Initial Capacity
// or set it to 0 (zero):
#define ZVECT_INITIAL_CAPACITY 8

// The following options are handled by Make
// So you should not need to modify them here.

// Choose which type of memory functions you want
// to use for your case:
// 0 = Use Standard memcpy and memmove
// 1 = Use Optimized memcpy and memmove
#define ZVECT_MEMX_METHOD 0

// Enable/Disable thread safe code:
#define ZVECT_THREAD_SAFE 1

// Enable/Disable reentrant code:
#define ZVECT_FULL_REENTRANT 0

// Enable/Disable DMF Extensions:
#define ZVECT_DMF_EXTENSIONS 1

// Enable/Disable SFMD Extensions:
#define ZVECT_SFMD_EXTENSIONS 1

// If you have enabled ZVector extensions
// you can choose to enable/disable
// which type of algorithms you want to use
// for some of the extensions:

// Linear search:
// Uncomment the following line to have
// traditional linear search (it's optimized)
// but it's still linear search.
// If you do not need linear search at all then
// comment all *_LINEAR_SEARCH defines.
#define TRADITIONAL_LINEAR_SEARCH
// Use cooperative ONLY on cooperative multitasking
// systems (like RISC OS or some embedded systems)
// If you use cooperative, please disable traditional
// #define COOPERATIVE_LINEAR_SEARCH

// Binary Search:
// Default is improved Adaptive Binary Search, but
// if you need traditional binary search then
// uncomment the following line:
// #define TRADITIONAL_BINARY_SEARCH
// Use cooperative ONLY on cooperative multitasking
// systems (like RISC OS or some embedded systems)
// If you use cooperative, please disable traditional
// #define COOPERATIVE_BINARY_SEARCH

// Quick Sort:
// Default is improved Quick Sort (3 ways partitioning),
// but if you need traditional quick sort then
// uncomment the following line:
//#define TRADITIONAL_QSORT
// uncomment the following line if you want
// cooperative quick sort (only on cooperative
// multitasking systems)
//#define COOPERATIVE_QSORT


#endif  // SRC_ZVECTOR_CONFIG_H_
