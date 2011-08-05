/**
 * stoneeffect.cpp
 * Autor: Sascha Hlusiak
 * Effekte fuer wirbelnde Steine und so.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include "timer.h"
#include "stoneeffect.h"
#include "gui.h"
#include "spielclient.h"
#include "constants.h"
#include "turn.h"


/**
 * Einen leeren CStoneEffect erzeugen, der nix kann. z.B. als Kopf einer Liste.
 **/
CStoneEffect::CStoneEffect()
{
	gui=0;
	next=0;
	x=y=0;
	player=0;
}

/**
 * StoneEffect mit CStone, player, x und y erzeugen. */
CStoneEffect::CStoneEffect(CGUI *vgui,CStone *vstone,int vplayer,int vx,int vy)
{
	x=vx;
	y=vy;
	next=0;
	player=vplayer;
	gui=vgui;

	/* Den stone kopieren und uebernehmen */
	stone=*vstone;
}

/**
 * StoneEffect aus einem CTurn und player erstellen.
 **/
CStoneEffect::CStoneEffect(CGUI *vgui,CTurn *turn,int vplayer)
{
	x=turn->get_x();
	y=turn->get_y();
	next=0;
	player=vplayer;
	gui=vgui;
	/* Stone kopieren und Daten aus CTurn uebernehmen */
	stone.init(turn->get_stone_number());
	stone.mirror_rotate_to(turn->get_mirror_count(),turn->get_rotate_count());
}

/**
 * Aufraeumen
 **/
CStoneEffect::~CStoneEffect()
{
	/* Wenn Nachfolger existieren, diese mit entfernen (rekursiv) */
	if (next)delete next;
	next=0;
}

/**
 * Verkettete Liste hier abschneiden, alle nachfolgenden CStoneEffects entfernen (rekursiv)
 **/
void CStoneEffect::clear()
{
	if (next)delete next;
	next=0;
}

/**
 * Stein animieren. true zurueckgeben, wenn Effekt entfernt werden will.
 **/
bool CStoneEffect::execute(double elapsed)
{
	/* Wenn Nachfolger entfernt werden will */
	if (next) if (next->execute(elapsed))
	{
		/* Stein aus Liste aushaengen. */
		CStoneEffect *n=next;
		next=next->next;
		n->next=0;
		/* Und loeschen. */
		delete n;
	}
	/* Wir wollen nicht sterben */
	return false;
}

/**
 * true, wenn das Feld x/y von diesem Effekt behandelt wird.
 **/
bool CStoneEffect::is_effected(int x,int y)
{
	/* Wenn irgendein Effekt das Feld x/y behandelt: true. */
	if (next)if (next->is_effected(x,y))return true;
	/* Wenn garkein richtiger Effekt (stone==0): false */
	//if (stone==0)return false; /* FIXME */
	if (gui==0)return false;
	y-=this->y;
	x-=this->x;
	/* Wenn x/y ausserhalb unserer Grenzen des Steins: false */
	if (x<0 || y<0 || x>=stone.get_stone_size() || y>=stone.get_stone_size())return false;

	/* Wenn unser Stein an der errechneten Stelle ein Element hat: true */
	if (stone.get_stone_field(y,x)!=STONE_FIELD_FREE)return true;
	/* Sonst: false */
	return false;
}


/**
 * Ein simpler Fade-Effekt. Zeit mit 0 initialisieren, mehr brauchts nicht.
 **/
CStoneFadeEffect::CStoneFadeEffect(CGUI *vgui,CStone *vstone,int vplayer,int vx,int vy)
:CStoneEffect(vgui,vstone,vplayer,vx,vy)
{
	time=0.0;
}

CStoneFadeEffect::CStoneFadeEffect(CGUI *vgui,CTurn *turn,int vplayer)
:CStoneEffect(vgui,turn,vplayer)
{
	time=0.0;
}

/**
 * Animieren.
 **/
bool CStoneFadeEffect::execute(double elapsed)
{
	/* Andere Effekte animieren. */
	CStoneEffect::execute(elapsed);
	/* Zeit erhoehen, und nach 3 sek soll der Effekt entfernt werden wollen. */
	time+=elapsed;
	return (time>3.0);
}

/**
 * Einfach rendern.
 **/
