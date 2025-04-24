#include "ftp.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

char *ftp_banner =
    "\n  █████▒██▓     ▒█████   █     █░  █████▒▄▄▄█████▓ ██▓███     "
    "\n▓██   ▒▓██▒    ▒██▒  ██▒▓█░ █ ░█░▓██   ▒ ▓  ██▒ ▓▒▓██░  ██▒   "
    "\n▒████ ░▒██░    ▒██░  ██▒▒█░ █ ░█ ▒████ ░ ▒ ▓██░ ▒░▓██░ ██▓▒   "
    "\n░▓█▒  ░▒██░    ▒██   ██░░█░ █ ░█ ░▓█▒  ░ ░ ▓██▓ ░ ▒██▄█▓▒ ▒ ▒ "
    "\n░▒█░   ░██████▒░ ████▓▒░░░██▒██▓ ░▒█░      ▒██▒ ░ ▒██▒ ░  ░ ░ "
    "\n ▒ ░   ░ ▒░▓  ░░ ▒░▒░▒░ ░ ▓░▒ ▒   ▒ ░      ▒ ░░   ▒▓▒░ ░  ░ ░ "
    "\n ░     ░ ░ ▒  ░  ░ ▒ ▒░   ▒ ░ ░   ░          ░    ░▒ ░        "
    "\n ░ ░     ░ ░   ░ ░ ░ ▒    ░   ░   ░ ░      ░      ░░        ░ "
    "\n           ░  ░    ░ ░      ░                               ";

// for monitaring the state of the PROMPT
static int connected = 0;

int connect_to_server_address(char *address, char *port)
{
  int socket_fd = -1, status = 0;
  //* filter for the type of address info we want IPV4 or IPV6 */
  struct addrinfo hints;
  struct addrinfo *listips, *temp;

  if (!address)
  {
    LOG(ERROR, "Invalid address");
    return -1;
  }

  // address mutation to be the same as the network address order

  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, address, &(sa.sin_addr));
  if (result < 1)
  {
    LOG(ERROR, "Address %s, is not correctly formated", address);
    return -1;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;       // type of ipv version
  hints.ai_socktype = SOCK_STREAM; // data stream TCP
  hints.ai_flags = AI_PASSIVE;     // create on from scratch

  status = getaddrinfo(address, port, &hints, &listips);
  if (status != 0)
  {
    LOG(ERROR, "Error: %s", gai_strerror(status));
    return -1;
  }

  temp = listips;
  for (; temp != NULL; temp = temp->ai_next)
  {
    socket_fd = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
    if (socket_fd == -1)
    {
      LOG(ERROR, "Error while establishing a new socket for connection");
      return -1;
    }
    // everything works out fine we need to bind it
    if (connect(socket_fd, temp->ai_addr, temp->ai_addrlen) == -1)
    {
      LOG(ERROR, "Error connecting the address to the socket_fd");
      close(socket_fd);
      continue;
    }
    break;
  }

  // free first
  freeaddrinfo(listips);

  if (temp)
  {
    LOG(DEBUG, "Connection established by this client: %d", socket_fd);
    return (socket_fd);
  }
  else
  {
    LOG(ERROR, "Failed to connect to the server");
    exit(1);
  }
}

