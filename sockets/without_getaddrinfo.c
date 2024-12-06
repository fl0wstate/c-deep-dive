#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
/**
 * simple_server - this will be a simple server without
 * using the getaddr method(basically DNS)
 * @port: port number where the communication will be established to
 * Return: 0 for success, < 0 fails
 */

int simple_server(int port)
{
  struct sockaddr_in ipv4;
  struct socketaddr_storage *client_sockets;
  int socketfd = 0, clientfd = 0, bindstatus = 0;
  socklen_t clientaddrlen = 0;

  /* port and the address family to be used durind socket creation */
  memset(&ipv4, 0, sizeof(ipv4));
  ipv4.sin_family = AF_INET;
  ipv4.sin_port = htons(port);
  ipv4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  /* create a socketfd  */
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  /* bind the socketfd */
  bindstatus = bind(socketfd, (struct sockaddr *)&ipv4, sizeof(ipv4));
  if (bindstatus != 0)
  {
    fprintf(stderr, "bind error: %s\n", strerror(bindstatus));
    return (-2);
  }
  /* listen to incomming connections */
  listen(socketfd, 5);
  /* accpet incomming connections */

  /* we need to get the length of the client_sockets(which is an array of
   * addresses either ipv4 or ipv6) */
  clientaddrlen = sizeof(struct sockaddr_storage);
  clientfd =
      accept(socketfd, (struct sockaddr *)&client_sockets, &clientaddrlen);

  if (clientfd == -1)
  {
    fprintf(stderr, "client error: %s\n", strerror(bindstatus));
    return (-3);
  }
  printf("socketfd: %d\n", socketfd);
  printf("clientfd: %d\n", clientfd);
  return (0);
}

int main(void)
{
  simple_server(6006);
  return (0);
}
