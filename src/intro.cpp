/**
 * intro.cpp
 * Autor: Sascha Hlusiak
 * Implementation der Intro-Sequenz mit allem drum und dran
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <math.h>
#include "constants.h"
#include "window.h"
#include "gui.h"
#include "intro.h"
#include "stoneeffect.h"
#include "spielclient.h"

/* Waehrend der Animation soll das Feld 20x20 Felder betragen. */
const int FIELD_SIZE_X=20,FIELD_SIZE_Y=20;

/**
 * Alles vorbereiten
 **/
CIntro::CIntro(CGUI *vgui)
{
	gui=vgui;
	anim=0.0;
	/* Leere verkettete Liste der Effekte erzeugen */
	effects=new CStoneEffect();
	phase=0;
	field_up=false;
	field_anim=0.0;
	/* Alle Steine initialisieren. */
	init();
}

/**
 * Destruktor: Verkettete Liste wieder frei geben
 **/
CIntro::~CIntro()
{
	if (effects)delete effects;
}

/**
 * Steine initialisieren (Formen)
 * Und das Wort "blokus" vorbereiten
 **/
void CIntro::init()
{
	stones[0].init(5);		// XXX
	stones[0].rotate_left();	//   X

	stones[1].init(8);		// X
					// X
					// X
					// X

	stones[2].init(10);		// XX
					//  X
					// XX

	stones[3].init(12);		// X
					// X
					// XXX

	stones[4].init(1);		// X
					// X

	stones[5].init(20);		// X
					// X
					// X
					// X
					// X

	stones[6].init(5);		//  X
					//  X
					// XX
		
	stones[7].init(2);		// XX
	stones[7].rotate_left();	//  X
	stones[7].rotate_left();
	stones[8].init(0);		// X

	stones[9].init(3);		// X
					// X
					// X

	stones[10].init(10);		// X X
	stones[10].rotate_right();	// XXX

	stones[11].init(1);		// XX
	stones[11].rotate_right();

	stones[12].init(5);		//   X
	stones[12].rotate_left();	// XXX
	stones[12].mirror_over_x();

	stones[13].init(5);		// XXX
	stones[13].rotate_right();	// X
	stones[13].mirror_over_x();

	addChar('b',0,1,9);
	addChar('l',1,4,8);
	addChar('o',2,7,9);
	addChar('k',3,10,8);
	addChar('u',1,13,9);
	addChar('s',2,16,10);
}

/**
 * Einen fliegenden Buchstaben 'c' der Farbe color hinzufuegen, dass er an x/y landet (linke obere Ecke)
 * Dazu wird er in einzelne Blokus-Steine zerlegt und diese hinzugefuegt.
 **/
void CIntro::addChar(char c,int color,int x,int y)
{
	switch (c)
	{
	case 'a':
		add(5,color,x-2,y);
		add(2,color,x+1,y);
		add(4,color,x+1,y+3);
		break;
	case 'b':
		add(0,color,x,  y-1);
		add(1,color,x-2,y+1);
		add(2,color,x+1,y+2);
		break;
	case 'c':
		add(5,color,x-2,y);
		add(11,color,x+1,y-1);
		add(11,color,x+1,y+3);
		break;
	case 'e':
		add(11,color,x+1,y-1);
		add(4,color,x-1,y);
		add(3,color,x,y+2);
		add(8,color,x+1,y+2);
		break;
	case 'f':
		add(13,color,x,y-1);
		add(9,color,x-1,y+2);
		add(8,color,x+1,y+2);
		break;
	case 'l':
		add(4,color,x-1,y);
		add(3,color,x,y+2);
		break;

	case 'o':
		add(5,color,x-2,y);
		add(6,color,x+1,y+2);
		add(7,color,x+1,y);
		break;
	case 'h':
		add(6,color,x+1,y);
		add(5,color,x-2,y);
		add(4,color,x+1,y+3);
		break;
	case 'k':
		add(5,color,x-2,y);
		add(8,color,x+1,y+2);
		add(4,color,x+1,y);
		add(4,color,x+1,y+3);
		break;
	case 'n':
		add(5,color,x-2,y);
		add(5,color,x,y);
		add(4,color,x,y+2);
		break;

	case 'u':
		add(9,color, x-1,y);
		add(9,color, x+1,y);
		add(10,color,x  ,y+3);
		break;

	case 'r':
		add(0,color,x,y-1);
		add(1,color,x-2,y+1);
		add(8,color,x+1,y+2);
		add(4,color,x+1,y+3);
		break;	
	case 's':
		add(3,color, x,  y);
		add(11,color,x+1,y-1);
		add(12,color,x,  y+3);
		break;
	case 'x':
		add(4,color,x-1,y);
		add(4,color,x+1,y);
		add(4,color,x-1,y+3);
		add(4,color,x+1,y+3);
		add(8,color,x+1,y+2);
		break;
	case 'y':
		add(4,color,x-1,y);
		add(4,color,x+1,y);
		add(9,color,x,y+2);
		break;

 	default: printf("Falscher char uebergeben: %c.\n",c);
 		break;
	}
}

