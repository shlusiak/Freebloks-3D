/**
 * options.h
 * Author: Sascha Hlusiak
 * Provides the interface for options, and the options dialog
 **/

#ifndef __OPTIONS_INCLUDED_
#define __OPTIONS_INCLUDED_

#include "widgets.h"

class CGUI;

/* Symbolic identifiers of the options. 0<= OPTION_* <OPTION_MAX */
typedef enum {
	OPTION_SHADOW, OPTION_ANIMATE_STONES, OPTION_AUTO_CHATBOX,OPTION_ANIMATE_ARROWS,
	OPTION_ENVIRONMENT,OPTION_SHOW_FPS,OPTION_SHOW_HINTS,OPTION_LIMITFPS,
	OPTION_FPS,
	OPTION_MAX
} OPTION;


struct TOptions
{
private:
	int opt[OPTION_MAX];
public:
	/* Set the options to their defaults. Everything 1 (on) */
	TOptions() { for (int i=0;i<OPTION_MAX;i++) opt[i]=1; opt[OPTION_FPS]=35; }
	int get(const OPTION what)const { return opt[what]; }
	/* Set the value of an option. */
	void set(const OPTION what,const int value) { opt[what]=value; }
};

/**
 * Options dialog providing a menu to change TOptions.
 **/
class COptionsDialog:public CDialog
{
private:
	/* Reference to a TOptions from somewhere, probably from CGUI */
	TOptions *options;

	/* I need to remember the check boxes to find out their state. */
	CCheckBox *shadow,*animatestones,*animatearrows,*env,*autochat,*showfps,*hints,*limitfps;
	CSpinBox *fps;
public:
	COptionsDialog(CGUI* vgui);

	virtual int processMouseEvent(TMouseEvent *event);
};

#endif
