/**
 * chatbox.h
 * Author: Sascha Hlusiak
 * Chat box for chatting over the network during a game
 **/

#ifndef __CHATBOX_H_INCLUDED_
#define __CHATBOX_H_INCLUDED_

#include "widgets.h"
#include "network.h"

class CGUI;

/* The chat box buffers CHAT_LINES lines. */
#define CHAT_LINES (9)
/* Compute the height of the widget from the number of lines */
#define CHAT_HEIGHT (CHAT_LINES*11.0+34)

/**
 * The chat box is derived from CFrame
 **/
class CChatBox:public CFrame
{
private:
	/* State of the animation, from 0.0 to 1.0 */
	double anim;
	bool visible;
	/* How long the chat box has been visible */
	double time;
	/* Animation of the most recently added chat line */
	double lineanim;

	/* Strings of the chat history */
	char* lines[CHAT_LINES];
	CTextEdit *text;
	CGUI* gui;
	/* If (!sticky), the chat box hides after a few seconds without
	   input. Otherwise it stays visible until it is closed. **/
	bool sticky;
public:
	CChatBox(double x,double y,CGUI* vgui);
	/* Clean up the rubbish again */
	virtual ~CChatBox();

	/* Animate the chat box */
	virtual void execute(double elapsed);
	/* Render the chat box */
	virtual void render(bool selection);
	/* Handle a key press (Esc) */
	virtual bool processKey(unsigned short key);
	/* Handle a mouse click, for the send button */
	virtual int processMouseEvent(TMouseEvent* event);

	/* Make the chat box visible and start the animation. */
	void show();
	/* Append a new chat line */
	void addLine(const char *t,int len=-1);
	/* Append a whole message from client (-1 is the server) */
	void addChatMessage(int client,const char* text);
	void setSticky(bool vsticky) { sticky=vsticky; }
};

#endif
