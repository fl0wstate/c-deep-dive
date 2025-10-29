#ifndef _SERVER_H_
#define _SERVER_H_

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_BACKLOG 5
#define handle_errror(fd, msg)                                                 \
  do                                                                           \
  {                                                                            \
    fprintf(stderr,                                                            \
            "Error thrown by this %s func with the following code: %s\n",      \
            strerror(fd), msg);                                                \
    close(fd);                                                                 \
  } while (0);

#define on_error(...)                                                          \
  {                                                                            \
    fprintf(stderr,                                                            \
            "Error thrown by this %s func with the following code: %s\n",      \
            __VA_ARGS__);                                                      \
    fflush(stderr);                                                            \
    exit(1);                                                                   \
  }

typedef enum LOG_LEVEL
{
  INFO,
  DEBUG,
  ERROR
} logll;

#define LOG(...)                                                               \
  {                                                                            \
    fprintf(stdout, __VA_ARGS__);                                              \
    fflush(stdout);                                                            \
  }

uint32_t create_server(uint32_t port);
void init_pollfd(struct pollfd *pollfds[], uint32_t *n_pollfds,
                 int32_t server_fd, uint32_t *max_pollfds);
void accept_connections(int server_fd, struct pollfd **pollfds,
                        uint32_t *n_pollfds, uint32_t *max_pollfds);
void read_from_client(struct pollfd **pollfds, uint32_t client_fd,
                      uint32_t server_fd, uint32_t *n_pollfds);
void add_clients_to_poll(struct pollfd *pollfds[], uint32_t new_client_fd,
                         uint32_t *n_pollfds, uint32_t *max_pollfds);
void del_client_from_poll(struct pollfd **pollfds, uint32_t *n_pollfds,
                          uint32_t client_fd);

#endif
