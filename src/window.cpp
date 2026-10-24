/**
 * window.cpp
 * Author: Sascha Hlusiak
 *
 * Wraps a window, either with the WIN32 API on Windows, or X11 on Linux
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
// Windows message for the mouse wheel. Defined here for simplicity so it also compiles with old
// Win-SDKs.
    #define WM_MOUSEWHEEL                   0x020A
  #endif
#else
  #ifdef HAVE_X11_XPM_H
    #include <X11/xpm.h>
  #endif
#endif

#include "window.h"
#include "gui.h"

static const char* WINDOW_TITLE="Freebloks 3D";
// Initial size of the window (on Windows the window is also maximised)
static const int WND_SIZE_X=800;
static const int WND_SIZE_Y=600;

#ifdef WIN32
// The HINSTANCE of the program. Set in WinMain() in freebloks.cpp.
HINSTANCE hInstance=0;

/**
 * The actual window procedure of the main window. USERDATA holds a pointer
 * to the CWindow instance, so the window's processMessage can be
 * called
 **/
LRESULT CALLBACK WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CWindow* wnd=(CWindow*)GetWindowLong(hWnd,GWL_USERDATA);
	if (wnd)return wnd->processMessage(hWnd,uMsg,wParam,lParam);
		else return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

#endif


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
 * Destructor: if a window was created before,
 * it is finally removed cleanly here
 **/
CWindow::~CWindow()
{
#ifdef WIN32
	wglMakeCurrent(0,0);
	if (hDC)ReleaseDC(hWnd,hDC);
	if (hWnd)DestroyWindow(hWnd);
#else
	if (ctx)glXMakeCurrent(dpy,0,0);
	if (ctx)glXDestroyContext(dpy,ctx);
	ctx=0;
	if (window)
	{
		XUnmapWindow(dpy,window);
		XDestroyWindow(dpy,window);
	}
	if (dpy)XCloseDisplay(dpy);
	dpy=nullptr;
#endif
}

/**
 * Called to set the window's OpenGL viewport to the right size
 * Parameters: size of the window's available area (client area)
 **/
void CWindow::reshape(int width,int height)
{
	// Always keep the viewport at 4:3
	int w=width,h=height;
// 	if ((w*3)/4 > height)w=(height*4)/3;
// 	else h=(width*3)/4;

	// Set the viewport of width w and height h and centre it in the window
	glViewport((width-w)/2, (height-h)/2, (GLint) w, (GLint) h);
}



#ifdef WIN32

/**
 * WIN32 implementation of createWindow(): create a window with all the OpenGL trimmings
 **/
bool CWindow::createWindow()
{
	// First the window class has to be registered
	WNDCLASS wndclass;
	ZeroMemory(&wndclass,sizeof(wndclass));
	wndclass.style=CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc=WndProc;
	wndclass.hInstance=::hInstance;
	wndclass.hCursor=LoadCursor(nullptr,IDC_ARROW);
	wndclass.hIcon=LoadIcon(::hInstance,MAKEINTRESOURCE(IDI_ICON));
	wndclass.lpszClassName="FreebloksWindowClass";
	if (!RegisterClass(&wndclass))return false;

	// Create a Win32 window, default position but given size. Nothing special.
	hWnd=CreateWindow("FreebloksWindowClass",WINDOW_TITLE,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,CW_USEDEFAULT,CW_USEDEFAULT,WND_SIZE_X,WND_SIZE_Y,nullptr,nullptr,::hInstance,nullptr);
	if (hWnd==0)return false;

	// USERDATA of the window must be associated with this here, so the global window procedure
	// can connect the window with this class and call our processMessage().
	SetWindowLong(hWnd,GWL_USERDATA,(unsigned long)this);

	// Choose a suitable pixel format, assign the hDC and create the OpenGL context
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormat;
	m_canShadow=true;

	hDC=GetDC(hWnd);

	ZeroMemory(&pfd,sizeof(pfd));
	pfd.nSize=sizeof(pfd);
	pfd.nVersion=1;
	// We want to render into the window, with OpenGL, and double buffering,
	// so we can render into the back buffer
	pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
	// First request at least 1 bit of stencil buffer.
	pfd.cStencilBits=1;
	// Choose a suitable pixel format.
	iPixelFormat=ChoosePixelFormat(hDC,&pfd);

	if (iPixelFormat==0)
	{	// If no pixel format was found, try the same again without stencil bits.
		pfd.cStencilBits=0;
		iPixelFormat=ChoosePixelFormat(hDC,&pfd);
		// In any case we can no longer do shadows
		m_canShadow=false;
	}
	if (iPixelFormat==0)return false;

	SetPixelFormat(hDC,iPixelFormat,&pfd);

	// And connect an OpenGL context to the hDC and thus to the hWnd (window).
	ctx=wglCreateContext(hDC);
	if (ctx==0)return false;

	// Activate the context; it applies globally to the whole application.
	wglMakeCurrent(hDC,ctx);

	closed=false;

	// SUCCESS!
	return true;
}

