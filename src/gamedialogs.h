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
	// Wir brauchen Zeiger auf GUI, um Rueckwaerts Zugriff auf bestimmte Fkt. zu haben
	CGUI* gui;
	// Sowie den SpielClients, mit dem wir interagieren sollen
	CSpielClient *client;
	// Der Text des aktuellen Server-Status. 
	CStaticText *statustext;
	// Ein Zeitzaehler, um periodisch das Fenster zu aktualisieren
	double time;
	// Und ein Chat-TextEdit
	CTextEdit *chat;

	// Den NET_SERVER_STATUS des SpielClient in dem statustext anzeigen
	void updateStatus(NET_SERVER_STATUS *status);
	// Aktuellen Text im Chat-Edit absenden.
	void sendChat();
public:
	// Constructor des StartGameDialog, servername ist zum Anzeigen des Servers
	CStartGameDialog(CSpielClient *vclient,CGUI* vgui,const char *servername);
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
	// Chat-Edit-Feld
	CTextEdit *chat;
	// Chat-Nachricht versenden
	void sendChat();

public:
	// Constructor. Er saugt sich alle Daten aus dem CSpielClient*
	CGameFinishDialog(CGUI *vgui,CSpielClient *vclient);
	// Maustaste verarbeiten
	virtual int processMouseEvent(TMouseEvent *event);
	// Taste verarbeiten
	virtual bool processKey(unsigned short key);
	// Schliessen des Fensters abfangen
	virtual void close();
	// Wir wollen nicht exklusiv die Maus, und hiermit kann man die Ansicht noch rotieren
	virtual bool handleUI() { return true; }
};

#endif

