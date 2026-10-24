/**
 * about.h
 * Author: Sascha Hlusiak
 **/

#ifndef __ABOUT_H_INCLUDED_
#define __ABOUT_H_INCLUDED_

#include "widgets.h"

/**
 * Dialog that provides the About box with information about the game.
 **/
class CAboutBox:public CDialog
{
public:
	/* Just a dialog with widgets, without functionality */
	CAboutBox();
};


#endif