void CStoneFadeEffect::render()
{
	int i,j;
	/* Eine weiche Animation des Alpha-Kanals errechnen. */
	const double alpha=(sin(time/3.0*M_PI*8.5)+1.0)*0.5*0.45+0.2;
	/* Andere Effekte rendern. */
	CStoneEffect::render();
	/* Und Stein an entsprechende Position auf dem Spielfeld rendern. */
	for (i=0;i<stone.get_stone_size();i++)
	for (j=0;j<stone.get_stone_size();j++)
		if (stone.get_stone_field(j,i)!=STONE_FIELD_FREE)
	{
		gui->renderStone(i+x,j+y,player,(float)alpha,false);
	}
}

/**
 * Wirbelnder Stein. Animation beim Legen eines Steins von einem Spieler.
 * Benoetigt Spielernummer, Stein, Stein-Nummer, Position
 * bei vreverse==true fliegt der Stein vom Spielfeld zum Spieler, sonst vom Spieler aufs Feld
 **/
CStoneRollEffect::CStoneRollEffect(CGUI *vgui, CStone *vstone,int stone_number,int vplayer,int vx,int vy,bool vreverse)
:CStoneEffect(vgui,vstone,vplayer,vx,vy)
{
	/* Alles brav initialisieren. */
	reverse=vreverse;
	if (reverse)time=1.0;
	else time=0.0;
	/* Der Stein startet bei einer Skalierung von 0.7 */
	scale=0.70;
	ang=0.0;
	/* Die Position des entsprechenden Steins des Spielers in Welt-Koordinaten erhalten.
	   Das ist unsere Startposition. */
	gui->getPlayerStonePos(vplayer,stone_number,&cx,&cy,&cz);
	
	/* Die Zielposition auf dem Feld in Welt-Koordinaten berechnen. */
	dx=-(gui->spiel->get_field_size_x()-1)*stone_size+((double)x+(double)stone.get_stone_size()/2.0)*stone_size*2.0-stone_size;
	dy=bevel_height;
	dz=-(gui->spiel->get_field_size_y()-1)*stone_size+((double)y+(double)stone.get_stone_size()/2.0)*stone_size*2.0-stone_size;

	{
		/* Eine zufaellige Rotationsachse berechnen. */
		double angx=double(rand()%360)/180.0*M_PI;
		double angy=double(rand()%360)/180.0*M_PI;
		axe_x=sin(angx)*cos(angy);
		axe_y=sin(angy);
		axe_z=cos(angx)*cos(angy);
	}
	/* Gravitation ist -1/2*80 */
	parabel_a=-40.0;
	/* Daraus das b der Parabel berechnen, dass die Parabel genau durch Start und Ziel geht */
	parabel_b=-parabel_a+dy-cy;
}

/**
 * Animieren
 **/
bool CStoneRollEffect::execute(double elapsed)
{
	/* Stein soll sich stets um 2*360 Grad rotieren */
	const double loops=2.0;
	/* Animation soll 1.0 sek dauern. */
	const double duration=1.0;
	CStoneEffect::execute(elapsed);

	/* Zeit entweder vorwaerts oder rueckwaerts zaehlen */
	if (reverse)time-=(elapsed/duration);
	else time+=(elapsed/duration);
	/* Rotation auf passenden Winkel zur Zeit setzen. */
	ang=(time)*360.0*loops;
	/* Wenn Animation ausserhalb 0 und 1, ist sie vorrueber. */
	return (time<0.0)||(time>1.0);
}

/**
 * Stein an antsprechender Stelle der Parabel rendern.
 **/
void CStoneRollEffect::render()
{
	int i,j;
	/* Die Skalierung des Steins errechnen und einpassen. */
	double s=scale+(1.0-scale)*(time*2.5);
	if (s>1.0)s=1.0;

	/* Aktuelle Position entlang der Parabel aus der Zeit errechnen. */
	const double x=cx+time*(dx-cx);
	const double y=parabel_a*time*time+parabel_b*time+cy;
	const double z=cz+time*(dz-cz);
	
	CStoneEffect::render();
	/* Position und Rotation setzen. */
	glPushMatrix();
	glTranslated(x,y,z);
	glScaled(s,s,s);
	glRotated(ang,axe_x,axe_y,axe_z);

	/* Und einfach Stein rendern. */
	for (i=0;i<stone.get_stone_size();i++)
	for (j=0;j<stone.get_stone_size();j++)
	if (stone.get_stone_field(j,i)!=STONE_FIELD_FREE)
	{
		glPushMatrix();
 		glTranslated(+stone_size+((double)i-(double)stone.get_stone_size()/2.0)*stone_size*2.0,
			0,
			     +stone_size+((double)j-(double)stone.get_stone_size()/2.0)*stone_size*2.0);
		gui->renderStone(player,0.65f,false);
		glPopMatrix();
	}
	glPopMatrix();
}

