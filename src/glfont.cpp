/**
 * glfont.cpp
 * Autor: Sascha Hlusiak
 *
 * Stellt Schnittstelle fuer Text in OpenGL bereit
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <string.h>
#include "glfont.h"

/* Globale, statische Textur fuer Jedermann. Wird vom erstem GLFont Objekt erstellt */
CTexture *CGLFont::m_texture=0;

/* Referenzcounter der Textur */
int CGLFont::ref_count=0;

/* DisplayLists aller Buchstaben */
GLuint CGLFont::m_displayLists=0;


CGLFont::CGLFont(const double vchar_width,const double vchar_height)
:char_width(vchar_width),char_height(vchar_height)
{
	/* Das erste GLFont Objekt soll die Textur initialisieren */
	if (ref_count==0)init();
	/* Referenzzaehler inkrementieren */
	ref_count++;
}

CGLFont::~CGLFont()
{
	/* Der Letzte macht das Licht aus */
	if ((ref_count--)==0)
	{
		delete m_texture;
		m_texture=0;
		glDeleteLists(m_displayLists,256);
		m_displayLists=0;
	}
}

/**
 * Laedt die Texturdatei "font.bmp" in die globale Textur.
 * Sie enthaelt die ersten 116 Zeichen.
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
		{	/* Ein Zeilenumbruch: Zurueck zu vorheriger Zeilenposition,
			   eine Zeile runter, und Position wieder merken. */
			glPopMatrix();
			glTranslated(0,1.0,0);
			glPushMatrix();
		}else{
			/* Die ersten 18 Zeichen wurden in der Textur weggelassen
			   Sonst sind 14 Zeichen in jeder Zeile, alphabetisch sortiert.
			   Daraus lassen sich die Texturkorrdinaten des aktuellen (char)s
			   berechnen. */
			int index=i;
			/* Deutsche Umlaute brauchen man wieder unbedingt ne 
			   Extrawurst. o_O */
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
			/* Ein einfaches Quad mit der Textur an aktuelle Position rendern. */
			glBegin(GL_QUADS);
			glTexCoord2d(tx1+tw,ty1+th);	glVertex3d(1,0,0);
			glTexCoord2d(tx1,ty1+th);  	glVertex3d(0,0,0);
			glTexCoord2d(tx1,ty1);  	glVertex3d(0,1,0);
			glTexCoord2d(tx1+tw,ty1);  	glVertex3d(1,1,0);
			glEnd();
			/* Aktuelle Position um eins nach rechts */
			glTranslated(1,0,0);
		}
		glEndList();
	}
}

/**
 * Zeigt von text an Position x,y,z die ersten len Zeichen in der ausgewaehlten Zeichengroesse an.
 **/
void CGLFont::drawText(double x,double y,double z,const char *text,unsigned int len)const
{
	/* Die Textur ist 512x512 Pixel gross, ein Buchstabe 36x32 Pixel. 
	   tw/th ist die Breite/Hoehe eines Zeichens in Texturmappingkoordinaten (0..1) */

	/* Dann aktuelle Matrix und Attribute merken */
	glPushMatrix();
	glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	/* Wir wollen nicht in den Z-Buffer schreiben */
	glDepthMask(GL_FALSE);

	/* Die Alpha-Werte werden aus der Farbe gewonnen. Weiß ist opak, Schwarz ist transparent */
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);

	/* Keine Beleuchtung */
	glDisable(GL_LIGHTING);

	/* Position an x/y Koordinate verschieben und Matrix merken */
	glTranslated(x,y,z);
	glScaled(char_width,char_height,1.0);
	glPushMatrix();

	/* Jetzt die DisplayListen aller Zeichen des Texts aufrufen */
	/* Erstmal Textur der Schrift aktivieren */
	m_texture->activate();

	glListBase(m_displayLists);

	for (unsigned int i=0;i<len;i+=255)
	{
		glCallLists((i+255>len)?(len-i):255,GL_UNSIGNED_BYTE,text+i);
	}

	glListBase(0);
	/* Textur wieder deaktivieren */
 	m_texture->deactivate();

	/* Matrix-Stack wieder zurueck */
	glPopMatrix();
	glPopMatrix();
	/* Attribute zuruecksetzen */
//	glDisable(GL_BLEND);
	glPopAttrib();
}

