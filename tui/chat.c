#include <locale.h>
#include <ncurses.h>

void draw_rounded_border(WINDOW *win);
// Function prototypes
void init_layout(WINDOW **main_win, WINDOW **input_win, int rows, int cols);
void handle_input(WINDOW *input_win, WINDOW *main_win);

int main()
{
  setocale(LC_ALL, "utf-8");
  initscr();
  cbreak();
  noecho();

  int rows, cols;
  getmaxyx(stdscr, rows, cols);

  WINDOW *main_win, *input_win;

  // Initialize windows
  init_layout(&main_win, &input_win, rows, cols);

  // Main event loop
  while (1)
  {
    handle_input(input_win, main_win);
  }

  // Cleanup
  delwin(main_win);
  delwin(input_win);
  endwin();

  return 0;
}

void init_layout(WINDOW **main_win, WINDOW **input_win, int rows, int cols)
{
  *main_win = newwin(rows - 3, cols, 0, 0);  // Main chat window
  *input_win = newwin(3, cols, rows - 3, 0); // Input box

  scrollok(*main_win, TRUE);
  draw_rounded_border(*main_win);
  draw_rounded_border(*input_win);

  wrefresh(*main_win);
  wrefresh(*input_win);
}

void handle_input(WINDOW *input_win, WINDOW *main_win)
{
  char input[256];
  mvwgetnstr(input_win, 1, 1, input, sizeof(input) - 1);

  // Display the message in the main window
  mvwprintw(main_win, 2, 2, "You: %s\n", input);
  wrefresh(main_win);

  // Clear the input window
  wclear(input_win);
  box(input_win, 0, 0);
  wrefresh(input_win);
}

void draw_rounded_border(WINDOW *win)
{
  int rows, cols;
  getmaxyx(win, rows, cols);

  // Draw corners
  mvwaddch(win, 0, 0, '╭');
  mvwaddch(win, 0, cols - 1, '╮');
  mvwaddch(win, rows - 1, 0, '╰');
  mvwaddch(win, rows - 1, cols - 1, '╯');

  // Draw horizontal lines
  for (int x = 1; x < cols - 1; x++)
  {
    mvwaddch(win, 0, x, '─');
    mvwaddch(win, rows - 1, x, '─');
  }

  // Draw vertical lines
  for (int y = 1; y < rows - 1; y++)
  {
    mvwaddch(win, y, 0, '│');
    mvwaddch(win, y, cols - 1, '│');
  }

  wrefresh(win);
}
