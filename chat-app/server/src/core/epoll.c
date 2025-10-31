#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../includes/log.h"

int epoll_setup(int socket_fd)
{
  int epfd = epoll_create1(0);

  if (epfd == -1)
  {
    LOG(ERROR, "Epoll create1 failed");
    exit(EXIT_FAILURE);
  }
  return (epfd);
}

void epoll_add(int epoll_fd, int socket_fd)
{
  struct epoll_event event;
  memset(&event, 0, sizeof(event));

  event.events = EPOLLIN;
  event.data.fd = socket_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1)
  {
    LOG(ERROR, "Failed to run epoll add on the server socket");
    exit(EXIT_FAILURE);
  }
}

void epoll_del(int epoll_fd, int socket_fd)
{
  struct epoll_event event;
  memset(&event, 0, sizeof(event));

  event.events = EPOLLIN;
  event.data.fd = socket_fd;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, &event) == -1)
  {
    LOG(ERROR, "Failed to delete an epoll event");
    exit(EXIT_FAILURE);
  }
}

void debug_epoll(struct epoll_event *e)
{
  LOG(DEBUG, "fd=%d events: %s%s%s%s\n", (*e).data.fd,
      ((*e).events & EPOLLIN) ? "EPOLLIN " : "",
      ((*e).events & EPOLLHUP) ? "EPOLLHUP " : "",
      ((*e).events & EPOLLERR) ? "EPOLLERR " : "",
      ((*e).events & EPOLLRDHUP) ? "EPOLLRDHUP " : "");
}

int accept_client(int socket_fd)
{
  int accept_sock = accept4(socket_fd, NULL, 0, SOCK_CLOEXEC | SOCK_NONBLOCK);
  if (accept_sock == -1)
  {
    LOG(ERROR, "Accept connection failed to the current fd %d", socket_fd);
  }

  return accept_sock;
}

void respond(int socket_fd, char *buffer)
{
  while (1)
  {
    size_t reads = read(socket_fd, buffer, 100);
    if (reads == -1)
    {
      if (errno == EINTR)
        continue;
    }

    if (reads == 0)
    {
      LOG(INFO, "Client closed the connection");
      close(socket_fd);
      break;
    }

    while (1)
    {
      if (write(1, "Hello World C\n", 13) == -1)
      {
        if (errno == EINTR)
          continue;
        LOG(ERROR, "Writing command failed");
        close(socket_fd);
        break;
      }
      break;
    }
    break;
  }
}
