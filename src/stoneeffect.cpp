/**
 * stoneeffect.cpp
 * Author: Sascha Hlusiak
 * Effects for whirling stones and the like.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include "timer.h"
#include "stoneeffect.h"
#include "gui.h"
#include "gameclient.h"
#include "constants.h"
#include "turn.h"


/**
 * Create an empty CStoneEffect that does nothing, e.g. as the head of a list.
 **/
CStoneEffect::CStoneEffect()
{
	gui=0;
	next=0;
	x=y=0;
	player=0;
}

/**
 * Create a StoneEffect with CStone, player, x and y. */
CStoneEffect::CStoneEffect(CGUI *vgui,const CStone *vstone,int vplayer,int vx,int vy)
{
	x=vx;
	y=vy;
	next=0;
	player=vplayer;
	gui=vgui;

	stone = *vstone;
}

/**
 * Create a StoneEffect from a CTurn and player.
 **/
CStoneEffect::CStoneEffect(CGUI *vgui,const CTurn *turn,int vplayer)
{
	x=turn->x;
	y=turn->y;
	next=0;
	player=vplayer;
	gui=vgui;
	/* Copy the stone and take over the data from CTurn */
	stone.init(turn->stone_number);
	stone.mirror_rotate_to(turn->mirror_count,turn->rotate_count);
}

/**
 * Clean up
 **/
CStoneEffect::~CStoneEffect()
{
	/* If successors exist, remove them too (recursively) */
	if (next)delete next;
	next=0;
}

/**
 * Cut the linked list here, removing all following CStoneEffects (recursively)
 **/
void CStoneEffect::clear()
{
	if (next)delete next;
	next=0;
}

/**
 * Animate the stone. Return true if the effect wants to be removed.
 **/
bool CStoneEffect::execute(double elapsed)
{
	/* If the successor wants to be removed */
	if (next) if (next->execute(elapsed))
	{
		CStoneEffect *n=next;
		next=next->next;
		n->next=0;
		delete n;
	}
	/* We do not want to die */
	return false;
}

/**
 * true if square x/y is handled by this effect.
 **/
bool CStoneEffect::is_effected(int x,int y)
{
	/* If any effect handles square x/y: true. */
	if (next)if (next->is_effected(x,y))return true;
	/* If not a real effect at all (stone==0): false */
	//if (stone==0)return false; /* FIXME */
	if (gui==0)return false;
	y-=this->y;
	x-=this->x;
	/* If x/y is outside the bounds of our stone: false */
	if (x<0 || y<0 || x>=stone.get_stone_size() || y>=stone.get_stone_size())return false;

	/* If our stone has an element at the computed position: true */
	if (stone.get_stone_field(y,x)!=STONE_FIELD_FREE)return true;
	return false;
}


/**
 * A simple fade effect. Initialise the time with 0; nothing else is needed.
 **/
CStoneFadeEffect::CStoneFadeEffect(CGUI *vgui,const CStone *vstone,int vplayer,int vx,int vy)
:CStoneEffect(vgui,vstone,vplayer,vx,vy)
{
	time=0.0;
}

CStoneFadeEffect::CStoneFadeEffect(CGUI *vgui,const CTurn *turn,int vplayer)
:CStoneEffect(vgui,turn,vplayer)
{
	time=0.0;
}

/**
 * Animate.
 **/
bool CStoneFadeEffect::execute(double elapsed)
{
	/* Animate the other effects. */
	CStoneEffect::execute(elapsed);
	/* Increase the time; after 3 s the effect wants to be removed. */
	time+=elapsed;
	return (time>3.0);
}

/**
 * Simply render.
 **/
void CStoneFadeEffect::render()
{
	int i,j;
	/* Compute a smooth animation of the alpha channel. */
	const double alpha=(sin(time/3.0*M_PI*8.5)+1.0)*0.5*0.45+0.2;
	/* Render the other effects. */
	CStoneEffect::render();
	/* And render the stone at the corresponding position on the board. */
	for (i=0;i<stone.get_stone_size();i++)
	for (j=0;j<stone.get_stone_size();j++)
		if (stone.get_stone_field(j,i)!=STONE_FIELD_FREE)
	{
		gui->renderStone(i+x,j+y,player,(float)alpha,false);
	}
}

