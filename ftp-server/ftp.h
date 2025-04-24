#ifndef __FTP_H_H__
#define __FTP_H_H__

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef enum LOG_LEVEL
{
  INFO,
  PROMPT,
  DEBUG,
  ERROR,
} logll;

// ANSI_COLORS
#define ANSI_RESET_ALL "\x1b[0m"
#define ANSI_COLOR_BLACK "\x1b[30m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_WHITE "\x1b[37m"

#define SMALL_BUFF 100
#define FTP_DELIMITERS " \t\r\n\a"
#define FTP_TOKEN_BUFF 64

#define PORT "1990"
// FLAGS
#define REOF 0

// Packet representation
#define BUFFSIZE 508
struct network_packet
{
  u_int8_t command_type;
  u_int8_t connection_id;
  u_int8_t command_id;
  u_int8_t command_len;
  char command_buffer[BUFFSIZE];
} __attribute__((packed));

// info about the connected clients
struct client_info
{
  u_int8_t client_socket_id;
  u_int8_t client_connection_id;
};

// handling different form of ftp communication protocal
enum TYPE
{
  DONE,
  INF,
  REQU,
  TERM,
  DATA,
  EOT,
};

enum COMMANDS
{
  GET,
  CD,
  PWD,
};

// LOGS
void LOG(logll level, const char *format, ...);
int create_a_socket(char *port);

// utility functions
struct client_info *client_info_storage(u_int8_t socket_fd,
                                        u_int8_t connection_id);
// not sure what to do with them yet
void host_to_network_presentation(struct network_packet *hp);
void network_to_host_presentation(struct network_packet *np);

// void function that only return a signal to the client
void terminate_connection(struct network_packet *return_packet,
                          struct network_packet *recieved_packet,
                          u_int8_t socket_fd);

void end_of_transfer(struct network_packet *return_packet,
                     struct network_packet *recieved_packet,
                     u_int8_t socket_fd);

// network packet handler
void packet_initializer(struct network_packet *);
void print_packet(struct network_packet *packet);

// data transfer
int recv_data(int socket_fd, struct network_packet *client_data);
int send_data(int socket_fd, struct network_packet *client_data);
#endif
