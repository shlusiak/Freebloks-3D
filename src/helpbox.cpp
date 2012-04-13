/**
 * helpbox.cpp
 * Autor: Sascha Hlusiak
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "widgets.h"
#include "helpbox.h"

/**
 * HelpBox Constructor. Dialog mit Steuerelementen fuettern.
 **/
CHelpBox::CHelpBox()
:CDialog(450,350,"Help")
{
	/* Farbe setzen */
	setColor(0.75,0.2,0.4);

	/* Drei Buttons am oberen Rand plazieren */
	addChild(t1=new CButton(75,30,90,20,1001,this,"Rules"));
	addChild(t2=new CButton(170,30,90,20,1002,this,"Mouse"));
	addChild(t3=new CButton(265,30,90,20,1003,this,"Keyboard"));

	/* Ok Knopf hinzufuegen */
	addChild(new CButton((w-80)/2.0,h-30,70,25,1,this,"Okay"));
	/* Das Text-Widget hinzufuegen, das die Beschreibung erhalten soll */
	addChild(text=new CStaticText(20,65,"",this));

	/* Topic #1 setzen (Rules). */
	setTopic(1);
}

/**
 * Setzt den Text des Widgets auf das angegebene Thema.
 **/
void CHelpBox::setTopic(int topic)
{
	static const char* topic1=
		"RULES:\n\nGoal:\n"
		" - Try to lay down all of your stones\n\n"
		"Rules:\n"
		" - Place the first stone in your corner of the field\n"
		" - In turn every one places one stone, so that it\n"
		"   touches at least on of your stones corners, but\n"
		"   never shares an edge with them.\n"
		" - Stones of different colors may touch\n\n"
		"Finish:\n"
		" - When no player is able to place a further stone.";

	static const char* topic2=
		"MOUSE:\n\n - Left click on your stone to select\n"
		" - Left click on arrow to rotate\n"
		" - Left click with selected stone on field to\n"
		"   place the stone\n\n"
		" - Mouse Wheel to rotate current stone\n"
		" - Right click to mirror current stone over\n"
		"   the Y-Axis of the screen\n"
		" - Mouse Wheel with right button down:\n"
		"   \"scroll\" through available stones\n\n"
		" - Move mouse with right button down: rotate view\n"
		" - Move mouse up/down with middle button down:\n"
		"   zoom view.\n";

	static const char* topic3=
		"KEYBOARD:\n\n"
		" - Escape: Show Main Menu\n"
		" - Space:  Mirror current stone over the Y-Axis of\n"
		"           the screen.\n"
		" - Left:   Select previous stone in list\n"
		" - Right:  Select next stone in list\n"
		" - Up:     Rotate current stone to the left\n"
		" - Down:   Rotate current stone to the right\n"
		" - h:      Show hint\n"
		" - Enter:  Open multiplayer chatbox\n"
		" - F1:     Show this help\n";

	/* Den Text setzen */	
	if (topic==1)text->setText(topic1,true);
	if (topic==2)text->setText(topic2,true);
	if (topic==3)text->setText(topic3,true);

	/* Das aktuelle Thema soll deaktiviert sein, die anderen aktiviert. */
	t1->setEnabled(topic!=1);
	t2->setEnabled(topic!=2);
	t3->setEnabled(topic!=3);
}

/** 
 * Mausereignis verarbeiten; auf Themawechsel reagieren
 **/
int CHelpBox::processMouseEvent(TMouseEvent *e)
{
	int r=CDialog::processMouseEvent(e);
	switch (r)
	{
	case 1001: setTopic(1);
		break;
	case 1002: setTopic(2);
		break;
	case 1003: setTopic(3);
		break;
	default: return r;
	}
	return 0;
}

