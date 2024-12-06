#include <ncurses.h>

int main(void)
{
  int x, y;

  initscr();
  clear();

  getyx(stdscr, y, x);
  printw("x = %d, y = %d", x, y);
  refresh();

  printw("\n\n");
  for (x = 10; x < 20; x++)
  {
    for (y = 10; y < x; y++)
    {
      mvprintw(y, x, "X");
      refresh();
    }
  }

  // Draw a rectangle
  for (x = 30; x < 40; x++)
  {
    for (y = 20; y < 40; y++)
    {
      mvprintw(y, x, "X");
      refresh();
    }
  }

  getch();
  endwin();
  return (0);
}
