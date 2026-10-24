/**
 * stoneeffect.h
 * Author: Sascha Hlusiak
 *
 * Effects on whole stones: fading in, whirling, etc.
 * Used as animations in the game and in the intro
 **/

#ifndef __STONE_EFFECT_H_INCLUDED_
#define __STONE_EFFECT_H_INCLUDED_

#include "timer.h"
#include "stone.h"

class CGUI;
class CTurn;

/**
 * An abstract StoneEffect class. Provides a linked list of StoneEffects
 **/
class CStoneEffect
{
protected:
	/* Coordinate on the board where the stone was placed */
	int x,y;
	CGUI *gui;
	/* The actual stone the animation refers to. */
	CStone stone;
	/* And the number of the player the stone belongs to. */
	int player;
public:
	/* Pointer to the next effect (linked list), or nullptr at the end. */
	CStoneEffect *next;

	/* Constructor that creates a truly empty effect (for the head of the list) */
	CStoneEffect();
	/* Various constructors for different parameters. */
	CStoneEffect(CGUI *vgui,const CStone *vstone,int vplayer,int vx,int vy);
	CStoneEffect(CGUI *vgui,const CTurn *turn,int vplayer);
	/* Clean up if necessary. Removes the list recursively. */
	virtual ~CStoneEffect();

	/* Remove all effects below this one in the list. */
	void clear();
	void add(CStoneEffect* eff) { if (next)next->add(eff); else next=eff; }
	/* Animate the effect. Return true if the effect wants to be removed */
	virtual bool execute(double elapsed);
	/* Render the effect. */
	virtual void render() { if (next) next->render(); }
	virtual void renderShadow() { if (next) next->renderShadow(); }
	/* true if x/y on the board belongs to this effect. Then the element is not
	   shown on the board, since the effect does that. */
	virtual bool is_effected(int x,int y);
	/* true if the effect belongs to a specific stone of a specific player.
	   If so, the player's stone is not shown. */
	virtual bool handle_player_stone(int player,int stone) { if (next)return next->handle_player_stone(player,stone); else return false; }
	/* true if the effect would render a shadow. */
	virtual bool hasShadow() { if (next)return next->hasShadow(); else return false; }
};

/**
 * This effect makes a stone on the board blink on and off.
 * Used by the move hint.
 **/
class CStoneFadeEffect:public CStoneEffect
{
private:
	/* Animation time. */
	double time;
public:
	CStoneFadeEffect(CGUI* vgui, const CStone *vstone, int vplayer,int vx,int vy);
	CStoneFadeEffect(CGUI* vgui, const CTurn *turn, int vplayer);
	
	/* Animate */
	virtual bool execute(double elapsed);
	/* Render */
	virtual void render();
};

/**
 * This effect makes a player's stone whirl from its place in front of the player exactly onto
 * the board (and back, on undo)
 **/
class CStoneRollEffect:public CStoneEffect
{
private:
	/* Time along the animation, from 0-1 */
	double time;
	/* Start position of the stone, in world coordinates. */
	double cx,cy,cz;
	/* Target position of the stone, in world coordinates. */
	double dx,dy,dz;
	/* Current rotation angle */
	double ang;
	/* The axis to rotate around */
	double axe_x,axe_y,axe_z;
	/* Current magnification, since the stone is bigger on the board than in front of
	   the player. */
	double scale;
	/* The stone flies along the parabola a*(time)^2+b*time */
	double parabola_a,parabola_b;
	/* forwards onto the board, or back from the board to the player? */
	bool reverse;
public:
	CStoneRollEffect(CGUI *vgui,const CStone *vstone,int stone_number,int vplayer,int vx,int vy,bool vreverse);

	/* Animate. */
	virtual bool execute(double elapsed);
	virtual void render();
	virtual void renderShadow();

	/* Whether the effect handles a specific player stone. Can happen with reverse=true */
	virtual bool handle_player_stone(int player,int stone);
	/* This effect would like to render shadows. */
	virtual bool hasShadow() { return true; }
};

/**
 * The effect simulates a stone that physically falls and rotates.
 * Only used by the intro.
 **/
class CPhysicalStone:public CStoneEffect
{
private:
	/* Current position of the stone in world coordinates. */
	double x,y,z;
	/* Current angle */
	double ang;
	/* Rotation speed, and the rotation axes. */
	double angspeed,ax,ay,az;
	/* Speed of the stone in units/s */
	double speedx,speedy,speedz;
	/* Intended position on the board. On landing the position is aligned to it
	   to compensate small numerical inaccuracies during the animation. */
	double dx,dy,dz;
public:
	CPhysicalStone(CGUI *vgui,CStone *vstone,int player);

	/* Set the current position. */
	void setPos(double sx,double sy,double sz);
	/* Set the current speed. */
	void setSpeed(double vx,double vy,double vz);
	/* Set the rotation speed and axis. */
	void setRotationSpeed(double ang,double ax,double ay,double az);
	/* Set the target position. */
	void setDestination(double destx,double desty,double destz);
	/* Set the target position rather far away, so the stone keeps falling more or
	   less "forever". */
	void unsetDestination() { dy=-200.0; }

	/* Return the position. */
	double getX() { return x;}
	double getY() { return y;}
	double getZ() { return z;}

	/* Animate. */
	virtual bool execute(double elapsed);
	/* Render. */
	virtual void render();
	/* Render the shadow. */
	virtual void renderShadow();
	/* This effect would like to render shadows. */
	virtual bool hasShadow() { return true; }
};

#endif
