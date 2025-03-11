#include "ftp.h"

int send_file(int socket_fd, const char *file_path)
{
  FILE *file;
  int file_length = 0;

  file = fopen(file_path, "r");

  if (!file)
  {
    LOG(ERROR, "Error reading file");
    return -1;
  }

  fseek(file, 0L, SEEK_END);
  file_length = (int)ftell(file);
  fseek(file, 0L, SEEK_SET);

  char filedata[file_length + 1];
  fread(filedata, sizeof(char), file_length, file);
  send(socket_fd, filedata, file_length, 0);

  fclose(file);
  close(socket_fd);
  return (0);
}

void execute_commands(int socket_fd, const char *command)
{
  if (strcmp(command, "RETR") == 0)
  {
    char file_path[100];
    int byte_reads = 0;

    byte_reads = recv(socket_fd, file_path, 100, 0);

    if (byte_reads > 0)
    {
      if (send_file(socket_fd, file_path) == -1)
      {
        LOG(ERROR, "You are cooked...");
        // you are now kill the child process
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      LOG(ERROR, "Error reading data from the connected file descriptor");
      LOG(DEBUG, "You probably want to handle this correctly");
    }
  }
  else
  {
    LOG(DEBUG, "Not yet implemented");
  }
}

int create_a_socket(char *port)
{
  int socket_fd = 0, status = 0;
  //* filter for the type of address info we want IPV4 or IPV6 */
  struct addrinfo hints;
  struct addrinfo *listips, *temp;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;       // type of ipv version
  hints.ai_socktype = SOCK_STREAM; // data stream TCP
  hints.ai_flags = AI_PASSIVE;     // create on from scratch

  status = getaddrinfo(NULL, port, &hints, &listips);
  if (status != 0)
  {
    LOG(ERROR, "Error: %s", gai_strerror(status));
    return -1;
  }

  temp = listips;
  while (temp)
  {
    socket_fd = socket(temp->ai_family, temp->ai_socktype, temp->ai_protocol);
    if (socket_fd == -1)
    {
      LOG(ERROR, "Error while establishing a new socket for connection");
      return -1;
    }
    // everything works out fine we need to bind it
    if (bind(socket_fd, temp->ai_addr, temp->ai_addrlen) == 1)
    {
      LOG(ERROR, "Error binding the address to the socket_fd");
      close(socket_fd);
      continue;
    }
    break;
  }
  free(listips);
  return (socket_fd);
}

// sending files over the instructed connection
// reading the command passed down by the client

int main(int argc, char *argv[])
{
  int server_socket = 0, client_socket = 0, status = 0;
  struct sockaddr_storage client_address;
  socklen_t address_len;

  // make a connection to the suggested port
  server_socket = create_a_socket("20");
  if (server_socket < 0)
  {
    LOG(ERROR, "Error setting up the socket to the specified port");
    exit(EXIT_FAILURE);
  }

  LOG(INFO, "Socket creation succeded...");

  if (listen(server_socket, 10) < 0)
  {
    LOG(ERROR, "Listen error");
    exit(EXIT_FAILURE);
  }

  LOG(INFO, "Socket is listening...");
  while (1)
  {
    // accept the incoming connection
    address_len = sizeof client_address;
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_address, &address_len);

    if (client_socket < 0)
    {
      LOG(ERROR, "Accept error");
      continue;
    }

    // multiprocessing
    if (!fork())
    {
      LOG(DEBUG, "You are in the child process...");
      // you are in the child process
      close(server_socket);

      while (1)
      {
        int byte_reads = 0;
        char command[20] = {0};
        // read the data from that file descriptor
        byte_reads = recv(client_socket, command, 20, 0);
        // basically has to hold the command

        LOG(DEBUG, "Command type: %s", command);

        if (byte_reads > 0)
          execute_commands(client_socket, command);
        // run the command
      }
    }
  }

  close(server_socket);
  printf("Hello World\n");
  return EXIT_SUCCESS;
}
