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


/**
 * Builder for the structured fields of a log line: event=<name> followed by
 * key=value pairs. Keys and token values are copied verbatim, so they must be
 * trusted constants without spaces, quotes or control characters. Free text
 * (anything user controlled) belongs into the msg argument of logEvent().
 * Fields that do not fit into the buffer are dropped.
 **/
class LogFields
{
private:
	char buf[256];
	size_t len;
	void append(const char* fmt, ...) LOGGER_PRINTF(2, 3);
public:
	explicit LogFields(const char* event);

	LogFields& num(const char* key, long long value);
	LogFields& dec(const char* key, double value, int digits = 2);
	LogFields& tok(const char* key, const char* value);
	LogFields& client(int slot) { return num("client", slot); }
	LogFields& err(int error_number) { return num("errno", error_number); }

	const char* str() const { return buf; }
};

class CLogger
{
private:
	CLogWriter& writer;
	void vlog(LogLevel level, const char* fields, const char* fmt, va_list va);
protected:
	/* Game number added as game=N to every line; 0 omits the field */
	virtual int gameNumber() const { return 0; }
	/* Unique game id added as game_id=...; nullptr omits the field */
	virtual const char* gameId() const { return nullptr; }
public:
	CLogger(CLogWriter& _writer): writer(_writer) { }
	virtual ~CLogger() { }

	/* Emits one line. The event name and fields come from f, fmt is a printf
	   format for the human readable msg field. */
	void logEvent(LogLevel level, const LogFields& f, const char* fmt, ...) LOGGER_PRINTF(4, 5);
};

class CGameLogger:public CLogger
{
public:
	/* 16 hex digits plus terminator */
	static const int GAME_ID_SIZE = 17;
private:
	int game_number;
	char game_id[GAME_ID_SIZE];
protected:
	virtual int gameNumber() const { return game_number; }
	virtual const char* gameId() const { return game_id[0] ? game_id : nullptr; }
public:
	/* _game_id may be nullptr for none. The game number can be set later,
	   once the game has started. */
	CGameLogger(CLogWriter& _writer, int _game_number, const char* _game_id = nullptr);

	void setGameNumber(int n) { game_number = n; }

	/* Writes a random 64 bit id as 16 lowercase hex digits into out */
	static void generateGameId(char out[GAME_ID_SIZE]);
};


#endif