/**
 * Schatten rendern. Dazu Stein an Position bringen, Richtung des Lichts errechnen,
 * Stein an Lichtstrahl extrudieren und in Stencil Buffer rendern.
 */
void CStoneRollEffect::renderShadow()
{
	int i,j;
	/* Skalierung ausrechnen. */
	double s=scale+(1.0-scale)*(time*3.5);
	if (s>1.0)s=1.0;
	/* Aktuelle Position des Steins errechnen. */
	const double x=cx+time*(dx-cx);
	const double y=parabel_a*time*time+parabel_b*time+cy;
	const double z=cz+time*(dz-cz);
	
	CStoneEffect::renderShadow();

	float dir[3];
	float M[16];
	/* Hier kommt unser Licht her. */
	float *v=(float*)&light0_pos;

	glPushMatrix();
	glLoadIdentity();
	/* Stein rueckwaerts rotieren. */
  	glRotated(-ang,axe_x,axe_y,axe_z);
	glGetFloatv(GL_MODELVIEW_MATRIX,M);
	/* Und resultierenden Richtungsvektor extrahieren. Die Richtung ist relativ zum gedrehten
	   Stein, zeigt also global immer in dieselbe Richtung. */
	dir[0]=-(M[ 0]*v[0]+M[ 4]*v[1]+M[ 8]*v[2]);
	dir[1]=-(M[ 1]*v[0]+M[ 5]*v[1]+M[ 9]*v[2]);
	dir[2]=-(M[ 2]*v[0]+M[ 6]*v[1]+M[10]*v[2]);

	/* In Position kommen. */
	glPopMatrix();
	glPushMatrix();
	glTranslated(x,y,z);
	glScaled(s,s,s);
	glRotated(ang,axe_x,axe_y,axe_z);

	for (i=0;i<stone.get_stone_size();i++)
	for (j=0;j<stone.get_stone_size();j++)
	if (stone.get_stone_field(j,i)!=STONE_FIELD_FREE)
	{
		glPushMatrix();
 		glTranslated(+stone_size+((double)i-(double)stone.get_stone_size()/2.0)*stone_size*2.0,
			0,
			     +stone_size+((double)j-(double)stone.get_stone_size()/2.0)*stone_size*2.0);
		/* Und den Schatten eines kleinen Steins entlang der Richtung des Lichts berechnen. */
		gui->renderStoneShadow(dir);
		glPopMatrix();
	}
	glPopMatrix();
}

/**
 * Behandelt dieser Effekt einen Stein des Spielers?
 * Nur wenn er rueckwaerts fliegt.
 **/
bool CStoneRollEffect::handle_player_stone(int player,int n_stone)
{
	if (reverse && this->player==player && stone.get_stone_shape()==n_stone)return true;
	return CStoneEffect::handle_player_stone(player,n_stone);
}



/**
 * Ein Stein mit physikalischen Eigenschaften. Wird im Intro bis zum Umfallen verwendet.
 * Konstruktor: Alles mit 0 initialisieren, wird nachher ueber Memberfunktionen gefuettert.
 **/
CPhysicalStone::CPhysicalStone(CGUI *vgui,CStone *vstone,int player)
:CStoneEffect(vgui,vstone,player,0,0)
{
	x=y=z=0.0;
	ang=0.0;
	angspeed=0.0;
	ax=az=speedx=speedy=speedz=0.0;
	ay=1.0;
}

/**
 * Aktuelle Position des Steins in Welt-Koordinaten setzen.
 **/
void CPhysicalStone::setPos(double sx,double sy,double sz)
{
	x=sx;
	y=sy;
	z=sz;
}

/**
 * Aktuelle Rotationsgeschwindigkeit und Achse setzen
 **/
void CPhysicalStone::setRotationSpeed(double angs,double ax,double ay,double az)
{
	angspeed=angs;
	this->ax=ax;
	this->ay=ay;
	this->az=az;
}

/**
 * Aktuelle Geschwindigkeit des Steins setzen
 **/
