/**
 * widgets.h
 * Author: Sascha Hlusiak
 * Classes of the engine for the 2D menus, called "widgets" here
 **/

#ifndef __WIDGETS_H_INCLUDED_
#define __WIDGETS_H_INCLUDED_

#include "window.h"
#include "network.h"

class CGameClient;
struct TOptions;


/**
 * Widget, the main class from which all other widgets are derived
 * Provides basic virtual methods without really doing much
 **/
class CWidget
{
private:
	/* next:  widgets that lie parallel to this
	   child: a widget found inside this
	   subchild: a widget that lies outside this and has higher priority */
	CWidget *next,*child,*subchild;
	// Flag whether the widget is to be removed. It is removed in the calling execute
	bool m_deleted;
	// The ID of the widget (should be unique).
	// the ID only matters for mouse operations, so the GUI can find the corresponding
	// widget again
	int m_id;
protected:
	// Coordinates of the widget; not important except for the selection buffer,
	// and since the parameters are there anyway...
	double x,y,w,h;

	// Called when the mouse moves inside the widget
	virtual void mouseEnter() {}
	// Called when the mouse moves outside the widget
	virtual void mouseLeave() {}
public:
	// Constructor
	CWidget(); 
	// Destructor
	virtual ~CWidget();

	// Append a new widget parallel to this in a list
	void add(CWidget* w) { if (next) next->add(w); else next=w; }
	// Attach a child widget as a direct child of this. It becomes a next of any existing child widgets
	void addChild(CWidget *w) { if (child)child->add(w); else child=w; }
	// Adds a subwindow; it gets messages somewhat more exclusively and is given priority, as if it had the focus
	void addSubChild(CWidget *subchild);
	
	bool deleted() { return m_deleted; }
	bool hasSubChild() { return (subchild!=0); }
	CWidget *getSubChild() { return subchild; }

	// If this returns true, the GUI skips its own mouse handling on the board (useful for modal dialogs)
	virtual bool handleUI() { bool r=false; if (next)r|=next->handleUI(); if (subchild)r|=subchild->handleUI(); if (child)r|=child->handleUI(); return r; }

	// Close the widget by simply removing it. Can be overridden to allow animations or similar
	virtual void close() { remove(); }

	// Called by the GUI with the ID of the widget currently under the mouse pointer (or 0 for no widget). This is the only way to find out which widget the mouse pointer is over.
	void processSelection(int id);

	// Called by the GUI when a mouse event occurred (e.g. a click)
	// The return value can be the ID of a widget; the caller learns
	// whether the widget wants attention or not (button click, check box click or similar)
	// >> This is how widgets report events to their parent widgets! <<
	virtual int processMouseEvent(TMouseEvent *event);
	// Called by the GUI when a key was pressed that wants to be handled. Returns true if the widget handled the key; on true NO other widget should process the key any further, since that already happened.
	virtual bool processKey(unsigned short key);

	void remove() { m_deleted=true; }
	// Return the ID of this widget
	const int getId()const { return m_id; }
	double getX() { return x; }
	double getY() { return y; }
	void setId(int id) { m_id=id; }

	// Animations and other automatic actions go here
	virtual void execute(double elapsed);
	// Render the widget; selection is true when rendering into the selection buffer
	virtual void render(bool selection);
	// Render the subchilds; they must always be rendered after the actual widget.
	void renderSubChild(bool selection);
};


/**
 * This class is used by the GUI as the widget container.
 **/
class CWidgetPane:public CWidget
{
public:
	// Prepares OpenGL for rendering the 2D widgets, then renders the actual widgets of the linked lists
	void render(bool selectionBuffer);
	virtual int processMouseEvent(TMouseEvent *event);
};

/**
 * A simple rectangle, 4 corners, transparent, with a colour gradient
 **/
class CFrame:public CWidget
{
private:
	double r,g,b,a;
public:
	// Hm...
	CFrame(double vx,double vy,double vw,double vh);
	// Unbelievable
	virtual void render(bool selection);
	// Whoever knows gets a biscuit
	void setColor(double vr,double vg,double vb) { r=vr; g=vg; b=vb; }
	// Yes, er...
	void setAlpha(double va) { a=va; }
	// Set the size
	void setSize(double vx,double vy,double vw,double vh);
};

