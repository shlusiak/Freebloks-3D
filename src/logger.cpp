/**
 * logger.h
 * Autor: Sascha Hlusiak
 *
 * Klasse zum Loggen von Ausgaben auf Konsole oder in Datei
 **/


#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "logger.h"



FILE* CLogger::logfile=NULL;


void CLogger::createFile(const char* filename)
{
	logfile=fopen(filename,"a");
	if (logfile)
	{
		logTime();
		fprintf(logfile,"Server starting\n");
		flush();
	} else {
		perror("fopen: ");
	}
}

void CLogger::closeFile()
{
	if (logfile)
	{
		if (fclose(logfile))
			perror("fclose: ");
	}
	logfile=NULL;
}

void CLogger::flush()
{
	if (logfile) fflush(logfile);
}

/**
 * Schreibt die aktuelle Uhrzeit in die Datei
 **/
void CLogger::logTime(FILE* file)
{
	if (file==NULL)return;
	char zeitstring[256];
	time_t zeit;
	char *c;
	zeit=time(NULL);
	c=ctime(&zeit);
	strcpy(zeitstring,c);
	zeitstring[strlen(zeitstring)-1]='\0';
	if (fprintf(file,"%s: ",zeitstring)==-1)
		perror("fprintf: ");
}





CGameLogger::CGameLogger(int _game_number)
{
	game_number=_game_number;
}

void CGameLogger::logHeader(FILE* file)
{
	if (file==NULL)return;
	if (fprintf(file,"[%d]: ",game_number)==-1)
		perror("fprintf: ");
}

