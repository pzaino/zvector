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

#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifdef __APPLE__

#   define CCPAL_INIT_LIB struct timespec tsi, tsf; \
      double elaps_s; long elaps_ns; \
      clock_serv_t cclock; \
      mach_timespec_t mts;

#else

#   define CCPAL_INIT_LIB struct timespec tsi, tsf; \
                         double elaps_s; long elaps_ns;

#endif

#ifdef __APPLE__

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
    elaps_s = difftime(tsf.tv_sec, tsi.tv_sec); \
    elaps_ns = tsf.tv_nsec - tsi.tv_nsec;

#else

# ifdef CLOCK_PROCESS_CPUTIME_ID
    /* cpu time in the current process */

    #define CCPAL_START_MEASURING clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tsi);

    #define CCPAL_STOP_MEASURING clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tsf); \
        elaps_s = difftime(tsf.tv_sec, tsi.tv_sec); \
        elaps_ns = tsf.tv_nsec - tsi.tv_nsec;

# else

    /* this one should be appropriate to avoid errors on multiprocessors systems */

#   define CCPAL_START_MEASURING clock_gettime(CLOCK_MONOTONIC_RAW, &tsi);

#   define CCPAL_STOP_MEASURING clock_gettime(CLOCK_MONOTONIC_RAW, &tsf); \
        elaps_s = difftime(tsf.tv_sec, tsi.tv_sec); \
        elaps_ns = tsf.tv_nsec - tsi.tv_nsec;

# endif

#endif

#define CCPAL_REPORT_ANALYSIS fprintf (stdout, "We have spent %lf seconds executing previous code section.\n", elaps_s + ((double)elaps_ns) / 1.0e9 );

#endif  // CCPAL_H_
