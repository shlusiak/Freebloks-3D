/**
 * about.cpp
 * Autor: Sascha Hlusiak
 * CAboutBox Dialog und CAboutWidget Widget, fuer den About-Dialog (ach).
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <math.h>
#include "widgets.h"
#include "about.h"
#include "glfont.h"
#include "constants.h"


/**
 * Die Daten werden in diesem extra Widget bereitgestellt, Nachfahre von CFrame
 **/
class CAboutWidget:public CFrame
{
private:
	/* Fortschritt der Animation */
	double anim;
public:
	/* Alles einrichten. */
	CAboutWidget(double x,double y,double w,double h);
	/* Animieren */
	virtual void execute(double elapsed);
	/* Und Widget rendern */
	virtual void render(bool selection);
};

/* Hier die Textzeilen der About-Box als Array of char*.
   NULL sind Leerzeilen. Die Texte werden zentriert ausgegeben. */
static const char *AboutText[]={
	"Freebloks 3D",
	"(Build: "__DATE__")",
	0,
	0,
	"This game was created",
	"during a practical training",
	"at the FH-Münster, Germany.",
	0,0,0,0,0,0,
	"Sascha Hlusiak:          ",
	"- OpenGL graphics        ",
	"- Network mode           ",
	"- Linux Port             ",
	0,0,
	"Frank Stolze:            ",
	"- Game logic             ",
	"- Artificial Intelligence",
	0,0,
	"Alex Besstschastnich:    ",
	"- Menu Design            ",
	"- Installer              ",
	"- Graphics               ",
	"- Testing                ",
	"- Autorun                ",
	0,0,0,0,0,0,

	"12 weeks of coding,",
	"more than 12000 lines of code!",0,0,
	":-)",
	0,0,0,0,0,0,
	"- THE END -",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"really",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"believe me",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"what are you waiting for?",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"Press Okay!",
	0,0,0,0,0,0,0,0,0,0,
	"NOW!",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"Need an invitation?",
	0,0,0,0,0,0,0,0,0,0,0,0,0,
	"You definitely HAVE time to waste",
	0,0,0,0,0,0,0,0,0,0,0,
	"Press any key to continue",
	"Press any other key to abort",
	0,0,"(joke, laugh here)",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"Are you expecting",
	"something new here?",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"1+2=3",
	0,0,0,0,0,0,
	":-)",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"Come on, it is not THAT",
	"hard to press okay",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"Still watching the credits?",
	0,0,0,0,
	"WAKE UP!",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"42",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"Come on, play FREEBLOKS!",
	0,"'couse that's the purpose","of the game",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"One thing left to make you","close this dialog:",
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	"The credits start again. ;)",
	0,
	":->",
};



/**
 * Das Widget initialisieren.
 **/
CAboutWidget::CAboutWidget(double x,double y,double w,double h)
:CFrame(x,y,w,h)
{
	/* Farbe ist schwarz. */
	setColor(0,0,0);
	/* Und fadet langsam rein, also Alpha bei 0 beginnen lassen. */
	setAlpha(0.0);
	/* Animation startet bei 0 */
	anim=0.0;
}

/**
 * Animation ausfuehren.
 **/
void CAboutWidget::execute(double elapsed)
{
	/* Animieren. */
	anim+=elapsed*8.0;
	/* Widget langsam einblenden, also Alpha animieren. */
	if (anim<12.0)setAlpha(0.6*anim/12.0);else setAlpha(0.6);
	/* Und andere Widgets auch animieren. */
	CFrame::execute(elapsed);
}

/**
 * Das Widget rendern.
 **/
void CAboutWidget::render(bool selection)
{
	/* Frame und andere Unterelemente Rendern */
	CFrame::render(selection);
	if (!selection)
	{
		unsigned int i;
		/* Wir wollen eine 8x20 Schrift. */
		CGLFont font(8,20);
		/* Wir wollen einen drehenden Zylinder, das ist der Radius. */
		const double radius=h/2.1;
		/* Nach jeder Zeile den Zylinder um folgenden Winkel drehen: */
		const double da=90.0/((M_PI/2.0*radius)/(double)font.height());
		double ang;

		/* Und Flaechen-Rueckseiten sollen sichtbar sein. */
		glDisable(GL_CULL_FACE);
		glPushMatrix();
		/* Zylinder in Mitte des Widgets verschieben und ausrichten. */
		glTranslated(x+w/2.0,y+h/2.0,radius);
		glRotated(20.0,0,1,0);
		/* Wir starten bei -105 Grad */
		ang=-100.0+anim;
		glRotated(ang,-1,0,0);
		for (i=0;i<sizeof(AboutText)/sizeof(AboutText[0]);i++)
		{
			/* <90 ist Vorderseite, also 100%, sonst 30% Opazitaet */
			if (ang<90.0)glColor3d(1,1,1);
				else glColor3d(0.3,0.3,0.3);
			/* Nur rendern, wenn innerhalb der ersten Drehung. */
			if (AboutText[i] && ang>-90.0 && ang<270.0)
				font.drawText(-font.width(AboutText[i])/2.0,-font.height()/2.0,-radius,AboutText[i]);
			/* Zylinder weiter drehen. */
			ang-=da;
			glRotated(da,1,0,0);
		}
		/* Wenn die letzte Zeile 270 ueberschreitet, wieder von vorn anfangen. */
		if (ang>270.0)anim=15.0;
		/* GL zuruecksetzen */
		glPopMatrix();
		glEnable(GL_CULL_FACE);
	}
}


/**
 * AboutBox Konstruktor.
 **/
CAboutBox::CAboutBox()
:CDialog(400,300,"About Freebloks")
{
	/* Tuffige Farbe setzen */
	setColor(0.7,0.2,0.4);
	/* Und Close-Button sowie AboutWidget einfuegen. */
	addChild(new CButton((w-70)/2,h-30,70,25,1,this,"Okay"));
	addChild(new CAboutWidget(x+10,y+30,w-20,h-70));
}

