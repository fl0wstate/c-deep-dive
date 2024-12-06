#include "server.h"
#include <fcntl.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/**
 * server_scoket - create a server socket
 * @port: port number
 * Return: socket file descriptor
 */
int socket_server(char *port)
{
  struct addrinfo hints, *res;
  struct sockaddr_storage *client_sock;
  uint32_t server_fd = 0, client_fd = 0;
  socklen_t addr_size = 0;
  uint8_t status = 0, bind_status = 0;

  memset(&hints, 0, sizeof(hints));
  // this will provides a best example of using a socket to act as a server
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // getaddrinfo() returns a list of address structures
  status = getaddrinfo(NULL, port, &hints, &res);
  if (status != 0)
    on_error("getaddrinfo", gai_strerror(status));

  // socket() creates an endpoint for communication and returns a file
  // descriptor
  server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  printf("server_fd: %d\n", server_fd);
  if (server_fd == -1)
    on_error("socket", strerror(server_fd));

  // bind the provided node and services to the current socket
  bind_status = bind(server_fd, res->ai_addr, res->ai_addrlen);
  if (bind_status != 0)
    on_error("bind", strerror(bind_status));

  listen(server_fd, 2);
  addr_size = sizeof(struct sockaddr_storage);
  client_fd = accept(server_fd, (struct sockaddr *)&client_sock, &addr_size);

  if (client_fd == -1)
    on_error("client", strerror(client_fd));

  fprintf(stderr, "New connection established, :%d, %d\n", server_fd,
          client_fd);

  freeaddrinfo(res);
  return server_fd;
}

int main(void)
{
  int status = 0;
  status = socket_server("8080");
  if (status < 0)
  {
    fprintf(stderr, "Your socket_server error: %d\n", status);
    return (-1);
  }
  return 0;
}
