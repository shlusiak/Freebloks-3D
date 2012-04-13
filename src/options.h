/**
 * options.h
 * Autor: Sascha Hlusiak
 * Stellt Schnittstelle fuer Optionen bereit, sowie den Optionsdialog
 **/

#ifndef __OPTIONS_INCLUDED_
#define __OPTIONS_INCLUDED_

#include "widgets.h"

class CGUI;

/* Symbolische Bezeichner fuer die Optionen. 0<= OPTION_* <OPTION_MAX */
typedef enum {
	OPTION_SHADOW, OPTION_ANIMATE_STONES, OPTION_AUTO_CHATBOX,OPTION_ANIMATE_ARROWS,
	OPTION_ENVIRONMENT,OPTION_SHOW_FPS,OPTION_SHOW_HINTS,OPTION_LIMITFPS,
	OPTION_FPS,
	OPTION_MAX
} OPTION;


/* Und eine Struktur fuer Zugriff auf die obig bezeichneten Optionen */
struct TOptions
{
private:
	/* Jede Option hat einen int-Wert. */
	int opt[OPTION_MAX];
public:
	/* Optionen auf Voreinstellung setzen. Alles auf 1 (an) */
	TOptions() { for (int i=0;i<OPTION_MAX;i++) opt[i]=1; opt[OPTION_FPS]=35; }
	/* Wert einer Option zurueck geben */
	const int get(const OPTION what)const { return opt[what]; }
	/* Wert einer Option setzen. */
	void set(const OPTION what,const int value) { opt[what]=value; }
};

/**
 * Optiondialog, der ein Menu bereitstellt, um TOptions zu veraendern.
 **/
class COptionsDialog:public CDialog
{
private:
	/* Referenz auf ein TOptions von irgendwo, wahrscheinlich von der CGUI */
	TOptions *options;

	/* Muss mir die CheckBoxes merken, um dessen Status heraus zu kriegen. */
	CCheckBox *shadow,*animatestones,*animatearrows,*env,*autochat,*showfps,*hints,*limitfps;
	CSpinBox *fps;
public:
	/* Konstruktor, braucht nur GUI */
	COptionsDialog(CGUI* vgui);

	/* Ereignis verarbeiten, also Klick auf eine Checkbox. */
	virtual int processMouseEvent(TMouseEvent *event);
};

#endif
