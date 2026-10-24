/**
 * texture.cpp
 * Autor: Sascha Hlusiak, teilweise Fremdcode
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window.h"
#include "texture.h"


/* Struktur Zum Laden von BMP Dateien */
typedef struct                       /**** BMP file info structure ****/
{
    unsigned int   biSize;           /* Size of info header */
    int            biWidth;          /* Width of image */
    int            biHeight;         /* Height of image */
    unsigned short biPlanes;         /* Number of color planes */
    unsigned short biBitCount;       /* Number of bits per pixel */
    unsigned int   biCompression;    /* Type of compression to use */
    unsigned int   biSizeImage;      /* Size of image data */
    int            biXPelsPerMeter;  /* X pixels per meter */
    int            biYPelsPerMeter;  /* Y pixels per meter */
    unsigned int   biClrUsed;        /* Number of colors used */
    unsigned int   biClrImportant;   /* Number of important colors */
    char *data;
} MYBITMAPINFOHEADER;


CTexture::CTexture()
{
	m_texture=0;
}

CTexture::CTexture(const char *filename)
{
	m_texture=0;
	load(filename);
}

CTexture::~CTexture()
{
	/* Textur freigeben */
	unload();
}

void CTexture::unload()
{
	if (m_texture)glDeleteTextures(1,&m_texture);
	m_texture=0;
}

/* Reads a little endian number of size bytes. Returns false if the file ends early. */
static bool freadnum(int size,FILE* file,unsigned int *out)
{
	unsigned int x=0;
	unsigned char b;
	for (int i=0;i<size;i++)
	{
		if (fread(&b,1,1,file)!=1)return false;
		x|=((unsigned int)b<<(i*8));
	}

	*out=x;
	return true;
}

/* Largest edge length accepted for a texture */
static const int TEXTURE_SIZE_MAX = 8192;


/**
 * Laedt die angegebene Datei (BMP Format) in die Textur
 * BMP-Lad'-Code irgendwo "gefunden" und ueberarbeitet
 **/
bool CTexture::load(const char *filename)
{
	/* Textur entladen */
	unload();

	FILE * file=nullptr;
	char temp;

	MYBITMAPINFOHEADER infoheader;

	/* Datei im binaer Modus oeffnen */
#ifdef DATA_PREFIX
	char mypath[1024];
	if (strlen(DATA_PREFIX)+strlen(filename)<1023)
	{
	    strcpy(mypath,DATA_PREFIX);
	    strcat(mypath,filename);
	    file=fopen(mypath,"rb");
	}
#endif
	if (file==nullptr)
	    if( (file = fopen(filename, "rb"))==nullptr) return false;

	fseek(file, 18, SEEK_CUR);  /* start reading width & height */

	unsigned int num;
	if (!freadnum(sizeof(int), file, &num)) { fclose(file); return false; }
	infoheader.biWidth=(int)num;
	if (!freadnum(sizeof(int), file, &num)) { fclose(file); return false; }
	infoheader.biHeight=(int)num;

	/* The size comes from the file; it is used to allocate and to read the pixel data */
	if (infoheader.biWidth <= 0 || infoheader.biHeight <= 0 ||
	    infoheader.biWidth > TEXTURE_SIZE_MAX || infoheader.biHeight > TEXTURE_SIZE_MAX) {
		printf("Invalid size of %s: %dx%d\n", filename, infoheader.biWidth, infoheader.biHeight);
		fclose(file);
		return false;
	}

	if (!freadnum(sizeof(short int), file, &num)) { fclose(file); return false; }
	infoheader.biPlanes=(unsigned short)num;
	if (infoheader.biPlanes != 1) {
		printf("Planes from %s is not 1: %u\n", filename, infoheader.biPlanes);
		fclose(file);
		return false;
	}

	/* Farbtiefe lesen (bits per Pixel). Es werden nur 24bpp Bilder geladen */
	if (!freadnum(sizeof(unsigned short int), file, &num)) { fclose(file); return false; }
	infoheader.biBitCount=(unsigned short)num;
	if (infoheader.biBitCount != 24) {
		printf("Bpp from %s is not 24: %d\n", filename, infoheader.biBitCount);
		fclose(file);
		return false;
	}

	fseek(file, 24, SEEK_CUR);

	// Platz fuer Daten zu reservieren. Bei 24bpp (=3byte per Pixel) sind das 3*w*h Byte */
	const size_t data_size = (size_t)infoheader.biWidth * (size_t)infoheader.biHeight * 3;
	infoheader.data = (char *) malloc(data_size);
	if (infoheader.data == nullptr) {
		printf("Error allocating memory for color-corrected image data\n");
		fclose(file);
		return false;
	}

	if (fread(infoheader.data, data_size, 1, file) != 1) {
		printf("Error reading image data from %s.\n", filename);
		free(infoheader.data);
		fclose(file);
		return false;
	}

	/* Farben umkehren ( BGR -> RGB ) */
	for (size_t k=0; k<data_size; k+=3) {
		temp = infoheader.data[k];
		infoheader.data[k] = infoheader.data[k+2];
		infoheader.data[k+2] = temp;
	}

	/* Datei schliessen */
	fclose(file);

	glGenTextures(1,&m_texture);
	/* Erhaltene Textur aktivieren */
	glBindTexture(GL_TEXTURE_2D, m_texture);

	/* Texturparameter setzen */
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, 3, infoheader.biWidth, infoheader.biHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);

	/* Mipmaps zur weichen Darstellung der Textur bei Verkleinerung */
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,infoheader.biWidth,infoheader.biHeight,GL_RGB,GL_UNSIGNED_BYTE,infoheader.data);

	/* Speicher wieder freigeben */
	free(infoheader.data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}






