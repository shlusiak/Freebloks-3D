/**
 * widgets.cpp
 * Author: Sascha Hlusiak
 *
 * 2D engine for the menus: classes for buttons, check boxes, dialogs, etc.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "widgets.h"
#include "window.h"
#include "glfont.h"
#include "constants.h"
#include "gameclient.h"
#include "gui.h"
#include "options.h"


/**
 * Initialises a CWidget to default values. It (theoretically) occupies the whole
 * screen
 **/
CWidget::CWidget()
{
	next=child=subchild=0; 
	/* Occupies the whole screen for now. Does no harm unless an
	   m_id is set. */
	x=y=0.0;
	w=640.0;
	h=480.0;
	m_id=0;
	/* And the widget is alive and does not want to be removed for now. */
	m_deleted=false;
}

/**
 * Tear down a CWidget.
 **/
CWidget::~CWidget()
{
	/* All following widgets of the linked lists are removed recursively too! */
	if (child)delete child;
	if (subchild)delete subchild;
	if (next)delete next;
}

/**
 * Appends a subchild to the list of subchilds. These are e.g. modal dialogs.
 * Simply append it to the end of the linked list, or use the widget as the head.
 **/
void CWidget::addSubChild(CWidget *sub)
{
	if (subchild)subchild->addSubChild(sub);
	else subchild=sub;
}

/**
 * Default handler for execute. elapsed is the time in s since the last call.
 * This recursively calls the execute methods of the elements of the linked lists.
 **/
void CWidget::execute(double elapsed)
{
	if (subchild)
	{
		/* If the subchild wants to be removed, it is removed here. */
		if (subchild->deleted())
		{
			/* The subchild to be removed is gently taken out of the linked list
			   ... */
			CWidget *w=subchild;
			subchild=subchild->subchild;
			w->subchild=0;
			/* ... and removed on its own. */
			delete w;
		}
		if (subchild)subchild->execute(elapsed);
	}
	if (child)
	{
		/* If the child wants to be removed, gently take it out of the linked list. */
		if (child->deleted())
		{
			CWidget* w=child;
			child=child->child;
			w->child=0;
			/* Delete only the child. Its successors are kept. */
			delete w;
		}
		/* Animate the child. */
		if (child)child->execute(elapsed);
	}
	if (next)
	{
		/* If next wants to be removed, take it out of the list and delete it gently. */
		if (next->deleted())
		{
			CWidget* w=next;
			next=next->next;
			w->next=0;
			delete w;
		}
		if (next)next->execute(elapsed);
	}
}

/**
 * Render a CWidget.
 * The implementation calls the render() methods of next and child.
 * The subchild is rendered in a separate call so the Z order is correct.
 * selection is true if the thing is to be rendered into the selection buffer.
 **/
void CWidget::render(bool selection)
{
	/* In case of the selection buffer, if no subchild exists and an id
	   is assigned, render a quad at the position defined by x,y,w,h, named
	   getId(). */
	if (selection && !subchild && getId()!=0)
	{
		glPushName(4);
		glPushName(getId());
		glBegin(GL_QUADS);
		glColor3f(1,1,1);
		glVertex3d(x,y,0);
		glVertex3d(x,y+h,0);
		glVertex3d(x+w,y+h,0);
		glVertex3d(x+w,y,0);
		glEnd();
		glPopName();
		glPopName();
	}
	/* Render next and child.
	   Do NOT render if selection && subchild. */
	if (next && !(subchild && selection))
	{
		next->render(selection);
	}
	if (child&& !(subchild && selection))
	{
		child->render(selection);
	}
}

/**
 * Render the subchild, if it exists.
 * This happens in an extra step so the Z order stays correct.
 **/
void CWidget::renderSubChild(bool selectionBuffer)
{
	/* If child and next have subchilds, they may be rendered,
	   unless selectionBuffer && subchild, since then the subchild may write
	   exclusively into the selection buffer. Man, this is complicated...
	   No widget may write into the selection buffer while a
	   modal subdialog exists. */
	if (child &&(!subchild || !selectionBuffer))child->renderSubChild(selectionBuffer);
	if (next && (!subchild || !selectionBuffer))next->renderSubChild(selectionBuffer);
	if (subchild)
	{
		/* Remember the current transformation (e.g. dialog deformations) */
		glPushMatrix();
		glLoadIdentity();
		/* Render the subchild normally. */
		subchild->render(selectionBuffer);
		glPopMatrix();
		subchild->renderSubChild(selectionBuffer);
	}
}

/**
 * The GUI reports that the mouse is currently over the widget with id id.
 **/
void CWidget::processSelection(int id)
{
	/* If the mouse is not over any widget at all, it is not over this either. So
	   inform it via mouseLeave. */
	if (getId()!=0 && id==0)mouseLeave();
	/* Inform the subchild about it. */
	if (subchild)subchild->processSelection(id);
	/* Inform child and next, but only if no subchild exists or
	   the mouse is over no widget, so that all of them can call mouseLeave. */
	if (child && (!subchild || id==0))child->processSelection(id);
	if (next && (!subchild || id==0))next->processSelection(id);
	/* If the widget belongs to the top level and id==getId(),
	   mouseEnter may be called. */
	if (!subchild && id!=0 && id==getId())mouseEnter();
	if (id!=0 && id!=getId())mouseLeave();
}