/**
 * Einen CPhysicalStone hinzufuegen, der an dx/dy landet
 * Startposition und Rotation ist zufaellig
 **/
void CIntro::add(int stone,int color,int dx,int dy)
{
	double x,y,z;
	/* Eine Rotationsachse berechnen */
	double angx=double(rand()%360)/180.0*M_PI;
	double angy=double(rand()%360)/180.0*M_PI;
	double axe_x=sin(angx)*cos(angy);
	double axe_y=sin(angy);
	double axe_z=cos(angx)*cos(angy);

	/* CPhysicalStone erstellen, aus stones[stone] */	
	CStone *st=&stones[stone];
	CPhysicalStone *s=new CPhysicalStone(gui,st,color);
	/* Lokale dx/dy des Feldes in globale Welt-Koordinaten umrechnen. */
	x=-(FIELD_SIZE_X-1)*stone_size+((double)dx+(double)st->get_stone_size()/2.0)*stone_size*2.0-stone_size;
	z=-(FIELD_SIZE_Y-1)*stone_size+((double)dy+(double)st->get_stone_size()/2.0)*stone_size*2.0-stone_size;
	/* Zufaellige Hoehe geben. */
	y=22.0+(double)(rand()%180)/10.0;

	/* Der Stein wird in <time> sek den Boden erreichen. 17 ist Gravitation*/
	double time=sqrt(2*y/17.0);
	/* x/z Koordinaten zufaellig verschieben */
	double xoffs=rand()%60-30.0;
	double zoffs=rand()%60-30.0;
	/* Position setzen */
	s->setPos(x+xoffs,y,z+zoffs);
	/* x/z Geschwindigkeit setzen, y Geschw. ist 0 */
	s->setSpeed(-xoffs/time,0,-zoffs/time);
	/* Gewuenschtes Ziel in Stein speichern */
	s->setDestination(x,bevel_height,z);
	/* Stein dreht sich exakt um 360 Grad in <time> sek. */
	s->setRotationSpeed(360.0/time,axe_x,axe_y,axe_z);
	/* Effekt der verketteten Liste hinzufuegen. */
	effects->add(s);
}

/* Geschw. in der das Feld auf-wippen soll */
const double wipe_speed=6.8;
/* Winkel, bis zu dem das Feld gekippt werden soll */
const double wipe_ang=32.0;

/**
 * Das Feld kippen und alle Steine entlang der Radialgeschwindigkeit beschleunigen
 **/
void CIntro::wipe()
{
	/* Zu Beginn das Feld hoch klappen */
	field_up=true;
	field_anim=0.0;
	/* Komplette verkettete Liste durchgehen und fuer jeden enthaltenen CPhysicalStone...*/
	CPhysicalStone *s=(CPhysicalStone*)effects->next;
	while (s)
	{
		/* ...Geschwindigkeit setzen, dass die Steine tangential zur Drehung des 
		   Felds wegfliegen */
		/* Winkel, in dem die Steine beschleunigt werden */
		const double ang=wipe_ang/180.0*M_PI;
		/* Radialgeschwindigkeit errechnen. */
		const double v=(ang*wipe_speed)*(s->getZ()-FIELD_SIZE_X*stone_size)-(double)(rand()%100)/10.0-8.0;
		/* Stein nur leicht rotieren lassen, und nicht ganz zufaellig */
		const double a1=0.95;
		const double a2=((rand()%2)?1:-1)*sqrt((1.0-a1*a1)/2.0);
		const double a3=((rand()%2)?1:-1)*sqrt((1.0-a1*a1)/2.0);
		/* Stein hauptsaechlich in Richtung der Felddrehung rotieren lassen */
		s->setRotationSpeed(wipe_ang*wipe_speed+(double)(rand()%100)/15.0,a1,a2,a3);
		/* Geschwindigkeit und Winkel in Kartesische Koordinaten umrechnen */
		s->setSpeed((double)(rand()%100)/20.0,cos(ang)*v,-sin(ang)*v);
		/* Stein soll kein Ziel mehr haben, d.h. er faellt unendlich tief */
		s->unsetDestination();
		/* Und naechsten Stein aus verketteter Liste schnappen */
		s=(CPhysicalStone*)s->next;
	}
}

/* Die Matrix Animation startet bei matrix_start, stoppt bei matrix_stop,
   und wird um matrix_duration_start langsam eingeblendet */
const double matrix_start=1.56;
const double matrix_stop=6.0;
const double matrix_duration_start=0.25;
const double matrix_duration_stop=0.25;

/**
 * Alles animieren. return true, wenn die Intro-Sequenz vorrueber ist.
 **/
