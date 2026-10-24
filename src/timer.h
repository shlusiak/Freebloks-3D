/**
 * timer.h
 * Author: Sascha Hlusiak
 *
 * CTimer wraps operating-system-dependent time functions in one interface
 **/

#ifndef _TIMER_H_INCLUDED_
#define _TIMER_H_INCLUDED_

#include <stdio.h>

class CTimer
{
private:
#ifdef WIN32
	/* On Windows the performance counter is used. Its frequency is stored here */
	static __int64 frequency;
#endif
	/* A relative time value, stored in seconds */
	double value;

	/* Returns the current, non-absolute timestamp, in seconds */
	const double getTick()const;
public:
	CTimer();

	void reset();

	/* Returns the time difference from now to the previous reset(), in seconds */
	const double elapsed()const;

	/* Returns the time difference since the last checkpoint and resets the timer */
	const double checkPoint() { double bla=elapsed(); reset(); return bla; }

	/* Waits the given number of milliseconds */
	static void sleep(const int ms);
};


/**
 * Turns CTimer into a simple stopwatch for debugging
 **/
class CTimeCheck:public CTimer
{
private:
	const char *text;
public:
	CTimeCheck(const char* vtext):text(vtext) { }

	/* The destructor prints the text and the exact lifetime of the object. */
	~CTimeCheck() { printf("%s: %.5f sek.\n",text,elapsed()); }
};

#endif


