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


/* Nur ein leerer Textur-Container */
CTexture::CTexture()
{
	m_texture=0;
}

/* Laedt die angegebene Textur */
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
	/* Eine einzelne OpenGL Textur freigeben */
	if (m_texture)glDeleteTextures(1,&m_texture);
	m_texture=0;
}

int freadnum(int size,FILE* file)
{
	unsigned int x=0;
	unsigned char b;
	for (int i=0;i<size;i++)
	{
		fread(&b,1,1,file);
		x|=(b<<(i*8));
	}

	return x;
}


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
	long i;

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

	/* Breite und Hoehe des Bilds ermitteln */
	infoheader.biWidth=freadnum(sizeof(int), file);
	infoheader.biHeight=freadnum(sizeof(int), file);


	/* Anzahl planes lesen und pruefen */
	infoheader.biPlanes=freadnum(sizeof(short int), file);
	if (infoheader.biPlanes != 1) {
		printf("Planes from %s is not 1: %u\n", filename, infoheader.biPlanes);
		return false;
	}

	/* Farbtiefe lesen (bits per Pixel). Es werden nur 24bpp Bilder geladen */
	infoheader.biBitCount=freadnum(sizeof(unsigned short int), file);
	if (infoheader.biBitCount != 24) {
		printf("Bpp from %s is not 24: %d\n", filename, infoheader.biBitCount);
		return false;
	}

	fseek(file, 24, SEEK_CUR);

	// Platz fuer Daten zu reservieren. Bei 24bpp (=3byte per Pixel) sind das 3*w*h Byte */
	infoheader.data = (char *) malloc(infoheader.biWidth * infoheader.biHeight * 3);
	if (infoheader.data == nullptr) {
		printf("Error allocating memory for color-corrected image data\n");
		return false;
	}

	/* Einlesen der Bilddaten in den Puffer */
	if ((i = fread(infoheader.data, infoheader.biWidth * infoheader.biHeight * 3, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
	}

	/* Farben umkehren ( BGR -> RGB ) */
	for (i=0; i<(infoheader.biWidth * infoheader.biHeight * 3); i+=3) { 
		temp = infoheader.data[i];
		infoheader.data[i] = infoheader.data[i+2];
		infoheader.data[i+2] = temp;
	}

	/* Datei schliessen */
	fclose(file);

	/* Bei OpenGL eine Textur anfordern */
	glGenTextures(1,&m_texture);
	/* Erhaltene Textur aktivieren */
	glBindTexture(GL_TEXTURE_2D, m_texture);

	/* Texturparameter setzen */
	/* Textur soll sich wiederholen */
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* Lineare Texturfilter fuer schoenere Ausgabe*/
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	/* Daten aus Puffer nach OpenGL in die neue Textur uebertragen */
	glTexImage2D(GL_TEXTURE_2D, 0, 3, infoheader.biWidth, infoheader.biHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, infoheader.data);

	/* Mipmaps zur weichen Darstellung der Textur bei Verkleinerung */
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,infoheader.biWidth,infoheader.biHeight,GL_RGB,GL_UNSIGNED_BYTE,infoheader.data);

	/* Speicher wieder freigeben */
	free(infoheader.data);

	/* Und Textur deaktivieren */
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}






