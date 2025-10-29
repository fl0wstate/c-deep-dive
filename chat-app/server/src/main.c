#include "./includes/server.h"

#define PORT 7000
#define BACKLOG 10
/**
 * main - Entry point
 * Return: 0 on success
 */
int main(void)
{
  struct pollfd *pollfds;
  uint32_t n_pollfds = 0;
  uint32_t max_pollfds = 12;
  int32_t status = 0;

  int server_fd = create_server(PORT);
  if (server_fd < 0)
  {
    handle_errror(server_fd, "create_server");
    return (-1);
  }

  // TODO: Why are you listening to the server twice
  status = listen(server_fd, BACKLOG);
  if (status < 0)
  {
    handle_errror(server_fd, "listen");
    return (-1);
  }

  LOG("[INFO] Server listening on port %d\n", PORT);
  init_pollfd(&pollfds, &n_pollfds, server_fd, &max_pollfds);

  while (1)
  {
    status = poll(pollfds, n_pollfds, 2000);
    if (status < 0)
    {
      handle_errror(status, "poll");
      return (-1);
    }
    else if (status == 0)
    {
      LOG("[INFO] Waiting for connections...\n");
      continue;
    }

    for (uint32_t i = 0; i < n_pollfds; i++)
    {
      // TODO: Learn how to skip or remove all non-listening  poll-file
      // descriptors
      if ((pollfds[i].revents & POLLIN) != 1)
      {
        LOG("Poll fd is not ready for reading moving on to the next one...\n");
        continue;
      }
      LOG("[INFO] New data available on fd %d\n", pollfds[i].fd);

      if (pollfds[i].fd == server_fd)
        accept_connections(server_fd, &pollfds, &n_pollfds, &max_pollfds);
      else
      {
        read_from_client(&pollfds, pollfds[i].fd, server_fd, &n_pollfds);
      }
    }
  }
  return (0);
}
