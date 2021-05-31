/**
 * window.cpp
 * Autor: Sascha Hlusiak
 *
 * Kapselt ein Fenster, entweder mit WIN32-API unter Windows, oder X11 unter Linux
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
  #include <windows.h>
  #include "resource.h"
  #ifndef WM_MOUSEWHEEL
// Windows-Message fuer das Maus-Rad. Einfachheithalber hier definiert, damit es auch mit alten
// Win-SDKs compiliert. 
    #define WM_MOUSEWHEEL                   0x020A
  #endif
#else
  #ifdef HAVE_X11_XPM_H
    #include <X11/xpm.h>
  #endif
#endif

#include "window.h"
#include "gui.h"

// Titel des Fensters
static const char* WINDOW_TITLE="Freebloks 3D";
// Startgroesse des Fensters (unter Windows ist das Fenster zusaetzlich noch maximiert
static const int WND_SIZE_X=800;
static const int WND_SIZE_Y=600;

#ifdef WIN32
// Die HINSTANCE des Programms. Wird in der Winmain() der freebloks.cpp gesetzt.
HINSTANCE hInstance=0;

/**
 * Die eigentliche Fensterfunktion des Hauptfensters. Mit USERDATA ist allerdings ein Pointer
 * assoziiert, der auf die CWindow Instanz zeigt. Somit kann die processMessage des Fensters
 * aufgerufen werden
 **/
LRESULT CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CWindow* wnd=(CWindow*)GetWindowLong(hWnd,GWL_USERDATA);
	if (wnd)return wnd->processMessage(hWnd,uMsg,wParam,lParam);
		else return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

#endif


/**
 * Simpler Konstruktor, initialisiert lediglich Variablen
 **/
CWindow::CWindow(CGUI *vgui)
{
	gui=vgui;
	m_canShadow=false;
#ifdef WIN32
	hWnd=0;
	hDC=0;
#else
	window=0;
	dpy=nullptr;
#endif
	closed=true;
	ctx=0;
}

/**
 * Destruktor: Sofern vorher ein Fenster erstellt wurde, 
 * so wird es hier schliesslich sauber entfernt
 **/
CWindow::~CWindow()
{
#ifdef WIN32
	wglMakeCurrent(0,0);
	if (hDC)ReleaseDC(hWnd,hDC);
	if (hWnd)DestroyWindow(hWnd);
#else
	if (ctx)glXMakeCurrent(dpy,0,0);
	if (window)
	{
		XUnmapWindow(dpy,window);
		XDestroyWindow(dpy,window);
	}
#endif
}

/**
 * Wird aufgerufen, um den OpenGL-Viewports des Fensters auf die richtige Groesse einzustellen
 * Parameter: Groesse des verfuegbaren Bereichs des Fensters (Clientbereich)
 **/
void CWindow::reshape(int width,int height)
{
	// Viewport auf jeden Fall 4:3 halten
	int w=width,h=height;
// 	if ((w*3)/4 > height)w=(height*4)/3;
// 	else h=(width*3)/4;

	// Viewport der Breite w und h setzen und im Fenster zentrieren
	glViewport((width-w)/2, (height-h)/2, (GLint) w, (GLint) h);
}



#ifdef WIN32

/**
 * WIN32-Implementation von createWindow(): Erstelle ein Fenster samt OpenGL-Firlefanz
 **/
