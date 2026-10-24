/**
 * chatbox.cpp
 * Author: Sascha Hlusiak
 * Implementation of the chat box for talking over the network during a game
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <math.h>
#include <string.h>
#include "chatbox.h"
#include "gui.h"
#include "gameclient.h"
#include "glfont.h"
#include "constants.h"

/**
 * Initialise the chat box
 **/
CChatBox::CChatBox(double x,double y,CGUI* vgui)
:CFrame(x,y,235,CHAT_HEIGHT)
{
	gui=vgui;
	setColor(0.5,0.8,0.7);
	setAlpha(0.65);
	anim=time=0.0;
	visible=false;
	/* and not sticky, i.e. it hides automatically. */
	sticky=false;
	lineanim=0.0;

	/* Add the required widgets */
	addChild(text=new CTextEdit(5,CHAT_HEIGHT-20,180,16,100,"",false,this));
	addChild(new CButton(190,CHAT_HEIGHT-22,40,20,101,this,"Chat"));
	CFrame *f=new CFrame(x+5,y+5,w-10,CHAT_HEIGHT-30);
	f->setColor(0.0,0.12,0.0);
	f->setAlpha(0.65);
	addChild(f);
	/* Clear the lines (nullptr pointers) */
	for (int i=0;i<CHAT_LINES;i++)lines[i]=0;
}

/**
 * Clean up the mess. Free memory
 **/
CChatBox::~CChatBox()
{
	for (int i=0;i<CHAT_LINES;i++)if (lines[i])free(lines[i]);
}

/**
 * Animate the chat box: fade in, fade out, animate a line.
 **/
void CChatBox::execute(double elapsed)
{
	CFrame::execute(elapsed);
	/* If the text edit was left with Enter, send the message. */
	if (text->committed())
	{
		gui->game->chat(text->getText());
		/* Clear the text edit, but do NOT give it focus. */
		text->setText("");
	}

	if (visible)anim+=elapsed*2.0;else anim-=elapsed*3.0;
	if (anim>1.0)anim=1.0;
	if (anim<0.0)anim=0.0;
	/* Count how long the edit field has NOT had focus. */
	if (!text->hasFocus())time+=elapsed;
	/* After 7 s the chat box should hide automatically if !sticky and the
	   corresponding option is set. */
	if (time>7.0 && visible && !sticky && gui->getOptions()->get(OPTION_AUTO_CHATBOX))visible=false;
	/* New lines should scroll up smoothly. Do that here. */
	if (lineanim<1.0)
	{
		lineanim+=elapsed*4.0;
		if (lineanim>1.0)lineanim=1.0;
	}
}

void CChatBox::render(bool selection)
{
	glPushMatrix();
	glLoadIdentity();
	glTranslated(0,y,0);
	/* Animate the chat box smoothly when visible */
	if (anim>0.0001)glScaled(sin(anim*M_PI/2.0),sin(anim*M_PI/2.0),1.0);
	else glRotated(-180,0,1,0);

	glTranslated(0,-y,0);
	CFrame::render(selection);

	if (anim>0.0001)
	{
		/* Font 6x11, white */
		CGLFont font(6,11);
		glColor3d(1,1,1);
		/* Compute the position of the top line */
		glTranslated(x+6,y+6,0);
		glTranslated(0,11.0-(sin(lineanim*M_PI-M_PI/2.0)/2.0+0.5)*11.0,0);
		/* Output all lines except one */
		for (int i=0;i<CHAT_LINES-1;i++)
		{
			if (lines[i])font.drawText(0,0,lines[i]);
			glTranslated(0,11,0);
		}
		/* Only render the last line once the animation has finished */
		if (lines[CHAT_LINES-1] && lineanim>0.99)
		{
			font.drawText(0,0,lines[CHAT_LINES-1]);
		}
	}
	glPopMatrix();
}

void CChatBox::show()
{
	/* Visible */
	visible=true;
	time=0.0;
	/* Give the input field focus right away */
	text->setFocus();
}

/**
 * Handle Escape
 **/
bool CChatBox::processKey(unsigned short key)
{
	/* If CFrame handled the key, the edit field presumably caught it */
	if (CFrame::processKey(key))return true;
	if (visible && key==VK_ESCAPE)
	{
		/* Then set visible to false, which triggers an animation */
		visible=false;
		return true;
	}
	return false;
}

/**
 * Handle a mouse event: click on the chat button
 **/
int CChatBox::processMouseEvent(TMouseEvent* event)
{
	int r=CFrame::processMouseEvent(event);
	switch (r)
	{
		/* On a click on the chat button, send the text and clear the edit field.
		   It keeps the focus, which does no harm. */
		case 101:gui->game->chat(text->getText());
			text->setText("");
			break;
		/* Pass the command on */
		default: return r;
	}
	/* Do not process any further */
	return 0;
}

/**
 * Add a line to the chat box
 * t is a string of which the first len characters are used.
 **/
void CChatBox::addLine(const char *t,int len)
{
	/* The top line scrolls out, so free its memory if necessary. */
	if (lines[0])free(lines[0]);
	for (int i=0;i<CHAT_LINES-1;i++)lines[i]=lines[i+1];
	char *l=(char*)malloc(strlen(t)+2);
	if (len==-1)strcpy(l,t);else 
	{
		strncpy(l,t,len);
		/* Terminate the string. */
		l[len]='\0';
	}
	/* Assign the last line. */
	lines[CHAT_LINES-1]=l;
}

/**
 * Add a complete message from client.
 * The message is wrapped if necessary.
 * With client==-1 the message comes from the server itself
 */
void CChatBox::addChatMessage(int client,const char *text)
{
	/* Maximum number of characters per line */
	const int maxlen=(int)(w-10-3)/6;
	
	if (lines[CHAT_LINES-1])lineanim=0.0; else lineanim=1.0;

	/* Build a new string with "Client %d: " in front. */
	char *t=(char*)malloc(strlen(text)+20);
	if (client==-1)
		sprintf(t,"  *  %s",text);
	else 	sprintf(t,"Client %d: %s",client,text);
	unsigned int i;
	/* Chop the string up and add the wrapped pieces one by one */
	for (i=0;i<strlen(t)/(maxlen);i++)
	{
		addLine(&t[i*maxlen],maxlen);
	}
	/* Add the last fragment unless it happened to fit exactly */
	if (i*maxlen<strlen(t))
	{
		addLine(&t[i*maxlen],-1);
	}

	/* Free the temporary memory. */
	free(t);
	/* Make the chat box visible, since a message has obviously arrived from
	   outside. */
	if (gui->getOptions()->get(OPTION_AUTO_CHATBOX))visible=true;
	/* Reset the timeout. */
	time=0.0;
}