class CButton:public CWidget
{
private:
	// Is the button highlighted (mouse over), is it enabled?
	bool hovered,enabled;
	// Animation of the mouse-over effect (0.0-1.0), and animation of the click
	double anim,pushanim;
	char *text;
protected:
	// The mouse moves over the widget, so hover=enabled
	virtual void mouseEnter() { hovered=enabled && true; }
	// The mouse moves outside the widget; the button is no longer hovered in any case
	virtual void mouseLeave() { hovered=false; }
public:
	// Constructor
	CButton(double vx,double vy,double vw,double vh,int id,CWidget* parent,const char *vtext);
	virtual ~CButton();

	virtual void render(bool selection);
	virtual void execute(double elapsed);
	// Handle a mouse event. If pressed and hovered, the ID of the button is returned
	virtual int processMouseEvent(TMouseEvent *event);
	// Enable / disable the button
	void setEnabled(bool enabled) { this->enabled=enabled; }
	bool isEnabled() { return enabled; }
};

/**
 * A check box; can be on or off, and can be organised in check box groups
 **/
class CCheckBox:public CWidget
{
private:
	// The mouse can be over the check box, the check box can be disabled, and it can be on or off (checked)
	bool hovered,enabled,checked;
	// Label
	char *text;
	// Animation when the mouse presses on it
	double pushanim;
protected:
	// The mouse is over the widget, so hovered=enabled
	virtual void mouseEnter() { hovered=enabled && true; }
	// The mouse is outside the widget; hovered is always false
	virtual void mouseLeave() { hovered=false; }
public:
	// For check box groups: pointers to the next and previous check box, or nullptr at the end
	CCheckBox *nextCheckBox,*prevCheckBox;

	// Constructor
	CCheckBox(double vx,double vy,double vw,double vh,int id,const char *vtext,bool vchecked,CWidget *parent);
	virtual ~CCheckBox();
	
	// Render
	virtual void render(bool selection);
	// Animate
	virtual void execute(double elapsed);
	// Handle a mouse event; returns the ID if the check box was clicked
	virtual int processMouseEvent(TMouseEvent *event);
	bool getCheck() { return checked; }
	// Set checked
	void setCheck(bool vchecked) { checked=vchecked; }
	// Enable or disable the check box; when disabled it no longer accepts mouse events
	void setEnabled(bool enabled) { this->enabled=enabled; }
	// Add another check box to the group. Behaves like a radio group
	void addCheckBox(CCheckBox *n);
};

/**
 * Just static text that is rendered. Can be centred; the text can be changed
 * afterwards.
 **/
class CStaticText:public CWidget
{
private:
	char *text;
	double a;
public:
	// Constructor; the text is left-aligned at x and y, relative to the coordinates of parent
	CStaticText(double x,double y,const char *vtext,CWidget *parent);
	// Constructor; the text is centred at x with width w, relative to the coordinates of parent
	CStaticText(double x,double y,double w,const char *vtext,CWidget *parent);
	// Clean up
	virtual ~CStaticText();
	// Set the text, freeing the old text if needed. adjustwidth: should the width adapt to the new text?
	void setText(const char *vtext,bool adjustwidth=false);
	virtual void render(bool selection);
	// Set the transparency
	void setAlpha(double va) { a=va; }
};


/**
 * A text edit field. Text can be entered, optionally restricted to digits
 **/