/**
 * Handle a key press. key is the ASCII code of the key.
 * Returns true if the key was handled, otherwise false.
 **/
bool CWidget::processKey(unsigned short key)
{
	/* If a subchild exists, let it handle the key and bail out immediately.
	   Do not even give next and child the chance to see the key. */
	if (subchild) {
	    if (subchild->processKey(key))return true;
	    else return false;
	}
	if (child) if (child->processKey(key))return true;
	if (next)if (next->processKey(key))return true;
	return false;
}

/**
 * A mouse event occurred (move, click, release, etc.)
 * The default handling is to pass it on to the linked lists.
 * The return value is a command to the caller, or 0 if the event was not
 * handled or there is no special return value.
 **/
int CWidget::processMouseEvent(TMouseEvent *event)
{
	int r=0;
	/* If there is a subchild, it has the exclusive pleasure of handling the event */
	if (subchild)
	{
		r=subchild->processMouseEvent(event);
		if (r!=0 || subchild->handleUI())return r;
	}
	if (child)r=child->processMouseEvent(event);
	/* If next and child have NOT handled it yet, handle it here. */
	if (next && r==0)return next->processMouseEvent(event);
	return r;
}


/**
 * The WidgetPane needs a very special render method.
 * It exists only once and prepares the 2D engine to render all other widgets
 * properly.
 **/
void CWidgetPane::render(bool selectionBuffer)
{
	glMatrixMode(GL_PROJECTION);
	if (!selectionBuffer)
	{
		glPushMatrix();
		glLoadIdentity();
	}else{
		glPopMatrix();
		glPushMatrix();
	}
	/* which provides exactly the coordinates (0/0) to (640/480) and scales with the
	   window. The Z range goes from -640 to +640 so a window can rotate completely. */
	glOrtho(0,640,480,0,-640,640);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	/* If we are NOT rendering into the selection buffer, switch on alpha blending */
	if (!selectionBuffer)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}

	const float mat[]={1.0,1.0,1.0,1.0};
	const float spec[]={0,0,0,1};
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,mat);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec);

	/* first render all widgets */
	CWidget::render(selectionBuffer);
	/* then render the subchilds one after another. */
	CWidget::renderSubChild(selectionBuffer);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glPopMatrix();

	/* Restore the previously saved projection matrix, if any. */
	if (!selectionBuffer)
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
}

/**
 * Handle a mouse event. This behaves slightly differently here
 * from the other widgets. If a subchild exists and a message is
 * not handled, return -1 as a sign that the GUI must not handle the event
 * any further.
 **/
int CWidgetPane::processMouseEvent(TMouseEvent *event)
{
	int r=CWidget::processMouseEvent(event);
	/* If the message was not handled but subchilds exist,
	   return -1! */
	if (r==0 && hasSubChild())return -1;
	if (r==0 && !hasSubChild())return 0;
	return r;
}


/**
 * Constructs a CButton at (vx/vy), vw wide, vh high, with id and text.
 * The position is computed relative to parent.
 **/
CButton::CButton(double vx,double vy,double vw,double vh,int id,CWidget* parent,const char *vtext)
{
	x=vx;
	y=vy;
	w=vw;
	h=vh;
	enabled=true;
	/* If parent!=nullptr, convert the position relative to parent into an absolute one. */
	if (parent) {
		x+=parent->getX();
		y+=parent->getY();
	}
	setId(id);
	hovered=false;
	/* No animations are in progress. */
	anim=pushanim=0.0;
	/* Clone the label text. */
	text=strdup(vtext);
}

CButton::~CButton()
{
	if (text)free(text);
}

