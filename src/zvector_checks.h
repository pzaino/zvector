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
#   define macOS
#endif

#if ( defined(__GNU__) || defined(__gnu_linux__) || \
      defined(__linux__) || defined(macOS) )
#   define OS_TYPE 1
#elif ( defined(__WIN32__) || defined(WIN32) || defined(_WIN32) )
#   define OS_TYPE 2
#else
#   define OS_TYPE 0
#endif

// Try to determine compiler being used:
#if defined(__GNUC__)
#   define compiler gcc
#   define ZVECT_COMPTYPE 1
#   define COMP_MAJRELEASE (__GNUC__)
#   define COMP_MINRELEASE (__GNUC_MINOR__)
#   define COMP_PATRELEASE (__GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
#   define compiler msc
#   define ZVECT_COMPTYPE 2
#   define COMP_MAJRELEASE (_MSC_VER)
#   define COMP_MINRELEASE 0
#   define COMP_PATRELEASE 0
#elif defined(__clang__)
#   define compiler clang
#   define ZVECT_COMPTYPE 3
#   define COMP_MAJRELEASE (__clang_major__)
#   define COMP_MINRELEASE (__clang_minor__)
#   define COMP_PATRELEASE (__clang_patchlevel__)
#elif defined(__INTEL_COMPILER) || defined(__ICC) || \
      defined(__ECC) || defined(__ICL)
// For intel c compiler please remember to specify:
// /Qstd=c99 (on Windows)
// -std=c99 on Linux and/or macOS
#   define compiler intelc
#   define ZVECT_COMPTYPE 4
#   define COMP_MAJRELEASE (__INTEL_COMPILER)
#   define COMP_MINRELEASE 0
#   define COMP_PATRELEASE 0
#elif defined (__LCC__)
#   define compiler lcc
#   define ZVECT_COMPTYPE 5
#   define COMP_MAJRELEASE (__LCC)
#   define COMP_MINRELEASE 0
#   define COMP_PATRELEASE 0
#elif defined(__NORCROFT_C__) || defined(__CC_NORCROFT) || \
      defined(__ARMCC_VERSION)
// For Norcroft C please remember to specify:
// -c99
#   define compiler norcroftc
#   define ZVECT_COMPTYPE 6
#   define COMP_MAJRELEASE (__ARMCC_VERSION)
#elif defined(_CRAYC)
// For Cray C please remember to specify:
// -hc99
#   define compiler crayc
#   define ZVECT_COMPTYPE 10
#   define COMP_MAJRELEASE (_RELEASE)
#   define COMP_MINRELEASE (_RELEASE_MINOR)
#   define COMP_PATRELEASE 0
#elif defined(__HP_cc)
// For HP CC please remember to specify:
// -ansi -std=c99
#   define compiler hpc
#   define ZVECT_COMPTYPE 11
#   define COMP_MAJRELEASE 1
#   define COMP_MINRELEASE 21
#   define COMP_PATRELEASE 0
#elif defined(__IBMC__)
// For IBM C please remember to specify:
// C99 flags
#   define compiler ibmc
#   define ZVECT_COMPTYPE 12
#elif defined(__TINYC__)
#   define compiler tinyc
#   define ZVECT_COMPTYPE 6
#   define COMP_MAJRELEASE 0
#   define COMP_MINRELEASE 0
#   define COMP_PATRELEASE 0
#else
#   define compiler unknown
#   define ZVECT_COMPTYPE 0
#endif

// Try to determine CPU Architecture:
#if defined(__aarch64__)
#   define CPU_TYPE ARM64
#   define Arch64
#elif defined(__aarch32__)
#   define CPU_TYPE ARM32
#   define Arch32
#elif defined(__amd64__) || defined(__x86_64__) || \
      defined(__ia64__) || defined(_M_IA64) || \
      defined(_M_AMD64) || defined(_M_X64)
#   define CPU_TYPE x86_64
#   define Arch64
#else
#   define CPU_TYPE unknown
#   define Arch32
#endif

// Start setting up macros based on the platform we detected
// above.

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

#endif  // SRC_ZVECTOR_CHECKS_H_
