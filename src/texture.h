/**
 * texture.h
 * Author: Sascha Hlusiak
 **/

#ifndef __TEXTURE_H_INCLUDED_
#define __TEXTURE_H_INCLUDED_

/**
 * Wraps an OpenGL texture for easier management and use
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

	/* Loads a file as a texture. true on success, otherwise false */
	bool load(const char *filename);
	
	void activate()const;

	/* Deactivate the texture -> no texture is active for now */
	void deactivate()const;
};


/**
 * Activate this texture in OpenGL
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

