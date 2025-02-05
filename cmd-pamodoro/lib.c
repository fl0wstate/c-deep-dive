#include "pamo.h"
#include <ncurses.h>
volatile bool running = true;

void handle_signal(int signal)
{
  if (signal == SIGINT)
    running = false;
}

/**
 * write_data - Allow to write the data received from the server to a file
 * @ptr: pointer to the data received from the server
 * @size: size of the data received
 * @nmemb: number of members
 * @stream: pointer to the stream
 */
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  char filename[] = "resp.json";
  FILE *file = fopen(filename, "w");
  if (!file)
  {
    perror("fopen");
    return (-1);
  }

  size_t written = fwrite(ptr, size, nmemb, file);

  fclose(file);
  return written;
}

/**
 * C_GET - Allow to make a GET request to the server/api endpoint
 * @http_req_url: string pointer to a url used to make the get request
 * Return: void
 */
void C_GET(char *http_req_url)
{
  CURL *curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  if (curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, http_req_url);

    // setting the callback function that is going to be called
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
}

/**
 * json_parser - Allow parsing of the json file to collect only the required
 * data that i need to display to the user.
 * @filename: pointer to a filename which already exists in the project folder
 * Return: void
 */
char *json_parser(char *filename)
{
  JSENSE *j = jse_from_file(filename);
  // TODO: Add the parser to handle multiple values passed to it
  return jse_get(j, "title");
}

/**
 * render_text - Allow to render the text to the screen
 * @text: const char pointer to the text to be rendered
 * Return: void
 */
void render_text(const char *text)
{
  int max_x, max_y;
  getmaxyx(stdscr, max_y, max_x);
  clear();
  mvprintw(max_y / 2, (max_x - strlen(text)) / 2, "%s", text);
  refresh();
}
/**
 * render_ascii_content - Allow to render the ascii content to the screen
 * @fp: pointer to the file containing the ascii content
 * @buffer: pointer to the buffer
 * @max_height: integer value of the max height
 * @max_y: integer value of the max y
 * @max_x: integer value of the max x
 * Return: void
 */
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
    mvprintw(start_y + y, start_x, "%s", buffer);
    y++;
  }
}

int main(int ac, char **av)
{
  if (ac != 2)
  {
    fprintf(stdout, "Usage: bin <json_file>\n");
    return (EXIT_FAILURE);
  }

  initscr();            // Start ncurses mode
  noecho();             // Disable echoing of typed characters
  curs_set(FALSE);      // Hide the cursor
  keypad(stdscr, TRUE); // Enable the keypad
  timeout(500);

  signal(SIGINT, handle_signal);

  C_GET("https://jsonplaceholder.typicode.com/posts/2");
  const char *text = json_parser(av[1]);
  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);
  char buffer[MAX_LINE_LENGTH];

  render_ascii_content(fopen("./asciigen/output.txt", "r"), buffer, max_y,
                       max_y, max_x);
  // you will need to implement a counter here to keep track of the time spent
  while (running)
  {
    if (getch() == KEY_RESIZE)
    {
      // run the display function again to get the new max_x and max_y
      render_text(text);
    }

    // still render the text even when the key pressed is not resize
    render_text(text);
  }

  // close the ncurses stdscr
  endwin();

  return (0);
}
