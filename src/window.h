/**
 * window.h
 * Autor: Sascha Hlusiak
 *
 * CWindow kapselt ein Fenster der grafischen Oberflaeche des Systems,
 * entweder ein normales WIN32-Fenster, oder ein X11-Fenster (unter Linux),
 * und richtet die Fenster fuer OpenGL ein.
 **/

#ifndef _WINDOW_H_INCLUDED_
#define _WINDOW_H_INCLUDED_


#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <GL/glx.h>
#include <GL/glu.h>
#include <X11/Xlib.h>
#endif


// FOV der 3D-Szene, in Grad
const double fov=60.0;


// Fuer betriebssytemunabhaengige Behandlung von Mausereignissen
struct TMouseEvent
{
	// Ist Ereignis das Druecken, Loslasen der Maustaste, oder eine Mausbewegung?
	bool press,release,move;

	// Welche Maustaste? 4 und 5 sind Mausrad
	bool button1,button2,button3,button4,button5;

	// Koordinaten des Mauszeigers in Fensterkoordinaten
	int x,y;
};


#ifndef WIN32
/* Die Keycodes unter X11 fuer diverse Sondertasten */
const unsigned short VK_LEFT=0xFF51;
const unsigned short VK_UP=0xFF52;
const unsigned short VK_RIGHT=0xFF53;
const unsigned short VK_DOWN=0xFF54;
const unsigned short VK_SPACE=0x0020;
const unsigned short VK_RETURN=0xFF0D;
const unsigned short VK_BACK=0xFF08;
const unsigned short VK_ESCAPE=0xFF1B;
const unsigned short VK_F1=0xFFBE;
#endif


class CGUI;

/**
 * Die Klasse bietet gekapselten Zugriff auf ein GUI Fenster
 **/
class CWindow
{
private:
	// CWindow ruft rueckwaerts Funktionen der GUI auf, also wird ein Zeiger darauf benoetigt
	CGUI *gui;

#ifdef WIN32
	// Unter Windows, ein HWND
	HWND hWnd;
	// Der HDC des obigen Fensters
	HDC hDC;
	// Der OpenGL Kontext, der mit dem hDC verknuepft wird
	HGLRC ctx;
#else
	// Unter X11 ein Window
	Window window;
	// Einen GLXContext, der mit dem Window verknuepft wird
	GLXContext ctx;
	// Das Display, fuer alle X11 Operationen
	Display *dpy;

	// Sortiert und wandelt X11-Events und leitet sie an die GUI weiter
	void processEvent(XEvent e);
#endif

	// Signalisiert, dass das Fenster geschlossen ist
	bool closed;

	// true, wenn der GL Kontext erfolgreich mit STENCIL-Buffer erstellt wurde,
	// also prinzipiell Shadow-Volumes moeglich sind. Ohne, werden Schatten garnicht erst gerendert
	bool m_canShadow;
public:
	CWindow(CGUI *vgui);
	~CWindow();

	// Erstellt das Fenster, mit allem drum und dran. Gibt true zurueck bei Erfolg, sonst false
	bool createWindow();

	// Macht das Fenster auf dem Bildschirm sichtbar
	void show();

	// Gilt das Fenster als geschlossen? GUI reagiert darauf, und beendet sich und das Programm bei true
	bool isClosed() { return closed; }

	// Setzt das Fenster als geschlossen. 
	void close() { closed=true; }

	// Grast alle anliegenden Events des Fensters ab und behandelt sie. Sollte immer mal aufgerufen werden,
	// damit das Fenster nicht 'einfriert'
	void processEvents();

	// Vertauscht primary und secondary OpenGL-Buffer, und macht somit alle Zeichnungen 
	// des Hintergrunds sichtbar.
	void swapBuffers();

	// Updated den OpenGL Zustand fuer die Verwendung einer neuen Fenstergroesse
	void reshape(int w,int h);

	// Koennen Schatten ueberhaupt gerendert werden?
	bool canShadow() { return m_canShadow; }

#ifdef WIN32
	// Die Fensterfunktion des Windows-Fensters. Sortiert und behandelt Window Messages.
	// Reicht ggf. Nachrichten an die GUI weiter.
	LRESULT processMessage(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
#endif
};


#endif
