/**
 * glfont.h
 * Autor: Sascha Hlusiak
 **/
#ifndef __GLFONT_H_INCLUDED_
#define __GLFONT_H_INCLUDED_

#include "window.h"
#include "texture.h"
#include <string.h>

/**
 * Stellt die Schnittstelle fuer OpenGL Textausgaben bereit
 **/

class CGLFont
{
private:
	/* Eine globale, statische Textur fuer alle Instanzen */
	static CTexture *m_texture;

	/* Die DisplayLists aller Zeichen */
	static GLuint m_displayLists;

	/* Ein Referenz-Zaehler. Ist keine Referenz mehr vorhanden, wird die Texur entfernt */
	static int ref_count;

	/* Lokale Variablen fuer Groesse der Schrift */
	const double char_width,char_height;

	/* Laedt die globale globale Textur der Schrift */
	static void init();
public:
	/* Neues Schriftobjekt mit angegebener Zeichengroesse erstellen */
	CGLFont(const double vchar_width,const double vchar_height);
	~CGLFont();

	/* Einen nullterminierten Text an x und y (links/oben) zeichnen */
	void drawText(double x,double y,const char *text)const;
	/* Einen nulltermierten Text an x,y,z zeichnen */
	void drawText(double x,double y,double z,const char *text)const;
	/* Einen Text der Laenge len an x, y (links/oben) zeichnen */
	void drawText(double x,double y,const char *text,unsigned int len)const;
	/* Einen nulltermierten Text an x,y,z zeichnen */
	void drawText(double x,double y,double z,const char *text,unsigned int len)const;
	/* Gibt Breite / Hoehe zurueck */
	const double width()const { return char_width; }
	const double height()const { return char_height; }
	/* Gibt Breite eines ganzen Textes zurueck. */
	const double width(const char *text)const { return width()*strlen(text); }
};


/**
 * Rendert einen kompletten nullterminierten Text an x/y
 **/
inline void CGLFont::drawText(double x,double y,const char *text)const
{
	drawText(x,y,text,strlen(text));
}

/**
 * Rendert einen kompletten nullterminierten Text an x/y/z
 **/
inline void CGLFont::drawText(double x,double y,double z,const char *text)const
{
	drawText(x,y,z,text,strlen(text));
}

/**
 * Rendert einen kompletten nullterminierten Text an x/y/z
 **/
inline void CGLFont::drawText(double x,double y,const char *text,unsigned int len)const
{
	drawText(x,y,0,text,len);
}







#endif
