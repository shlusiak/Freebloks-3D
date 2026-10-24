/**
 * about.cpp
 * Author: Sascha Hlusiak
 * CAboutBox dialog and CAboutWidget widget for the About dialog.
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
 * The content is provided by this separate widget, a descendant of CFrame
 **/
class CAboutWidget:public CFrame
{
private:
	/* Progress of the animation */
	double anim;
public:
	CAboutWidget(double x,double y,double w,double h);
	/* Animate */
	virtual void execute(double elapsed);
	virtual void render(bool selection);
};

/* The text lines of the About box as an array of char*.
   nullptr entries are empty lines. The lines are rendered centred. */
static const char *AboutText[]={
	"Freebloks 3D",
	"(v" VERSION ", build: " __DATE__ ")",
	0,
	0,
	"This game was created",
	"during a practical training",
	"at the FH-Muenster, Germany.",
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



CAboutWidget::CAboutWidget(double x,double y,double w,double h)
:CFrame(x,y,w,h)
{
	setColor(0,0,0);
	/* It fades in slowly, so start with alpha 0. */
	setAlpha(0.0);
	anim=0.0;
}

/**
 * Run the animation.
 **/
void CAboutWidget::execute(double elapsed)
{
	/* Animate. */
	anim+=elapsed*8.0;
	/* Fade the widget in slowly, i.e. animate the alpha. */
	if (anim<12.0)setAlpha(0.6*anim/12.0);else setAlpha(0.6);
	CFrame::execute(elapsed);
}

void CAboutWidget::render(bool selection)
{
	CFrame::render(selection);
	if (!selection)
	{
		unsigned int i;
		CGLFont font(8,20);
		/* We want a rotating cylinder; this is its radius. */
		const double radius=h/2.1;
		/* After each line, rotate the cylinder by this angle: */
		const double da=90.0/((M_PI/2.0*radius)/(double)font.height());
		double ang;

		/* Back faces should be visible too. */
		glDisable(GL_CULL_FACE);
		glPushMatrix();
		/* Move the cylinder to the centre of the widget and align it. */
		glTranslated(x+w/2.0,y+h/2.0,radius);
		glRotated(20.0,0,1,0);
		ang=-100.0+anim;
		glRotated(ang,-1,0,0);
		for (i=0;i<sizeof(AboutText)/sizeof(AboutText[0]);i++)
		{
			/* <90 is the front side, so 100%, otherwise 30% opacity */
			if (ang<90.0)glColor3d(1,1,1);
				else glColor3d(0.3,0.3,0.3);
			/* Only render while within the first rotation. */
			if (AboutText[i] && ang>-90.0 && ang<270.0)
				font.drawText(-font.width(AboutText[i])/2.0,-font.height()/2.0,-radius,AboutText[i]);
			/* Keep rotating the cylinder. */
			ang-=da;
			glRotated(da,1,0,0);
		}
		/* When the last line passes 270, start over from the beginning. */
		if (ang>270.0)anim=15.0;
		/* Reset GL */
		glPopMatrix();
		glEnable(GL_CULL_FACE);
	}
}


CAboutBox::CAboutBox()
:CDialog(400,300,"About Freebloks")
{
	/* Set a fancy colour */
	setColor(0.7,0.2,0.4);
	addChild(new CButton((w-70)/2,h-30,70,25,1,this,"Okay"));
	addChild(new CAboutWidget(x+10,y+30,w-20,h-70));
}

