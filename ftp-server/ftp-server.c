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
  int socketfd;
  struct addrinfo hints, *servinfo, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = AI_PASSIVE;     // use own address

  int status = getaddrinfo(NULL, "20", &hints, &servinfo);

  if (status != 0)
  {
    fprintf(stderr, "server: getaddrinfo: %s\n", gai_strerror(status));
    exit(1);
  }

  // loop through the list of endpoints we recieved from getaddrinfo.
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    // attempt to create the socket - IPv4 and TCP
    socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (socketfd == -1)
    {
      perror("createConnection");
      continue;
    }

    // attempt to bind the socket
    if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(socketfd);
      perror("Failed to bind to specifed address.");
      continue;
    }

    break;
    // if both of those functions complete successfully then
    // we successfully created the connection.
  }

  freeaddrinfo(servinfo);
  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind to port %s\n", port);
    exit(1);
  }
  else
  {
    return socketfd;
  }
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
