/*
 *    Name: ZVector_Checks
 * Purpose: Header used by ZVector Library to identify for which
 *          platform ZVector is being compiled.
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright 2021 by Paolo Fabio Zaino, all rights reserved
 *          Distributed under MIT license
 *          
 */

#ifndef SRC_ZVECTOR_CHECKS_H_
#define SRC_ZVECTOR_CHECKS_H_
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

// Try to determine the Operating System being used:
#if defined(__APPLE__) && defined(__MACH__)
#define macOS
#endif

#if ( defined(__GNU__) || defined(__gnu_linux__) || \
      defined(__linux__) || defined(macOS) )
#   define OS_TYPE 1
#elif ( defined(__WIN32__) || defined(WIN32) || defined(_WIN32) )
#   define OS_TYPE 2
#else
#   define OS_TYPE 0
#endif

#if ( OS_TYPE == 1 )
    // We are on a Unix-like OS so we can use pthreads!
#   define MUTEX_TYPE 1
#   elif ( OS_TYPE == 2 ) && ( defined(__CYGWIN__) || \
           defined(__MINGW32__) || defined(__MINGW64__) )
    // We are on MS Windows using CIGWIN so we can use pthreads!
#   define MUTEX_TYPE 1    
#   elif ( OS_TYPE == 2 ) && ( !defined(__CYGWIN__) && \
           !defined(__MINGW32__) && !defined(__MINGW64__) )
    // We are on MS Windows, so we need to use
    // Windows stuff:
#   define MUTEX_TYPE 2
#else
    // I have no idea on which platform are we,
    // hence I have to use fake mutexes and go with the flow!
#   define MUTEX_TYPE 0
#endif

// Try to determine CPU architecture's features:
#if defined(__amd64__) || defined(__x86_64__) || \
    defined(__aarch64__) || defined(__ia64__) || \
    defined(_M_IA64) || defined(_M_AMD64) || \
    defined(_M_X64)
#define Arch64
#else
#define Arch32
#endif

// Try to determine compiler being used:
#if ( defined(__GNUC__) )
#define ZVECT_COMPTYPE 1
#define COMP_MAJRELEASE (__GNUC__)
#define COMP_MINRELEASE (__GNUC_MINOR__)
#define COMP_PATRELEASE (__GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
#define ZVECT_COMPTYPE 2
#else
#define ZVECT_COMPTYPE 0
#endif

#endif  // SRC_ZVECTOR_CHECKS_H_