#include <asm-generic/errno-base.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "./includes/epoll_server.h"

int main(void)
{

  int ret, event, acc;
  char buffer[1024];
  struct epoll_event events[10];

  epoll_server();
  int server_sock = tcp_server();
  int epoll_fd = epoll_setup(server_sock);

  memset(&buffer, 0, 1024);
  memset(&events, 0, sizeof(events));

  // Entry Point
  while (1)
  {
    // epoll wait
    ret = epoll_wait(epoll_fd, events, 10, -1);
    if (ret == -1)
    {
      if (ret == EINTR)
        continue;
      LOG(ERROR, "Epoll wait failed");
      exit(EXIT_FAILURE);
    }

    // loop over the epoll structures that you have be returned
    for (event = 0; event < ret; event++)
    {
      // handle each epoll structure event that's fired up
      struct epoll_event e = events[event];
      debug_epoll(&e);
      // handle the server file_descriptor and start accepting connections
      if (e.data.fd == server_sock)
      {
        acc = accept_client(server_sock);
        if (acc == -1)
          continue; // connection refused doesn't mean we close the file
        epoll_add(epoll_fd, server_sock);
        continue;
      }

      // handle the case of file descriptors that are ready for reading.
      if (e.events & EPOLLIN)
        respond(server_sock, buffer);
      // handle any other form of errors that might occur
      if (e.events & (EPOLLIN | EPOLLHUP))
        close(server_sock);
    }
  }
  LOG(INFO, "Exiting server...");
  close(epoll_fd);
  return 0;
}
