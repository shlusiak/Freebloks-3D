/**
 * widgets.h
 * Autor: Sascha Hlusiak
 * Klassen der Engine fuer die 2D Menues, hier "Widgets" genannt
 **/

#ifndef __WIDGETS_H_INCLUDED_
#define __WIDGETS_H_INCLUDED_

#include "window.h"
#include "network.h"

class CSpielClient;
struct TOptions;


/**
 * Widget, die Hauptklasse, davon werden alle anderen Widgets abgeleitet
 * Stellt grundlegende virtuelle Methoden zur Verfuegung, ohne wirklich viel zu machen
 **/
class CWidget
{
private:
	/* next:  Widgets, die parallel zu this liegen
	   child: Ein Widget, das innerhalb von this zu finden ist
	   subchild: Ein Widget, das ausserhalb von this liegt, und hoehere Prioritaet hat */
	CWidget *next,*child,*subchild;
	// Flag, ob das Widget entfernt werden soll. Wird in aufrufender execute entfernt
	bool m_deleted;
	// Die ID des Widgets (sollte eindeutig sein).
	// die ID spielt nur bei Mausoperationen eine Rolle, damit die GUI das entsprechende
	// Widget wiederfinden kann
	int m_id;
protected:
	// Koordinaten des Widgets, sind nicht wichtig, ausser fuer SelectionBuffer,
	// und wenn die Parameter schonmal da sind...
	double x,y,w,h;

	// Wird aufgerufen, wenn die Maus innerhalb des Widgets bewegt wurde
	virtual void mouseEnter() {}
	// Wird aufgerufen, wenn die Maus ausserhalb des Widgets bewegt wurde
	virtual void mouseLeave() {}
public:
	// Constructor
	CWidget(); 
	// Destructor
	virtual ~CWidget();

	// Ein neues Widget parallel zu this in einer Liste einhaengen
	void add(CWidget* w) { if (next) next->add(w); else next=w; }
	// Ein Kind-Widget anhaengen, als direktes Kind von this. Dies ist ein next von ggf. vorhandenen Widgets
	void addChild(CWidget *w) { if (child)child->add(w); else child=w; }
	// Fuegt ein Unterfenster an, dieses kriegt etwas exklusiver Nachrichten und wird bevorzugt behandelt, als haette es den Fokus
	void addSubChild(CWidget *subchild);
	
	// true, wenn Widget im Begriff ist, entfernt zu werden.
	bool deleted() { return m_deleted; }
	// true, wenn das Widget ein (wichtigeres) Subchild hat
	bool hasSubChild() { return (subchild!=0); }
	// das Subchild zurueckgeben. 
	CWidget *getSubChild() { return subchild; }

	// Kommt hier true zustande, verzichtet die GUI auf eigene Mausbehandlung beim Spielfeld (sinnvoll bei modalen Dialogen)
	virtual bool handleUI() { bool r=false; if (next)r|=next->handleUI(); if (subchild)r|=subchild->handleUI(); if (child)r|=child->handleUI(); return r; }

	// Widget schliessen, einfach entfernen. Kann ueberladen werden, um Animationen o.ae. zu ermoeglichen
	virtual void close() { remove(); }

	// Wird von GUI aufgerufen, mit der ID des Widgets, ueber dem sich gerade der Mauszeiger bewegt (oder 0, bei keinem Widget). Das ist die einzige Moeglichkeit festzustellen, ueber welchem Widget der Mauszeiger ist.
	void processSelection(int id);

	// Wird von GUI aufgerufen, wenn ein Mausereignis eingetreten ist (z.B. klick)
	// Der Rueckgabewert kann die ID eines Widgets sein, der Aufrufer kriegt mit,
	// ob das Widget Beachtung will, oder nicht (Button Klick, Checkbox Klick, oder so)
	// >> So teilen Widgets dem uebergeordneten Widgets Events mit! <<
	virtual int processMouseEvent(TMouseEvent *event);
	// Wird von GUI aufgerufen, wenn eine Taste gedrueckt wurde, die verarbeitet werden will. Gibt true, wenn das Widget die Taste verarbeitet hat, bei true sollte KEIN anderes Widget die Taste weiter verarbeiten, da schon geschehen.
	virtual bool processKey(unsigned short key);

