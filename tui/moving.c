#include <ncurses.h>
#include <unistd.h>

int main(void)
{
  int key, x, y;
  initscr();
  keypad(stdscr, TRUE);
  noecho();

  printw("Press the arrow keys to move 0, q to quit");
  refresh();
  x = y = 5;

  while (key != 'q')
  {
    key = getch();
    move(y, x);

    mvprintw(y, x, ".");
    refresh();

    if (key == 'h')
    {
      x--;
      if (x < 0)
        x = 0;
    }

    if (key == 'k')
    {
      y--;
      if (y < 0)
        y = 0;
    }

    if (key == 'j')
    {
      y++;
      if (y > 40)
        y = 40;
    }

    if (key == 'l')
    {
      x++;
      if (x > 40)
        x = 40;
    }
  }
  endwin();
  return (0);
}
