/**
 * freebloks.cpp
 * Autor: Sascha Hlusiak
 *
 * Hauptprogramm! Hier befindet sich die main() Routine
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
 * Unter Windows der WinMain()-Einsprungspunkt.
 **/
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance, LPSTR lpcmdline, int cmdShow)
{
	::hInstance=hInstance;
	/* Zufallsgenerator starten */
	srand((unsigned int)time(0));
	CGUI *gui=new CGUI();
	gui->run();
	delete gui;
	return 0;
}

#else 

/**
 * Unter Linux oder Sonstwas eine handelsuebliche main()
 **/
int main(int argc, char *argv[])
{
#ifndef WIN32
    /* A write to a closed socket must fail with EPIPE instead of killing the process */
    signal(SIGPIPE, SIG_IGN);
#endif
    /* Zufallsgenerator starten */
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