void CButton::render(bool selectionBuffer)
{
	/* Only actually render something if this is not the selection buffer.
	   Rendering into the selection buffer is done by CWidget::render(true); */
	if (!selectionBuffer)
	{
		const double text_width=h*0.38;
		const double scale=(sin(anim*M_PI/2.0)*0.5+0.5)*0.2;

		double r,g,b,a;
		a=0.7+anim*0.3;
		r=0.1+anim*0.3+pushanim*0.2;
		g=0.2+anim*0.4+pushanim*0.3;
		b=0.4+anim*0.4+pushanim*0.2;

		CGLFont font(text_width,h*0.9);
		glPushMatrix();
		glTranslated(x+w/2.0,y+h/2.0,0);
		glScaled(1.0+scale*0.2,1.0+scale*0.25,1);
		
		if (!enabled)a=0.3;
		{
			glBegin(GL_QUADS);
			glColor4d(r,g,b,a);
			glVertex3d(-w/2.0,-h/2.0,0);
			glColor4d(r*0.5,g,b*0.8,a*0.7);
			glVertex3d(-w/2.0,h/2.0,0);
			glColor4d(r*0.5,g*0.7,b*0.7,a*0.7);
			glVertex3d(w/2.0,h/2.0,0);
			glColor4d(r*0.4,g*0.6,b*0.7,a*0.75);
			glVertex3d(w/2.0,-h/2.0,0);
			glEnd();
		}
		/* While the pressed animation runs, render a few rectangles around it */
		if (pushanim>0.001)
		{
			glPushMatrix();
			glColor4d(0.9,0.95,1.0,pushanim*0.45);
			glScaled(1.0/(1.0+scale*0.5),1.0/(1.0+scale*0.25),1);
			for (int i=0;i<2;i++)
			{
				glScaled(1.1-pushanim*0.1,1.4-pushanim*0.4,1);
				glBegin(GL_LINE_LOOP);
				glVertex3d(-w/2.0,-h/2.0,0);
				glVertex3d(-w/2.0,h/2.0,0);
				glVertex3d(w/2.0,h/2.0,0);
				glVertex3d(w/2.0,-h/2.0,0);
				glEnd();
			}
			glPopMatrix();
		}
		/* Now render the text properly. Shaded, not stirred. */
		glScaled(1+scale*0.6,1+scale,1+scale);
		glColor4d(0.2,0.2,0.2,1.0);
		font.drawText((-text_width*strlen(text))/2.0+1,-h*0.45+1,text);
		if (enabled)glColor4d(1,1,1,1);
		else glColor4d(0.45,0.45,0.45,1);
		font.drawText((-text_width*strlen(text))/2.0,-h*0.45,text);
		/* During the hover animation, render the text again slightly enlarged. */
		if (hovered && anim<0.999)
		{
			glScaled(1+scale*1.3,1+scale*2.9,1);
			glColor4d(0.8-anim*0.8,0.8-anim*0.8,0.8-anim*0.8,1);
			font.drawText((-text_width*strlen(text))/2.0,-h*0.45,text);
		}
		glPopMatrix();
	}
	/* And CWidget::render takes care of the next and child lists and
	   of rendering into the selection buffer. */
	CWidget::render(selectionBuffer);
}

/**
 * Animate the button if an animation is in progress (hover / press).
 **/
void CButton::execute(double elapsed)
{
	if (hovered)
	{
		anim+=elapsed*3.9;
		if (anim>1.0)anim=1.0;
	}else{
		anim-=elapsed*1.4;
		if (anim<0.0)anim=0.0;
	}
	/* Animate pushanim down to 0 */
	pushanim-=elapsed*2.5;
	if (pushanim<0.0)pushanim=0.0;
	CWidget::execute(elapsed);
}

/**
 * Handle the CButton's mouse event. If the button was pressed, return id.
 **/
int CButton::processMouseEvent(TMouseEvent *event)
{
	/* All other widgets have priority. If handled, pass the command down */
	int r=CWidget::processMouseEvent(event);
	if (r)return r;
	/* And if no next, child or subchild handled the message,
	   and <hovered>... */
	if (!hovered)return 0;
	
	/* ...and the left mouse button was pressed, and the button has an id */
	if (event->press && event->button1 && getId())
	{
		/* Then exactly this button was pressed. Start pushanim. */
		pushanim=1.0;
		return getId();
	}
	return 0;
}


/**
 * A check box
 * The position is defined by vx,vy,vw,vh
 * id and a label are passed as id and vtext. vchecked gives
 * the initial state. parent is only used so that vx/vy can be given
 * relative to it.
 **/
CCheckBox::CCheckBox(double vx,double vy,double vw,double vh,int id,const char *vtext,bool vchecked,CWidget* parent)
{
	/* Remember the position. */
	x=vx;
	y=vy;
	w=vw;
	h=vh;
	enabled=true;
	/* If parent was passed, vx and vy are relative coordinates.
	   Convert them into absolute ones here. */
	if (parent) {
		x+=parent->getX();
		y+=parent->getY();
	}
	/* pushanim has reached its final state. */
	pushanim=1.0;
	/* Remember the id */
	setId(id);
	hovered=false;
	/* Remember checked */
	checked=vchecked;
	text=strdup(vtext);
	nextCheckBox=prevCheckBox=0;
}

CCheckBox::~CCheckBox()
{
	if (text)free(text);
}

/**
 * Animate the CCheckBox.
 **/
void CCheckBox::execute(double elapsed)
{
	CWidget::execute(elapsed);
	/* Animate pushanim up from 0 to 1. */
	pushanim+=elapsed*2.0;
	if (pushanim>1.0)pushanim=1.0;
}

/**
 * Render the check box.
 **/
