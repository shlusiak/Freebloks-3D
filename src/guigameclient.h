/**
 * guigameclient.h
 * Author: Sascha Hlusiak
 **/

#ifndef __GUIGAMECLIENT_H_INCLUDED_
#define __GUIGAMECLIENT_H_INCLUDED_

#include "gameclient.h"

/**
 * The class extends the game client with callbacks for the GUI
 **/

class CGUIGameClient:public CGameClient
{
private:
	/* Sometimes the game client has to call GUI functions */
	CGUI *gui;
public:
	CGUIGameClient(CGUI* vgui);

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
