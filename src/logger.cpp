/**
 * logger.cpp
 * Author: Sascha Hlusiak
 *
 * Class for logging output to the console or to a file
 **/


#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include "logger.h"


CLogWriter::~CLogWriter()
{
	delete next;
	next = nullptr;
}

void CLogWriter::log(const char* fmt, va_list va) {
	if (next) {
		va_list vc;
		va_copy(vc, va);
		next->log(fmt, vc);
		va_end(vc);
	}
}

void CLogWriter::addLogWriter(CLogWriter* _next) {
	CLogWriter* cur = this;
	while (cur->next)
		cur = cur->next;
	cur->next = _next;
}



void CStdoutWriter::log(const char* fmt, va_list va) {
	CLogWriter::log(fmt, va);

	vprintf(fmt,va);
}




CLogFileWriter::CLogFileWriter()
{
	logfile = nullptr;
}

CLogFileWriter::~CLogFileWriter()
{
	closeFile();
}

bool CLogFileWriter::createFile(const char* filename)
{
	logfile=fopen(filename,"a");
	if (logfile == nullptr)
	{
		perror("fopen: ");
		return false;
	}
	return true;
}

void CLogFileWriter::closeFile()
{
	if (logfile)
	{
		flush();
		if (fclose(logfile))
			perror("fclose: ");
	}
	logfile=nullptr;
}

void CLogFileWriter::flush()
{
	if (logfile)
		fflush(logfile);
}

void CLogFileWriter::log(const char* fmt, va_list va)
{
	CLogWriter::log(fmt, va);
	if (logfile == nullptr)
		return;
	vfprintf(logfile, fmt, va);
	flush();
}



void CLogger::logva(const char* fmt, va_list va)
{
	writer.log(fmt, va);
}

void CLogger::log(const char* fmt, ...)
{
	va_list va;
	va_start(va,fmt);
	logva(fmt, va);
	va_end(va);
}

void CLogger::logLine(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	logHeader();
	logva(fmt, va);
	if (fmt[0] == '\0' || fmt[strlen(fmt) - 1] !=  '\n')
		log("\n");
	va_end(va);
}


void CLogger::logTime()
{
	char time_string[256];
	time_t now;
	char *c;
	now=time(nullptr);
#ifdef WIN32
	/* The MS runtime keeps this buffer per thread */
	c=ctime(&now);
#else
	/* Game threads log concurrently, ctime() would share one static buffer */
	char time_buffer[64];
	c=ctime_r(&now, time_buffer);
#endif
	if (c == nullptr)
		return;
	strncpy(time_string,c,sizeof(time_string)-1);
	time_string[sizeof(time_string)-1]='\0';
	if (time_string[0] != '\0')
		time_string[strlen(time_string)-1]='\0';
	log("         %s\n", time_string);
}




CGameLogger::CGameLogger(CLogWriter& _writer, int _game_number)
:CLogger(_writer)
{
	game_number=_game_number;
}

void CGameLogger::logHeader()
{
	if (game_number == 0) {
		log("[-]: ", game_number);
	} else {
		log("[%d]: ", game_number);
	}
}


