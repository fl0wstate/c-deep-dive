#include "server.h"
/**
 * server - this will create a server app that will be used to make connections
 * to the client
 * @port: pointer to a memory address where the port number is stored
 */
int server(uint16_t PORT)
{
  struct sockaddr_in ipv4_addr;
  struct sockaddr_storage *client_addrs;
  int sockfd = 0, clientfd = 0, bindstatus = 0;
  socklen_t clientaddrlen;

  memset(&ipv4_addr, 0, sizeof(ipv4_addr));
  /* you need to understand how to do it on a remote server */
  ipv4_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ipv4_addr.sin_family = AF_INET;
  ipv4_addr.sin_port = htons(PORT);

  sockfd = socket(ipv4_addr.sin_family, SOCK_STREAM, 0);
  // sockfd = socket(AF_INET, SOCK_STREAM, 0);
  printf("Generated socket_fd = %d\n", sockfd);
  if (sockfd == -1)
    handle_errror(sockfd, "socket");

  /* We need to make the address available each time we kill the server
   * unexpectadly*/
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    handle_errror(sockfd, "setsockopt");

  bindstatus = bind(sockfd, (struct sockaddr *)&ipv4_addr, sizeof(ipv4_addr));
  if (bindstatus == -1)
    handle_errror(bindstatus, "bind");

  listen(sockfd, SERVER_BACKLOG);
  clientaddrlen = sizeof(struct sockaddr_storage);
  clientfd = accept(sockfd, (struct sockaddr *)&client_addrs, &clientaddrlen);
  if (clientfd == -1)
    handle_errror(clientfd, "client");

  fprintf(stdout, "From the server here is the clientfd: %d\n", clientfd);

  /* close the socket */
  /* TODO: Implement poll in this single file */
  return (sockfd);
}

int main(void)
{
  server(4242);
  return (0);
}
