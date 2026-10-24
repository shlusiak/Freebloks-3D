/**
 * timer.cpp
 * Author: Sascha Hlusiak
 *
 * Provides encapsulated access to precise time measurement
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "timer.h"

#ifdef WIN32
__int64 CTimer::frequency;
#endif

CTimer::CTimer()
{
#ifdef WIN32
	/* On Windows the performance counter is used.
	   Request its frequency here and remember it for later. */
	QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
#endif
	reset();
}

/**
 * Returns a timestamp in seconds, but with very high resolution
 * The timestamp is relative to some arbitrary value and can only be used for time differences
 **/
double CTimer::getTick()const
{
#ifdef WIN32
	__int64 tick;
	QueryPerformanceCounter((LARGE_INTEGER*)&tick);
	/* PerformanceCounter/PerformanceCounterFrequency=timestamp in s */
	return (double)tick/(double)frequency;
#else
	timeval t1;
	/* Get the time of day */
	gettimeofday(&t1,0);
	/* gettimeofday has a resolution in the microsecond range
	   s+us/1000000=timestamp in s */
	return (double)(t1.tv_usec)/1000000.0+(double)(t1.tv_sec);
#endif
}

void CTimer::reset()
{
	value=getTick();
}

/**
 * Elapsed time is (current time)-(previously remembered time)
 **/
double CTimer::elapsed()const
{
	return getTick()-value;
}

/**
 * Function to sleep for ms milliseconds
 **/
void CTimer::sleep(const int ms)
{
#ifdef WIN32
	Sleep(ms);
#else
	/* The Linux function expects microseconds */
	usleep(ms*1000);
#endif
}


