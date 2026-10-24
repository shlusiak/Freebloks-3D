/**
 * glfont.cpp
 * Author: Sascha Hlusiak
 *
 * Provides an interface for text in OpenGL
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <string.h>
#include "glfont.h"

/* Global, static texture shared by everyone. Created by the first GLFont object */
CTexture *CGLFont::m_texture=0;

int CGLFont::ref_count=0;

/* Display lists of all characters */
GLuint CGLFont::m_displayLists=0;


CGLFont::CGLFont(const double vchar_width,const double vchar_height)
:char_width(vchar_width),char_height(vchar_height)
{
	/* The first GLFont object initialises the texture */
	if (ref_count==0)init();
	/* Increment the reference counter */
	ref_count++;
}

CGLFont::~CGLFont()
{
	/* The last one turns off the lights */
	if (--ref_count==0)
	{
		delete m_texture;
		m_texture=0;
		glDeleteLists(m_displayLists,256);
		m_displayLists=0;
	}
}

/**
 * Loads the texture file "font.bmp" into the global texture.
 * It contains the first 116 characters.
 **/
void CGLFont::init()
{
	static const double tw=18.0/256.0;
	static const double th=31.0/256.0;

	if (m_texture)delete m_texture;
	m_texture=new CTexture("font.bmp");

	m_displayLists=glGenLists(256);
	for (int i=0;i<256;i++)
	{
		glNewList(m_displayLists+i,GL_COMPILE);

		if (i=='\n')
		{	/* A line break: go back to the previous line position,
			   move down one line, and remember the position again. */
			glPopMatrix();
			glTranslated(0,1.0,0);
			glPushMatrix();
		}else{
			/* The first 18 characters were left out of the texture.
			   Otherwise there are 14 characters per row, sorted alphabetically.
			   From this the texture coordinates of the current char
			   can be computed. */
			int index=i;
			/* German umlauts need special treatment yet again.
			   o_O */
			switch ((unsigned char)i)
			{
				case 228: index=128; break; // 0xE4
				case 246: index=129; break;
				case 252: index=130; break;
				case 196: index=131; break;
				case 214: index=132; break;
				case 220: index=133; break;
				case 223: index=134; break;
			}

			double tx1=tw*((index-18)%14);
			double ty1=1.0-th*((index-18)/14);
			glBegin(GL_QUADS);
			glTexCoord2d(tx1+tw,ty1+th);	glVertex3d(1,0,0);
			glTexCoord2d(tx1,ty1+th);  	glVertex3d(0,0,0);
			glTexCoord2d(tx1,ty1);  	glVertex3d(0,1,0);
			glTexCoord2d(tx1+tw,ty1);  	glVertex3d(1,1,0);
			glEnd();
			glTranslated(1,0,0);
		}
		glEndList();
	}
}

/**
 * Draws the first len characters of text at position x,y,z in the selected character size.
 **/
void CGLFont::drawText(double x,double y,double z,const char *text,unsigned int len)const
{
	/* The texture is 512x512 pixels, a character is 36x32 pixels.
	   tw/th is the width/height of a character in texture coordinates (0..1) */

	glPushMatrix();
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glDepthMask(GL_FALSE);

	/* The alpha values are derived from the colour. White is opaque, black is transparent */
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);

	glDisable(GL_LIGHTING);

	glTranslated(x,y,z);
	glScaled(char_width,char_height,1.0);
	glPushMatrix();

	/* Now call the display lists of all characters of the text */
	m_texture->activate();

	glListBase(m_displayLists);

	for (unsigned int i=0;i<len;i+=255)
	{
		glCallLists((i+255>len)?(len-i):255,GL_UNSIGNED_BYTE,text+i);
	}

	glListBase(0);
	/* Deactivate the texture again */
 	m_texture->deactivate();

	glPopMatrix();
	glPopMatrix();
	/* Reset the attributes */
//	glDisable(GL_BLEND);
	glPopAttrib();
}