	// Widget zur Entfernung kennzeichnen
	void remove() { m_deleted=true; }
	// ID dieses Widgets zurueckgeben
	const int getId()const { return m_id; }
	// X-Koordinate, bzw. Y-Koordinate des Widgets zurueckgeben
	double getX() { return x; }
	double getY() { return y; }
	// ID des Widgets setzen
	void setId(int id) { m_id=id; }

	// Hier kommen Animationen und sonstige automatische Aktionen rein
	virtual void execute(double elapsed);
	// Widget rendern, selection ist true, wenn in SelectionBuffer gerendert werden soll
	virtual void render(bool selection);
	// Subchilds rendern, muessen immer nach eigentlichem Widget gerendert werden.
	void renderSubChild(bool selection);
};


/**
 * Diese Klasse wird von der GUI als Widget-Speicher benutzt.
 **/
class CWidgetPane:public CWidget
{
public:
	// Hier wird OpenGL fuers Rendern der 2D Widgets vorbereitet, dann werden die eigentlichen Widgets der verketteten Listen gerendert
	void render(bool selectionBuffer);
	// Verwaltet Mausereignisse der Widgets
	virtual int processMouseEvent(TMouseEvent *event);
};

/**
 * Ein simples Rechteck, 4 Ecken, transparent, mit Farbverlauf
 **/
class CFrame:public CWidget
{
private:
	// Farbwerte des Rechtecks
	double r,g,b,a;
public:
	// Hm...
	CFrame(double vx,double vy,double vw,double vh);
	// Unglaublich
	virtual void render(bool selection);
	// Wers weiss, darf sich nen Keks nehmen
	void setColor(double vr,double vg,double vb) { r=vr; g=vg; b=vb; }
	// Ja, aeh...
	void setAlpha(double va) { a=va; }
	// Groesse setzen
	void setSize(double vx,double vy,double vw,double vh);
};

/**
 * Ein Knopf
 **/
class CButton:public CWidget
{
private:
	// Ist der Knopf hervorgehoben (Maus-over), ist er enabled?
	bool hovered,enabled;
	// Animation des Mouse-Over Effekts (0.0-1.0), und Animation des Klicks
	double anim,pushanim;
	// Die Beschriftung des Buttons
	char *text;
protected:
	// Maus bewegt sich ueber Widget, damit ist hover=enabled
	virtual void mouseEnter() { hovered=enabled && true; }
	// Maus bewegt sich ausserhalb des Widgets, Button ist in jedem Fall nicht mehr hovered
	virtual void mouseLeave() { hovered=false; }
public:
	// Constructor
	CButton(double vx,double vy,double vw,double vh,int id,CWidget* parent,const char *vtext);
	// Destructor zum Aufraeumen
	virtual ~CButton();

	// Rendern des Buttons
	virtual void render(bool selection);
	// Animieren des Buttons
	virtual void execute(double elapsed);
	// Mausereignis verarbeiten. Wenn gedrueckt und hovered, dann wird die ID des Buttons zurueck gegeben
	virtual int processMouseEvent(TMouseEvent *event);
	// Button enablen / disablen
	void setEnabled(bool enabled) { this->enabled=enabled; }
	bool isEnabled() { return enabled; }
};

/**
 * Eine Checkbox, kann aus oder an sein, koennen in CheckBox-Groups organisieren werden
 **/
class CCheckBox:public CWidget
{
private:
	// Maus kann ueber Checkbox sein, Checkbox kann deaktiviert sein, und an oder aus (checkt)
	bool hovered,enabled,checked;
	// Beschriftung
	char *text;
	// Animation, wenn man mit der Maus draufdrueckt
	double pushanim;
protected:
	// Maus ist ueber Widget, also ist hovered=enabled
	virtual void mouseEnter() { hovered=enabled && true; }
	// Maus ist ausserhalb Widget, hovered ist immer false
	virtual void mouseLeave() { hovered=false; }
public:
	// Fuer CheckBox-Gruppen, Zeiger auf naechste und vorherige Checkbox, oder nullptr, wenn es das Ende ist
	CCheckBox *nextCheckBox,*prevCheckBox;