void CPhysicalStone::setSpeed(double sx,double sy,double sz)
{
	speedx=sx;
	speedy=sy;
	speedz=sz;
}

/**
 * Die Zielkoordinate des Steins setzen.
 * Sobald die (Y-Koordinate<=desty) wird, wird der Stein auf dx/dy/dz gesetzt, um
 * Ungenauigkeiten in der Animation zu beheben.
 **/
void CPhysicalStone::setDestination(double destx,double desty,double destz)
{
	dx=destx;
	dy=desty;
	dz=destz;
}

/**
 * Animieren.
 **/
bool CPhysicalStone::execute(double elapsed)
{
	CStoneEffect::execute(elapsed);
	/* Gradlinige Bewegung, einfach zu animieren. */
	x+=speedx*elapsed;
	y-=speedy*elapsed;
	z+=speedz*elapsed;
	/* Konstante Winkelgeschwindigkeit bei Rotation. Linear animieren. */
	ang+=elapsed*angspeed;
	/* wenn y unterhalb von dy gefallen ist, Stein auf dx/dy/dz setzen. */
	if (y<dy && dy>-100.0)
	{
		/* Stein soll wieder nach oben huepfen, aber mit gedaempfter vertikaler Geschw. */
		if (speedy>0.5)speedy=-speedy*0.32;else speedy=0.0;
		/* Soll nicht mehr rotieren, und soll genau parallel zur Erde sein. */
		angspeed=0.0;
		ang=0.0;
		/* x und y Geschw. auf 0 setzen. */
		speedx=0.0;
		speedz=0.0;
		/* Position auf Ziel setzen. */
		x=dx;
		y=dy;
		z=dz;
	}
	/* Stein vertikal beschleunigen (Gravitation). */
	speedy+=elapsed*17.0;
	/* Der Stein will nie entfernt werden, verschwindet nie automatisch. */
	return false;
}

/**
 * Rendern.
 **/
void CPhysicalStone::render()
{
	CStoneEffect::render();

	int i,j;

	/* Stein in Position bringen, und normal rendern, wie alle anderen auch. */
	glPushMatrix();
	glTranslated(x,y,z);
	glRotated(ang,ax,ay,az);

	for (i=0;i<stone.get_stone_size();i++)
	for (j=0;j<stone.get_stone_size();j++)
	if (stone.get_stone_field(j,i)!=STONE_FIELD_FREE)
	{
		glPushMatrix();
 		glTranslated(+stone_size+((double)i-(double)stone.get_stone_size()/2.0)*stone_size*2.0,
			0,
			     +stone_size+((double)j-(double)stone.get_stone_size()/2.0)*stone_size*2.0);
		gui->renderStone(player,0.65f,false);
		glPopMatrix();
	}
	glPopMatrix();
}

/**
 * Schatten des Steins rendern.
 **/
void CPhysicalStone::renderShadow()
{
	CStoneEffect::renderShadow();

	int i,j;
	float dir[3];
	float M[16];
	float *v=(float*)&light0_pos;
	
	glPushMatrix();
	glLoadIdentity();
	/* Rueckwaerts rotieren, um Richtung des Lichts relativ zum Stein zu erhalten. */
	glRotated(-ang,ax,ay,az);
	glGetFloatv(GL_MODELVIEW_MATRIX,M);
	dir[0]=-(M[ 0]*v[0]+M[ 4]*v[1]+M[ 8]*v[2]);
	dir[1]=-(M[ 1]*v[0]+M[ 5]*v[1]+M[ 9]*v[2]);
	dir[2]=-(M[ 2]*v[0]+M[ 6]*v[1]+M[10]*v[2]);

	/* Stein in Position bringen. */
	glPopMatrix();
	glPushMatrix();
	glTranslated(x,y,z);
	glRotated(ang,ax,ay,az);

	for (i=0;i<stone.get_stone_size();i++)
	for (j=0;j<stone.get_stone_size();j++)
	if (stone.get_stone_field(j,i)!=STONE_FIELD_FREE)
	{
		glPushMatrix();
 		glTranslated(+stone_size+((double)i-(double)stone.get_stone_size()/2.0)*stone_size*2.0,
			0,
			     +stone_size+((double)j-(double)stone.get_stone_size()/2.0)*stone_size*2.0);
		/* Und extrudiertes Schatten-Volume in Licht-Richtung rendern. */
		gui->renderStoneShadow(dir);
		glPopMatrix();
	}
	glPopMatrix();
}

