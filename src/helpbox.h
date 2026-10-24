/**
 * helpbox.h
 * Author: Sascha Hlusiak
 **/
#ifndef __HELPBOX_H_INCLUDED_
#define __HELPBOX_H_INCLUDED_

#include "widgets.h"

/**
 * Help box that can be opened from the main menu and gives brief help about the game.
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
