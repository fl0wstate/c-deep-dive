#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "../includes/log.h"

void _log_message(logll level, const char *format, ...)
{
  va_list args;
  time_t now;
  struct tm *timeinfo;
  char timestamp[20];

  time(&now);
  timeinfo = localtime(&now);

  strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

  switch (level)
  {
  case INFO:
    fprintf(stdout, GREEN "%s:[INFO] " RESET, timestamp);
    break;

  case ERROR:
    fprintf(stderr, RED "%s:[ERROR] " RESET, timestamp);
    break;

  case DEBUG:
    fprintf(stdout, YELLOW "%s:[DEBUG] " RESET, timestamp);
    break;

  default:
    fprintf(stderr, GREEN "%s:[UNKNOWN] " RESET, timestamp);
    break;
  }

  va_start(args, format);
  switch (level)
  {
  case INFO:
  case DEBUG:
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    break;
  case ERROR:
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    break;
  default:
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    break;
  }
  va_end(args);
}
