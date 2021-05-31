/**
 * chatbox.cpp
 * Autor: Sascha Hlusiak
 * Implementation der ChatBox zum Unterhalten ueber Netzwerk waehrend des Spiels
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <math.h>
#include <string.h>
#include "chatbox.h"
#include "gui.h"
#include "spielclient.h"
#include "glfont.h"
#include "constants.h"

/**
 * Chat-Box initialisieren
 **/
CChatBox::CChatBox(double x,double y,CGUI* vgui)
:CFrame(x,y,235,CHAT_HEIGHT)
{
	gui=vgui;
	setColor(0.5,0.8,0.7);
	setAlpha(0.65);
	anim=time=0.0;
	/* erstmal nicht sichtbar. */
	visible=false;
	/* und nicht sticky, d.h. verschwindet automatisch. */
	sticky=false;
	lineanim=0.0;

	/* Benoetigte Widgets hinzufuegen */
	addChild(text=new CTextEdit(5,CHAT_HEIGHT-20,180,16,100,"",false,this));
	addChild(new CButton(190,CHAT_HEIGHT-22,40,20,101,this,"Chat"));
	CFrame *f=new CFrame(x+5,y+5,w-10,CHAT_HEIGHT-30);
	f->setColor(0.0,0.12,0.0);
	f->setAlpha(0.65);
	addChild(f);
	/* Zeilen leeren (nullptr-Pointer) */
	for (int i=0;i<CHAT_LINES;i++)lines[i]=0;
}

/**
 * Saustall aufraeumen. Speicher freigeben
 **/
CChatBox::~CChatBox()
{
	/* Wenn eine Zeile eine Zeichenkette beinhaltet, freigeben. */
	for (int i=0;i<CHAT_LINES;i++)if (lines[i])free(lines[i]);
}

/**
 * Chat-Box animieren: einblenden, ausblenden, Zeile animieren.
 **/
void CChatBox::execute(double elapsed)
{
	CFrame::execute(elapsed);
	/* Wenn das Text-Edit mit Enter verlassen wurde, die Nachricht senden. */
	if (text->committed())
	{
		/* Nachricht senden */
		gui->spiel->chat(text->getText());
		/* Text-Edit leeren, aber NICHT Fokus geben. */
		text->setText("");
	}

	/* Wenn sichtbar, rein-animieren, sonst raus. */
	if (visible)anim+=elapsed*2.0;else anim-=elapsed*3.0;
	/* Animation auf gueltigen Bereich begrenzen */
	if (anim>1.0)anim=1.0;
	if (anim<0.0)anim=0.0;
	/* Zeit zaehlen, wie lange das Edit-Feld NICHT den Fokus hat. */
	if (!text->hasFocus())time+=elapsed;
	/* Nach 7 sek soll die Chat-Box automatisch verschwinden, wenn !sticky und die
	   entsprechende Option gesetzt ist. */
	if (time>7.0 && visible && !sticky && gui->getOptions()->get(OPTION_AUTO_CHATBOX))visible=false;
	/* Neue Zeilen sollen fluessig hochgeschoben werden. Dies hier tun. */
	if (lineanim<1.0)
	{
		lineanim+=elapsed*4.0;
		if (lineanim>1.0)lineanim=1.0;
	}
}

/**
 * Die Chat-Box rendern.
 **/
void CChatBox::render(bool selection)
{
	/* Status merken und Chat-Box in Position bringen. */
	glPushMatrix();
	glLoadIdentity();
	glTranslated(0,y,0);
	/* Chat-Box sauber animieren, wenn sichtbar */
	if (anim>0.0001)glScaled(sin(anim*M_PI/2.0),sin(anim*M_PI/2.0),1.0);
	else glRotated(-180,0,1,0);

	glTranslated(0,-y,0);
	/* Frame rendern, mit Widgets. */
	CFrame::render(selection);

	/* Die Texte rendern, wenn sichtbar */
	if (anim>0.0001)
	{
		/* Schriftart 6x11, weiss */
		CGLFont font(6,11);
		glColor3d(1,1,1);
		/* Position oberster Zeile berechnen */
		glTranslated(x+6,y+6,0);
		glTranslated(0,11.0-(sin(lineanim*M_PI-M_PI/2.0)/2.0+0.5)*11.0,0);
		/* Zeilenweise alle Zeilen bis auf eine ausgeben */
		for (int i=0;i<CHAT_LINES-1;i++)
		{
			if (lines[i])font.drawText(0,0,lines[i]);
			glTranslated(0,11,0);
		}
		/* Die letzte Zeile nur rendern, wenn die Animation vorrueber ist */
		if (lines[CHAT_LINES-1] && lineanim>0.99)
		{
			font.drawText(0,0,lines[CHAT_LINES-1]);
		}
	}
	glPopMatrix();
}

