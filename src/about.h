/**
 * about.h
 * Autor: Sascha Hlusiak
 **/

#ifndef __ABOUT_H_INCLUDED_
#define __ABOUT_H_INCLUDED_

#include "widgets.h"

/**
 * Dialog, der die AboutBox bereitstellt, mit Infos ueber das Spiel.
 **/
class CAboutBox:public CDialog
{
public:
	/* Nur ein Dialog, mit Widgets, ohne Funktionialitaet */
	CAboutBox();
};


#endif
