#include "ftp.h"

void LOG(logll level, const char *format, ...)
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
      fprintf(stdout, ANSI_COLOR_GREEN "%s:[INFO]", timestamp);
      break;
    case DEBUG:
      fprintf(stdout, ANSI_COLOR_CYAN "%s:[DEBUG]", timestamp);
      break;
    case ERROR:
      fprintf(stderr, ANSI_COLOR_RED "%s:[ERROR]", timestamp);
      break;
    }

  va_start(args, format);
  switch (level)
    {
    case INFO:
      vfprintf(stdout, format, args);
      fprintf(stdout, "\n");
      break;
    case DEBUG:
      vfprintf(stdout, format, args);
      fprintf(stdout, "\n");
      break;
    case ERROR:
      vfprintf(stderr, format, args);
      fprintf(stderr, "\n");
      break;
    }
  va_end(args);
}
