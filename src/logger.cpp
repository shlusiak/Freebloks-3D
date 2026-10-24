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
#include <chrono>
#include <mutex>
#include <string>
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
	/* Stdout is block buffered when piped, which would delay lines in a log collector */
	fflush(stdout);
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



static void writeLine(CLogWriter& writer, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	writer.log(fmt, va);
	va_end(va);
}

/* Serializes whole lines over all loggers, so concurrent games cannot interleave */
static std::mutex logMutex;

static const char* levelName(LogLevel level)
{
	switch (level) {
	case LOGLEVEL_WARN: return "warn";
	case LOGLEVEL_ERROR: return "error";
	default: return "info";
	}
}

static bool isBareValue(const char* v)
{
	if (v[0] == '\0')
		return false;
	for (; *v; v++) {
		char c = *v;
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '.' || c == '_' || c == '-' || c == ':' || c == '/' || c == '+'))
			return false;
	}
	return true;
}

/* Appends key=value, quoting and escaping the value when necessary */
static void appendValue(std::string& out, const char* v)
{
	if (isBareValue(v)) {
		out += v;
		return;
	}
	out += '"';
	for (; *v; v++) {
		unsigned char c = (unsigned char)*v;
		if (c == '"' || c == '\\') {
			out += '\\';
			out += (char)c;
		} else if (c == '\n') {
			out += "\\n";
		} else if (c == '\r') {
			out += "\\r";
		} else if (c == '\t') {
			out += "\\t";
		} else if (c < 0x20 || c == 0x7f) {
			char esc[8];
			snprintf(esc, sizeof(esc), "\\x%02x", c);
			out += esc;
		} else {
			out += (char)c;
		}
	}
	out += '"';
}

static void appendTimestamp(std::string& out)
{
	using namespace std::chrono;
	system_clock::time_point now = system_clock::now();
	time_t secs = system_clock::to_time_t(now);
	int ms = (int)(duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000);
	struct tm tm;
#ifdef WIN32
	gmtime_s(&tm, &secs);
#else
	gmtime_r(&secs, &tm);
#endif
	char buf[40];
	snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
	out += buf;
}

void CLogger::vlog(LogLevel level, const char* fields, const char* fmt, va_list va)
{
	char msg[1024];
	vsnprintf(msg, sizeof(msg), fmt, va);

	std::string line;
	line.reserve(256);
	line += "ts=";
	appendTimestamp(line);
	line += " level=";
	line += levelName(level);
	int game = gameNumber();
	if (game != 0) {
		char buf[32];
		snprintf(buf, sizeof(buf), " game=%d", game);
		line += buf;
	}
	if (fields && fields[0] != '\0') {
		line += ' ';
		line += fields;
	}
	line += " msg=";
	appendValue(line, msg);

	std::lock_guard<std::mutex> lock(logMutex);
	writeLine(writer, "%s\n", line.c_str());
}

void CLogger::log(LogLevel level, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vlog(level, nullptr, fmt, va);
	va_end(va);
}

void CLogger::logClient(LogLevel level, int client, const char* fmt, ...)
{
	char fields[32];
	snprintf(fields, sizeof(fields), "client=%d", client);
	va_list va;
	va_start(va, fmt);
	vlog(level, fields, fmt, va);
	va_end(va);
}

void CLogger::logFields(LogLevel level, const char* fields, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vlog(level, fields, fmt, va);
	va_end(va);
}


CGameLogger::CGameLogger(CLogWriter& _writer, int _game_number)
:CLogger(_writer)
{
	game_number=_game_number;
}
