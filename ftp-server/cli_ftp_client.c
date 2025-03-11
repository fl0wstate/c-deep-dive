#include "ftp.h"
#include <stdlib.h>
void ftp_execute_command(int socket_fd, char **command_line_args);

// this will retun an array of tokenized argumenst passed by th user
char **ftp_commands(char *command_line_buffer, const char *delimiter)
{
  // allocate memory to hold enough tokens
  size_t position = 0;
  char **commands = malloc(FTP_TOKEN_BUFF * (sizeof(char *)));
  if (!commands)
  {
    LOG(ERROR, "memory allocation to the command holding the tokens failed");
    return NULL;
  }

  char *command_token = NULL;

  command_token = strtok(command_line_buffer, delimiter);
  while (command_token)
  {
    commands[position] = command_token;
    position++;

    // if then reallocate
    if (position >= FTP_TOKEN_BUFF)
    {
      position *= 2;
      commands = realloc(commands, position * sizeof(char *));
      if (!commands)
      {
        LOG(ERROR, "reallocation failed @%d", __LINE__);
        return NULL;
      }
      command_token = strtok(NULL, delimiter);
    }
  }

  commands[position] = NULL;

  return (commands);
}
// start implementing the strtok

char *ftp_getline(int len, FILE *stream)
{
  size_t inital_size = (!len) ? SMALL_BUFF : len, i = 0;
  int count = 0;
  ssize_t bytes_read = 0;

  char *command_line_buffer;
  // allocate memory to the local buffer
  command_line_buffer = malloc(inital_size);
  if (!command_line_buffer)
  {
    LOG(ERROR,
        "<FTP_LOG> Failed to allocate memory to the command line buffer @line "
        "[%d]",
        __LINE__);
    return NULL;
  }
  while ((bytes_read = read(stream->_fileno, &(command_line_buffer)[i++], 1)) >
         REOF)
  {
    // if the storage is not enough realocate more storage
    if (i >= inital_size)
    {
      count++;
      inital_size *= 2;
      command_line_buffer = realloc(command_line_buffer, inital_size);
      if (!command_line_buffer)
      {
        LOG(ERROR,
            "<FTP_LOG> Failed to allocate memory to the command line buffer "
            "@line "
            "[%d]",
            __LINE__);
        return NULL;
      }
    }
    if (command_line_buffer[i - 1] == '\n' || command_line_buffer[i] == EOF)
      break;
  }

  command_line_buffer[i] = '\0';

  if (count)
    LOG(DEBUG, "This is the number of reallocation [%d]", count);

  return command_line_buffer;
}

// handling the commands inputed by the user
void ftp_cli_parser()
{
  while (1)
  {
    char *command_line;
    char **command_line_args;

    fprintf(stdout, "ftp>> ");
    fflush(stdout);
    command_line = ftp_getline(SMALL_BUFF, stdin);
    if (!command_line)
    {
      LOG(ERROR, "Parser Failed closing this interactive ftp shell");
      exit(EXIT_FAILURE);
    }
    fprintf(stdout, ANSI_COLOR_YELLOW "[OUT] %s" ANSI_RESET_ALL, command_line);

    free(command_line);
    free(command_line_args);
  }
}

// Entry point
int main(int argc, char *argv[])
{
  ftp_cli_parser();
  return EXIT_SUCCESS;
}