bool CWindow::createWindow()
{
	// Zuerst will die Klasse des Fensters registriert sein
	WNDCLASS wndclass;
	ZeroMemory(&wndclass,sizeof(wndclass));
	wndclass.style=CS_HREDRAW|CS_VREDRAW;	// Fenster neuzeichnen, beim Groesse-Aendern
	wndclass.lpfnWndProc=WndProc;		// Die Fensterfunktion ist global.
	wndclass.hInstance=::hInstance;		// Unsere HINSTANCE des Progz
	wndclass.hCursor=LoadCursor(nullptr,IDC_ARROW);	// Fenster soll Pfeil als Cursor
	wndclass.hIcon=LoadIcon(::hInstance,MAKEINTRESOURCE(IDI_ICON));	// Fenster soll eigenes Icon haben
	wndclass.lpszClassName="FreebloksWindowClass";	// Der Name unserer Fenster-Klasse. 
	if (!RegisterClass(&wndclass))return false;	// Klasse bei Windows registrieren, bei Fehler raus

	// Erstelle ein Win32-Fenster, standard Position, aber vorgegebene Groesse. Nix besonderes.
	hWnd=CreateWindow("FreebloksWindowClass",WINDOW_TITLE,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,CW_USEDEFAULT,CW_USEDEFAULT,WND_SIZE_X,WND_SIZE_Y,nullptr,nullptr,::hInstance,nullptr);
	if (hWnd==0)return false; // Bei Fehler raus

	// Hier muss USERDATA des Fensters mit this assoziiert werden, damit die globale Fensterfunktion
	// Das Fenster mit dieser Klasse verbinden und unsere processMessage() aufrufen kann. 
	SetWindowLong(hWnd,GWL_USERDATA,(unsigned long)this);

	// Passendes PixelFormat auswaehlen, hDC zuordnen und OpenGL Kontext erstellen
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormat;
	m_canShadow=true;

	// Wir brauchen den DisplayContext des erstellten Fensters.
	hDC=GetDC(hWnd);

	// PixelFormatDescriptor leeren und initialisieren. 
	ZeroMemory(&pfd,sizeof(pfd));
	pfd.nSize=sizeof(pfd);
	pfd.nVersion=1;
	// Wir wollen ins Fenster rendern koennen, mit OpenGL, und Doublebuffer, 
	// damit wir in den Hintergrund rendern koennen
	pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
	// Zuerst mindestens 1 Bit Stencil Buffer anfordern.
	pfd.cStencilBits=1;
	// Passendes PixelFormat auswaehlen. 
	iPixelFormat=ChoosePixelFormat(hDC,&pfd);

	if (iPixelFormat==0)
	{	// Wenn kein PixelFormat gefunden wurde, dasselbe nochmal, aber ohne StencilBits versuchen.
		pfd.cStencilBits=0;
		iPixelFormat=ChoosePixelFormat(hDC,&pfd);
		// Auf jeden Fall koennen wir nun keine Schatten mehr
		m_canShadow=false;
	}
	// Wenn bis hier kein PixelFormat gefunden wurde, schlagen wir mal fehl.
	if (iPixelFormat==0)return false;

	// Das gefundenen PixelFormat weisen wir dem hDC zu.
	SetPixelFormat(hDC,iPixelFormat,&pfd);

	// Und verbinden einen OpenGL Kontext zu dem hDC und damit dem hWnd (Fenster). 
	ctx=wglCreateContext(hDC);
	// Ohne OpenGL-Kontext kein Spiel!
	if (ctx==0)return false;

	// Kontext aktivieren, gilt global fuer die gesamte Anwendung.
	wglMakeCurrent(hDC,ctx);

	// Das Fenster ist NICHT geschlossen.
	closed=false;

	// ERFOLG!
	return true;
}

/**
 * Nachrichtenschleife fuer Win32, verarbeitet alle anstehenden Nachrichten, und kehrt danach zurueck
 **/
void CWindow::processEvents()
{
	MSG msg;
	// Schleife, solange Nachricht abgeholt werden konnte
	while (PeekMessage(&msg, nullptr,0,0,PM_REMOVE))
	{
		// WM_QUIT setzt das WM_DESTROY des Fensters ab. Damit wird das Fenster als geschlossen betrachtet
		if (msg.message==WM_QUIT)
			closed=true;
		else {
			// Eventuelle WM_KEYDOWN Messages in WM_CHAR uebersetzen. Wichtig fuer Tastatureingaben
			TranslateMessage(&msg);
			// Nachricht verarbeiten. Ruft i.A. die Fensterfunktion des Fensters auf.
			DispatchMessage(&msg);
		}
	}
}

/**
 * Die Fensterfunktion des Fensters. Wird von der globalen Fensterfunktion aufgerufen, die durch
 * DispatchMessage aufgerufen wird. Hier werden alle Fensternachrichten verarbeitet, und ggf. an die GUI
 * weitergeleitet.
 **/
