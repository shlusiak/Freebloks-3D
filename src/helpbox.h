/** 
 * helpbox.h
 * Autor: Sascha Hlusiak
 **/
#ifndef __HELPBOX_H_INCLUDED_
#define __HELPBOX_H_INCLUDED_

#include "widgets.h"

/**
 * Help-Box, die man im MainMenu aufrufen kann und kurze Hilfe zum Spiel bietet.
 **/
class CHelpBox:public CDialog
{
private:
	/* Die 3 Buttons am oberen Rand merken */
	CButton *t1,*t2,*t3;
	/* Widget fuer Text des Hilfethemas */
	CStaticText *text;
	/* Das angegebene Thema setzen; Buttons de-/aktivieren */
	void setTopic(int topic);
public:
	/* Konstruktor */
	CHelpBox();
	/* Mausereignis verarbeiten; auf Knoepfe reagieren */
	virtual int processMouseEvent(TMouseEvent *e);
};



#endif