int ftp_execute_command(char **command_line_args)
{
  struct network_packet *client_data =
      (struct network_packet *)malloc(sizeof(struct network_packet));
  // check for errors
  packet_initializer(client_data);

  int status = 0, data_read = 0;
  const char *command = command_line_args[0];

  if (strcmp(command, "connect") == 0)
  {
    char *address = command_line_args[1];
    if (address)
    {
      // this will generate the socket_fd that has a connection to the server
      connected = connect_to_server_address(address, PORT);
      if (connected == -1)
      {
        LOG(ERROR,
            "There was an error establishing a connection to this address: %s",
            address);
      }
      else
      {
        // after making a succesful connection you will need your server to
        // reply with a 220
        LOG(INFO, "Connection to the server has been established...");
        // this will keep track of the connection
        // now that the connection has been established i think you will need to
        // run all the commands from here on
      }
    }
    else
    {
      LOG(ERROR, "missing address argument <conect> <address>");
    }
  }

  LOG(DEBUG, "client fd -> %d", connected);

  // this commands will only run once the connection above is established
  if (strcmp(command, "PWD") == 0)
  {
    if (!connected)
    {
      LOG(ERROR, "Not connected to a server. Use 'connect' first.");
      free(client_data);
      return -1;
    }

    client_data->command_type = REQU;
    client_data->command_id = PWD;
    client_data->connection_id = 0;
    client_data->command_len = 0; // No data in command_buffer for PWD

    print_packet(client_data, 1);

    host_to_network_presentation(client_data);

    print_packet(client_data, 0);

    size_t total_sent = 0;
    while (total_sent < sizeof(struct network_packet))
    {
      data_read = send(connected, (char *)client_data + total_sent,
                       sizeof(struct network_packet) - total_sent, 0);
      if (data_read < 0)
      {
        LOG(ERROR, "Send failed: %s", strerror(data_read));
        free(client_data);
        return -1;
      }
      total_sent += data_read;
    }

    // Receive into client_data
    size_t total_read = 0;
    while (total_read < sizeof(struct network_packet))
    {
      data_read = recv(connected, (char *)client_data + total_read,
                       sizeof(struct network_packet) - total_read, 0);
      if (data_read <= 0)
      {
        LOG(ERROR, "Recv failed: %s",
            data_read == 0 ? "Connection closed" : strerror(data_read));
        free(client_data);
        return -1;
      }
      total_read += data_read;
    }

    LOG(DEBUG, "total read %d", total_read);
    client_data->command_buffer[BUFFSIZE - 1] = '\0';
    print_packet(client_data, 1);

    network_to_host_presentation(client_data); // Also in-place

    // print_packet(client_data, 1);

    if (client_data->command_type == DATA && client_data->command_id == PWD &&
        strlen(client_data->command_buffer) > 0)
    {
      LOG(INFO, "\t%s", client_data->command_buffer);
    }
    else
    {
      LOG(ERROR, "\tError receiving data information");
    }
  }

  if (strcmp(command, "help") == 0)
  {
    const char *helpString = "Welcome to the help page!";
    fprintf(stdout, "%s\n", helpString);
  }

  if (strcmp(command, "exit") == 0)
  {
    LOG(INFO, "Bye!");
    // you will need to send a connection close data
    // reply with 221 for a connection closed by the server
    // ensure the conneciton is closed
    close(connected);
    return 1;
  }

  // clear the screen
  if (strcmp(command, "clear") == 0)
    system("clear");

  free(client_data);
  return (0);
}

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
    }
    command_token = strtok(NULL, delimiter);
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
  fprintf(stdout, "%s", ftp_banner);
  putchar('\n');

  // remove the socket_fd here and move it inside the ftp_execute_command
  // int socket_fd = 0;

  while (1)
  {
    char *command_line;
    char **command_line_args;
    int commands_len = 0;

    // change the state of the function to make sure it indicates when the
    // connection is established and when the connection is not established...

    if (connected)
    {
      fprintf(stdout, ANSI_COLOR_YELLOW "ftp>.< " ANSI_RESET_ALL);
    }
    else
    {
      fprintf(stdout, "ftp>> ");
    }

    fflush(stdout);
    command_line = ftp_getline(SMALL_BUFF, stdin);
    if (!command_line)
    {
      LOG(ERROR, "Parser Failed closing this interactive ftp shell");
      exit(EXIT_FAILURE);
    }
    // fprintf(stdout, ANSI_COLOR_YELLOW "[OUT] %s" ANSI_RESET_ALL,
    // command_line);

    command_line_args = ftp_commands(command_line, FTP_DELIMITERS);
    // checking if the length is correct

    /*for (int i = 0; command_line_args[i] != NULL; i++)*/
    /*  fprintf(stdout, ANSI_COLOR_CYAN "[%s]\n" ANSI_RESET_ALL,*/
    /*          command_line_args[i]);*/

    // testing out first
    if (ftp_execute_command(command_line_args) == 1)
    {
      LOG(INFO, "Cleaning up...");
      free(command_line);
      free(command_line_args);
      exit(EXIT_SUCCESS);
    }

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