/**
 * Whirling stone. Animation when a player places a stone.
 * Needs the player number, stone, stone number and position
 * With vreverse==true the stone flies from the board to the player, otherwise from the player onto the board
 **/
CStoneRollEffect::CStoneRollEffect(CGUI *vgui, const CStone *vstone,int stone_number,int vplayer,int vx,int vy,bool vreverse)
:CStoneEffect(vgui,vstone,vplayer,vx,vy)
{
	reverse=vreverse;
	if (reverse)time=1.0;
	else time=0.0;
	/* The stone starts at a scale of 0.7 */
	scale=0.70;
	ang=0.0;
	/* Get the position of the player's corresponding stone in world coordinates.
	   This is our start position. */
	gui->getPlayerStonePos(vplayer,stone_number,&cx,&cy,&cz);
	
	/* Compute the target position on the board in world coordinates. */
	dx=-(gui->game->get_field_size_x()-1)*stone_size+((double)x+(double)stone.get_stone_size()/2.0)*stone_size*2.0-stone_size;
	dy=bevel_height;
	dz=-(gui->game->get_field_size_y()-1)*stone_size+((double)y+(double)stone.get_stone_size()/2.0)*stone_size*2.0-stone_size;

	{
		/* Compute a random rotation axis. */
		double angx=double(rand()%360)/180.0*M_PI;
		double angy=double(rand()%360)/180.0*M_PI;
		axe_x=sin(angx)*cos(angy);
		axe_y=sin(angy);
		axe_z=cos(angx)*cos(angy);
	}
	/* Gravity is -1/2*80 */
	parabola_a=-40.0;
	/* From this compute b of the parabola so that it passes exactly through start and target */
	parabola_b=-parabola_a+dy-cy;
}

/**
 * Animate
 **/
bool CStoneRollEffect::execute(double elapsed)
{
	/* The stone always rotates by 2*360 degrees */
	const double loops=2.0;
	/* The animation lasts 1.0 s. */
	const double duration=1.0;
	CStoneEffect::execute(elapsed);

	/* Count the time either forwards or backwards */
	if (reverse)time-=(elapsed/duration);
	else time+=(elapsed/duration);
	/* Set the rotation to the angle matching the time. */
	ang=(time)*360.0*loops;
	/* If the animation is outside 0 and 1, it is over. */
	return (time<0.0)||(time>1.0);
}

void CStoneRollEffect::render()
{
	int i,j;
	/* Compute the stone's scale and apply it. */
	double s=scale+(1.0-scale)*(time*2.5);
	if (s>1.0)s=1.0;

	/* Compute the current position along the parabola from the time. */
	const double x=cx+time*(dx-cx);
	const double y=parabola_a*time*time+parabola_b*time+cy;
	const double z=cz+time*(dz-cz);
	
	CStoneEffect::render();
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
		gui->renderStone(player,0.65f,false);
		glPopMatrix();
	}
	glPopMatrix();
}

/**
 * Render the shadow. Move the stone into position, compute the direction of the light,
 * extrude the stone along the light ray and render into the stencil buffer.
 */
void CStoneRollEffect::renderShadow()
{
	int i,j;
	/* Compute the scale. */
	double s=scale+(1.0-scale)*(time*3.5);
	if (s>1.0)s=1.0;
	const double x=cx+time*(dx-cx);
	const double y=parabola_a*time*time+parabola_b*time+cy;
	const double z=cz+time*(dz-cz);
	
	CStoneEffect::renderShadow();

	float dir[3];
	float M[16];
	/* This is where our light comes from. */
	float *v=(float*)&light0_pos;

	glPushMatrix();
	glLoadIdentity();
	/* Rotate the stone backwards. */
  	glRotated(-ang,axe_x,axe_y,axe_z);
	glGetFloatv(GL_MODELVIEW_MATRIX,M);
	/* And extract the resulting direction vector. The direction is relative to the rotated
	   stone, so it always points the same way globally. */
	dir[0]=-(M[ 0]*v[0]+M[ 4]*v[1]+M[ 8]*v[2]);
	dir[1]=-(M[ 1]*v[0]+M[ 5]*v[1]+M[ 9]*v[2]);
	dir[2]=-(M[ 2]*v[0]+M[ 6]*v[1]+M[10]*v[2]);

	/* Move into position. */
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
		/* And compute the shadow of a small stone along the direction of the light. */
		gui->renderStoneShadow(dir);
		glPopMatrix();
	}
	glPopMatrix();
}

