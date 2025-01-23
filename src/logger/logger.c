#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"

#define TYPE_DEBUG 0
#define TYPE_INFO 1
#define TYPE_WARNING 2
#define TYPE_ERROR 3
#define TYPE_CRITICAL 4

char debug_enabled = 0;
char info_enabled = 0;
char warning_enabled = 1;
char error_enabled = 1;
char critical_enabled = 1;

const char BLANK_LABEL[] = "NOTSET";
const char DEBUG_LABEL[] = "DEBUG";
const char INFO_LABEL[] = "INFO";
const char WARNING_LABEL[] = "WARNING";
const char ERROR_LABEL[] = "ERROR";
const char CRITICAL_LABEL[] = "CRITICAL";

static const char* get_logging_label(const int logging_type) {
	switch (logging_type) {
	case TYPE_DEBUG:
		return DEBUG_LABEL;
	case TYPE_INFO:
		return INFO_LABEL;
	case TYPE_WARNING:
		return WARNING_LABEL;
	case TYPE_ERROR:
		return ERROR_LABEL;
	case TYPE_CRITICAL:
		return CRITICAL_LABEL;
	}
	return BLANK_LABEL;
}

static int logging_printf(const int logging_type, const char* fmt, va_list args) {
	const char* label = get_logging_label(logging_type);
	const size_t extended_fmt_length = strlen(label) + strlen(fmt) + 4;
	char extended_fmt[extended_fmt_length];
	(void) strcpy(extended_fmt, "[");
	(void) strcat(extended_fmt, label);
	(void) strcat(extended_fmt, "] ");
	(void) strcat(extended_fmt, fmt);
	return vfprintf(stderr, extended_fmt, args);
}

void config_logging(char enable_debug, char enable_info, char enable_warning, char enable_error, char enable_critical) {
	debug_enabled = enable_debug;
	info_enabled = enable_info;
	warning_enabled = enable_warning;
	error_enabled = enable_error;
	critical_enabled = enable_critical;
}

int debug_printf(const char* fmt, ...) {
	va_list args;
	if (!debug_enabled) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_printf(TYPE_DEBUG, fmt, args);
	va_end(args);
	return ret;
}

int info_printf(const char* fmt, ...) {
	va_list args;
	if (!info_enabled) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_printf(TYPE_INFO, fmt, args);
	va_end(args);
	return ret;
}

int warning_printf(const char* fmt, ...) {
	va_list args;
	if (!warning_enabled) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_printf(TYPE_WARNING, fmt, args);
	va_end(args);
	return ret;
}

int error_printf(const char* fmt, ...) {
	va_list args;
	if (!error_enabled) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_printf(TYPE_ERROR, fmt, args);
	va_end(args);
	return ret;
}

int critical_printf(const char* fmt, ...) {
	va_list args;
	if (!critical_enabled) {
		return 0;
	}
	va_start(args, fmt);
	int ret = logging_printf(TYPE_CRITICAL, fmt, args);
	va_end(args);
	return ret;
}
