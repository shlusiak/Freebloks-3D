/**
 * intro.h
 * Autor: Sascha Hlusiak
 * Klasse zum Behandeln der Intro Animation
 **/
#ifndef __INTRO_H_INCLUDED_
#define __INTRO_H_INCLUDED_


#include "stone.h"

class CGUI;
class CStoneEffect;

/**
 * Klasse kuemmert sich um die gesamte Intro Animation
 **/
class CIntro
{
private:
	CGUI *gui;
	/* Verkettete Liste aller Effekte, nur CPhysicalStone's */
	CStoneEffect *effects;
	/* Die einzelnen Formen der Steine muss ich selbst erstellen und merken. */
	CStone stones[14];
	/* Animatioin */
	double anim;
	/* Feld-Animation: Wippen */
	double field_anim;
	/* Wippt das Feld gerade hoch, oder runter? */
	bool field_up;
	/* Phase des Intros. */
	int phase;

	/* Alles initialisieren. */
	void init();
	/* Einen fliegenden Stein erstellen, der auf dx/dy landet */
	void add(int stone,int color,int dx,int dy);
	/* Alle Steine eines Buchstabens erstellen, dass er an x/y landet. */
	void addChar(char c,int color,int x,int y);

	/* Das Feld abraeumen, Wippen starten. */
	void wipe();
public:
	/* Konstruktor, alles initialisieren. */
	CIntro(CGUI *vgui);
	/* Aufraeumen */
	~CIntro();

	/* Alles animieren. */
	bool execute(double elapsed);
	/* Intro rendern. */
	void render();
	/* Intro rendert Schatten der Steine. */
	void renderShadow();
};

#endif
