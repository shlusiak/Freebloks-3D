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
	CButton *t1,*t2,*t3;
	CStaticText *text;
	void setTopic(int topic);
public:
	CHelpBox();
	virtual int processMouseEvent(TMouseEvent *e);
};



#endif
