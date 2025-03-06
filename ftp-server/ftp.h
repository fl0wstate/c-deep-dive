#ifndef __FTP_H_H__
#define __FTP_H_H__

#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum LOG_LEVEL
{
  INFO,
  DEBUG,
  ERROR,
} logll;

// ANSI_COLORS
#define ANSI_RESET_ALL "\x1b[0m"
#define ANSI_COLOR_BLACK "\x1b[30m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_WHITE "\x1b[37m"

// LOGS
void LOG(logll level, const char *format, ...);

#endif
