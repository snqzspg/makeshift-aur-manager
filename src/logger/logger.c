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
	const char* label = get_logging_label(logging_type);
	const size_t extended_fmt_length = snprintf(NULL, 0, "[%s]%s", label, fmt);
	char extended_fmt[extended_fmt_length + 1];
    (void) snprintf(extended_fmt, extended_fmt_length + 1, "[%s]%s", label, fmt);
	// (void) strcpy(extended_fmt, "[");
	// (void) strcat(extended_fmt, label);
	// (void) strcat(extended_fmt, "] ");
	// (void) strcat(extended_fmt, fmt);
	return vfprintf(stderr, extended_fmt, args);
}

void config_logging(enum __logging_level set_logging_type, int show_color) {
	set_level = set_logging_type;
	color_set = show_color;
}

int debug_printf(const char* fmt, ...) {
	va_list args;
	if (!is_logging_type_enabled(DEBUG)) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_vprintf(DEBUG, fmt, args);
	va_end(args);
	return ret;
}

int info_printf(const char* fmt, ...) {
	va_list args;
	if (!is_logging_type_enabled(INFO)) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_vprintf(INFO, fmt, args);
	va_end(args);
	return ret;
}

int note_printf(const char* fmt, ...) {
	va_list args;
	if (!is_logging_type_enabled(NOTE)) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_vprintf(NOTE, fmt, args);
	va_end(args);
	return ret;
}

int warning_printf(const char* fmt, ...) {
	va_list args;
	if (!is_logging_type_enabled(WARNING)) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_vprintf(WARNING, fmt, args);
	va_end(args);
	return ret;
}

int error_printf(const char* fmt, ...) {
	va_list args;
	if (!is_logging_type_enabled(ERROR)) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_vprintf(ERROR, fmt, args);
	va_end(args);
	return ret;
}

int critical_printf(const char* fmt, ...) {
	va_list args;
	if (!is_logging_type_enabled(CRITICAL)) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_vprintf(CRITICAL, fmt, args);
	va_end(args);
	return ret;
}