class CTextEdit:public CFrame
{
protected:
	// Is the mouse over the edit field, does the field accept keyboard input, are ONLY digits allowed, or everything?
	bool hovered,focused,numbers;
	// State of the focus animation, and of the animation of an entered character
	double animatefocused,animatechar;
	// true if the user pressed Enter.
	bool m_committed;
	// The character removed with Backspace, so it can be removed with an animation
	char lastchar;
	// Buffer of the entered characters. 100 should be enough; nobody types novels here
	char text[100];
	// Should the text be left-aligned in the edit field?
	bool left_align;
	virtual void mouseEnter() { hovered=true; }
	virtual void mouseLeave() { hovered=false; }
	// Called when the TextEdit loses focus; checks could go here (as in SpinBox)
	virtual void endEdit() {}
public:
	// Constructor, self-explanatory I hope
	CTextEdit(double x,double y,double w,double h,int id,const char *vtext,bool numbers_only,CWidget *parent);
	// Clean up
	virtual ~CTextEdit();

	// The text field should have focus; can be called from outside when initialising dialogs
	void setFocus() { focused=true; }
	bool hasFocus() { return focused; }
	// Returns the entered text. The pointer should be copied and not modified
	const char *getText();
	void setText(const char *vtext);
	bool committed() { return m_committed; }
	void unsetCommitted() { m_committed=false; }

	// Animate
	virtual void execute(double elapsed);
	// Render
	virtual void render(bool selection);
	// Handle a mouse event
	virtual int processMouseEvent(TMouseEvent *event);
	// Handle a key press.
	virtual bool processKey(unsigned short key);
};

/**
 * A spin box: a TextEdit that only accepts digits and
 * has two buttons for the arrows as "children" :)
 **/
class CSpinBox:public CTextEdit
{
protected:
	// Valid range of the value. The current value is stored as text in the TextEdit
	int min,max,def;
	// Buttons for + and -, so they can be disabled when the limit is reached
	CButton *up,*down;
	
	// When editing is finished, the value should be validated
	virtual void endEdit();
public:
	CSpinBox(double x,double y,double w,double h,int id,int min,int max,int value,CWidget *parent,bool rightButtons=true);

	// Return the current value as a number
	int getValue();
	// Set the value
	void setValue(int value);

	// Set the limits
	void setMin(int min) { this->min=min; endEdit(); }
	void setMax(int max) { this->max=max; endEdit(); }

	// Handle a mouse event
	virtual int processMouseEvent(TMouseEvent *event);
};

/**
 * A dialog. Extends CFrame with a fancy animation when created/removed,
 * and a title bar text
 **/

class CDialog:public CFrame
{
private:
	// Animation when sliding in or out
	double anim;
	// Is the dialog currently fading in or fading out?
	bool starting;
	// Title bar text
	char *caption;
public:
	// Create a dialog of width vw and height vh. The dialog is centred exactly in the window and gets the title bar "vcaption"
	CDialog(double vw,double vh,const char *vcaption);
	// Clean up
	virtual ~CDialog();

	// Render the dialog and its subwindows, computing the animation first if needed
	virtual void render(bool selection);
	// Animate
	virtual void execute(double elapsed);
	// Handle a mouse button
	virtual int processMouseEvent(TMouseEvent *event);
	// Handle a key
	virtual bool processKey(unsigned short key);
	// While any dialog is open, the GUI must not handle mouse events
	virtual bool handleUI() { return starting; }
	// Extra close() functions to give the dialog an animated demise
	virtual void close();
	virtual void close(bool recursive);

	// Show the dialog fully, end the animation.
	void finishAnim() { anim=1.0; }

	// Set a new title bar text
	void setCaption(const char* caption);

	// Set the window size
	void setSize(double vw,double vh);
};

/**
 * A message box; shows an error message in a red window with an Okay button
 **/
class CMessageBox:public CDialog
{
public:
	// Message window; the colour can be defined and is red by default
	CMessageBox(double w,double h,const char *caption,const char *text,double r=1.0,double g=0.2,double b=0.1);
};


/**
 * A counter that counts the number of execute() calls and computes an FPS value from it every 500 ms.
 **/
class CFPS:public CStaticText
{
private:
	int counter;
	/* And the time for which frames have been rendered. */
	double time;
	TOptions *options;
public:
	CFPS(double x,double y,CGUI* gui);
	/* execute. */
	virtual void execute(double elapsed);
	virtual void render(bool selectionBuffer);
};


#endif