void CCheckBox::render(bool selectionBuffer)
{
	/* Only render when not rendering into the selection buffer. Rendering into
	   that is done by CWidget::render(true); */
	if (!selectionBuffer)
	{
		static const double text_width=8.0;
		CGLFont font(text_width,h*0.9);
		
		const double a=(enabled?1.0:0.4);
		glBegin(GL_QUADS);
		if (hovered)
		{
			glColor4d(0.3,0.7,0.8,0.7);
			glVertex3d(x+h*0.05,y+h*0.05,0);
			glColor4d(0.2,0.35,0.45,0.5);
			glVertex3d(x+h*0.05,y+h*0.95,0);
			glColor4d(0.1,0.35,0.45,0.4);
			glVertex3d(x+h*0.95,y+h*0.95,0);
			glColor4d(0.2,0.6,0.7,0.5);
			glVertex3d(x+h*0.95,y+h*0.05,0);
		}else{
			glColor4d(0.1,0.2,0.3,a*0.7);
			glVertex3d(x+h*0.05,y+h*0.05,0);
			glColor4d(0.1,0.1,0.2,a*0.6);
			glVertex3d(x+h*0.05,y+h*0.95,0);
			glColor4d(0.1,0.1,0.2,a*0.5);
			glVertex3d(x+h*0.95,y+h*0.95,0);
			glColor4d(0.1,0.2,0.2,a*0.6);
			glVertex3d(x+h*0.95,y+h*0.05,0);
		}
		glEnd();

		if (checked || pushanim<1.0) {
			/* Render the tick, rotating it first if needed */
			glPushMatrix();
			glTranslated(x+h*0.5,y+h*0.5,0);
			glPushMatrix();
			if (checked)glRotated(90.0-pushanim*90.0,0.2,0.9,0);
			else glRotated(pushanim*90.0,0.2,0.9,0);;
		
			glBegin(GL_TRIANGLE_STRIP);
			glColor4d(1,1,0.8,0.7*a);
			glVertex3d(-h*0.4,-h*0.2,0);
			glVertex3d(-h*0.1,h*0.4,0);
			glVertex3d(-h*0.1,0,0);
			glVertex3d(h*0.4,-h*0.4,0);
			glEnd();
			glPopMatrix();
			if (pushanim<1.0)
			{
				/* During the animation, render the tick once more,
				   enlarged. */
				glScaled(1.0+pushanim*3.0,1.0+pushanim*3.0,1);
				if (checked)glRotated(-45.0+pushanim*45.0,0,0,-1);
				else glRotated(pushanim*45.0,0,0,-1);
				glBegin(GL_TRIANGLE_STRIP);
				if (checked)glColor4d(0.3,1,0.3,1.0-pushanim);
				else glColor4d(1.0,0.3,0.3,1.0-pushanim);
				glVertex3d(-h*0.4,-h*0.2,0);
				glVertex3d(-h*0.1,h*0.4,0);
				glVertex3d(-h*0.1,0,0);
				glVertex3d(h*0.4,-h*0.4,0);
				glEnd();
			}
			glPopMatrix();
		}
		glPushMatrix();
		glTranslated(x+40.0+h,y+h/2.0,0);
		glColor4d(0.2,0.2,0.2,1);
		/* Shadow first. */
		font.drawText(1-30,-h*0.9/2.0+1,text);
		if (hovered)glColor4d(1,1,1,1);
		else if (enabled)glColor4d(0.65,0.65,0.65,1);
		else glColor4d(0.3,0.3,0.3,1);
		font.drawText(-30,-h*0.9/2.0,text);
		/* While the animation runs, draw the text again, larger, on top. */
		if (pushanim<1.0)
		{
			if (!checked)glScaled(1.0+pushanim/3.0,1.0+pushanim*1.3,1);
			else glScaled(1.3-pushanim/3.0,2.3-pushanim*1.3,1);
			glColor4d(1.0-pushanim,1.0-pushanim,1.0-pushanim,1);
			font.drawText(-30,-h*0.9/2.0,text);
		}
		glPopMatrix();
	}
	/* Render the remaining widgets. */
	CWidget::render(selectionBuffer);
}

/**
 * Handle the mouse event, unless someone else does.
 * When the check box gets checked, automatically uncheck all other check boxes of the
 * same group. :)
 **/
int CCheckBox::processMouseEvent(TMouseEvent *event)
{
	int r=CWidget::processMouseEvent(event);
	if (r)return r;
	/* If the check box is not hovered, the event is not for it either */
	if (!hovered)return 0;

	/* On a left mouse button press, someone is clicking the check box. */
	if (event->press && event->button1)
	{
		/* Only allow unchecking if the check box is alone in its group;
		   then it is a real check box (on/off). */
		if (checked && !nextCheckBox && !prevCheckBox)
		{
			pushanim=0.0;
			checked=false;
			return getId();
		}else 
		/* Only continue if the check box is off. */
			if (!checked)
		{
			checked=true;
			pushanim=0.0;
			/* If the check box belongs to a group, set all elements of the
			   nextCheckBox list and the prevCheckBox list to unchecked here.
			   That gives us a radio group. */
			CCheckBox *b=nextCheckBox;
			/* Walk the linked lists iteratively. */
			while (b)
			{
				b->setCheck(false);
				b=b->nextCheckBox;
			}
			b=prevCheckBox;
			while (b)
			{
				b->setCheck(false);
				b=b->prevCheckBox;
			}
			return getId();
		}
	}
	return 0;
}

void CCheckBox::addCheckBox(CCheckBox *b)
{
	if (nextCheckBox)nextCheckBox->addCheckBox(b);
	else
	{
		/* Link them up. The current check box must reach the other through nextCheckBox,
		   and the other must be able to go back through
		   prevCheckBox. */
		nextCheckBox=b;
		b->prevCheckBox=this;
	}
}