bool CIntro::execute(double elapsed)
{
	elapsed*=1.2;
	anim+=elapsed;

	if (field_up || field_anim>0.000001)
	{
		if (field_up)
		{
			/* Feld rauf-animieren. */
			field_anim+=elapsed*wipe_speed;
			/* Wenn oben, Animation umkehren */
			if (field_anim>1.0)
			{
				field_anim=1.0;
				field_up=false;
			}
		}else{
			/* Runter animieren. */
			field_anim-=elapsed*2.5;
			if (field_anim<0.0)field_anim=0.0;
		}
	}
	if (phase==0)
	{
		/* In phase 0 kommt ein Matrix-Mode zwischen die fliegenden Steine */
		if (anim<matrix_start+matrix_duration_start)
		{
			if (anim<matrix_start)effects->execute(elapsed);
			else effects->execute(elapsed*(matrix_duration_start-anim+matrix_start)/matrix_duration_start);
		}
		if (anim>matrix_stop)
		{
			if (anim>matrix_stop+matrix_duration_stop)effects->execute(elapsed);
			else effects->execute(elapsed*(anim-matrix_stop)/matrix_duration_stop);
		}
		if (anim>10.5)
		{
			/* Nach 10.5 Zeiteinheiten Feld leeren und Phase auf 1 */
			phase=1;
			wipe();
		}
	}else{
		/* Effekte animieren */
		effects->execute(elapsed);
		/* Bei den Phasen fallen Steine vom Himmel, diese sollen zuegig fallen */
		if (phase==2 || phase==4 || phase==5 || phase==6)
			effects->execute(elapsed*0.7);
		/* Jede Phase dauert 12 Zeiteinheiten */
		if (anim>12.0)
		{
			/* Neue Phase und entweder Feld leeren */
			phase++;
			if (phase==3)
			{
				anim=10.8;
				wipe();
			}
			if (phase==7)
			{
				anim=9.5;
				wipe();
			}
			/* Oder neue Steine regnen lassen. */
			if (phase==2)
			{
				anim=9.1;
				/* Alle Steine entfernen */
				effects->clear();
				addChar('b',-1,6,9);
				addChar('y',-1,10,9);
			}
			if (phase==4)
			{
				effects->clear();
				anim=10.2;
				addChar('a',0,4,1);
				addChar('l',1,7,1);
				addChar('e',2,10,1);
				addChar('x',3,13,1);
			}
			if (phase==5)
			{
				anim=10.2;
				addChar('f',0,3,7);
				addChar('r',1,6,7);
				addChar('a',2,9,7);
				addChar('n',3,12,7);
				addChar('k',0,15,7);
			}
			if (phase==6)
			{
				anim=8.5;
				addChar('s',0,1,13);
				addChar('a',2,4,13);
				addChar('s',3,7,13);
				addChar('c',2,10,13);
				addChar('h',1,13,13);
				addChar('a',0,16,13);
			}
			/* Nach der 7. Phase ist das Intro vorrueber */
			if (phase==8)return true;
		}
	}
	return false;
}

/**
 * Intro rendern
 **/
void CIntro::render()
{
	/* Kaera ist 30 Einheiten vom Feld entfernt */
	const double distance=30.0;
	glLoadIdentity();
	/* Kamera positionieren */
	glTranslated(0,4,0);
	gluLookAt(0,
		0,
		distance,
		0,0,0,
		0,1,0);

	/* Kamera drehen, evtl. durch Matrix move */
	glRotated(60,1,0,0);
	const double winkel1=180.0;
	const double winkel2=-70.0;
	const double matrix_anim=(sin((anim-matrix_start)/(matrix_stop-matrix_start)*M_PI-M_PI/2.0)/2.0+0.5);
	if (anim<matrix_start)
	{
		glRotated(winkel2,1,0,0);
		glRotated(winkel1,0,1,0);
	}
	else if (anim<matrix_stop)
	{
		glRotated(winkel2-(matrix_anim*matrix_anim)*winkel2,1,0,0);
		glRotated(winkel1-matrix_anim*winkel1,0,1,0);
	}

	if (anim<matrix_start)   glTranslated(0,-17+(anim/matrix_start)*4.0,0);
		else if (anim<matrix_stop)
	{
 		glTranslated(0,-13+13*(matrix_anim*matrix_anim),0);
	}

	/* Licht setzen der neuen Kameraposition anpassen*/
	glLightfv(GL_LIGHT0,GL_POSITION,light0_pos);

	/* Umgebung und Feld rendern. */
	gui->renderGround();
	glPushMatrix();
	if (field_anim>0.0001)
	{
		glTranslated(0,0,FIELD_SIZE_X*stone_size);
		glRotated(field_anim*wipe_ang,1,0,0);
		glTranslated(0,0,-FIELD_SIZE_X*stone_size);
	}
	gui->renderField(false,FIELD_SIZE_X,FIELD_SIZE_Y);
	glPopMatrix();
	/* Alle Steine rendern. */
	effects->render();
}

/**
 * Schatten im Intro rendern.
 **/
void CIntro::renderShadow()
{
	/* Schatten der Steine rendern. */
	effects->renderShadow();
}
