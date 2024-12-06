#include <ncurses.h>

/*
 * main - first introduction to ncurses
 * Reuturn : 0 on success
 */
int main(void)
{
  char letter;

  initscr();
  printw("Press any key");

  letter = getch();
  clear();
  printw("The key you pressed is: %c", letter);
  refresh();
  getch();
  endwin();
  return (0);
}
