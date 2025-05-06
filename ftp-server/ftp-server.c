#include "ftp.h"
#include <pthread.h>

static volatile u_int8_t keep_running = 1;

void signal_handler(int sig)
{
  (void)sig;
  keep_running = 0;
}

void execute_commands(u_int8_t socket_fd, struct network_packet *client_data)
{
  char temp_buffer[BUFFSIZE];
  int x = 0;
  switch (client_data->command_id)
  {
  case PWD:
  {
    if (!getcwd(temp_buffer, BUFFSIZE))
    {
      LOG(ERROR, "PWD not working well...");
    }
    client_data->command_type = DATA;
    client_data->command_len = (u_int8_t)BUFFSIZE;
    strcpy(client_data->command_buffer, temp_buffer);

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

    if (!getcwd(temp_buffer, BUFFSIZE))
      LOG(ERROR, "Error reading the current working directory");

    /* fill the network_packet with relevant data */
    client_data->command_type = DATA;
    client_data->command_len = (u_int8_t)BUFFSIZE;

    if (!(pdir = opendir(temp_buffer)))
      LOG(ERROR, "Unable to open the current working directory");

    while ((directory_info = readdir(pdir)) != NULL)
    {
      sprintf(client_data->command_buffer, "%s\t%s",
              directory_info->d_type == 4   ? "DIR:"
              : directory_info->d_type == 8 ? "FILE:"
                                            : "UNDEF",
              directory_info->d_name);

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

  case MKDIR:
  {
    struct stat st = {0};
    DIR *pdir = opendir(client_data->command_buffer);
    LOG(DEBUG, "%s", client_data->command_buffer);

    if (stat(client_data->command_buffer, &st) == -1)
    {
      /* mkdir(client_data->command_buffer, S_IRWXU | S_IRWXG | S_IROTH |
       * S_IXOTH); */
      mkdir(client_data->command_buffer, 0777);
      strcpy(temp_buffer, "success");
    }
    else if (stat(client_data->command_buffer, &st) == 0)
    {
      strcpy(client_data->command_buffer, "Directory already exists");
      closedir(pdir);
    }
    else
    {
      LOG(ERROR, "Could not create directory, invalid path!");
      strcpy(temp_buffer, "fail");
    }

    client_data->command_type = INF;

    strcpy(client_data->command_buffer, temp_buffer);

    send_packet(client_data, socket_fd, "MKDIR");

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
    struct stat st = {0};
    client_data->command_type = INF;
    client_data->command_id = RM;

    if (stat(client_data->command_buffer, &st) == 0)
    {
      remove(client_data->command_buffer);
      sprintf(client_data->command_buffer, "command success");
    }
    else
    {
      LOG(ERROR, "Failed to remove %s, file does not exist",
          client_data->command_buffer);
      sprintf(client_data->command_buffer, "failed");
    }
    send_packet(client_data, socket_fd, "RM");
    break;
  }

  default:
    LOG(ERROR, "No Such Command Implemented");
    break;
  }
}

/* Re-emplementing it all with multithreading...*/
void *client_thread(void *args)
{
  struct client_info *ci = (struct client_info *)args;
  struct network_packet *data = malloc(sizeof(struct network_packet));

  if (!data)
  {
    LOG(ERROR, "FATAL!  Malloc failed");
    return NULL;
  }

  data->command_id = CONNECT;
  data->command_type = INFO;
  sprintf(data->command_buffer, "220 Welcome to flowftp!...");

  send_packet(data, ci->client_socket_id, "INITIAL");

  while (keep_running)
  {
    if (recv_data(ci->client_socket_id, data) < 0)
    {
      terminate_connection(data, ci->client_socket_id);
      break;
    }

    if (data->command_type == TERM)
    {
      terminate_connection(data, ci->client_socket_id);
      free(data);
      break;
    }

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
      LOG(ERROR, "Error handling the packet...closing the connection");
      terminate_connection(data, ci->client_socket_id);
    }
  }

  return NULL;
}
/* end */

int create_a_socket(char *port)
{
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  int socketfd;
  /* char buf[INET6_ADDRSTRLEN]; */
  int attempts = 0, status = 0;

  /**
   * thinking in the context of what actually makes up an socket:
   *  - transport protocol (TPC, UDP)
   *  - Internet protocal (IPV4, IPV6)
   *  - Port Number -> uniquely tells the computer which port to use during
   communication
   *
   * with this hints provided we only need this type of socket that have the
   same setup
   * passing a NULL value of the node so that we can get a suitable socket for
   binging()
   */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

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
      LOG(ERROR, "Error creating a server socket: %s", strerror(socketfd));
      continue;
    }

    /* attempt to bind the socket */
    if (bind(socketfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(socketfd);
      LOG(ERROR, "Failed to bind the specified address: %s", p->ai_addr);
      continue;
      attempts++;
    }
    break;
    /* finally found a better socket for the server to listen and accept for the
     * incomming request*/
  }

  /* debbugging */
  /* struct sockaddr_in *IPv4 = (struct sockaddr_in *)p->ai_addr; */
  /* inet_ntop(p->ai_family, &(IPv4->sin_addr), buf, sizeof(buf)); */
  /* LOG(INFO, "IPV4: %s:%s", buf, port); */

  freeaddrinfo(servinfo);

  if (p == NULL)
  {
    LOG(ERROR, "Failed to create an Internet address to use...");
    socketfd = -1;
    return socketfd;
  }

  return socketfd;
}

int main(int argc, char *argv[])
{
  int server_socket = 0, client_socket = 0;
  int client_connection_id = 0;
  struct sockaddr_storage client_address;
  struct client_info *ci;
  socklen_t address_len;
  pthread_t thread;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

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

  while (keep_running)
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
      LOG(INFO, "Client connection has been established");
      ci = client_info_storage(client_socket, client_connection_id);
    }

    if (pthread_create(&thread, NULL, client_thread, (void *)ci) != 0)
    {
      LOG(ERROR, "Failed to create thread for client %d: %s",
          client_connection_id, strerror(errno));
      close(client_socket);
      free(ci);
      continue;
    }

    pthread_detach(thread);
  }

  close(client_socket);
  free(ci);
  close(server_socket);
  printf("I hope i served you well...World\n");
  return EXIT_SUCCESS;
}