/**
 * Animation zum Anzeigen starten
 **/
void CChatBox::show()
{
	/* Sichtbar */
	visible=true;
	/* Ist noch 0 sek offen. */
	time=0.0;
	/* Eingabefeld direkt Fokus geben */
	text->setFocus();
}

/**
 * Auf Escape reagieren
 **/
bool CChatBox::processKey(unsigned short key)
{
	/* Wenn CFrame die Taste bearbeitet, hat sie wohl das Edit-Feld abgefangen */
	if (CFrame::processKey(key))return true;
	/* Wenn sichtbar, und Escape */
	if (visible && key==VK_ESCAPE)
	{
		/* Dann setz visible auf false, was eine Animation zur Folge haben wird */
		visible=false;
		/* Taste wurde bearbeitet */
		return true;
	}
	/* Taste wurde nicht bearbeitet */
	return false;
}

/**
 * Mausereignis verarbeiten: Klick auf Chat-Button
 **/
int CChatBox::processMouseEvent(TMouseEvent* event)
{
	int r=CFrame::processMouseEvent(event);
	switch (r)
	{
		/* Bei Klick auf Chat-Button, den Text versenden, und Edit-Feld leeren.
		   Es behaelt dabei den Fokus, stoert ja auch nicht. */
		case 101:gui->spiel->chat(text->getText());
			text->setText("");
			break;
		/* Kommando weiter durch reichen */
		default: return r;
	}
	/* Nicht weiter verarbeiten */
	return 0;
}

/**
 * Eine Zeile der Chat-Box hinzufuegen
 * t ist eine Zeichenkette, von der die erste len Zeichen verwendet werden sollen.
 **/
void CChatBox::addLine(const char *t,int len)
{
	/* Die oberste Zeile rutscht raus, also Speicher freigeben, wenn noetig. */
	if (lines[0])free(lines[0]);
	/* Alle Zeilen um eins nach oben schieben. */
	for (int i=0;i<CHAT_LINES-1;i++)lines[i]=lines[i+1];
	/* Speicher fuer unterste Zeile reservieren. */
	char *l=(char*)malloc(strlen(t)+2);
	/* wenn len==-1, kopiere kompletten String */
	if (len==-1)strcpy(l,t);else 
	{
		/* Sonst nehme nur die ersten len Zeichen */
		strncpy(l,t,len);
		/* Zeichenkette terminieren. */
		l[len]='\0';
	}
	/* Letzte Zeile zuweisen. */
	lines[CHAT_LINES-1]=l;
}

/**
 * Eine komplette Nachricht von client hinzufuegen. 
 * Die Nachricht wird umgebrochen, wenn noetig.
 * Bei client==-1 kommt die Nachricht vom Server selbst
 */
void CChatBox::addChatMessage(int client,const char *text)
{
	/* Maximale Anzahl Zeichen pro Zeile */
	const int maxlen=(int)(w-10-3)/6;
	
	/* Unterste Zeile animieren, wenn vorhanden. */
	if (lines[CHAT_LINES-1])lineanim=0.0; else lineanim=1.0;

	/* Wir basteln uns eine neue Zeichenkette mit "Client %d: " davor. */
	char *t=(char*)malloc(strlen(text)+20);
	if (client==-1)
		sprintf(t,"  *  %s",text);
	else 	sprintf(t,"Client %d: %s",client,text);
	unsigned int i;
	/* Zeichenkette zerhacken und einzeln umgebrochen hinzufuegen */
	for (i=0;i<strlen(t)/(maxlen);i++)
	{
		addLine(&t[i*maxlen],maxlen);
	}
	/* Letztes Bruchstueck hinzufuegen, wenn es nicht zufaellig ganz gepasst hat */
	if (i*maxlen<strlen(t))
	{
		addLine(&t[i*maxlen],-1);
	}

	/* Temporaeren Speicher freigeben. */
	free(t);
	/* Chat-Box sichtbar machen, denn es ist offensichtlich eine Nachricht von aussen
	   eingetroffen. */
	if (gui->getOptions()->get(OPTION_AUTO_CHATBOX))visible=true;
	/* Timeout zuruecksetzen. */
	time=0.0;
}