/**
 * A CStaticText is just static text
 * The position is (x/y), optionally relative to parent, with label vtext.
 * vtext may contain \n as a line break. The text is left-aligned.
 **/
CStaticText::CStaticText(double x,double y,const char *vtext,CWidget* parent)
:CWidget()
{
	a=1.0;
	/* Remember the position. */
	this->x=x;
	this->y=y;
	/* A character is 8 units wide, so compute the width of the widget here so that the
	   text is left-aligned at x. */
	this->w=8*strlen(vtext);
	if (parent)
	{
		this->x+=parent->getX();
		this->y+=parent->getY();
	}
	/* Copy the text. */
	text=strdup(vtext);
}

/**
 * Constructor as above, except that a parameter w is passed
 * which gives the width of the text. The text is centred within w.
 **/
CStaticText::CStaticText(double x,double y,double w,const char *vtext,CWidget* parent)
:CWidget()
{
	a=1.0;
	this->x=x;
	this->y=y;
	this->w=w;
	if (parent)
	{
		this->x+=parent->getX();
		this->y+=parent->getY();
	}
	text=strdup(vtext);
}

CStaticText::~CStaticText()
{	
	if (text)free(text);
}

/**
 * Set a new text.
 * With adjustwidth=false, <w> is kept, so the new text is centred within the
 * old width. */
void CStaticText::setText(const char *vtext,bool adjustwidth)
{
	/* Free the old memory. */
	if (text)free(text);
	text=strdup(vtext);
	if (adjustwidth) w=8*strlen(vtext);
	/* Set some transparency */
	setAlpha(0.625);
}

/**
 * Render the StaticText. */
void CStaticText::render(bool selectionbuffer)
{
	/* If not rendering into the selection buffer. Nothing is rendered into the selection buffer
	   anyway, since probably (getId()==0) */
	if (!selectionbuffer && !deleted())
	{
		CGLFont font(8,15);
		/* Set the degree of transparency. */
		glColor4d(a,a,a,1.0);
		font.drawText(x+(w-8*strlen(text))/2.0,y,text);
	}
	/* Render the other widgets. */
	CWidget::render(selectionbuffer);
}



/**
 * A text field. The position is defined by x,y,w,h, optionally relative to parent.
 * id is the id and vtext the initial text.
 * If numbers_only=true, only digits can be entered, otherwise letters too.
 **/
CTextEdit::CTextEdit(double x,double y,double w,double h,int id,const char *vtext,bool numbers_only,CWidget *parent)
:CFrame(x,y,w,h)
{
	/* Convert the coordinates. */
	if (parent) {
		this->x+=parent->getX();
		this->y+=parent->getY();
	}
	setId(id);
	/* text is a fixed char array, so just copy vtext into it. Do not allocate memory. */
	setText(vtext);
	numbers=numbers_only;
	animatefocused=0.0;
	animatechar=1.0;
	lastchar='\0';
	hovered=focused=false;
	m_committed=false;
	/* The text in the edit is left-aligned. Could also be set to false
	   for right-aligned. That is what CSpinBox does. */
	left_align=true;
	setAlpha(0.55);
}

CTextEdit::~CTextEdit()
{ }

const char* CTextEdit::getText()
{
	return &text[0];
}

/**
 * Set a new text.
 * m_committed indicates whether the text was confirmed with <Enter>. It must be reset
 * here.
 **/
void CTextEdit::setText(const char *vtext)
{
	m_committed=false;
	strncpy(text,vtext,sizeof(text)-1);
	text[sizeof(text)-1]='\0';
}

/**
 * Render the TextEdit. Part of it is rendered by CFrame::render()    o_O
 **/
