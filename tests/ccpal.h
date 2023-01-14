/*
 *        Name: CCPal = C Code Performance Analysis Library
 *     Purpose: Measure performance (in terms of execution time) of
 *              C routines (useful to measure perfs as Unit Test)
 *
 *    	Author: Paolo Fabio Zaino
 *     License: Copyright by Paolo Fabio Zaino, all rights reserved
 *              Distributed under GPL v2 license
 *
 * Description:
 * CCPal is a SIMPLE  C Code runtime  Performance Analysis Library
 * This  simple  library allow the user to measure  code execution
 * time  and  works on a number of  Operating Systems natively, so
 * you just need to add it to your project and use it.
 *
 * At  the  moment  this  library  seems to be working well on ALL
 * tested  Intel based Linux Distro  (RHEL,  CentOS,  PacketLinux,
 * ScientificLinux,  Debian, Ubuntu, KALI) and all modern releases
 * of Apple MacOS.
 */
#ifndef CCPAL_H_
#define CCPAL_H_
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

 // Try to determine the Operating System being used:
#if defined(__APPLE__) && defined(__MACH__)
#   ifndef macOS
#       define macOS
#   endif
#endif

#if ( defined(__GNU__) || defined(__gnu_linux__) || \
      defined(__linux__) || defined(macOS) )
#   ifndef OS_TYPE
#       define OS_TYPE 1
#   endif
#elif ( defined(__WIN32__) || defined(WIN32) || defined(_WIN32) )
#   ifndef OS_TYPE
#       define OS_TYPE 2
#   endif
#else
#   ifndef OS_TYPE
#       define OS_TYPE 0
#   endif
#endif


#include <stdio.h>
#include <time.h>

#if ( OS_TYPE == 1 ) && !defined(macOS)
#include <sys/time.h>
#endif

#if  defined(macOS)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#if  defined(macOS)

#   define CCPAL_INIT_LIB struct timespec tsi, tsf; \
      double elaps_s; long elaps_ns; \
      clock_serv_t cclock; \
      mach_timespec_t mts;

#else

#   ifdef _PTHREAD_H_

#     define CCPAL_INIT_LIB struct timeval tsi, tsf; \
                            double elaps_s; long elaps_us;
#   endif
#   ifndef _PTHREAD_H_
#     define CCPAL_INIT_LIB struct timespec tsi, tsf; \
                            double elaps_s; long elaps_ns;
#   endif

#endif

#if defined(macOS)

# define CCPAL_START_MEASURING \
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock); \
    clock_get_time(cclock, &mts); \
    mach_port_deallocate(mach_task_self(), cclock); \
    tsi.tv_sec = mts.tv_sec; \
    tsi.tv_nsec = mts.tv_nsec;

# define CCPAL_STOP_MEASURING \
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock); \
    clock_get_time(cclock, &mts); \
    mach_port_deallocate(mach_task_self(), cclock); \
    tsf.tv_sec = mts.tv_sec; \
    tsf.tv_nsec = mts.tv_nsec; \
    elaps_s = tsf.tv_sec - tsi.tv_sec; \
    elaps_ns = tsf.tv_nsec - tsi.tv_nsec;

#else

#  ifndef _PTHREAD_H_
    /* cpu time in nanoseconds */

    #define CCPAL_START_MEASURING clock_gettime(CLOCK_MONOTONIC_RAW, &tsi);

    #define CCPAL_STOP_MEASURING clock_gettime(CLOCK_MONOTONIC_RAW, &tsf); \
        elaps_s = difftime(tsf.tv_sec, tsi.tv_sec); \
        elaps_ns = tsf.tv_nsec - tsi.tv_nsec;

#  else

    /* this one should be appropriate to avoid errors on multithreaded processes */

#   define CCPAL_START_MEASURING gettimeofday(&tsi, NULL);

#   define CCPAL_STOP_MEASURING gettimeofday(&tsf, NULL); \
        elaps_s = tsf.tv_sec - tsi.tv_sec; \
        elaps_us = tsf.tv_usec - tsi.tv_usec;

#  endif

#endif


#if  defined(macOS)
#    define CCPAL_REPORT_ANALYSIS fprintf (stdout, "We have spent %lf seconds executing previous code section.\n", elaps_s + ((double)elaps_ns) / 1.0e9 );
#else
#  ifndef _PTHREAD_H_
#    define CCPAL_REPORT_ANALYSIS fprintf (stdout, "We have spent %lf seconds executing previous code section.\n", elaps_s + ((double)elaps_ns) / 1.0e9 );
#  endif
#  ifdef _PTHREAD_H_
#   define CCPAL_REPORT_ANALYSIS fprintf (stdout, "We have spent %lf seconds executing previous code section.\n", elaps_s + ((double)elaps_us) / 1.0e6 );
#  endif
#endif

#endif  // CCPAL_H_
