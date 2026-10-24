/**
 * gamedialogs.h
 * Author: Sascha Hlusiak
 * Start game dialog (network lobby) and game over dialog
 **/
#ifndef __GAMEDIALOGS_H_INCLUDED_
#define __GAMEDIALOGS_H_INCLUDED_

#include "widgets.h"



/**
 * The lobby of a multiplayer game. Shows the connected clients and offers a button to start
 * a round once enough players are connected. Offers a chat
 **/
class CStartGameDialog:public CDialog
{
private:
	CGUI* gui;
	CGameClient *client;
	CStaticText *statustext;
	// A timer to refresh the window periodically
	double time;
	CTextEdit *chat;

	// Show the game client's NET_SERVER_STATUS in the status text
	void updateStatus(NET_SERVER_STATUS *status);
	// Send the current text in the chat edit.
	void sendChat();
public:
	// Constructor of the StartGameDialog; servername is for displaying the server
	CStartGameDialog(CGameClient *vclient,CGUI* vgui,const char *servername);
	// Handle a mouse button
	virtual int processMouseEvent(TMouseEvent *event);
	// Animate, refresh periodically
	virtual void execute(double elapsed);
	// Handle a key
	virtual bool processKey(unsigned short key);
};


/**
 * The dialog that reports the outcome of a match and offers an option to restart
 * a round.
 **/
class CGameFinishDialog:public CDialog
{
private:
	// Remember the GUI
	CGUI *gui;
	CTextEdit *chat;
	void sendChat();

public:
	// Constructor. It pulls all data from the CGameClient*
	CGameFinishDialog(CGUI *vgui,CGameClient *vclient);
	// Handle a mouse button
	virtual int processMouseEvent(TMouseEvent *event);
	// Handle a key
	virtual bool processKey(unsigned short key);
	virtual void close();
	// We do not want the mouse exclusively, so the view can still be rotated
	virtual bool handleUI() { return true; }
};

#endif

