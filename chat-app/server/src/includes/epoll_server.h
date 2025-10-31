#ifndef EPOLL_H
#define EPOLL_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../includes/log.h"
#include <sys/epoll.h>

int tcp_server();
void epoll_server(void);
int epoll_setup(int server_sock);
void epoll_add(int epoll_fd, int server_sook);
void epoll_del(int epoll_fd, int server_sook);
void debug_epoll(struct epoll_event *);
int accept_client(int server_sock);
void respond(int server_sock, char *);

#endif
