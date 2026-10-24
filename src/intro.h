/**
 * intro.h
 * Author: Sascha Hlusiak
 * Class handling the intro animation
 **/
#ifndef __INTRO_H_INCLUDED_
#define __INTRO_H_INCLUDED_


#include "stone.h"

class CGUI;
class CStoneEffect;

/**
 * The class takes care of the whole intro animation
 **/
class CIntro
{
private:
	CGUI *gui;
	/* Linked list of all effects, CPhysicalStones only */
	CStoneEffect *effects;
	/* I have to create and keep the individual stone shapes myself. */
	CStone stones[14];
	/* Animation */
	double anim;
	/* Board animation: tipping */
	double field_anim;
	/* Is the board currently tipping up or down? */
	bool field_up;
	/* Phase of the intro. */
	int phase;

	void init();
	/* Create a flying stone that lands at dx/dy */
	void add(int stone,int color,int dx,int dy);
	/* Create all stones of a letter so that it lands at x/y. */
	void addChar(char c,int color,int x,int y);

	/* Clear the board, start the tipping. */
	void wipe();
public:
	CIntro(CGUI *vgui);
	/* Clean up */
	~CIntro();

	bool execute(double elapsed);
	/* Render the intro. */
	void render();
	void renderShadow();
};

#endif