LRESULT CWindow::processMessage(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	// Das Fenster wurde geschlossen. Damit soll auch die Anwendung selbst beendet werden. 
	case WM_DESTROY:PostQuitMessage(0);
		break;

	// Das Fenster wurde frisch erstellt.
	case WM_CREATE:
		{
			RECT r;
			// Hole Groesse des Clientbereichs (Fenster ohne Rahmen/Titelleiste)
			GetClientRect(hWnd,&r);
			// Informiere OpenGL ueber den verfuegbaren Platz zum Rendern
			reshape(r.right,r.bottom);
		}
		break;

	// Die linke Maustaste wurde gedrueckt
	case WM_LBUTTONDOWN:
		{
			// Baue ein passendes TMouseEvent, pressed und button1, Position steht in LOWORD(lParam) und HIWORD(lParam)
			TMouseEvent me={true,false,false, true,false,false,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			// Wir wollen exklusive Nachrichten der Maus, solange die Taste gedrueckt wurde
			SetCapture(hWnd);
			// Nachricht an GUI weiterreichen
			gui->processMouseEvent(&me);
		}
		break;
	case WM_RBUTTONDOWN:
		{
			TMouseEvent me={true,false,false, false,true,false,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			SetCapture(hWnd);
			gui->processMouseEvent(&me);	
		}
		break;
	case WM_MBUTTONDOWN:
		{
			TMouseEvent me={true,false,false, false,false,true,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			SetCapture(hWnd);
			gui->processMouseEvent(&me);	
		}
		break;
	// Das Mausrad wurde bewegt
	case WM_MOUSEWHEEL:
		{
			// Die Koordinaten liegen seltsamerweise in Screen-Koordinaten vor
			POINT p={LOWORD(lParam),HIWORD(lParam)};
			// Konvertiere Screen in Client-Koordinaten, also relativ zum Fenster
			ScreenToClient(hWnd,&p);
			// Stricke wieder ein TMouseEvent
			TMouseEvent me={true,false,false, false,false,false,false,false,p.x,p.y};
			// Das delta des Mausrads gibt die Richtung an, button4 ist hoch, button5 ist runter
			me.button1=(LOWORD(wParam)&MK_LBUTTON)!=0;
			me.button2=(LOWORD(wParam)&MK_RBUTTON)!=0;
			me.button3=(LOWORD(wParam)&MK_MBUTTON)!=0;
			if ((short)HIWORD(wParam)>0)me.button4=true;
			if ((short)HIWORD(wParam)<0)me.button5=true;
			// Und Nachricht an GUI uebergeben
			gui->processMouseEvent(&me);	
			break;
		}

	// Die Maustaste wurde losgelassen
	case WM_LBUTTONUP:
		{
			// TMouseEvent fuer das Ereignis bauen
			TMouseEvent me={false,true,false, true,false,false,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			// Ereignis an GUI weitergeben
			gui->processMouseEvent(&me);
			// Maus unbedingt wieder freigeben, die wir zuvor mit SetCapture() uebernommen haben,
			// Damit Mausereignisse wieder andere Fenster erreichen koennen. 
			ReleaseCapture();
		}
		break;
	case WM_RBUTTONUP:
		{
			TMouseEvent me={false,true,false, false,true,false,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			gui->processMouseEvent(&me);
			ReleaseCapture();
		}
		break;
	case WM_MBUTTONUP:
		{
			TMouseEvent me={false,true,false, false,false,true,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			gui->processMouseEvent(&me);
			ReleaseCapture();
		}
		break;
	
	// Die Maus wurde bewegt.
	case WM_MOUSEMOVE:
		{
			// Ein TMouseEvent dazu erstellen
			TMouseEvent me;
			me.move=true;	// Maus wurde bewegt
			me.press=me.release=false;	// Und nicht gedrueckt, oder losgelassen
			me.button1=(wParam&MK_LBUTTON)!=0;	// Ist dabei LBUTTON gedrueckt?
			me.button2=(wParam&MK_RBUTTON)!=0;	// ...
			me.button3=(wParam&MK_MBUTTON)!=0;	// ...
			me.button4=me.button5=false;	// "Mausrad" ist generell nicht bewegt
			me.x=(short)LOWORD(lParam);	// Koordinaten setzen
			me.y=(short)HIWORD(lParam);	// ...
			// Ereignis weiterreichen
			gui->processMouseEvent(&me);
		}
		break;

	// Obige TranslateMessage hat uns ein WM_CHAR generiert, das ein char in wParam enthaelt
	case WM_CHAR:
		// Die gedrueckte Taste an die GUI weiterreichen
		gui->processKeyEvent(wParam);
		break;

	// Manche originale Sondertasten an GUI uebergeben
	case WM_KEYDOWN:
		if (wParam==VK_UP || wParam==VK_DOWN || wParam==VK_LEFT || wParam==VK_RIGHT || wParam==VK_F1)
			gui->processKeyEvent(wParam);
		break;

	// Das Fenster wurde in der Groesse geaendert. 
	case WM_SIZE:
		// Wir sollten OpenGL von der neuen Groesse in Kenntnis setzen. 
		reshape(LOWORD(lParam),HIWORD(lParam));
		break;
	
	// Fuer alle nicht behandelten Nachrichten bietet Windows eine Standardimplementierung:
	default: return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	// 0 zurueckgeben, als Zeichen, dass wir die Nachricht behandelt haben.
	return 0;
}

/**
 * Win32: Fenster sichtbar machen
 **/
void CWindow::show()
{
	// Fenster im Zustand Maximiert anzeigen
	ShowWindow(hWnd,SW_SHOWMAXIMIZED);
	UpdateWindow(hWnd);
}

/**
 * OpenGL-Puffer des Fensters tauschen, um die letzten Renderings sichtbar zu machen
 **/
void CWindow::swapBuffers()
{
	SwapBuffers(hDC);
}

#else

/**
 * X11-Implementierung von createWindow()
 * Erstellt ein X11-Fenster samt OpenGL-Rattenschwanz
 **/
bool CWindow::createWindow()
{
	/* Unsere Forderungen. Wir wollen mindestens:
	   RGBA, 1 Bit von jeder Farbe, DoubleBuffer, Z-Buffer und Stencil-Buffer */
	int attrib[] = { GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 1,
		None };
	int scrnum;	// DefaultScreen
	XSetWindowAttributes attr;	// Attribute des zu erstellenden Fensters. 
	unsigned long mask;
	Window root,wnd;
	XVisualInfo *visinfo;

	// Das Standard-Display oeffnen (aus $DISPLAY Variable der Shell)
	dpy=XOpenDisplay(nullptr);
	if (dpy==0)
	{
		printf("Error: could not open display\n");
		return false;	// Bei Fehler, stopp.
	}

	// Standard-Screen des Displays holen
	scrnum = DefaultScreen( dpy );
	root = RootWindow( dpy, scrnum );	// Sowie das root-Fenster des Screens (Desktop)

	// Wir wollen ein OpenGL-Visual mit den obigen Anforderungen
	visinfo = glXChooseVisual( dpy, scrnum, attrib );
	// Kein Visual gefunden? Dann Fehler. 
	if (!visinfo) {
		printf("Error: couldn't get an RGB, Double-buffered visual\n");
		return false;
	}

	// Fenster Attribute waehlen
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	// X11-Fenster erstellen, mit angegebener Fenstergroesse, Position wird durch Fenstermanager gewaehlt
	wnd= XCreateWindow( dpy, root, 0, 0, WND_SIZE_X, WND_SIZE_Y,
		0, visinfo->depth, InputOutput,
		visinfo->visual, mask, &attr );
	// Bei Fehler, raus.
	if (wnd==0)
	{
		printf("Error: couldn't create window\n");
		return false;
	}

	// Wir wollten einen Stencil Buffer, sind wir hier, haben wir ihn auch gekriegt, also koennen wir Schatten rendern
	m_canShadow=true;

	// Einige Eigenschaften des Fensters setzen
	{
		XSizeHints sizehints;
		sizehints.x = 0;
		sizehints.y = 0;
		sizehints.width  = WND_SIZE_X;
		sizehints.height = WND_SIZE_Y;
		sizehints.flags = USSize /*| USPosition*/;
		XSetNormalHints(dpy, wnd, &sizehints);
		// Setze hints und Titel des Fensters
		XSetStandardProperties(dpy, wnd, WINDOW_TITLE, WINDOW_TITLE,None, (char **)nullptr, 0, &sizehints);
	}

#ifdef HAVE_X11_XPM_H
	// Fenster ein Icon verpassen
	{
		#include "freebloks.xpm"
  		XpmImage I;
  		int ret;
		Pixmap p,s;
		XWMHints hints;
		XWindowAttributes root_attr;
		XpmAttributes attributes;

  		ret=XpmCreateXpmImageFromData(freebloks_xpm,&I,nullptr);

		if (ret==0)
		{
	  		XGetWindowAttributes (dpy, root,&root_attr);
  			attributes.colormap = root_attr.colormap;
  			attributes.valuemask = XpmCloseness | XpmColormap;
  			attributes.closeness = 40000;

  			ret=XpmCreatePixmapFromXpmImage (dpy, root, &I, &p, &s, &attributes);
  			XpmFreeXpmImage(&I);

    			hints.flags=IconPixmapHint|IconMaskHint;
    			hints.icon_pixmap=p;
    			hints.icon_mask=s;
    			if (ret==0)XSetWMHints(dpy,wnd,&hints);
		}
	}
#endif

	// Erstelle einen OpenGL-Kontext, mit dem vorher gewaehltem visual
	ctx = glXCreateContext( dpy, visinfo, nullptr, True );
	if (!ctx) {	// Bei Fehlschlag raus
		printf("Error: glXCreateContext failed\n");
		return false;
	}

	// Den Speicher fuer das visual muessen wir selbst wieder freigeben
	XFree(visinfo);

	// Fenster merken
	window=wnd;
	closed=false;

	glXMakeCurrent(dpy,window,ctx);
	// OpenGL ueber die Fenstergroesse informieren
	reshape(WND_SIZE_X,WND_SIZE_Y);

	// Erfolg!!
	return true;
}

/**
 * Fenster sichtbar machen
 **/
void CWindow::show()
{
	// Macht Fenster auf dem Display sichtbar
	XMapWindow(dpy, window);
	// Aktiviert den OpenGL Kontext, er ist fuer die Instanz global
	glXMakeCurrent(dpy,window,ctx);
}

/**
 * Nachrichtenschleife, verarbeitet alle X11-Events, solang welche vorhanden sind.
 */
void CWindow::processEvents()
{
	// Solange XEvents vorhanden sind,
	while (XPending(dpy) > 0) 
	{
		XEvent event;
		// Hole naechstes X11-Event ab
		XNextEvent(dpy, &event);
		// Und verarbeite es, ueber eigene Nachrichtenfunktion
		processEvent(event);
	}
}

/**
 * Eigene X11 Nachrichtenfunktion. Verarbeitet ein XEvent und leitet so manche an die GUI weiter
 **/
void CWindow::processEvent(XEvent event)
{
	switch (event.type) {
	// Das Fenster wurde frisch erstellt. Informiere OpenGL ueber die Groesse
	case ConfigureNotify:
 		reshape(event.xconfigure.width, event.xconfigure.height);
      		break;

	// Eine Taste wurde gedrueckt
	case KeyPress: {
		KeySym mykeysym;
		char buffer[16];
		// Verwandle den Keycode in ein char (nur ein Byte, koennten ja auch mehrere sein)
		XLookupString((XKeyEvent*)&event, buffer,15, &mykeysym, nullptr);
		gui->processKeyEvent(mykeysym);
		break;
		}

	// Es wurde eine Maustaste gedrueckt
	case ButtonPress:
		{
			// Transferriere die Daten aus dem XButtonEvent an ein TMouseEvent
			XButtonEvent* e=(XButtonEvent*)(&event);
			TMouseEvent me;
			me.press=true;
			me.move=me.release=false;
			me.x=e->x;
			me.y=e->y;
			me.button1=(e->button==Button1)||(e->state & Button1Mask);
			me.button2=(e->button==Button3)||(e->state & Button3Mask);
			me.button3=(e->button==Button2)||(e->state & Button2Mask);
			me.button4=(e->button==Button4);
			me.button5=(e->button==Button5);

			// Und leite es an die GUI weiter
			// X11 gibt uns automatisch alle weiteren MouseEvents, solang die Taste noch gedrueckt ist
			gui->processMouseEvent(&me);
		}
		break;
	// Eine Maustaste wuede losgelassen.
	case ButtonRelease:
		{
			// Wandle das XButtonEvent in ein TMouseEvent
			XButtonEvent* e=(XButtonEvent*)(&event);
			TMouseEvent me;
			me.release=true;
			me.move=me.press=false;
			me.x=e->x;
			me.y=e->y;
			me.button1=e->button==Button1;
			me.button2=e->button==Button3;
			me.button3=e->button==Button2;
			me.button4=e->button==Button4;
			me.button5=e->button==Button5;
			// Und uebergib es an die GUI
			gui->processMouseEvent(&me);
		}
		break;

	// Die Maus wurde bewegt
	case MotionNotify:
		{
			// Wandle das XMotionEvent in ein TMouseEvent
			XMotionEvent *e=(XMotionEvent*)(&event);
			TMouseEvent me;
			me.move=true;
			me.press=me.release=false;
			me.x=e->x;
			me.y=e->y;
			me.button1=e->state & Button1Mask;
			me.button2=e->state & Button3Mask;
			me.button3=e->state & Button2Mask;
			me.button4=e->state & Button4Mask;
			me.button5=e->state & Button5Mask;
			// Und gib es an die GUI weiter
			gui->processMouseEvent(&me);
		}
		break;
	}
}

/**
 * Vertausche OpenGL-Buffer, und mach Renderings von vorher sichtbar
 **/
void CWindow::swapBuffers()
{
	glXSwapBuffers(dpy, window);
}

#endif

