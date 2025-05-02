#include "ftp.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void execute_commands(u_int8_t socket_fd, struct network_packet *client_data)
{
  char listpwd[BUFFSIZE];
  int x = 0;
  switch (client_data->command_id)
  {
  case PWD:
  {
    if (!getcwd(listpwd, BUFFSIZE))
    {
      LOG(ERROR, "PWD not working well...");
    }
    client_data->command_type = DATA;
    client_data->command_len = (u_int8_t)BUFFSIZE;
    strcpy(client_data->command_buffer, listpwd);

    if ((x = send(socket_fd, client_data, sizeof(struct network_packet), 0)) !=
        sizeof(struct network_packet))
      LOG(ERROR, "Sending Packets");

    LOG(DEBUG, "You are supposed to print the current working directory");
    break;
  }

  case LS:
  {
    struct dirent *directory_info;
    DIR *pdir;

    if (!getcwd(listpwd, BUFFSIZE))
      LOG(ERROR, "Error reading the current working directory");

    /* fill the network_packet with relevant data */
    client_data->command_type = DATA;
    client_data->command_len = (u_int8_t)BUFFSIZE;

    if (!(pdir = opendir(listpwd)))
      LOG(ERROR, "Unable to open the current working directory");

    while ((directory_info = readdir(pdir)) != NULL)
    {
      sprintf(client_data->command_buffer, "%s\t%s",
              directory_info->d_type == 4   ? "DIR:"
              : directory_info->d_type == 8 ? "FILE:"
                                            : "UNDEF",
              directory_info->d_name);

      network_to_host_presentation(client_data);

      if ((x = send(socket_fd, client_data, sizeof(struct network_packet),
                    0)) != sizeof(struct network_packet))
        LOG(ERROR, "Sending LS Packets");
    }
    end_of_transfer(client_data, socket_fd);
    break;
  }

  case GET:
  {
    FILE *fpr = fopen(client_data->command_buffer, "rb");
    client_data->command_type = INF;
    client_data->command_id = GET;

    sprintf(client_data->command_buffer,
            fpr ? "File found, ready for processing..."
                : "Error opening the file...");
    send_packet(client_data, socket_fd, "GET");

    /* LOG(DEBUG, "Is the file ready %s", fp ? "TRUE" : "FALSE"); */

    if (fpr)
    {
      off_t fln = get_file_size(fpr);
      if (fln == -1)
      {
        LOG(ERROR, "Error reading file");
        client_data->command_type = INF;
        sprintf(client_data->command_buffer, "Error reading file data");
        send_packet(client_data, socket_fd, "GET");
        fclose(fpr);
        break;
      }

      client_data->command_type = DATA;
      x = 0;
      while ((x = fread(client_data->command_buffer, 1, BUFFSIZE, fpr)))
      {
        client_data->command_len = x;
        /* error handle the send_packet command */
        send_packet(client_data, socket_fd, "GET");
        LOG(DEBUG, "this bytes were sent over the network... %d", x);
      }
    }
    fclose(fpr);
    end_of_transfer(client_data, socket_fd);
    break;
  }

  case PUT:
  {
    FILE *fp = fopen(client_data->command_buffer, "wb");

    client_data->command_type = INF;
    client_data->command_id = PUT;

    sprintf(client_data->command_buffer,
            fp ? "File created successfully, ready to recieve data..."
               : "Error creating the file...");
    send_packet(client_data, socket_fd, "PUT");

    /* receiving data to store to file from the client side... */
    recv_data(socket_fd, client_data);
    while (client_data->command_type == DATA)
    {
      if ((fwrite(client_data->command_buffer, 1, client_data->command_len,
                  fp) != client_data->command_len))
      {
        LOG(ERROR, "Error writing to file...");
        break;
      }
      recv_data(socket_fd, client_data);
    }
    fclose(fp);
    break;
  }

  case CD:
  {
    client_data->command_type = INF;
    client_data->command_id = CD;

    if (strcmp(client_data->command_buffer, "~") == 0)
    {
      char *home = getenv("HOME");
      strcpy(client_data->command_buffer, home);
    }

    if (strcmp(client_data->command_buffer, "-") == 0)
    {
      char *home = getenv("OLDPWD");
      strcpy(client_data->command_buffer, home);
    }

    if ((x = chdir(client_data->command_buffer)) == -1)
    {
      LOG(ERROR, "Incorrect path provided...");
      break;
    }
    sprintf(client_data->command_buffer, "command success");
    send_packet(client_data, socket_fd, "CD");
    break;
  }

  case RM:
  {
    client_data->command_type = INF;
    client_data->command_id = RM;

    if (remove(client_data->command_buffer) != 0)
    {
      LOG(ERROR, "Failed to remove the file...(should not happen)");
      break;
    }
    sprintf(client_data->command_buffer, "command success");
    send_packet(client_data, socket_fd, "RM");
    break;
  }

  default:
    LOG(ERROR, "No Such Command Implemented");
    break;
  }
}

int create_a_socket(char *port)
{
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  int socketfd;
  /* char buf[INET6_ADDRSTRLEN]; */
  int attempts = 0, status = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  /* testing buffer to see the actual ip addresses */

  status = getaddrinfo(NULL, port, &hints, &servinfo);

  if (status != 0)
  {
    fprintf(stderr, "server: getaddrinfo: %s\n", strerror(status));
    exit(1);
  }

  /* loop through the list of endpoints we recieved from getaddrinfo. */
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    /* attempt to create the socket - IPv4 and TCP */
    socketfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (socketfd == -1)
    {
      perror("createConnection");
      continue;
    }

    /* attempt to bind the socket */
    if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(socketfd);
      perror("Failed to bind to specifed address.");
      continue;
      attempts++;
    }

    break;
    /* if both of those functions complete successfully then */
    /* we successfully created the connection. */
  }
  /* debbugging */
  /* struct sockaddr_in *IPv4 = (struct sockaddr_in *)p->ai_addr; */
  /* inet_ntop(p->ai_family, &(IPv4->sin_addr), buf, sizeof(buf)); */
  /* LOG(INFO, "IPV4: %s:%s", buf, port); */

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
  int server_socket = 0, client_socket = 0;
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
      /* client send back the client a confirmation message 220 : Connection */
      /* established */
      LOG(INFO, "Client connection has been established");
      ci = client_info_storage(client_socket, client_connection_id);
    }

    /* multiprocessing */
    if (!fork())
    {
      LOG(DEBUG, "You are in the child process...");

      close(server_socket);

      while (1)
      {
        /*********************************************/
        /* create space for the network packet */
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

        /* print_packet(data); */

        /* command type being sent is a Termination signal */
        if (data->command_type == TERM)
          break;

        /* attach a unique connection id to the current connected client */
        if (data->connection_id == 0)
          data->connection_id = ci->client_connection_id;

        if (data->command_type == REQU)
        {
          LOG(INFO, "You sent a REQU command");
          execute_commands(ci->client_socket_id, data);
        }
        else
        {
          /* something happened that the server can't figure out yet sendinga */
          /* termination signal */
          LOG(ERROR, "Error handling the packet...closing the connection");
          terminate_connection(data, ci->client_socket_id);
        }

        free(data);
        /*********************************************/
      }
    }
    free(ci);
    close(client_socket);
    /* handle signal termination */
  }
  close(server_socket);
  printf("Hello World\n");
  return EXIT_SUCCESS;
}
