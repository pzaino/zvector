#ifndef SRC_ZVECTOR_CHECKS_H_
#define SRC_ZVECTOR_CHECKS_H_

/*
 *    Name: ZVector_Checks
 * Purpose: Header used by ZVector Library to identify for which
 *          platform ZVector is being compiled.
 *  Author: Paolo Fabio Zaino
 *  Domain: General
 * License: Copyright 2021 by Paolo Fabio Zaino, all right reserved
 *          Distributed under MIT license
 *          
 */

// preliminary checks:
#if defined(__APPLE__) && defined(__MACH__)
#define macOS
#endif

#if ( defined(__GNU__) || \
      defined(__gnu_linux__) || defined(__linux__) || \
      defined(macOS) )
    // We are on a Unix-like OS so we can use pthreads!
#   define OS_TYPE 1
#   define MUTEX_TYPE 1
#   elif ( defined(__WIN32__) && defined(__CYGWIN__) )
    // We are on MS Windows using CIGWIN so we can use pthreads!
#   define OS_TYPE 2
#   define MUTEX_TYPE 1    
#   elif ( defined(__WIN32__) && !defined(__CYGWIN__) )
    // We are on MS Windows, so we need to use
    // Windows stuff:
#   define OS_TYPE 2
#   define MUTEX_TYPE 2
#else
    // I have no idea on which platform are we,
    // hence I have to use fake mutexes and go with the flow!
#   define OS_TYPE 0
#   define MUTEX_TYPE 0
#endif

#if ( defined(__GNUC__) )
#define COMP_MAJRELEASE (__GNUC__)
#define COMP_MINRELEASE (__GNUC_MINOR__)
#define COMP_PATRELEASE (__GNUC_PATCHLEVEL__)
#endif

#endif  // SRC_ZVECTOR_CHECKS_H_