/**
 * intro.cpp
 * Author: Sascha Hlusiak
 * Implementation of the intro sequence with all the trimmings
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
#include "gameclient.h"

/* During the animation the board is 20x20 squares. */
const int FIELD_SIZE_X=20,FIELD_SIZE_Y=20;

CIntro::CIntro(CGUI *vgui)
{
	gui=vgui;
	anim=0.0;
	effects=new CStoneEffect();
	phase=0;
	field_up=false;
	field_anim=0.0;
	init();
}

CIntro::~CIntro()
{
	if (effects)delete effects;
}

/**
 * Initialise the stones (shapes)
 * and prepare the word "Free Bloks"
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

	addChar('f',3,4,5);
	addChar('r',2,7,6);
	addChar('e',1,10,5);
	addChar('e',0,13,6);

	addChar('b',0,2,12);
	addChar('l',1,5,11);
	addChar('o',2,8,12);
	addChar('k',3,11,11);
	addChar('s',2,14,13);
}

/**
 * Add a flying letter 'c' of colour color so that it lands at x/y (top left corner)
 * It is split into individual stones, which are added.
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

 	default: printf("Invalid char passed: %c.\n",c);
 		break;
	}
}

/**
 * Add a CPhysicalStone that lands at dx/dy
 * Start position and rotation are random
 **/
void CIntro::add(int stone,int color,int dx,int dy)
{
	double x,y,z;
	/* Compute a rotation axis */
	double angx=double(rand()%360)/180.0*M_PI;
	double angy=double(rand()%360)/180.0*M_PI;
	double axe_x=sin(angx)*cos(angy);
	double axe_y=sin(angy);
	double axe_z=cos(angx)*cos(angy);

	CStone *st=&stones[stone];
	CPhysicalStone *s=new CPhysicalStone(gui,st,color);
	/* Convert the board-local dx/dy into global world coordinates. */
	x=-(FIELD_SIZE_X-1)*stone_size+((double)dx+(double)st->get_stone_size()/2.0)*stone_size*2.0-stone_size;
	z=-(FIELD_SIZE_Y-1)*stone_size+((double)dy+(double)st->get_stone_size()/2.0)*stone_size*2.0-stone_size;
	/* Give it a random height. */
	y=22.0+(double)(rand()%180)/10.0;

	/* The stone will reach the ground in <time> s. 17 is gravity */
	double time=sqrt(2*y/17.0);
	/* Shift the x/z coordinates randomly */
	double xoffs=rand()%60-30.0;
	double zoffs=rand()%60-30.0;
	/* Set the position */
	s->setPos(x+xoffs,y,z+zoffs);
	/* Set the x/z speed; the y speed is 0 */
	s->setSpeed(-xoffs/time,0,-zoffs/time);
	s->setDestination(x,bevel_height,z);
	/* The stone rotates by exactly 360 degrees in <time> s. */
	s->setRotationSpeed(360.0/time,axe_x,axe_y,axe_z);
	effects->add(s);
}

/* Speed at which the board tips up */
const double wipe_speed=6.8;
/* Angle up to which the board is tilted */
const double wipe_ang=32.0;

/**
 * Tilt the board and accelerate all stones along the radial velocity
 **/
void CIntro::wipe()
{
	field_up=true;
	field_anim=0.0;
	/* Go through the whole linked list and, for every CPhysicalStone in it...*/
	CPhysicalStone *s=(CPhysicalStone*)effects->next;
	while (s)
	{
		/* ...set the speed so the stones fly off tangentially to the rotation of the
		   board */
		/* Angle at which the stones are accelerated */
		const double ang=wipe_ang/180.0*M_PI;
		/* Compute the radial velocity. */
		const double v=(ang*wipe_speed)*(s->getZ()-FIELD_SIZE_X*stone_size)-(double)(rand()%100)/10.0-8.0;
		/* Let the stone rotate only slightly, and not entirely at random */
		const double a1=0.95;
		const double a2=((rand()%2)?1:-1)*sqrt((1.0-a1*a1)/2.0);
		const double a3=((rand()%2)?1:-1)*sqrt((1.0-a1*a1)/2.0);
		/* Rotate the stone mainly in the direction of the board's rotation */
		s->setRotationSpeed(wipe_ang*wipe_speed+(double)(rand()%100)/15.0,a1,a2,a3);
		/* Convert speed and angle into Cartesian coordinates */
		s->setSpeed((double)(rand()%100)/20.0,cos(ang)*v,-sin(ang)*v);
		/* The stone has no target any more, i.e. it falls forever */
		s->unsetDestination();
		s=(CPhysicalStone*)s->next;
	}
}

/* The matrix animation starts at matrix_start, stops at matrix_stop,
   and fades in slowly over matrix_duration_start */
const double matrix_start=1.56;
const double matrix_stop=6.0;
const double matrix_duration_start=0.25;
const double matrix_duration_stop=0.25;

/**
 * Animate everything. Returns true when the intro sequence is over.
 **/
bool CIntro::execute(double elapsed)
{
	elapsed*=1.2;
	anim+=elapsed;

	if (field_up || field_anim>0.000001)
	{
		if (field_up)
		{
			field_anim+=elapsed*wipe_speed;
			if (field_anim>1.0)
			{
				field_anim=1.0;
				field_up=false;
			}
		}else{
			/* Animate downwards. */
			field_anim-=elapsed*2.5;
			if (field_anim<0.0)field_anim=0.0;
		}
	}
	if (phase==0)
	{
		/* In phase 0 a matrix mode appears between the flying stones */
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
			/* After 10.5 time units clear the board and set the phase to 1 */
			phase=1;
			wipe();
		}
	}else{
		/* Animate the effects */
		effects->execute(elapsed);
		/* In these phases stones fall from the sky; they should fall quickly */
		if (phase==2 || phase==4 || phase==5 || phase==6)
			effects->execute(elapsed*0.7);
		/* Each phase lasts 12 time units */
		if (anim>12.0)
		{
			/* New phase, and either clear the board */
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
			/* or let new stones rain down. */
			if (phase==2)
			{
				anim=9.1;
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
			/* After the 7th phase the intro is over */
			if (phase==8)return true;
		}
	}
	return false;
}

/**
 * Render the intro
 **/
void CIntro::render()
{
	/* The camera is 30 units away from the board */
	const double distance=30.0;
	glLoadIdentity();
	/* Position the camera */
	glTranslated(0,4,0);
	gluLookAt(0,
		0,
		distance,
		0,0,0,
		0,1,0);

	/* Rotate the camera, possibly by the matrix move */
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

	/* Adjust the light to the new camera position */
	glLightfv(GL_LIGHT0,GL_POSITION,light0_pos);

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
	effects->render();
}

/**
 * Render the shadows in the intro.
 **/
void CIntro::renderShadow()
{
	effects->renderShadow();
}
