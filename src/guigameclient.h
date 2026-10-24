/**
 * guispielclient.h
 * Autor: Sascha Hlusiak
 **/

#ifndef __GUIGAMECLIENT_H_INCLUDED_
#define __GUIGAMECLIENT_H_INCLUDED_

#include "gameclient.h"

/**
 * Die Klasse erweitert den Spielclient um Callbacks fuer die GUI
 **/

class CGUISpielClient:public CSpielClient
{
private:
	/* Manchmal muss der SpielClient Funktionen der GUI aufrufen */
	CGUI *gui;
public:
	CGUISpielClient(CGUI* vgui);

private:
	virtual void newCurrentPlayer(const int player);
	virtual void stoneWasSet(NET_SET_STONE *s);
	virtual void hintReceived(NET_SET_STONE *s);
	virtual void gameFinished();
	virtual void chatReceived(NET_CHAT* c);
	virtual void gameStarted();
	virtual void stoneUndone(const CStone &s, const CTurn *t);
};

#endif
