#include "../includes/server.h"

void LOG_LEVEL(logll level, const char *msg)
{
  switch (level)
  {
  case INFO:
    fprintf(stdout, "[INFO] %s\n", msg);
    break;
  case DEBUG:
    fprintf(stdout, "[DEBUGGING] %s\n", msg);
  case ERROR:
    fprintf(stderr, "[ERROR] %s\n", msg);
  }
}

/**
 * poll_init - This function will be used to initalize the pollfd array
 * @pollfds: pointer to the array of file descriptor
 * @n_pollfds: pointer to the number poll file descriptor set
 * @server_fd: server file descriptor which will be the first one to be set in
 * the array
 * @max_pollfds: pointer to the maximum number of file descriptor to be
 * initialized with Return: void
 */
void init_pollfd(struct pollfd *pollfds[], uint32_t *n_pollfds,
                 int32_t server_fd, uint32_t *max_pollfds)
{
  (*pollfds) = calloc((*max_pollfds) + 1, sizeof((**pollfds)));
  if (!pollfds)
  {

    LOG_LEVEL(ERROR, "Memory allocation failed");
    exit(EXIT_FAILURE);
  }
  (*pollfds)[0].fd = server_fd;
  (*pollfds)[0].events = POLLIN;
  (*n_pollfds) = 1;
  LOG_LEVEL(INFO, "pollfds array is successfully");
}
/**
 * create_server - This function creates a server that listens on the given
 * port.
 * @port: The port to listen on.
 * Return: positive integer(new sockect file descriptor) on success, -1 on
 * failure.
 */
uint32_t create_server(uint32_t port)
{
  int server_fd;
  struct sockaddr_in server_addr = {0};

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
    handle_errror(server_fd, "socket");

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  // chaning the port from little edian to big edain which is the prefered mode
  // of communication.
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    handle_errror(server_fd, "bind");

  if (listen(server_fd, SERVER_BACKLOG) < 0)
    handle_errror(server_fd, "listen");

  return (server_fd);
}

/**
 * accept_connections - This function accepts incoming connections on the given
 * server file descriptor.
 * @server_fd: The server file descriptor.
 * @pollfds: The pollfds array.
 * @n_pollfds: The number of pollfds.
 * @max_pollfds: The max number of pollfds.
 */
void accept_connections(int server_fd, struct pollfd **pollfds,
                        uint32_t *n_pollfds, uint32_t *max_pollfds)
{
  uint32_t client_fd;
  int status;
  char msg_buffer[BUFSIZ];

  /* accept connections */
  if ((client_fd = accept(server_fd, NULL, NULL)) < 0)
    handle_errror(server_fd, "accept");

  /* collect the address comming in from the clients */
  add_clients_to_poll(pollfds, client_fd, n_pollfds, max_pollfds);

  LOG("[INFO] Accepted new connection from: [%d]\n", client_fd);
  /* set the buffer for writting the messages */
  memset(&msg_buffer, 0, BUFSIZ);
  sprintf(
      msg_buffer,
      "[INFO] Client connections established by this file descriptor: [%d]\n",
      client_fd);

  /* send the message */
  status = send(client_fd, (void *)&msg_buffer, strlen(msg_buffer), 0);
  if (status < 0)
    handle_errror(client_fd, "send");
}

/**
 * read_from_client - This function reads data from a client.
 * @client_fd: The client file descriptor.
 * @pollfds: The pollfds array.
 * @n_pollfds: The number of pollfds.
 * Return: void;
 */
void read_from_client(struct pollfd **pollfds, uint32_t client_fd,
                      uint32_t server_fd, uint32_t *n_pollfds)
{
  char buffer[BUFSIZ];
  char msg_buffer[BUFSIZ];
  int bytes_read;
  int32_t status;
  uint32_t broadcast_fd;

  memset(&buffer, 0, BUFSIZ);
  bytes_read = recv(client_fd, (void *)&buffer, BUFSIZ, 0);
  if (bytes_read <= 0)
  {
    LOG("[DEBUGGING] bytes read: %d\n", bytes_read)
    if (bytes_read == 0)
    {
      LOG("[INFO] Client disconnected\n");
    }
    else
    {
      handle_errror(client_fd, "recv");
    }

    close(client_fd);
    del_client_from_poll(pollfds, n_pollfds, client_fd);
  }
  else
  {
    /* Broadcast everything to all the other fds except the client who sent the
     * message and the server socket
     */
    LOG("[INFO] From: [%d]\n[MESSAGE]: %s\n", client_fd, buffer);
    memset(&msg_buffer, 0, sizeof(msg_buffer));
    sprintf(msg_buffer, "%s^", buffer);
    for (int j = 0; j < (*n_pollfds); j++)
    {
      if ((*pollfds)[j].fd != client_fd && (*pollfds)[j].fd != server_fd)
      {
        LOG_LEVEL(INFO, "Broadcasting message to all connected clients");
        status =
            send((*pollfds)[j].fd, (void *)msg_buffer, sizeof msg_buffer, 0);
        if (status == -1)
          handle_errror(status, "send");
      }
    }
  }
}

/**
 * add_clients_to_poll - This function will add all the new incomming clients
 * connections to the array of pollfds, this will help to keep track of the
 * number of connections made
 * @pollfds: pointer to an array of client_fd that are currently connected to
 * the server
 * @n_pollfds: pointer to the number of connected client_fd in the server
 * @max_pollfds: pointer to a memory address holding the max number of client_fd
 * that can connect
 */
void add_clients_to_poll(struct pollfd *pollfds[], uint32_t new_client_fd,
                         uint32_t *n_pollfds, uint32_t *max_pollfds)
{
  if (*n_pollfds == *max_pollfds)
  {
    *max_pollfds *= 2; /* Double the size of the max number of connection to be
                          made to the server*/
    *pollfds = realloc((void *)*pollfds, (sizeof(**pollfds) * (*max_pollfds)));
  }
  (*pollfds)[*n_pollfds].fd = new_client_fd;
  (*pollfds)[*n_pollfds].events = POLLIN;
  (*n_pollfds)++;
}

/**
 * del_client_from_poll - This function behaves like it's deleting a file
 * descriptor from pollfds, but instead fills up the space with the last
 * connected clients fd
 * @pollfds: pointer to an array of client_fd that are currently connected to
 * @n_pollfds: pointer to the number of connected client_fd in the server
 * @client_fd: the client file descriptor to be deleted from the pollfds array
 * Return: void
 */
void del_client_from_poll(struct pollfd **pollfds, uint32_t *n_pollfds,
                          uint32_t client_fd)
{
  (*pollfds)[client_fd] = (*pollfds)[*n_pollfds - 1];
  (*n_pollfds)--;
}
