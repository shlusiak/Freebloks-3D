/**
 * gui.h
 * Autor: Sascha Hlusiak
 *
 * Bildet die Hauptklasse der GUI. Beherbergt das CWindow, reagiert auf Ereignisse des
 * Users, und rendert den ganzen Kappes. Nicht zu unterschaetzen!
 **/

#ifndef _WINDOW_INCLUDED_
#define _WINDOW_INCLUDED_

#include "timer.h"
#include "window.h"
#include "stoneeffect.h"
#include "widgets.h"
#include "glfont.h"
#include "guispielclient.h"
#include "options.h"

class CStone;
class CIntro;
class CChatBox;


// Sooo gross ist ein Stein in Raum-Koordinaten (das ist der Radius, nicht der Durchmesser)
const double stone_size=0.45;
// So hoch ist die Erhoehung auf dem Spielfeld, also ist ein Spielstein doppelt so hoch
const double bevel_height=0.15;
// Die Richtung unserer direktionalen Lichtquelle. Wird u.a. fuer Schatten benoetigt
const float light0_pos[]    = {2.5f, 5.0f, -2.0f, 0.0f};

// Namen der Spielerfarben
const char * const COLOR_NAME[4]={"Blue","Yellow","Red","Green"};


/**
 * Die GUI Klasse, quasi das Herzstueck des Programms
 **/
class CGUI
{
private:
	// Wir brauchen ein CWindow, also ein Hauptfenster mit OpenGL Unterstuetzung
	CWindow * window;

	// Einen Timer, fuer hochgenaue Timings, die das Rendern und die Animationen betreffen
	CTimer timer;

	// Verkettete Liste fuer die 2D Widgets, also die Steuerelemente der GUI
	CWidgetPane widgets;

	// Chat-Box ist stets vorhanden
	CChatBox *chatbox;

	// Hier sollten Unterfenster hinzugefuegt werden, die parallel zur Menuleiste laufen sollen
	CWidget *subchilds;

	// Zeiger auf wichtige, dynamische Widgets
	CFrame *playerframe;
	CStaticText *playertext;

	// Zeiger auf das 2D-Hauptmenu, sofern sichtbar
	CDialog *menu;

	// Effekte und Animationen von Steinen kommen in diese verkettete Liste
	CStoneEffect *effects;

	// Das Intro Objekt, sofern es gerade abgespielt wird.
	CIntro *intro;

	// Die Textur des Untergrunds, und der Wand
	CTexture texture_ground,texture_wall,texture_field;

	// Die eingestellten Optionen
	TOptions options;

	// Aktuelle Winkel der Ansicht, um Y-Achse und X-Achse und Entfernung der Kamera vom Mittelpunkt
	double angx,angy,camera_distance;

	// Letzte Mausposition
	int lastx,lasty;

	// Koordinate des letzten Mausklicks. Gebraucht bei Rechts-Klick zum spiegeln ODER drehen
	int clickx,clicky;

	// Aktuelle X und Y Koordinate des selektierten Spielsteins auf dem Spielfeld in Feld-Koordinaten
	int selectedx,selectedy;

	// Ist der Zug, der auf dem Spielfeld ausgewaehlt ist, erlaubt?
	char selected_allowed;

	// Aktueller ausgewaehlter Stein, oder -1 fuer kein Stein
	int current_stone;

	// Zeitzaehler fuer Animationen
	double anim;

	// Stein, auf dem gerade der Mauszeiger steht. Dieser wird grafisch hervorgehoben
	int hovered_stone;

	// Pfeil (Rotation, Spiegelung), der gerade mit der Maus selektiert ist
	int selected_arrow;

	// Globaler Zeiger auf ein GLFont Objekt.
	CGLFont *font;

	// DisplayLists
	GLuint displayLists;

	CButton *hint,*undo;

	// Startupparameter
	struct PARAMS
	{
		char* remotehost;
		int remoteport;
		bool firststart,intro;
		int humans,threads;
	}startupParams;
public:
	CGUI();
	~CGUI();
	
	// Hauptmethode, wird von der main() aus freebloks.cpp aufgerufen.
	void run();

	// Hilfetext ausgeben
	void printHelp()const;

	// Parameter parsen
	bool parseCmdLine(int argc,char **argv);

	// Funktion Animiert und Rendert, aber erst nachdem mintime sek seit dem letzten Mal verstrichen sind.
	bool checkPoint(double mintime=0.0);

	// CWindow ruft dies auf, wenn ein TMouseEvent anliegt. Wird hier von der GUI verarbeitet
	void processMouseEvent(TMouseEvent *e);

	// CWindow teilt uns hier einen Tastendruck mit
	void processKeyEvent(unsigned short key);

	// CSpielClient teilt hier mit, dass ein Spielerwechsel stattgefunden hat
	void newCurrentPlayer(int player);

	// Setze Rotation um Y-Achse
	void setAngy(double a) { angy=a; }