void CTextEdit::render(bool selection)
{
	/* Adapt the CFrame colours to the circumstances every time here. Costs nothing and is quick. */
	if (focused)setColor(0.4,0.5,0.65);
	else if (hovered)setColor(0.25,0.3,0.45);
	else setColor(0.1,0.15,0.2);
	/* Render the CFrame. This first renders a coloured rectangle and all other widgets
	   on the same level, and the widget into the selection buffer if selection=true. */
	CFrame::render(selection);

	if (!selection)
	{
		
		double tw=h*0.4;
		double otw=tw;
		/* If the width of the text exceeds that of the widget, recompute the character width
		   dynamically, taking the animation into account. No need to understand it.
		   (I do not get it myself any more. :) */
		if ((strlen(text)+2)*tw>w)tw=w/(double)((double)strlen(text)+1.0+(lastchar?(1.0-animatechar):animatechar));
		CGLFont font(tw,h*0.90);
	
		glColor4d(1,1,1,hovered?0.8:0.4);
		glBegin(GL_LINE_LOOP);
		glVertex3d(x+w,y,0);
		glVertex3d(x,y,0);
		glVertex3d(x,y+h,0);
		glVertex3d(x+w,y+h,0);
		glEnd();
	
		/* If the edit field has focus, draw further gently animated rectangles around it. */
		if (focused) {
			glPushMatrix();
			glTranslated(x+w/2.0,y+h/2.0,0);
			/* two of them */
			for (int i=0;i<2;i++)
			{
				/* the second one offset by exactly half a period. */
				const double anim=fmod(animatefocused+(double)i/2.0,1);
				glPushMatrix();
				glColor4d(1,1,1,0.3-anim*0.3);
				glScaled(1.0+(anim)*0.10,1.0+(anim)*1.0,1);
				glBegin(GL_LINE_LOOP);
				glVertex3d(+w/2.0,-h/2.0,0);
				glVertex3d(-w/2.0,-h/2.0,0);
				glVertex3d(-w/2.0,+h/2.0,0);
				glVertex3d(+w/2.0,+h/2.0,0);
				glEnd();
				glPopMatrix();
			}

			glPopMatrix();
		}
	
		glPushMatrix();
		glTranslated(x+4,y+h*0.05,0);
		glColor4d(1,1,1,1);
		/* If a character was removed or a new one typed, the last character
		   is shown animated here. */
		if (animatechar<1.0)
		{
			/* lastchar=='\0' means a new character was typed. The one to animate
			   is the last one of text[]. */
			if (lastchar=='\0')
			{
				if (strlen(text)>1)font.drawText(left_align?0:(w-tw*strlen(text)-otw),0,text,strlen(text)-1);
				/* And the last one is shown animated separately here. */
				if (strlen(text)>=1)
				{
					const double s=5.0-animatechar*4.0;
					glColor4d(0.5+animatechar*0.5,0.5+animatechar*0.5,0.5+animatechar*0.5,1);
					if (left_align)glTranslated((strlen(text)-1)*tw+tw/2.0,h*0.45,0);
					else glTranslated(w-otw*1.5,h*0.45,0);
					glScaled(s,s,s);
					font.drawText(-tw/2.0,-h*0.45,&text[strlen(text)-1],1);
				}
			}else{
				/* Otherwise the last character was removed; it is stored in lastchar.
				   It is shown animated here. */
				font.drawText(left_align?0:(w-tw*strlen(text)-otw),0,text);
				const double s=1.0+animatechar*4.0;
				glColor4d(1.0-animatechar,1.0-animatechar,1.0-animatechar,1);
				if (left_align)glTranslated((strlen(text))*tw+tw/2.0,h*0.45,0);
				else glTranslated(w-otw/2.0,h*0.45,0);
				glScaled(s,s,s);
				font.drawText(-tw/2.0,-h*0.45,&lastchar,1);
			}
		}else 
		/* If no animation is running, the text can be rendered in one piece. */
			font.drawText(left_align?0:(w-tw*strlen(text)-otw),0,text);
		glPopMatrix();
	}
}

/**
 * Animate the TextEdit.
 **/
void CTextEdit::execute(double elapsed)
{
	CFrame::execute(elapsed);
	if (focused){
		animatefocused+=elapsed*0.6;
	}
	animatechar+=elapsed*8.0;
	if (animatechar>1.0)animatechar=1.0;
}

/**
 * Handle a mouse event.
 **/
int CTextEdit::processMouseEvent(TMouseEvent *event)
{
	int r=CWidget::processMouseEvent(event);
	if (r)return r;

	if (event->press && event->button1)
	{
		if (!focused)animatefocused=0.0;
		/* If the mouse is not over the text field, take away the focus. */
		if (!hovered)
		{
			/* If the field had the focus before, validate the input so far. */
			if (focused)endEdit();
			focused=false; 
		}else focused=true;
	}
	if (event->press && event->button2)
	{
		if (hovered)
		{
			/* Clear the text field! */
			setText("");
			/* (validate the input) */
			endEdit();
			animatefocused=0.0;
			focused=true;
			return 0;
		}else{
			/* Validate the input if we had the focus before. */
			if (focused)endEdit();
			focused=false;
		}
	}
	/* Handling of the event may continue. */
	return 0;
}

/**
 * Handle a key press
 **/
bool CTextEdit::processKey(unsigned short key)
{
	if (CFrame::processKey(key))return true;
	if (!focused)return false;
	
	if (isascii(key) && isprint(key)
	   && ((!numbers) || (numbers && isdigit(key))))
	{
		/* Stop accepting new characters early. Our input buffer has a fixed size. */
		if (strlen(text)>sizeof(text)-2)return false;
		/* Append the character and take care of the terminating \0 (especially for strlen) */
		text[strlen(text)+1]='\0';
		text[strlen(text)]=(unsigned char)(key&0xFF);
		animatechar=0.0;
		/* And indicate that the animated character is the last one of text[]. */
		lastchar='\0';
	}else{

		if (key==VK_BACK && strlen(text)>0)
		{
			/* Remember the last character, since it is still to be animated. */
			lastchar=text[strlen(text)-1];
			animatechar=0.0;
			text[strlen(text)-1]='\0';
		}
		/* Escape. The text field only loses the focus. */
		if (key==VK_ESCAPE)
		{
			/* Validate the input. */
			endEdit();
			focused=false;
		}
		/* Enter. The text field loses the focus, but m_committed becomes true. */
		if (key==VK_RETURN)
		{
			/* Validate the input. */
			endEdit();
			focused=false;
			/* But remember that Enter was pressed, so all sorts of things can still happen. */
			m_committed=true;
		}
	}
	/* We do not want other widgets to process the key any further, since we have the focus. */
	return true;
}


