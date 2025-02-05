#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define BOX_WIDTH 30 // Width of the side box

void render_ascii_content(FILE *fp, char *buffer, int max_height, int max_y,
                          int max_x)
{
  // First pass to get dimensions
  int content_height = 0;
  int content_width = 0;

  while (fgets(buffer, MAX_LINE_LENGTH, fp) && content_height < max_height)
  {
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
      len--; // Don't count newline in width
    }
    if (len > content_width)
    {
      content_width = len;
    }
    content_height++;
  }

  // Calculate centering positions
  int start_y = (max_y - content_height) / 2;
  int start_x = (max_x - content_width) / 2;

  // Reset file position for second pass
  rewind(fp);

  // Clear screen
  clear();

  // Second pass to display content
  int y = 0;
  while (fgets(buffer, MAX_LINE_LENGTH, fp) && y < max_height)
  {
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
      buffer[len - 1] = '\0'; // Remove newline
    }
    mvprintw(start_y + y, 0, "%s", buffer);
    y++;
  }
}

// i want to provide a link to an image
// use asciigen to generate a output.txt file
// render it to the stdscr
// make a get request with curllib api
// get some quotes from an api
// make an ascii timer

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <ascii_file>\n", argv[0]);
    return 1;
  }

  // Open the ASCII art file
  FILE *file = fopen(argv[1], "r");
  if (file == NULL)
  {
    perror("Error opening file");
    return 1;
  }

  // Initialize ncurses
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0); // Hide cursor

  // Get terminal dimensions
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  // Create two windows: one for content and one for the box
  int content_width = max_x - BOX_WIDTH;
  WINDOW *content_win = newwin(max_y, content_width, 0, 0);
  WINDOW *box_win = newwin(max_y, BOX_WIDTH, 0, content_width);

  // Buffer for reading lines
  char buffer[MAX_LINE_LENGTH];

  // Render ASCII content in the left window
  render_ascii_content(file, buffer, max_y, max_y, content_width);
  box(content_win, 0, 0);
  mvwprintw(content_win, max_y - 2, 2, "Press 'q' to quit");

  // Draw box and add content to the right window
  box(box_win, 0, 0);
  mvwprintw(box_win, 1, 2, "Side Box");
  mvwprintw(box_win, 2, 2, "----------");
  mvwprintw(box_win, 3, 2, "Information:");
  mvwprintw(box_win, 4, 2, "Add your");
  mvwprintw(box_win, 5, 2, "content here");

  // Refresh both windows
  wrefresh(content_win);
  wrefresh(box_win);

  // Wait for key
  getch();

  // Clean up
  delwin(content_win);
  delwin(box_win);
  fclose(file);
  endwin();

  return 0;
}