	// Constructor
	CCheckBox(double vx,double vy,double vw,double vh,int id,const char *vtext,bool vchecked,CWidget *parent);
	// Destructor zum Aufraeumen
	virtual ~CCheckBox();
	
	// Rendern
	virtual void render(bool selection);
	// Animieren
	virtual void execute(double elapsed);
	// Mausereignis verarbeiten, gibt ID zurueck, wenn auf Checkbox geklickt wurde
	virtual int processMouseEvent(TMouseEvent *event);
	// Ist Checkbox gecheckt?
	bool getCheck() { return checked; }
	// check setzen
	void setCheck(bool vchecked) { checked=vchecked; }
	// Checkbox aktivieren oder deaktivieren, nimmt bei Deaktivierung keine Mausereignisse mehr an
	void setEnabled(bool enabled) { this->enabled=enabled; }
	// Eine weitere Checkbox der Gruppe anfuegen. Verhaelt sich wie eine Radiogroup
	void addCheckBox(CCheckBox *n);
};

/**
 * Nur ein statischer Text, der gerendert wird. Kann zentriert sein, Text kann nachtraeglich
 * geaendert werden.
 **/
class CStaticText:public CWidget
{
private:
	// Der Text
	char *text;
	// Alpha Wert fuer Transparenz
	double a;
public:
	// Konstruktor, Text wird links an x und y ausgerichtet, relativ zur Koordinate von parent
	CStaticText(double x,double y,const char *vtext,CWidget *parent);
	// Konstruktor, Text wird zentriert an x mit Breite w ausgegeben, relativ zur Koordinate von parent
	CStaticText(double x,double y,double w,const char *vtext,CWidget *parent);
	// Aufraeumen
	virtual ~CStaticText();
	// Den Text setzen, ggf. alten Text freigeben. adjustwidth: soll neue Breite dem Text angepasst werden?
	void setText(const char *vtext,bool adjustwidth=false);
	// Den Text rendern
	virtual void render(bool selection);
	// Transparenz setzen
	void setAlpha(double va) { a=va; }
};


/**
 * Ein TextEdit-Feld. Man kann Text eingeben, ggf. auch nur auf Zahlen beschraenkt
 **/
class CTextEdit:public CFrame
{
protected:
	// Ist die Maus ueber dem Edit-Feld, akzeptiert das Feld Tastatureingaben, sind NUR Zahlen erlaubt, oder alles?
	bool hovered,focused,numbers;
	// Status der Animation des Fokuses, und der Animation eines eingebenenen Zeichens
	double animatefocused,animatechar;
	// true, wenn der Benutzer Enter gedrueckt hat. 
	bool m_committed;
	// Das Zeichen, das mit Backspace entfernt wurde, damit es angemessen entfernt werden kann
	char lastchar;
	// Puffer der eingegebenen Zeichen. 100 sollten reichen, werden ja keine Romane eingegeben
	char text[100];
	// Soll der Text links im Edit Feld ausgerichtet werden?
	bool left_align;
	// Maus ist ueber Edit-Feid *gaehn*
	virtual void mouseEnter() { hovered=true; }
	// Und nicht mehr drueber, also hovered anpassen
	virtual void mouseLeave() { hovered=false; }
	// Aufgerufen, wenn TextEdit den Fokus verliert, hier koennten Ueberpruefungen rein (wie bei SpinBox)
	virtual void endEdit() {}
public:
	// Constructor, selbsterklaerend hoff ich
	CTextEdit(double x,double y,double w,double h,int id,const char *vtext,bool numbers_only,CWidget *parent);
	// Aufraeumen
	virtual ~CTextEdit();

	// Textfeld soll den Fokus haben, kann von extern bei Initialisierung von Dialogen aufgerufen werden
	void setFocus() { focused=true; }
	// Hat das Feld den Fokus?
	bool hasFocus() { return focused; }
	// Gibt eingegebenen Text zurueck. Zeiger sollte kopiert und nicht veraendert werden
	const char *getText();
	// Text vorinitialisieren oder fest setzen
	void setText(const char *vtext);
	// Wurde das Feld mit Enter verlassen?
	bool committed() { return m_committed; }
	void unsetCommitted() { m_committed=false; }