/**
 * Does this effect handle one of the player's stones?
 * Only when it flies backwards.
 **/
bool CStoneRollEffect::handle_player_stone(int player,int n_stone)
{
	if (reverse && this->player==player && stone.get_stone_shape()==n_stone)return true;
	return CStoneEffect::handle_player_stone(player,n_stone);
}



/**
 * A stone with physical properties. Used in the intro until it falls over.
 * Constructor: initialise everything with 0; it is fed via member functions later.
 **/
CPhysicalStone::CPhysicalStone(CGUI *vgui,CStone *vstone,int player)
:CStoneEffect(vgui,vstone,player,0,0)
{
	x=y=z=0.0;
	ang=0.0;
	angspeed=0.0;
	ax=az=speedx=speedy=speedz=0.0;
	ay=1.0;
	dx=dy=dz=0.0;
}

void CPhysicalStone::setPos(double sx,double sy,double sz)
{
	x=sx;
	y=sy;
	z=sz;
}

void CPhysicalStone::setRotationSpeed(double angs,double ax,double ay,double az)
{
	angspeed=angs;
	this->ax=ax;
	this->ay=ay;
	this->az=az;
}

void CPhysicalStone::setSpeed(double sx,double sy,double sz)
{
	speedx=sx;
	speedy=sy;
	speedz=sz;
}

/**
 * Set the stone's target coordinate.
 * As soon as (Y coordinate<=desty), the stone is set to dx/dy/dz to
 * correct inaccuracies in the animation.
 **/
void CPhysicalStone::setDestination(double destx,double desty,double destz)
{
	dx=destx;
	dy=desty;
	dz=destz;
}

/**
 * Animate.
 **/
bool CPhysicalStone::execute(double elapsed)
{
	CStoneEffect::execute(elapsed);
	/* Straight-line motion, easy to animate. */
	x+=speedx*elapsed;
	y-=speedy*elapsed;
	z+=speedz*elapsed;
	/* Constant angular velocity for the rotation. Animate linearly. */
	ang+=elapsed*angspeed;
	/* If y has fallen below dy, set the stone to dx/dy/dz. */
	if (y<dy && dy>-100.0)
	{
		/* The stone should bounce up again, but with damped vertical speed. */
		if (speedy>0.5)speedy=-speedy*0.32;else speedy=0.0;
		/* It should stop rotating and lie exactly parallel to the ground. */
		angspeed=0.0;
		ang=0.0;
		speedx=0.0;
		speedz=0.0;
		x=dx;
		y=dy;
		z=dz;
	}
	/* Accelerate the stone vertically (gravity). */
	speedy+=elapsed*17.0;
	/* The stone never wants to be removed and never disappears automatically. */
	return false;
}

/**
 * Render.
 **/
void CPhysicalStone::render()
{
	CStoneEffect::render();

	int i,j;

	/* Move the stone into position and render it normally, like all the others. */
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

void CPhysicalStone::renderShadow()
{
	CStoneEffect::renderShadow();

	int i,j;
	float dir[3];
	float M[16];
	float *v=(float*)&light0_pos;
	
	glPushMatrix();
	glLoadIdentity();
	/* Rotate backwards to get the direction of the light relative to the stone. */
	glRotated(-ang,ax,ay,az);
	glGetFloatv(GL_MODELVIEW_MATRIX,M);
	dir[0]=-(M[ 0]*v[0]+M[ 4]*v[1]+M[ 8]*v[2]);
	dir[1]=-(M[ 1]*v[0]+M[ 5]*v[1]+M[ 9]*v[2]);
	dir[2]=-(M[ 2]*v[0]+M[ 6]*v[1]+M[10]*v[2]);

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
		/* And render the extruded shadow volume in the direction of the light. */
		gui->renderStoneShadow(dir);
		glPopMatrix();
	}
	glPopMatrix();
}

