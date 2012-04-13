/**
 * chatbox.h
 * Autor: Sascha Hlusiak
 * ChatBox zum Chatten ueber Netzwerk waehrend eines Spiels
 **/

#ifndef __CHATBOX_H_INCLUDED_
#define __CHATBOX_H_INCLUDED_

#include "widgets.h"
#include "network.h"

class CGUI;

/* Die Chat-Box soll CHAT_LINES Zeilen zwischenspeichern. */
#define CHAT_LINES (9)
/* Aus der Anzahl Zeilen die Hoehe des Widgets errechnen */
#define CHAT_HEIGHT (CHAT_LINES*11.0+34)

/**
 * Die ChatBox ist von einem CFrame abgeleitet
 **/
class CChatBox:public CFrame
{
private:
	/* Status der Animation, von 0.0 bis 1.0 */
	double anim;
	/* Soll ChatBox sichtbar sein */
	bool visible;
	/* Zeit, wie lange die ChatBox gerade sichtbar ist */
	double time;
	/* Animation der zuletzt hinzugefuegten Chat-Zeile */
	double lineanim;

	/* Zeichenketten des Chat-Verlaufs */
	char* lines[CHAT_LINES];
	/* Das TextEdit Widget */
	CTextEdit *text;
	/* GUI brauche ich auch noch */
	CGUI* gui;
	/* Wenn (!sticky) verschwindet die Chat-Box nach ein paar Sekunden, wenn keine 
	   Eingabe geschieht. Ansonsten bleibt sie sichtbar, bis sie geschlossen wird. **/
	bool sticky;
public:
	/* Konstruktor */
	CChatBox(double x,double y,CGUI* vgui);
	/* Muell wieder aufraeumen */
	virtual ~CChatBox();

	/* Chat-Box animieren */
	virtual void execute(double elapsed);
	/* Chat-Box rendern */
	virtual void render(bool selection);
	/* Tastendruck verarbeiten (Esc) */
	virtual bool processKey(unsigned short key);
	/* Mausklick verarbeiten, wg. Send-Button */
	virtual int processMouseEvent(TMouseEvent* event);

	/* Die Chat-Box sichtbar machen, Animation starten. */
	void show();
	/* Eine Neue Chat-Nachricht anfuegen */
	void addLine(const char *t,int len=-1);
	/* Eine ganze Nachricht von client anfuegen (-1 ist Server) */
	void addChatMessage(int client,const char* text);
	/* Chat-Box sticky setzen, oder auch nicht. */
	void setSticky(bool vsticky) { sticky=vsticky; }
};

#endif
