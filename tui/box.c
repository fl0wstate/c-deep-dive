#include <ncurses.h>

/* This program demonstrates the use of the box() function to draw a box around
 * a window. */
int main(void)
{
  initscr();
  int start_y, start_x, height, width, row, col;
  height = 10;
  width = 20;

  getmaxyx(stdscr, col, row);
  start_y = (col - height) / 2;
  start_x = (row - width) / 2;

  WINDOW *win = newwin(height, width, start_y, start_x);
  refresh();

  /* box(win, 0, 0); */
  // rounded corners
  wborder(win, 0, 0, 0, 0, '󱞫', '+', '+', '+');
  mvwprintw(win, 0, 2, "box");
  wrefresh(win);

  int ch = getch();
  endwin();
  return (0);
}
