#include <curl/curl.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 1024
#define BOX_WIDTH 30
#define QUOTE_LENGTH 256

struct MemoryStruct
{
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void render_ascii_content(FILE *fp, WINDOW *win, int max_height, int max_y,
                          int max_x)
{
  char buffer[MAX_LINE_LENGTH];
  int content_height = 0;
  int content_width = 0;

  // First pass to get dimensions
  while (fgets(buffer, MAX_LINE_LENGTH, fp) && content_height < max_height)
  {
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
      len--;
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

  // Reset file position
  rewind(fp);

  // Clear window
  werase(win);

  // Second pass to display content
  int y = 0;
  while (fgets(buffer, MAX_LINE_LENGTH, fp) && y < max_height)
  {
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n')
    {
      buffer[len - 1] = '\0';
    }
    mvwprintw(win, start_y + y, start_x, "%s", buffer);
    y++;
  }
}

void fetch_quote(WINDOW *win)
{
  CURL *curl = curl_easy_init();
  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.quotable.io/random");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
    {
      // Simple parsing of the JSON response
      char *content_start = strstr(chunk.memory, "\"content\":\"");
      if (content_start)
      {
        content_start += 11; // Skip past "content":"
        char *content_end = strchr(content_start, '"');
        if (content_end)
        {
          *content_end = '\0';
          mvwprintw(win, 7, 2, "Quote:");
          // Word wrap the quote
          int y = 8;
          int x = 2;
          char *word = strtok(content_start, " ");
          while (word)
          {
            if (x + strlen(word) > BOX_WIDTH - 2)
            {
              y++;
              x = 2;
            }
            mvwprintw(win, y, x, "%s ", word);
            x += strlen(word) + 1;
            word = strtok(NULL, " ");
          }
        }
      }
    }
    curl_easy_cleanup(curl);
  }
  free(chunk.memory);
}

void update_timer(WINDOW *win, time_t start_time)
{
  int max_y, max_x;
  getmaxyx(win, max_y, max_x);
  time_t current_time = time(NULL);
  int elapsed = difftime(current_time, start_time);
  int hours = elapsed / 3600;
  int minutes = (elapsed % 3600) / 60;
  int seconds = elapsed % 60;
  mvwprintw(win, max_y - 3, 2, "Time: %02d:%02d:%02d", hours, minutes, seconds);
  wrefresh(win);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <ascii_file>\n", argv[0]);
    return 1;
  }

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
  curs_set(0);

  // Initialize colors if terminal supports them
  if (has_colors())
  {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
  }

  // Get terminal dimensions
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  // Create windows
  int content_width = max_x - BOX_WIDTH;
  WINDOW *content_win = newwin(max_y, content_width, 0, 0);
  WINDOW *box_win = newwin(max_y, BOX_WIDTH, 0, content_width);

  // Initialize curl
  curl_global_init(CURL_GLOBAL_ALL);

  // Get start time for timer
  time_t start_time = time(NULL);

  // Main loop
  int ch;
  do
  {
    // Render ASCII content
    render_ascii_content(file, content_win, max_y, max_y, content_width);
    box(content_win, 0, 0);
    mvwprintw(content_win, max_y - 2, 2,
              "Are you the same big dwang 'q' to quit");

    // Draw box and add content
    box(box_win, 0, 0);
    wattron(box_win, COLOR_PAIR(1));
    mvwprintw(box_win, 1, 2, "Side Box");
    wattroff(box_win, COLOR_PAIR(1));
    mvwprintw(box_win, 2, 2, "----------");

    // Update timer
    update_timer(box_win, start_time);

    // Fetch and display quote
    fetch_quote(box_win);

    // Refresh windows
    wrefresh(content_win);
    wrefresh(box_win);

    // Check for input with timeout
    timeout(1000); // 1 second timeout
    ch = getch();
  } while (ch != 'q');

  // Clean up
  curl_global_cleanup();
  delwin(content_win);
  delwin(box_win);
  fclose(file);
  endwin();

  return 0;
}
