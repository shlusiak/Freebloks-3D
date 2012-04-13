/**
 * logger.h
 * Autor: Sascha Hlusiak
 *
 * Klasse zum Loggen von Ausgaben auf Konsole oder in Datei
 **/

#ifndef _LOGGER_H_INCLUDED_
#define _LOGGER_H_INCLUDED_


class CLogger
{
public:
	static FILE *logfile;
	CLogger() {}
	static void createFile(const char* filename);
	static void closeFile();
	static void flush();
	static void logTime(FILE* file=logfile);
};

class CGameLogger:public CLogger
{
private:
	int game_number;
public:
	CGameLogger(int _game_number);
	void logHeader(FILE* file=logfile);
};




#endif
