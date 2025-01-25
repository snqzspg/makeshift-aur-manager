#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

// #define TYPE_DEBUG 0
// #define TYPE_INFO 1
// #define TYPE_WARNING 2
// #define TYPE_ERROR 3
// #define TYPE_CRITICAL 4

// char debug_enabled = 0;
// char info_enabled = 0;
// char warning_enabled = 1;
// char error_enabled = 1;
// char critical_enabled = 1;

enum __logging_level set_level = NOTE;
int                  color_set = 0;

const char BLANK_LABEL[]    = "NOTSET";
const char DEBUG_LABEL[]    = "DEBUG";
const char INFO_LABEL[]     = "INFO";
const char NOTE_LABEL[]     = "NOTE";
const char WARNING_LABEL[]  = "WARNING";
const char ERROR_LABEL[]    = "ERROR";
const char CRITICAL_LABEL[] = "CRITICAL";

const char BLANK_COLOURED_LABEL[]    = "\033[1;35mNOTSET\033[0m";
const char DEBUG_COLOURED_LABEL[]    = "\033[1;32mDEBUG\033[0m";
const char INFO_COLOURED_LABEL[]     = "\033[1;34mINFO\033[0m";
const char NOTE_COLOURED_LABEL[]     = "\033[1;36mNOTE\033[0m";
const char WARNING_COLOURED_LABEL[]  = "\033[1;33mWARNING\033[0m";
const char ERROR_COLOURED_LABEL[]    = "\033[1;31mERROR\033[0m";
const char CRITICAL_COLOURED_LABEL[] = "\033[1;31mCRITICAL\033[0m";

static const char* get_logging_label(enum __logging_level logging_type) {
	switch (logging_type) {
	case DEBUG:
		return color_set ? DEBUG_COLOURED_LABEL    : DEBUG_LABEL;
	case INFO:
		return color_set ? INFO_COLOURED_LABEL     : INFO_LABEL;
	case NOTE:
		return color_set ? NOTE_COLOURED_LABEL     : NOTE_LABEL;
	case WARNING:
		return color_set ? WARNING_COLOURED_LABEL  : WARNING_LABEL;
	case ERROR:
		return color_set ? ERROR_COLOURED_LABEL    : ERROR_LABEL;
	case CRITICAL:
		return color_set ? CRITICAL_COLOURED_LABEL : CRITICAL_LABEL;
	}
	return BLANK_LABEL;
}

int is_logging_type_enabled(enum __logging_level type) {
    return type >= set_level;
}

static int logging_vprintf(enum __logging_level logging_type, const char* fmt, va_list args) {
	if (!is_logging_type_enabled(logging_type)) {
		return 0;
	}

	const char* label = get_logging_label(logging_type);
	const size_t extended_fmt_length = snprintf(NULL, 0, "[%s]%s", label, fmt);
	char extended_fmt[extended_fmt_length + 1];
    (void) snprintf(extended_fmt, extended_fmt_length + 1, "[%s]%s", label, fmt);
	return vfprintf(stderr, extended_fmt, args);
}

void logging_perror(enum __logging_level logging_type, const char* prefix) {
	if (!is_logging_type_enabled(logging_type)) {
		return;
	}

	const char* label = get_logging_label(logging_type);
	size_t l = snprintf(NULL, 0, "[%s]%s", label, prefix);
	char error_pfx[l + 1];
	(void) snprintf(error_pfx, l + 1, "[%s]%s", label, prefix);
	perror(error_pfx);
}

void config_logging(enum __logging_level set_logging_type, int show_color) {
	set_level = set_logging_type;
	color_set = show_color;
}

int debug_printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = logging_vprintf(DEBUG, fmt, args);
	va_end(args);
	return ret;
}

void debug_perror(const char* prefix) {
	logging_perror(DEBUG, prefix);
}

int info_printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = logging_vprintf(INFO, fmt, args);
	va_end(args);
	return ret;
}

void info_perror(const char* prefix) {
	logging_perror(INFO, prefix);
}

int note_printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = logging_vprintf(NOTE, fmt, args);
	va_end(args);
	return ret;
}

void note_perror(const char* prefix) {
	logging_perror(NOTE, prefix);
}

int warning_printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = logging_vprintf(WARNING, fmt, args);
	va_end(args);
	return ret;
}

void warning_perror(const char* prefix) {
	logging_perror(WARNING, prefix);
}

int error_printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = logging_vprintf(ERROR, fmt, args);
	va_end(args);
	return ret;
}

void error_perror(const char* prefix) {
	logging_perror(ERROR, prefix);
}

int critical_printf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int ret = logging_vprintf(CRITICAL, fmt, args);
	va_end(args);
	return ret;
}

void critical_perror(const char* prefix) {
	logging_perror(CRITICAL, prefix);
}
