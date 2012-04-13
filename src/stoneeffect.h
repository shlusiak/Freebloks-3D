/**
 * stoneeffect.h
 * Autor: Sascha Hlusiak
 *
 * Effekte, von ganzen Steinen, einblenden, wirbeln, usw.
 * Wird im Spiel als Animation, sowie im Intro gebraucht
 **/

#ifndef __STONE_EFFECT_H_INCLUDED_
#define __STONE_EFFECT_H_INCLUDED_

#include "timer.h"
#include "stone.h"

class CGUI;
class CTurn;

/**
 * Eine abstrakte StoneEffect Klasse. Bietet eine verkettete Liste von StoneEffects
 **/
class CStoneEffect
{
protected:
	/* Koordinate auf dem Spielfeld, wo der Stein hingelegt wurde */
	int x,y;
	/* GUI zum Rendern der Steine */
	CGUI *gui;
	/* Der eigentliche Stein, auf den sich die Animation bezieht. */
	CStone *stone;
	/* Und Spielernummer, dem der Stein gehoert. */
	int player;
public:
	/* Zeiger auf naechsten Effekt (verkettete Liste), oder NULL, wenn Ende. */
	CStoneEffect *next;

	/* Konstruktor, der wirklich leeren Effekt erstellt (fuer Kopf der Liste) */
	CStoneEffect();
	/* Diverse Konstruktoren fuer verschiedene Parameter. */
	CStoneEffect(CGUI *vgui,CStone *vstone,int vplayer,int vx,int vy);
	CStoneEffect(CGUI *vgui,CTurn *turn,int vplayer);
	/* ggf. aufraeumen, wenn notwendig. Liste rekursiv entfernen. */
	virtual ~CStoneEffect();

	/* Alle Effekte unterhalb this in der Liste entfernen. */
	void clear();
	/* Rekursiv einen Effekt anhaengen. */
	void add(CStoneEffect* eff) { if (next)next->add(eff); else next=eff; }
	/* Effekt animieren. true zurueckgeben, wenn Effekt entfernt werden will */
	virtual bool execute(double elapsed);
	/* Effekt rendern. */
	virtual void render() { if (next) next->render(); }
	/* Schatten des Effekts rendern, wenn vorhanden. */
	virtual void renderShadow() { if (next) next->renderShadow(); }
	/* true, wenn x/y auf dem Spielfeld zu diesem Effekt gehoeren. Dann wird das Element
	   auf dem Feld nicht angezeigt, da dies der Effekt hier tut. */
	virtual bool is_effected(int x,int y);
	/* true, wenn der Effekt zu einem bestimmten Stein eines bestimmten Spielers gehoert. 
	   wenn ja, wird der Stein des Spielers nicht angezeigt. */
	virtual bool handle_player_stone(int player,int stone) { if (next)return next->handle_player_stone(player,stone); else return false; }
	/* true, wenn der Effekt einen Schatten rendern wuerde. */
	virtual bool hasShadow() { if (next)return next->hasShadow(); else return false; }
};

/**
 * Dieser Effekt laesst einen Stein auf dem Spielfeld auf und ab blinken.
 * Wird vom Zugvorschlag benutzt.
 **/
class CStoneFadeEffect:public CStoneEffect
{
private:
	/* Animationszeit. */
	double time;
public:
	CStoneFadeEffect(CGUI* vgui,CStone *vstone,int vplayer,int vx,int vy);
	CStoneFadeEffect(CGUI* vgui,CTurn *turn,int vplayer);
	
	/* Animieren */
	virtual bool execute(double elapsed);
	/* Rendern */
	virtual void render();
};

/**
 * Dieser Effekt laesst einen Spielerstein von seinem Platz vor dem Spieler genau aufs
 * Spielfeld wirbeln (und umgekehrt, bei Zugzuruecknahme)
 **/
class CStoneRollEffect:public CStoneEffect
{
private:
	/* Zeit entlang der Animation, von 0-1 */
	double time;
	/* Startposition des Steins, in Welt-Koordinaten. */
	double cx,cy,cz;
	/* Zielposition des Steins, in Welt-Koordinaten. */
	double dx,dy,dz;
	/* Aktueller Rotationswinkel */
	double ang;
	/* Die Achse, um die rotiert werden soll */
	double axe_x,axe_y,axe_z;
	/* Aktueller Vergroesserungsfaktor, da der Stein auf dem Feld groesser ist, als vor
	   dem Spieler. */
	double scale;
	/* Stein fliegt entlang der Parabel a*(time)^2+b*time */
	double parabel_a,parabel_b;
	/* vorwaerts aufs Feld, oder vom Feld zum Spieler zurueck? */
	bool reverse;
public:
	CStoneRollEffect(CGUI *vgui,CStone *vstone,int stone_number,int vplayer,int vx,int vy,bool vreverse);

	/* Animieren. */
	virtual bool execute(double elapsed);
	/* Stein rendern. */
	virtual void render();
	/* Schatten des Steins rendern. */
	virtual void renderShadow();

	/* Ob der Effekt einen bestimmten Spielerstein behandelt. Kann vorkommen, bei reverse=true */
	virtual bool handle_player_stone(int player,int stone);
	/* Dieser Effekt moechte gerne Schatten rendern. */
	virtual bool hasShadow() { return true; }
};

/**
 * Der Effekt simuliert einen Stein, der physikalisch faellt und rotiert.
 * Wird nur vom Intro benutzt.
 **/
class CPhysicalStone:public CStoneEffect
{
private:
	/* Aktuelle Position des Steins in Welt-Koordinaten. */
	double x,y,z;
	/* Aktuellen Winkel */
	double ang;
	/* Rotationsgeschwindigkeit, sowie Rotationsachsen. */
	double angspeed,ax,ay,az;
	/* Geschwindigkeit des Steins in Einheiten/sek */
	double speedx,speedy,speedz;
	/* Angestrebte Position auf dem Feld. Bei Landung wird die Position daran ausgerichtet,
	   um kleine Rechenungenauigkeiten waehrend der Animation auszugleichen. */
	double dx,dy,dz;
public:
	CPhysicalStone(CGUI *vgui,CStone *vstone,int player);

	/* Aktuelle Position setzen. */
	void setPos(double sx,double sy,double sz);
	/* Aktuelle Geschwindigkeit setzen. */
	void setSpeed(double vx,double vy,double vz);
	/* Rotationsgeschwindigkeit, sowie Achse setzen. */
	void setRotationSpeed(double ang,double ax,double ay,double az);
	/* Zielposition setzen. */
	void setDestination(double destx,double desty,double destz);
	/* Die Zielposition ziemlich weit weg setzen, sodass der Stein quasi "unendlich"
	   weiter faellt. */
	void unsetDestination() { dy=-200.0; }

	/* Position zurueckgeben. */
	double getX() { return x;}
	double getY() { return y;}
	double getZ() { return z;}

	/* Animieren. */
	virtual bool execute(double elapsed);
	/* Rendern. */
	virtual void render();
	/* Schatten rendern. */
	virtual void renderShadow();
	/* Dieser Effekt moechte gerne Schatten rendern. */
	virtual bool hasShadow() { return true; }
};

#endif
