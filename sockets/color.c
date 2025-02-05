/**
 * For compiling program run:
 * gcc -std=c99 color_output.c -o color_output.run
 */
#include <stdio.h>

#define ANSI_RESET_ALL "\x1b[0m"

#define ANSI_COLOR_BLACK "\x1b[30m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_WHITE "\x1b[37m"

#define ANSI_BACKGROUND_BLACK "\x1b[40m"
#define ANSI_BACKGROUND_RED "\x1b[41m"
#define ANSI_BACKGROUND_GREEN "\x1b[42m"
#define ANSI_BACKGROUND_YELLOW "\x1b[43m"
#define ANSI_BACKGROUND_BLUE "\x1b[44m"
#define ANSI_BACKGROUND_MAGENTA "\x1b[45m"
#define ANSI_BACKGROUND_CYAN "\x1b[46m"
#define ANSI_BACKGROUND_WHITE "\x1b[47m"

#define ANSI_STYLE_BOLD "\x1b[1m"
#define ANSI_STYLE_ITALIC "\x1b[3m"
#define ANSI_STYLE_UNDERLINE "\x1b[4m"

int main(int argc, const char **argv)
{
  puts("## Print color ##");
  printf("=> " ANSI_COLOR_BLACK "This text is BLACK!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_COLOR_RED "This text is RED!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_COLOR_GREEN "This text is GREEN!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_COLOR_YELLOW "This text is YELLOW!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_COLOR_BLUE "This text is BLUE!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_COLOR_MAGENTA "This text is MAGENTA!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_COLOR_CYAN "This text is CYAN!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_COLOR_WHITE "This text is WHITE!" ANSI_RESET_ALL "\n");

  puts("\n## Print style ##");
  printf("=> " ANSI_STYLE_BOLD "This text is BOLD!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_STYLE_ITALIC "This text is ITALIC!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_STYLE_UNDERLINE "This text is UNDERLINE!" ANSI_RESET_ALL
         "\n");

  puts("\n## Print background ##");
  printf("=> " ANSI_BACKGROUND_BLACK "This BG is BLACK!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_BACKGROUND_RED "This BG is RED!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_BACKGROUND_GREEN "This BG is GREEN!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_BACKGROUND_YELLOW "This BG is YELLOW!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_BACKGROUND_BLUE "This BG is BLUE!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_BACKGROUND_MAGENTA "This BG is MAGENTA!" ANSI_RESET_ALL
         "\n");
  printf("=> " ANSI_BACKGROUND_CYAN "This BG is CYAN!" ANSI_RESET_ALL "\n");
  printf("=> " ANSI_BACKGROUND_WHITE "This BG is WHITE!" ANSI_RESET_ALL "\n");

  return 0;
}
