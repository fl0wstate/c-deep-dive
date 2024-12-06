#ifndef _SERVER_H_
#define _SERVER_H_

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
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
#define LOG(...)                                                               \
  {                                                                            \
    fprintf(stdout, __VA_ARGS__);                                              \
    fflush(stdout);                                                            \
  }
#endif
