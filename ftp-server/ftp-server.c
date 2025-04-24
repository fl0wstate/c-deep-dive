#include "ftp.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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

void execute_commands(u_int8_t socket_fd, struct network_packet *client_data)
{
  char listpwd[BUFFSIZE];
  int x = 0;
  switch (client_data->command_id)
  {
  case PWD:
    if (!getcwd(listpwd, BUFFSIZE))
    {
      LOG(ERROR, "PWD not working well...");
    }
    client_data->command_type = DATA;
    client_data->command_len = (u_int8_t)BUFFSIZE;
    strcpy(client_data->command_buffer, listpwd);

    print_packet(client_data);

    // host_to_network_presentation(client_data);
    if ((x = send(socket_fd, client_data, sizeof(struct network_packet), 0)) !=
        sizeof(struct network_packet))
      LOG(ERROR, "Sending Packets");

    LOG(DEBUG, "You are supposed to print the current working directory");
    break;
  default:
    LOG(ERROR, "No Such Command Implemented");
    break;
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

  // testing buffer to see the actual ip addresses
  char buf[INET6_ADDRSTRLEN];
  int attempts = 0;

  int status = getaddrinfo(NULL, port, &hints, &servinfo);

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
      attempts++;
    }

    break;
    // if both of those functions complete successfully then
    // we successfully created the connection.
  }
  // debbugging
  struct sockaddr_in *IPv4 = (struct sockaddr_in *)p->ai_addr;
  inet_ntop(p->ai_family, &(IPv4->sin_addr), buf, sizeof(buf));
  LOG(INFO, "IPV4: %s:%s", buf, port);

  /*you will need to display the ip address*/
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

int main(int argc, char *argv[])
{
  int server_socket = 0, client_socket = 0, status = 0;
  int client_connection_id = 0;
  struct sockaddr_storage client_address;
  struct client_info *ci;
  socklen_t address_len;

  server_socket = create_a_socket("1990");
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
    address_len = sizeof client_address;
    client_socket =
        accept(server_socket, (struct sockaddr *)&client_address, &address_len);

    if (client_socket < 0)
    {
      LOG(ERROR, "Accept error");
      break;
    }

    if (client_socket)
    {
      client_connection_id++;
      // client send back the client a confirmation message 220 : Connection
      // established
      LOG(INFO, "Client connection has been established");
      ci = client_info_storage(client_socket, client_connection_id);
    }

    // multiprocessing
    if (!fork())
    {
      LOG(DEBUG, "You are in the child process...");

      close(server_socket);

      while (1)
      {
        /*********************************************/
        // create space for the network packet
        int byte_reads = 0;
        struct network_packet *data =
            (struct network_packet *)malloc(sizeof(struct network_packet));

        if (!data)
          LOG(ERROR, "Malloc failed");

        byte_reads =
            recv(client_socket, data, sizeof(struct network_packet), 0);

        if (byte_reads <= 0)
        {
          LOG(ERROR, "Read error");
          break;
        }

        network_to_host_presentation(data);

        print_packet(data);

        // command type being sent is a Termination signal
        if (data->command_type == TERM)
          break;

        // attach a unique connection id to the current connected client
        if (data->connection_id == 0)
          data->connection_id = ci->client_connection_id;

        if (data->command_type == REQU)
        {
          LOG(INFO, "You sent a REQU command");
          execute_commands(ci->client_socket_id, data);
        }
        else
        {
          // something happened that the server can't figure out yet sendinga
          // termination signal
          LOG(ERROR, "Error handling the packet...closing the connection");
          terminate_connection(data, ci->client_socket_id);
        }

        free(data);
        /*********************************************/
      }
    }
    free(ci);
    close(client_socket);
    // handle signal termination
  }
  close(server_socket);
  printf("Hello World\n");
  return EXIT_SUCCESS;
}
