/**
 * texture.h
 * Autor: Sascha Hlusiak
 **/

#ifndef __TEXTURE_H_INCLUDED_
#define __TEXTURE_H_INCLUDED_

/**
 * Kapselt eine OpenGL Textur fur leichtere Verwaltung und Benutzung
 **/
class CTexture
{
private:
	GLuint m_texture;

	void unload();
public:
	CTexture();

	CTexture(const char *filename);

	~CTexture();

	/* Laedt eine Datei als Textur. true bei Erfolg, sonst false */
	bool load(const char *filename);
	
	void activate()const;

	/* Textur deaktivieren -> es ist vorerst keine Textur mehr aktiv */
	void deactivate()const;
};


/**
 * Diese Textur in OpenGL aktivieren
 **/
inline void CTexture::activate()const
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,m_texture);
}

inline void CTexture::deactivate()const
{
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,0);
}

#endif

