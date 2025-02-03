#include <stdarg.h>
#include <stdio.h>
#include <time.h>

typedef enum LOG_LEVEL
{
  INFO,
  DEBUG,
  ERROR
} logll;

void LOG(logll level, const char *format, ...)
{
  va_list args;
  time_t now;
  struct tm *timeinfo;
  char timestamp[20];

  // get the time
  time(&now);
  // store the time in a timeinfo struct
  timeinfo = localtime(&now);
  // collect the relevant information from the timeinfo strcut
  strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

  // prefix timestamps to the logs
  switch (level)
  {
  case INFO:
    fprintf(stdout, "[%s INFO] ", timestamp);
    break;
  case DEBUG:
    fprintf(stdout, "[%s DEBUG] ", timestamp);
    break;
  case ERROR:
    fprintf(stderr, "[%s ERROR] ", timestamp);
    break;
  }

  va_start(args, format);
  switch (level)
  {
  case INFO:
  case DEBUG:
    vfprintf(stdout, format, args);
    break;
  case ERROR:
    vfprintf(stderr, format, args);
    break;
  }
  va_end(args);
}

int main(void)
{

  for (int i = 0; i < 5; i++)
  {
    LOG(ERROR, "Test for each timestamp in every element [%d]\n", i);
  }
  return (0);
}