	// Setze Zoom-Faktor
	void setZoom(double z) { camera_distance=z; }
private:
	// Belege die OpenGL Statusmachine mit brauchbaren Voreinstellungen
	void initGL();

	// Erstellt die GL Display Lists
	void createDisplayLists();

	// Startet das erste Spiel
	void startFirstGame();

	// Rendere einen Frame in den Backbuffer
	void render();

	// Animiere alles bei einer Zeitdifferenz von elapsed, seit dem letzten Frame
	void execute(double elapsed);
	
	// Prueft, auf welches Objekt der Benutzer gerade mit der Maus zeigt (Fensterkoordinate x/y) und setzt entsprechende Variablen.
	void testSelection(int x,int y);

	// true, wenn das uebergebene Feld in Feldkoordinaten, zur aktuellen Auswahl auf dem Feld gehoert, also der User gerade dort einen Stein platzieren will
	bool isInSelection(int x,int y)const;

	// Rendert einen stinknormalen Wuerfel
	void renderCube(double x1,double z1,double x2,double z2,double y1,double y2)const;

	// Rendert alle Steine des Spielers direkt vor den Betrachter
	void renderPlayerStones(int player,double alpha,bool selectionbuffer)const;

	// Rendert einen einzigen Stein aus dem Sortiment des Spielers, ggf. samt Pfeile
	void renderPlayerStone(int player,CStone* stone,int stone_number,bool current,bool hovered,double alpha,bool selectionbuffer)const;

	// Rendert die Pfeile eines Stein des Spielers
	void renderPlayerStoneArrows(int player,CStone* stone,int stone_number,bool selectionbuffer)const;

	// Rendert exakt einen Pfeil an den Ursprung des aktuellen Koordinatensystems
	void renderArrow(bool doublearrow,bool selected)const;

	// Rendert ein 4eck an den Ursprung
	void renderQuad(double size)const;

	// Beendet das Intro, z.B. bei Mausklick oder Tastendruck
	void stopIntro();

	// Zeigt den Zug an, den die schwere KI jetzt setzen wuerde
	void showHint();
public:
	// Das eigentliche Spiel (der Client), connected oder nicht
	CGUISpielClient * spiel;

	// Rendert einen einzelnen Stein nach x/y in Feldkoordinaten, mit Farbe, alpha
	void renderStone(int x,int y,int color,float alpha,bool doublesided)const;

	// Rendert einen einzelnen Stein der Farbe und Transparenz an den Ursprung des Koordinatensystems
	void renderStone(int color,float alpha,bool selectionbuffer)const;

	// Rendert, ausgehend vom Ursprung, das Shadowvolume eines Steines, projiziert entlang der Richtung des Lichts
	void renderStoneShadow(float lightdir[3])const;

	// Rendert die Unterlage des Spielfeldes (z.B. ein Holztisch)
	void renderGround()const;

	// Rendert das Spielfeld (ohne Steine), ggf mit farbiger markierung moeglicher Steine
	void renderField(bool withgame=true)const;
	void renderField(bool withgame,int w,int h)const;

	// Fuegt einen CStoneEffect der verketteten Liste hinzu. Wird vom CSpielClient aufgerufen, nachdem ein Computer einen Zug gemacht hat. 
	void addEffect(CStoneEffect *effect) { effects->add(effect); }

	// Fuegt ein (vollstaendig modales) Unterfenster den widgets hinzu!
	void addSubChild(CWidget *w,bool modal=true) { if (modal)widgets.addSubChild(w); else subchilds->addSubChild(w);}

	// Gibt die Weltkoordinaten des Spielersteins zurueck, der vor der Kamera schwebt.
	// Anhaenig von den Betrachtungswinkeln. Wird auch von CStoneRollEffect benutzt
	void getPlayerStonePos(int player,int stone,double *dx,double *dy,double *dz)const;

	// Startet ein Einzelspielerspiel
	bool startSingleplayerGame(GAMEMODE gamemode,int localplayers,int diff,int width,int height,int einer,int zweier,int dreier,int vierer,int fuenfer,int ki_threads);
	// Hostet und startet ein Mehrspielerspiel
	bool startMultiplayerGame(GAMEMODE gamemode,int maxhumans,int localplayers,int diff,int width,int height,int einer,int zweier,int dreier,int vierer,int fuenfer,int ki_threads, const char* name);
	// Tritt einem Mehrspielerspiel bei
	bool joinMultiplayerGame(const char *host,int port,int localplayers, const char* name);

	// Startet das Intro
	void showIntro();

	// Fuegt der CChatBox die uebergebene Textnachricht an, die vom Netzwerk empfangen wurde
	void addChatMessage(int client,const char* text);

	// Ist die ChatBox sticky, so verschwindet sie NICHT automatisch nach 7 sek. 
	void setChatBoxSticky(bool sticky);

	// Gebe die Einstellungen der GUI als Zeiger zurueck
	TOptions* getOptions() { return &options; }

	// Aktuellen Stein setzen
	void setCurrentStone(int s) { current_stone=s; selected_allowed=false; }

	void setHintEnable(bool b);
};


#endif