/**
 * A CSpinBox is a CTextEdit extended by two buttons and the range min to max.
 * min, max and value are the range and the current numeric value of the spin box.
 **/
CSpinBox::CSpinBox(double x,double y,double w,double h,int id,int min,int max,int value,CWidget *parent,bool rightButtons)
/* Pass the parameters on to CTextEdit. */
:CTextEdit(x,y,w-21,h,id,"",true,parent)
{
	left_align=false; 
	/* Remember the limits. */
	this->min=min;
	this->max=max;
	this->def=value;
	/* Attach two child widgets, the up and down buttons, to the TextEdit ancestor.
	   As IDs they get 10000000+id for up and 20000000+id for down. */
	if (rightButtons){
		addChild(up=new CButton(w-19,-h/10.0,18,h/1.9,10000000+id,this,"^"));
		addChild(down=new CButton(w-19,h-h/2.0+h/10.0,18,h/1.9,20000000+id,this,"v"));
	}else{
		setSize(x+21,y,w-21,h);
		if (parent) {
			this->x+=parent->getX();
			this->y+=parent->getY();
		}
		addChild(up=new CButton(-20,-h/10.0,18,h/1.9,10000000+id,this,"^"));
		addChild(down=new CButton(-20,h-h/2.0+h/10.0,18,h/1.9,20000000+id,this,"v"));
	}
	setValue(value);
}

int CSpinBox::getValue()
{
	return atoi(text);
}

/**
 * Put the given value as text into the CTextEdit.
 * value is automatically clamped to min<=value<=max, and the up/down buttons are
 * enabled or disabled accordingly.
 **/
void CSpinBox::setValue(int value)
{
	if (value<min)value=min;
	if (value>max)value=max;
	sprintf(text,"%d",value);

	/* up is enabled if value<max, i.e. the value could still be increased. */
	up->setEnabled(value<max);
	down->setEnabled(value>min);
}

/**
 * Handle a mouse event.
 **/
int CSpinBox::processMouseEvent(TMouseEvent *event)
{
	int r=CTextEdit::processMouseEvent(event);
	/* If 10000000+getId() (up was pressed) or the mouse wheel was moved up,
	   increase the value by 1. */
	if (r==10000000+getId() || (hovered && event->press && event->button4))setValue(getValue()+1);
	/* If 20000000+getId() (down was pressed) or the mouse wheel was moved down,
	   decrease the value by 1. */
	else if (r==20000000+getId()  || (hovered && event->press && event->button5))setValue(getValue()-1);
	else return r;
	/* Otherwise the value was changed. :) So return getId(). */
	return getId();
}

/**
 * For validation, simply set the text currently present via setValue.
 * setValue does the validation and clamps the value to the limits.
 **/
void CSpinBox::endEdit()
{
	if (strlen(text)==0)setValue(def);
	else setValue(getValue());
}



/**
 * A frame is just a rectangular area on the screen, with colour and transparency, but without
 * interactivity.
 **/
CFrame::CFrame(double vx,double vy,double vw,double vh)
{
	/* Remember the frame position. */
	x=vx;
	y=vy;
	w=vw;
	h=vh;
	setColor(0.8,0.8,0.8);
	setAlpha(0.92);
}

void CFrame::setSize(double vx,double vy,double vw,double vh)
{
	x=vx;
	y=vy;
	w=vw;
	h=vh;
}

void CFrame::render(bool selectionBuffer)
{
	if (!selectionBuffer)
	{
		/* Do not render into the selection buffer, but otherwise a rectangle of roughly the
		   given colour and transparency. */
		glBegin(GL_QUADS);
		glColor4d(r*0.75,g*0.75,b*0.75,a*0.9);
		glVertex3d(x+w,y,0);
		glColor4d(r,g,b,a);
		glVertex3d(x,y,0);
		glColor4d(r*0.75,g*0.75,b*0.75,a*0.9);
		glVertex3d(x,y+h,0);
		glColor4d(r*0.5,g*0.5,b*0.5,a*0.8);
		glVertex3d(x+w,y+h,0);
		glEnd();
	}
	/* Render the other widgets. */
	CWidget::render(selectionBuffer);
}




/**
 * Construct the CDialog.
 **/
CDialog::CDialog(double vw,double vh,const char *vcaption)
/* A dialog is a centred CFrame of a given size. */
:CFrame((640.0-vw)/2.0,(480.0-vh)/2.0,vw,vh)
{
	anim=0.0;
	starting=true;
	/* Copy the title bar text. */
	caption=strdup(vcaption);
	/* Set the default colour. */
	setColor(0.6,0.6,0.6);
}

/* Free the title bar text again. */
CDialog::~CDialog()
{
	if (caption)free(caption);
}

void CDialog::setSize(double vw,double vh)
{
	CFrame::setSize((640.0-vw)/2.0,(480.0-vh)/2.0,vw,vh);
}

