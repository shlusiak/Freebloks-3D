/**
 * window.h
 * Author: Sascha Hlusiak
 *
 * CWindow wraps a window of the system's graphical user interface,
 * either a normal WIN32 window or an X11 window (on Linux),
 * and sets the window up for OpenGL.
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


// FOV of the 3D scene, in degrees
const double fov=60.0;


// For operating-system-independent handling of mouse events
struct TMouseEvent
{
	// Is the event a mouse button press, a release, or a mouse movement?
	bool press,release,move;

	// Which mouse button? 4 and 5 are the mouse wheel
	bool button1,button2,button3,button4,button5;

	// Coordinates of the mouse pointer in window coordinates
	int x,y;
};


#ifndef WIN32
/* The X11 keycodes for various special keys */
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
 * The class provides encapsulated access to a GUI window
 **/
class CWindow
{
private:
	// CWindow calls back into GUI functions, so it needs a pointer to it
	CGUI *gui;

#ifdef WIN32
	HWND hWnd;
	HDC hDC;
	// The OpenGL context associated with the hDC
	HGLRC ctx;
#else
	Window window;
	// A GLXContext associated with the Window
	GLXContext ctx;
	Display *dpy;

	// Sorts and converts X11 events and forwards them to the GUI
	void processEvent(XEvent e);
#endif

	bool closed;

	// true if the GL context was successfully created with a stencil buffer,
	// i.e. shadow volumes are possible in principle. Without it, shadows are not rendered at all
	bool m_canShadow;
public:
	CWindow(CGUI *vgui);
	~CWindow();

	// Creates the window with all the trimmings. Returns true on success, otherwise false
	bool createWindow();

	void show();

	// Is the window considered closed? The GUI reacts to it and ends itself and the program on true
	bool isClosed() { return closed; }

	void close() { closed=true; }

	// Polls and handles all pending events of the window. Should be called regularly
	// so the window does not 'freeze'
	void processEvents();

	// Swaps the primary and secondary OpenGL buffers, making everything drawn
	// in the background visible.
	void swapBuffers();

	// Updates the OpenGL state for a new window size
	void reshape(int w,int h);

	bool canShadow() { return m_canShadow; }

#ifdef WIN32
	// The window procedure of the Windows window. Sorts and handles window messages.
	// Forwards messages to the GUI if needed.
	LRESULT processMessage(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
#endif
};


#endif
