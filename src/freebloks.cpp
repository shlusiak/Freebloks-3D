/**
 * freebloks.cpp
 * Author: Sascha Hlusiak
 *
 * Main program. This is where main() lives
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include "gui.h"


#ifdef WIN32

extern HINSTANCE hInstance;

/**
 * The WinMain() entry point on Windows.
 **/
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance, LPSTR lpcmdline, int cmdShow)
{
	::hInstance=hInstance;
	/* Seed the random number generator */
	srand((unsigned int)time(0));
	CGUI *gui=new CGUI();
	gui->run();
	delete gui;
	return 0;
}

#else 

/**
 * A plain main() on Linux and everything else
 **/
int main(int argc, char *argv[])
{
#ifndef WIN32
    /* A write to a closed socket must fail with EPIPE instead of killing the process */
    signal(SIGPIPE, SIG_IGN);
#endif
    /* Seed the random number generator */
    srand(time(0));
    CGUI *gui=new CGUI();
    if (gui->parseCmdLine(argc,argv))
    {
    	gui->run();
    }
    delete gui;
    return EXIT_SUCCESS;
}

#endif 
