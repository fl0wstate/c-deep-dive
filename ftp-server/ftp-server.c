#include "ftp.h"
#include <netdb.h>
#include <sys/socket.h>

int connect_to_server(char *port)
{
  int socket_fd = 0, status = 0;
  //* filter for the type of address info we want IPV4 or IPV6 */
  struct addrinfo hints;
  struct addrinfo *listips, *temp;

  memset(&hints, NULL, sizeof hints);
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
      // bind
    }
  }

  return (socket_fd);
}

int main(int argc, char *argv[])
{
  printf("Hello World\n");
  return EXIT_SUCCESS;
}
