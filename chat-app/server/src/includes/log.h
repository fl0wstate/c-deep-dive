#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

typedef enum
{
  INFO,
  PROMPT,
  DEBUG,
  ERROR
} logll;

/* ANSI color codes */
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

void _log_message(logll level, const char *format, ...);
#define LOG(level, format, ...) _log_message(level, format, ##__VA_ARGS__)

#endif
