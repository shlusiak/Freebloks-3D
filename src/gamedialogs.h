/**
 * gamedialogs.h
 * Autor: Sascha Hlusiak
 * Spiel-Starten-Dialog (Netzwerk Lobby) und Spiel-Ende-Dialog
 **/
#ifndef __GAMEDIALOGS_H_INCLUDED_
#define __GAMEDIALOGS_H_INCLUDED_

#include "widgets.h"



/**
 * Die Lobby eines Multiplayer-Spiels. Zeigt verbundene Clients an, und bietet Knopf zum Starten
 * einer Runde, wenn genug Spieler verbunden sind. Bietet Moeglichkeit zum Chatten
 **/
class CStartGameDialog:public CDialog
{
private:
	CGUI* gui;
	CGameClient *client;
	CStaticText *statustext;
	// Ein Zeitzaehler, um periodisch das Fenster zu aktualisieren
	double time;
	CTextEdit *chat;

	// Den NET_SERVER_STATUS des SpielClient in dem statustext anzeigen
	void updateStatus(NET_SERVER_STATUS *status);
	// Aktuellen Text im Chat-Edit absenden.
	void sendChat();
public:
	// Constructor des StartGameDialog, servername ist zum Anzeigen des Servers
	CStartGameDialog(CGameClient *vclient,CGUI* vgui,const char *servername);
	// Maustaste verarbeiten
	virtual int processMouseEvent(TMouseEvent *event);
	// Animieren, periodisch aktualisieren
	virtual void execute(double elapsed);
	// Taste verarbeiten
	virtual bool processKey(unsigned short key);
};


/**
 * Der Dialog, der ueber Ausgang einer Partie berichtet, sowie ne Option zum neustarten 
 * einer Runde bietet.
 **/
class CGameFinishDialog:public CDialog
{
private:
	// GUI merken
	CGUI *gui;
	CTextEdit *chat;
	void sendChat();

public:
	// Constructor. Er saugt sich alle Daten aus dem CGameClient*
	CGameFinishDialog(CGUI *vgui,CGameClient *vclient);
	// Maustaste verarbeiten
	virtual int processMouseEvent(TMouseEvent *event);
	// Taste verarbeiten
	virtual bool processKey(unsigned short key);
	virtual void close();
	// Wir wollen nicht exklusiv die Maus, und hiermit kann man die Ansicht noch rotieren
	virtual bool handleUI() { return true; }
};

#endif

