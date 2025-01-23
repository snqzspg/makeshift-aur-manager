#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

void config_logging(char enable_debug, char enable_info, char enable_warning, char enable_error, char enable_critical);

int debug_printf(const char* fmt, ...);
int info_printf(const char* fmt, ...);
int warning_printf(const char* fmt, ...);
int error_printf(const char* fmt, ...);
int critical_printf(const char* fmt, ...);

#endif // LOGGER_H_INCLUDED