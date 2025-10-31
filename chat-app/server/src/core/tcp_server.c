#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "../includes/epoll_server.h"
#include "../includes/log.h"

// Testing the codebase layout
void epoll_server(void) { LOG(INFO, "hello world"); }

int tcp_server()
{
  int protocol_opt = 0, socket_fd = 0;
  struct sockaddr_in server_address = {0};

  socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  {
    LOG(ERROR, "Failed to spawn a socket: %s");
    exit(EXIT_FAILURE);
  }

  protocol_opt = 1;
  if (setsockopt(socket_fd, SO_REUSEPORT, protocol_opt, &protocol_opt,
                 sizeof(protocol_opt)))
  {
    LOG(ERROR, "Failed to set socket protocol");
    exit(EXIT_FAILURE);
  }

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_LOOPBACK;
  server_address.sin_port = htons(7000);

  if (bind(socket_fd, (struct sockaddr *)&server_address,
           sizeof(server_address)) < 0)
  {
    LOG(ERROR, "Bind error");
    exit(EXIT_FAILURE);
  }

  if (listen(socket_fd, 100) < 0)
  {
    LOG(ERROR, "Listen error");
    exit(EXIT_FAILURE);
  }

  return (socket_fd);
}
