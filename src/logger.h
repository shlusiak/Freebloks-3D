/**
 * logger.h
 * Author: Sascha Hlusiak
 *
 * Class for logging output to the console or to a file.
 *
 * Every call emits exactly one logfmt line:
 *   ts=2026-10-24T01:02:03.456Z level=warn game=3 client=1 msg="Ignoring move"
 **/

#ifndef _LOGGER_H_INCLUDED_
#define _LOGGER_H_INCLUDED_

#include <stdio.h>
#include <stdarg.h>

#if defined(__GNUC__)
  #define LOGGER_PRINTF(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
  #define LOGGER_PRINTF(fmt_idx, arg_idx)
#endif

enum LogLevel
{
	LOGLEVEL_INFO,
	LOGLEVEL_WARN,
	LOGLEVEL_ERROR
};

class CLogWriter
{
private:
	CLogWriter* next;
public:
	CLogWriter() { next = nullptr; }
	virtual ~CLogWriter();

	void addLogWriter(CLogWriter* _next);
	virtual void log(const char* fmt, va_list va);
};

class CStdoutWriter:public CLogWriter
{
public:
	virtual void log(const char* fmt, va_list va);
};

class CLogFileWriter:public CLogWriter
{
private:
	FILE *logfile;
public:
	CLogFileWriter();
	virtual ~CLogFileWriter();
	void closeFile();
	void flush();

	/* Returns false if the file could not be opened */
	bool createFile(const char* filename);
	virtual void log(const char* fmt, va_list va);
};


class CLogger
{
private:
	CLogWriter& writer;
	void vlog(LogLevel level, const char* fields, const char* fmt, va_list va);
protected:
	/* Game number added as game=N to every line; 0 omits the field */
	virtual int gameNumber() const { return 0; }
public:
	CLogger(CLogWriter& _writer): writer(_writer) { }
	virtual ~CLogger() { }

	/* fmt is a printf format for the msg field. */
	void log(LogLevel level, const char* fmt, ...) LOGGER_PRINTF(3, 4);

	/* Same, with a client slot index added as client=N */
	void logClient(LogLevel level, int client, const char* fmt, ...) LOGGER_PRINTF(4, 5);

	/* Same, with additional preformatted fields. fields is a space separated
	   list of key=value tokens and is copied verbatim: keys and values must be
	   trusted and free of spaces, quotes and control characters. */
	void logFields(LogLevel level, const char* fields, const char* fmt, ...) LOGGER_PRINTF(4, 5);
};

class CGameLogger:public CLogger
{
private:
	int game_number;
protected:
	virtual int gameNumber() const { return game_number; }
public:
	CGameLogger(CLogWriter& _writer, int _game_number);
};


#endif
