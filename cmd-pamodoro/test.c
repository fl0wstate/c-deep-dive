#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

// Global flag to control the main loop
volatile bool running = true;

// Signal handler for SIGINT (Ctrl+C)
void handle_sigint(int signum) { running = false; }

// Function to dynamically center and display text
void display_centered_text(const char *text)
{
  int rows, cols;
  getmaxyx(stdscr, rows, cols); // Get the current terminal size

  int text_len = strlen(text);
  int start_row = rows / 2;              // Middle row
  int start_col = (cols - text_len) / 2; // Center the text horizontally

  clear(); // Clear the screen before redrawing
  mvprintw(start_row, start_col, "%s", text); // Display the text
  refresh();                                  // Refresh the screen
}

int main()
{
  // Initialize ncurses
  initscr();            // Start ncurses mode
  noecho();             // Disable echoing of typed characters
  curs_set(FALSE);      // Hide the cursor
  keypad(stdscr, TRUE); // Enable special keys (e.g., KEY_RESIZE)
  timeout(100);         // Set a timeout for getch() to avoid blocking

  // Set up the SIGINT (Ctrl+C) signal handler
  signal(SIGINT, handle_sigint);

  // Define the text to display
  const char *text = "Hello, ncurses!";

  // Main loop
  while (running)
  {
    // Check for terminal resize
    int ch = getch();
    if (ch == KEY_RESIZE)
    {
      // Redraw the text if the terminal is resized
      display_centered_text(text);
    }

    // Display the text initially or after resizing
    display_centered_text(text);
  }

  // End ncurses mode
  endwin();

  return 0;
}