void CDialog::setCaption(const char* caption)
{
	if (this->caption)free(this->caption);
	this->caption=strdup(caption);
}

/**
 * Render the CDialog. This is mostly an animation; the CFrame does the rest.
 **/
void CDialog::render(bool selectionBuffer)
{
	glPushMatrix();
	glLoadIdentity();
	/* Rotate/scale the dialog according to the animation to fade it in nicely. */
	glRotated(-90.0+sin(anim*M_PI/2.0)*90.0,0.63,0.63,0.2);
	glScaled(0.9+sin(anim*M_PI/2.0)*0.1,1,1);
	glTranslated(320,240,0);
	glRotated(-30.0+sin(anim*M_PI/2.0)*30.0,0.1,0,-1);
	glTranslated(-320,-240,0);

	if (!selectionBuffer)
	{
		/* Render a transparent, slightly offset shadow. */
		const double shadowsize=9.0;
		glBegin(GL_QUADS);
		glColor4d(0,0,0,0.35);
		glVertex3d(x+w+shadowsize/1.5,y+shadowsize,0);
		glVertex3d(x+shadowsize/1.5,y+shadowsize,0);
		glVertex3d(x+shadowsize/1.5,y+h+shadowsize,0);
		glVertex3d(x+w+shadowsize/1.5,y+h+shadowsize,0);
		glEnd();
	}
	CFrame::render(selectionBuffer);
	
	CGLFont font(10,20);
	glColor4d(1,1,1,1);
	font.drawText(x+(w-10*strlen(caption))/2.0,y+5,caption);

	glPopMatrix();
}

/**
 * Animate the dialog.
 **/
void CDialog::execute(double elapsed)
{
	CWidget::execute(elapsed);
	const double speed1=1.3;
	const double speed2=1.9;

	if (starting)
	{
		/* Fade the dialog in. Animation forwards. */
		anim+=elapsed*speed1;
		if (anim>1.0)anim=1.0;
	}else{
		/* Fade the dialog out. Animation backwards. */
		anim-=elapsed*speed2;
		/* And when the animation is over, mark the dialog for removal. */
		if (anim<0.0)
		{
			anim=0.0;
			remove();
		}
	}
}

/**
 * Handle a mouse event / commands.
 **/
int CDialog::processMouseEvent(TMouseEvent *event)
{
	if (!starting)return 0;
	int r=CWidget::processMouseEvent(event);
	switch (r)
	{
	// Close the dialog. ID 1 is simply defined as the close button, which exists almost everywhere.
	case 1:	close();
		break;
	default: return r;
	}
	// Return 0, since we handled the button.
	return 0;
}

/**
 * Handle a key.
 **/
bool CDialog::processKey(unsigned short key)
{
	if (CFrame::processKey(key))return true;
	
	/* If the dialog is being closed, do not handle anything. */
	if (starting==false)return false;

	if (key==VK_ESCAPE){
		close();
		return true;
	}
	/* A dialog ALWAYS handles the key, modal or not.
	   Nobody else should process it after us. */
	return true;
}

/* Close the dialog, including all subchilds. */
void CDialog::close()
{
	close(true);
}

/* Close the dialog. Recursively, or only this one dialog? */
void CDialog::close(bool recursive)
{
	/* If recursive, subdialogs are closed too, otherwise only the current dialog. */
	if (recursive && hasSubChild())getSubChild()->close();
	/* Start the closing animation. */
	starting=false;
}

/**
 * Just a message box, with title bar, text and window colour in (r/g/b)
 **/
CMessageBox::CMessageBox(double w,double h,const char *caption,const char *text,double r,double g,double b)
:CDialog(w,h,caption)
{
	/* Set the colour. */
	setColor(r,g,b);
	/* Insert the OK button. */
	addChild(new CButton((w-70)/2.0,h-30,70,25,1,this,"Okay"));
	/* Place the text in the window as a StaticText. */
	addChild(new CStaticText(10,40,text,this));
}





/**
 * An FPS counter as a CStaticText. The constructor sets the counter to 0
 **/
CFPS::CFPS(double x,double y,CGUI *gui)
:CStaticText(x,y,"",nullptr)
{
	counter=0;
	time=0.0;
	options=gui->getOptions();
}

/**
 * Checks whether the time for a measurement has elapsed, and updates the label.
 **/
void CFPS::execute(double elapsed)
{
	CStaticText::execute(elapsed);
	time+=elapsed;
	if (time>0.5)
	{
		/* Update the label. */
		char c[100];
		sprintf(c,"fps: %.1f",(double)counter / time);
		setText(c,true);
		/* Reset the counter. */
		time=0.0;
		counter=0;
	}
}

void CFPS::render(bool selectionBuffer)
{
	/* If the text is to be rendered, call CStaticText::render, which calls CWidget::render
	   in turn. Otherwise call CWidget::render directly and bypass CStaticText. */
	if (options->get(OPTION_SHOW_FPS))CStaticText::render(selectionBuffer);
	else CWidget::render(selectionBuffer);

	counter++;
}

