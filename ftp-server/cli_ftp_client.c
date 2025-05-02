#include "ftp.h"
#include <complex.h>
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

    host_to_network_presentation(client_data);

    // print_packet(client_data);

    // sending data
    send_data(connected, client_data);

    // Receive into client_data
    recv_data(connected, client_data);

    // not needed will definetly check it out
    network_to_host_presentation(client_data); // Also in-place

    if (client_data->command_type == DATA && client_data->command_id == PWD &&
        strlen(client_data->command_buffer) > 0)
    {
      LOG(INFO, "[DATA]: %s", client_data->command_buffer);
    }
    else
    {
      LOG(ERROR, "\tError receiving data information");
    }
  }

  if (strcmp(command, "HELP") == 0)
  {
    const char *helpString1 = "Welcome to the help page!";
    fprintf(stdout, "%s\n", helpString1);
    char *helpString;

    helpString =
        "Usage client [Address]."
        "\nCOMMANDS:"
        "\n\t quit \t\t\t- Disconnect from the server and exit the client."
        "\n\t connect (address) \t- Connect to a FTP server specifed at the "
        "given addres. "
        "\n\t pwd \t\t\t- Print the current working directory on the server. "
        "\n\t cd (path) \t\t- Change the directory on the server to the "
        "specified path. "
        "\n\t ls \t\t\t- List the contents of the working directory on the "
        "server. "
        "\n\t retrieve (file) [path]\t- Recieve a file that is hosted on the "
        "server. "
        "\n\t\t\t\t The first argument is file you want to recieve, "
        "\n\t\t\t\t and the second argument is an option to specify "
        "\n\t\t\t\t where the file should be saved to. "
        "\n\t space \t\t\t- Get the disk space available on the server. "
        "\n\t get (file) [path]\t- Get(download) file from the remote server "
        "to your local machine"
        "\n\t put (file) [path]\t- Get(upload) file from the locally "
        "to the remote server"
        "\n\t mkdir \t\t\t- Create a directory on the server."
        "\n\t undo \t\t\t- Undo the last command."
        "\n\t close \t\t\t- to terminate a connection with another computer."
        "\n\t rename (path) (new)\t- Rename the specifed file. "
        "\n\t rm (path) (new)\t- Delete the specifed file. "
        "\n\t help \t\t\t- displays this help message ";

    printf("%s\n", helpString);
  }

  if (strcmp(command, "EXIT") == 0)
  {
    // terminate_connection(client_data,connected);
    LOG(INFO, "Bye!");
    // you will need to send a connection close data
    // reply with 221 for a connection closed by the server
    // ensure the conneciton is closed
    close(connected);
    return 1;
  }

  if (strcmp(command, "CL") == 0)
    system("clear");

  if (strcmp(command, "LS") == 0)
  {
    int status = 0;
    client_data->command_type = REQU;
    client_data->command_id = LS;
    client_data->connection_id = 0;
    client_data->command_len = 0;

    // host_to_network_presentation(client_data);

    // send the data
    if ((status = send(connected, client_data, sizeof(struct network_packet),
                       0)) != sizeof(struct network_packet))
      LOG(ERROR, "Error sending LS network packet");

    while (client_data->command_type != EOT)
    {
      if (client_data->command_type == DATA && client_data->command_id == LS &&
          strlen(client_data->command_buffer))
      {
        LOG(INFO, "%s", client_data->command_buffer);
      }
      // recv
      if ((status = recv(connected, client_data, sizeof(struct network_packet),
                         0)) == 0)
      {
        LOG(ERROR, "Error reading LS network data");
        break;
      }
    }
  }

  if (strcmp(command, "GET") == 0)
  {
    FILE *fp = fopen(command_line_args[1], "wb");
    if (!fp)
      LOG(ERROR, "Error opening file....");

    client_data->command_type = REQU;
    client_data->command_id = GET;
    client_data->command_len = 0;

    strcpy(client_data->command_buffer, command_line_args[1]);
    send_packet(client_data, connected, "GET");
    recv_data(connected, client_data);

    if (client_data->command_type == INFO && client_data->command_id == GET &&
        strlen(client_data->command_buffer))
    {
      LOG(INFO, "%s", client_data->command_buffer);

      // more than one recv means you are sending data back in chuncks
      recv_data(connected, client_data);
      while (client_data->command_type == DATA)
      {
        if (fwrite(client_data->command_buffer, 1, client_data->command_len,
                   fp) != client_data->command_len)
        {
          LOG(ERROR, "Error writing to file...");
          break;
        }
        recv_data(connected, client_data);
      }
      fclose(fp);
    }
  }

  if (strcmp(command, "PUT") == 0)
  {
    size_t x = 0;
    FILE *fp = fopen(command_line_args[1], "rb");

    if (!fp)
      LOG(ERROR, "Error opening local file...");

    client_data->command_type = REQU;
    client_data->command_id = PUT;
    client_data->command_len = 0;

    strcpy(client_data->command_buffer, command_line_args[1]);
    send_packet(client_data, connected, "PUT");

    recv_data(connected, client_data);
    if (client_data->command_type == INFO && client_data->command_id == PUT &&
        strlen(client_data->command_buffer))
    {
      LOG(INFO, "%s", client_data->command_buffer);

      // the read logic
      size_t total_read = 0;
      while ((x = fread(client_data->command_buffer, 1, BUFFSIZE, fp)))
      {
        client_data->command_len = x;
        client_data->command_type = DATA;
        // error handle the send_packet command
        send_packet(client_data, connected, "PUT");
        total_read += x;
      }
      LOG(DEBUG, "Size of the file: %d", get_file_size(fp));
      LOG(DEBUG, "Size of data transfered: %zu", total_read);
      fclose(fp);
      end_of_transfer(client_data, connected);
    }
  }

  if (strcmp(command, "CLOSE") == 0)
  {
  }

  if (strcmp(command, "CD") == 0)
  {
    client_data->command_type = REQU;
    client_data->command_id = CD;
    client_data->connection_id = 0;
    client_data->command_len = 0; // No data in command_buffer for PWD

    host_to_network_presentation(client_data);

    strcpy(client_data->command_buffer, command_line_args[1]);
    send_packet(client_data, connected, "CD");

    recv_data(connected, client_data);

    if (client_data->command_type == INFO && client_data->command_id == CD &&
        !strcmp(client_data->command_buffer, "command success"))
      ;
    else
      LOG(ERROR, "Error recieving data from the server..");
  }

  if (strcmp(command, "MKDIR") == 0)
  {
  }

  if (strcmp(command, "RM") == 0)
  {
    client_data->command_type = REQU;
    client_data->command_id = RM;
    client_data->connection_id = 0;
    client_data->command_len = 0; // No data in command_buffer for PWD

    if (!command_line_args[1])
    {
      LOG(ERROR, "filename missing: RM <filename>");
      return -1;
    }

    strcpy(client_data->command_buffer, command_line_args[1]);
    // host_to_network_presentation(client_data);
    // fix ^^^

    send_packet(client_data, connected, "RM");

    recv_data(connected, client_data);

    if (client_data->command_type == INFO && client_data->command_id == RM &&
        !strcmp(client_data->command_buffer, "command success"))
      ;
    else
      LOG(ERROR, "Error recieving data from the server..");
  }

  if (strcmp(command, "") == 0)
  {
  }
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
    command_line_args = ftp_commands(command_line, FTP_DELIMITERS);

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
