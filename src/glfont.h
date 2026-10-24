/**
 * glfont.h
 * Author: Sascha Hlusiak
 **/
#ifndef __GLFONT_H_INCLUDED_
#define __GLFONT_H_INCLUDED_

#include "window.h"
#include "texture.h"
#include <string.h>

/**
 * Provides the interface for OpenGL text output
 **/

class CGLFont
{
private:
	static CTexture *m_texture;

	static GLuint m_displayLists;

	/* A reference counter. When no reference is left, the texture is removed */
	static int ref_count;

	const double char_width,char_height;

	static void init();
public:
	CGLFont(const double vchar_width,const double vchar_height);
	~CGLFont();

	void drawText(double x,double y,const char *text)const;
	void drawText(double x,double y,double z,const char *text)const;
	void drawText(double x,double y,const char *text,unsigned int len)const;
	void drawText(double x,double y,double z,const char *text,unsigned int len)const;
	double width()const { return char_width; }
	double height()const { return char_height; }
	double width(const char *text)const { return width()*strlen(text); }
};


inline void CGLFont::drawText(double x,double y,const char *text)const
{
	drawText(x,y,text,strlen(text));
}

inline void CGLFont::drawText(double x,double y,double z,const char *text)const
{
	drawText(x,y,z,text,strlen(text));
}

inline void CGLFont::drawText(double x,double y,const char *text,unsigned int len)const
{
	drawText(x,y,0,text,len);
}







#endif