	// Animieren
	virtual void execute(double elapsed);
	// Rendern
	virtual void render(bool selection);
	// Mausereignis verarbeiten
	virtual int processMouseEvent(TMouseEvent *event);
	// Tastendruck verarbeiten. 
	virtual bool processKey(unsigned short key);
};

/**
 * Eine Spinbox, ist ein TextEdit, das nur Zahlen akzeptiert, und 
 * zwei Buttons fuer die Pfeile als "child" hat :)
 **/
class CSpinBox:public CTextEdit
{
protected:
	// Gueltiger Bereich des Wertes. Aktueller Wert steht als Text im TextEdit
	int min,max,def;
	// Buttons fuer + und -, damit sie bei Erreichen der Grenze deaktiviert werden koennen
	CButton *up,*down;
	
	// Wenn das Editieren beendet ist, soll eine Ueberpruefung des Werts stattfinden
	virtual void endEdit();
public:
	// Constructor, mit initialisierenden Werten
	CSpinBox(double x,double y,double w,double h,int id,int min,int max,int value,CWidget *parent,bool rightButtons=true);

	// Aktuellen Wert als Zahl zurueckgeben
	int getValue();
	// Wert setzen
	void setValue(int value);

	// Grenzen setzen
	void setMin(int min) { this->min=min; endEdit(); }
	void setMax(int max) { this->max=max; endEdit(); }

	// Mausereignis verarbeiten
	virtual int processMouseEvent(TMouseEvent *event);
};

/**
 * Ein Dialog. Erweitert CFrame um eine schicke Animation beim Erstellen/Entfernen, 
 * sowie einen Titelleistentext
 **/

class CDialog:public CFrame
{
private:
	// Animation beim herein-, bzw. herausgleiten
	double anim;
	// Wird der Dialog gerade eingeblendet, oder ausgeblendet?
	bool starting;
	// Titelleistentext
	char *caption;
public:
	// Einen Dialog der Breite vw und Hoehe vh erstellen. Der Dialog wird im Fenster genau zentriert und erhaelt die Titelleiste "vcaption"
	CDialog(double vw,double vh,const char *vcaption);
	// Aufraeumen
	virtual ~CDialog();

	// Dialog und Unterfenster rendern, ggf. vorher die Animation mit berechnen
	virtual void render(bool selection);
	// Animieren
	virtual void execute(double elapsed);
	// Maustaste verarbeiten
	virtual int processMouseEvent(TMouseEvent *event);
	// Taste verarbeiten
	virtual bool processKey(unsigned short key);
	// Wenn irgendein Dialog geoeffnet ist, darf die GUI keine Mausereignisse verarbeiten
	virtual bool handleUI() { return starting; }
	// Extra close() Funktionen, um dem Dialog ein animiertes Ableben zu verschaffen
	virtual void close();
	// Dialog schliessen, mit Unterfenstern?
	virtual void close(bool recursive);

	// Dialog voll anzeigen, Animation beenden.
	void finishAnim() { anim=1.0; }

	// Titelleistentext neu setzen
	void setCaption(const char* caption);

	// Fenster Groesse setzen
	void setSize(double vw,double vh);
};

/**
 * Eine MessageBox, zeigt eine Fehlemeldung in einem roten Fenster mit Okay-Knopf an
 **/
class CMessageBox:public CDialog
{
public:
	// Nachrichtenfenster, Farbe kann definiert werden, ist per default rot
	CMessageBox(double w,double h,const char *caption,const char *text,double r=1.0,double g=0.2,double b=0.1);
};


/**
 * Ein Zaehler, der die Anzahl von execute() Aufrufen zaehlt, und nach 500ms eine FPS daraus errechnet.
 **/
class CFPS:public CStaticText
{
private:
	/* Zaehlt die Anzahl Frames */
	int counter;
	/* Und die Zeit, wie lange Frames gerendert werden. */
	double time;
	/* Die Optionen der GUI merken. */
	TOptions *options;
public:
	/* Konstruktor, soso. */
	CFPS(double x,double y,CGUI* gui);
	/* execute. */
	virtual void execute(double elapsed);
	/* Und rendern. */
	virtual void render(bool selectionBuffer);
};


#endif