/**
 * Message loop for Win32: processes all pending messages and then returns
 **/
void CWindow::processEvents()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr,0,0,PM_REMOVE))
	{
		// WM_QUIT is posted by the window's WM_DESTROY. The window is then considered closed
		if (msg.message==WM_QUIT)
			closed=true;
		else {
			// Translate any WM_KEYDOWN messages into WM_CHAR. Important for keyboard input
			TranslateMessage(&msg);
			// Process the message. Usually calls the window's window procedure.
			DispatchMessage(&msg);
		}
	}
}

/**
 * The window procedure of the window. Called by the global window procedure, which is called by
 * DispatchMessage. All window messages are processed here and forwarded to the GUI
 * if needed.
 **/
LRESULT CWindow::processMessage(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	// The window was closed. This also ends the application itself.
	case WM_DESTROY:PostQuitMessage(0);
		break;

	case WM_CREATE:
		{
			RECT r;
			// Get the size of the client area (window without border/title bar)
			GetClientRect(hWnd,&r);
			reshape(r.right,r.bottom);
		}
		break;

	case WM_LBUTTONDOWN:
		{
			// Build a matching TMouseEvent, pressed and button1; the position is in LOWORD(lParam) and HIWORD(lParam)
			TMouseEvent me={true,false,false, true,false,false,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			// We want the mouse messages exclusively while the button is pressed
			SetCapture(hWnd);
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
	case WM_MOUSEWHEEL:
		{
			// Oddly, the coordinates are in screen coordinates
			POINT p={LOWORD(lParam),HIWORD(lParam)};
			// Convert screen to client coordinates, i.e. relative to the window
			ScreenToClient(hWnd,&p);
			// Knit a TMouseEvent again
			TMouseEvent me={true,false,false, false,false,false,false,false,p.x,p.y};
			// The delta of the mouse wheel gives the direction: button4 is up, button5 is down
			me.button1=(LOWORD(wParam)&MK_LBUTTON)!=0;
			me.button2=(LOWORD(wParam)&MK_RBUTTON)!=0;
			me.button3=(LOWORD(wParam)&MK_MBUTTON)!=0;
			if ((short)HIWORD(wParam)>0)me.button4=true;
			if ((short)HIWORD(wParam)<0)me.button5=true;
			gui->processMouseEvent(&me);	
			break;
		}

	case WM_LBUTTONUP:
		{
			TMouseEvent me={false,true,false, true,false,false,false,false,(short)LOWORD(lParam),(short)HIWORD(lParam)};
			// Pass the event on to the GUI
			gui->processMouseEvent(&me);
			// Be sure to release the mouse we captured with SetCapture() before,
			// so mouse events can reach other windows again.
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
	
	case WM_MOUSEMOVE:
		{
			TMouseEvent me;
			me.move=true;
			me.press=me.release=false;
			me.button1=(wParam&MK_LBUTTON)!=0;
			me.button2=(wParam&MK_RBUTTON)!=0;	// ...
			me.button3=(wParam&MK_MBUTTON)!=0;	// ...
			me.button4=me.button5=false;	// The "mouse wheel" is generally not moved
			me.x=(short)LOWORD(lParam);	// Set the coordinates
			me.y=(short)HIWORD(lParam);	// ...
			// Pass the event on
			gui->processMouseEvent(&me);
		}
		break;

	// The TranslateMessage above generated a WM_CHAR for us, which holds a char in wParam
	case WM_CHAR:
		gui->processKeyEvent(wParam);
		break;

	// Pass some original special keys to the GUI
	case WM_KEYDOWN:
		if (wParam==VK_UP || wParam==VK_DOWN || wParam==VK_LEFT || wParam==VK_RIGHT || wParam==VK_F1)
			gui->processKeyEvent(wParam);
		break;

	case WM_SIZE:
		reshape(LOWORD(lParam),HIWORD(lParam));
		break;
	
	default: return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	// Return 0 to indicate that we handled the message.
	return 0;
}

/**
 * Win32: make the window visible
 **/
void CWindow::show()
{
	// Show the window maximised
	ShowWindow(hWnd,SW_SHOWMAXIMIZED);
	UpdateWindow(hWnd);
}

/**
 * Swap the window's OpenGL buffers to make the latest rendering visible
 **/
void CWindow::swapBuffers()
{
	SwapBuffers(hDC);
}

#else

/**
 * X11 implementation of createWindow()
 * Creates an X11 window with all the OpenGL baggage
 **/
bool CWindow::createWindow()
{
	/* Our requirements. We want at least:
	   RGBA, 1 bit of each colour, double buffering, Z buffer and stencil buffer */
	int attrib[] = { GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 1,
		None };
	int scrnum;	// DefaultScreen
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root,wnd;
	XVisualInfo *visinfo;

	// Open the default display (from the shell's $DISPLAY variable)
	dpy=XOpenDisplay(nullptr);
	if (dpy==0)
	{
		printf("Error: could not open display\n");
		return false;
	}

	scrnum = DefaultScreen( dpy );
	root = RootWindow( dpy, scrnum );

	// We want an OpenGL visual with the requirements above
	visinfo = glXChooseVisual( dpy, scrnum, attrib );
	if (!visinfo) {
		printf("Error: couldn't get an RGB, Double-buffered visual\n");
		XCloseDisplay(dpy);
		dpy=nullptr;
		return false;
	}

	// Choose the window attributes
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	// Create the X11 window with the given window size; the position is chosen by the window manager
	wnd= XCreateWindow( dpy, root, 0, 0, WND_SIZE_X, WND_SIZE_Y,
		0, visinfo->depth, InputOutput,
		visinfo->visual, mask, &attr );
	if (wnd==0)
	{
		printf("Error: couldn't create window\n");
		XFree(visinfo);
		XCloseDisplay(dpy);
		dpy=nullptr;
		return false;
	}

	// We asked for a stencil buffer; if we got here we got it, so we can render shadows
	m_canShadow=true;

	{
		XSizeHints sizehints;
		sizehints.x = 0;
		sizehints.y = 0;
		sizehints.width  = WND_SIZE_X;
		sizehints.height = WND_SIZE_Y;
		sizehints.flags = USSize /*| USPosition*/;
		XSetNormalHints(dpy, wnd, &sizehints);
		XSetStandardProperties(dpy, wnd, WINDOW_TITLE, WINDOW_TITLE,None, (char **)nullptr, 0, &sizehints);
	}

#ifdef HAVE_X11_XPM_H
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

	ctx = glXCreateContext( dpy, visinfo, nullptr, True );
	if (!ctx) {
		printf("Error: glXCreateContext failed\n");
		XFree(visinfo);
		XDestroyWindow(dpy,wnd);
		XCloseDisplay(dpy);
		dpy=nullptr;
		return false;
	}

	// We have to free the memory of the visual ourselves
	XFree(visinfo);

	// Remember the window
	window=wnd;
	closed=false;

	glXMakeCurrent(dpy,window,ctx);
	reshape(WND_SIZE_X,WND_SIZE_Y);

	// Success!!
	return true;
}

/**
 * Make the window visible
 **/
void CWindow::show()
{
	XMapWindow(dpy, window);
	// Activates the OpenGL context; it is global for the instance
	glXMakeCurrent(dpy,window,ctx);
}

/**
 * Message loop: processes all X11 events as long as there are any.
 */
void CWindow::processEvents()
{
	while (XPending(dpy) > 0) 
	{
		XEvent event;
		// Fetch the next X11 event
		XNextEvent(dpy, &event);
		processEvent(event);
	}
}

/**
 * Our own X11 message function. Processes an XEvent and forwards some of them to the GUI
 **/
void CWindow::processEvent(XEvent event)
{
	switch (event.type) {
	// The window was just created. Tell OpenGL about its size
	case ConfigureNotify:
 		reshape(event.xconfigure.width, event.xconfigure.height);
      		break;

	case KeyPress: {
		KeySym mykeysym;
		char buffer[16];
		// Convert the keycode into a char (only one byte, though there could be several)
		XLookupString((XKeyEvent*)&event, buffer,15, &mykeysym, nullptr);
		/* processKeyEvent takes 16 bits; truncating a larger keysym (e.g. 0x100ff0d for a Unicode
		   character) would turn it into an unrelated key like Return. */
		if (mykeysym > 0xFFFF)break;
		gui->processKeyEvent(mykeysym);
		break;
		}

	case ButtonPress:
		{
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

			// X11 automatically gives us all further mouse events while the button is still held
			gui->processMouseEvent(&me);
		}
		break;
	case ButtonRelease:
		{
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
			gui->processMouseEvent(&me);
		}
		break;

	case MotionNotify:
		{
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
			gui->processMouseEvent(&me);
		}
		break;
	}
}

void CWindow::swapBuffers()
{
	glXSwapBuffers(dpy, window);
}

#endif

