#include "ftp.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static size_t size_packet = sizeof(struct network_packet);
void network_to_host_presentation(struct network_packet *np)
{
  if (np->command_len > (u_int8_t)BUFFSIZE)
  {
    LOG(ERROR, "Invalid command_len: %d", np->command_len);
    np->command_len = (u_int8_t)BUFFSIZE;
  }
}

void host_to_network_presentation(struct network_packet *hp)
{
  if (hp->command_len > (u_int8_t)BUFFSIZE)
  {
    LOG(ERROR, "Invalid command_len: %d", hp->command_len);
    hp->command_len = (u_int8_t)BUFFSIZE;
  }
  hp->command_buffer[hp->command_len] = '\0';
}

struct client_info *client_info_storage(u_int8_t socket_fd,
                                        u_int8_t connection_id)
{
  struct client_info *client_global_information =
      (struct client_info *)malloc(sizeof(struct client_info));

  if (!client_global_information)
    LOG(ERROR, "Local information failed to be stored, malloc failed");

  client_global_information->client_socket_id = socket_fd;
  client_global_information->client_connection_id = connection_id;

  return client_global_information;
}

void print_packet(struct network_packet *packet)
{
  LOG(DEBUG, "Connection id: \t%d", packet->connection_id);
  LOG(DEBUG, "Command id: \t%d", packet->command_id);
  LOG(DEBUG, "Command type: \t%d", packet->command_type);
  LOG(DEBUG, "Command len: \t%d", packet->command_len);
  LOG(DEBUG, "Command buffer: \t%s", packet->command_buffer);
}

void terminate_connection(struct network_packet *recieved_packet,
                          u_int8_t socket_fd)
{
  int x;
  LOG(INFO, "YOU ARE INSIDE THE TERMINATION FUNCTION...");
  recieved_packet->command_type = TERM;
  host_to_network_presentation(recieved_packet);
  if ((x = send(socket_fd, recieved_packet, sizeof(struct network_packet),
                0)) != sizeof(struct network_packet))
    LOG(ERROR, "Sending termination packet error");
}

void end_of_transfer(struct network_packet *return_packet, u_int8_t socket_fd)
{
  int x;
  return_packet->command_type = EOT;
  host_to_network_presentation(return_packet);
  if ((x = send(socket_fd, return_packet, sizeof(struct network_packet), 0)) !=
      sizeof(struct network_packet))
    LOG(ERROR, "Sending end of transfer packet error");
}

void packet_initializer(struct network_packet *packet)
{
  memset(packet, 0, sizeof(struct network_packet));
}

int recv_data(int socket_fd, struct network_packet *client_data)
{
  size_t total_read = 0;
  size_t data_read = 0;

  while (total_read < size_packet)
  {
    data_read = recv(socket_fd, (char *)client_data + total_read,
                     size_packet - total_read, 0);

    if (data_read <= 0)
    {
      LOG(ERROR, "receiving failed %s",
          data_read == 0 ? "Connection closed" : strerror(data_read));
      return -1;
    }
    total_read += data_read;
  }
  client_data->command_buffer[BUFFSIZE - 1] = '\0';
  return 0;
}

int send_data(int socket_fd, struct network_packet *client_data)
{
  size_t total_send = 0;
  size_t data_sent = 0;

  while (total_send < size_packet)
  {
    data_sent = send(socket_fd, (char *)client_data + total_send,
                     size_packet - total_send, 0);
    if (data_sent <= 0)
    {
      LOG(ERROR, "sending failed %s",
          data_sent == 0 ? "Connection closed" : strerror(data_sent));
      free(client_data);
      return -1;
    }
    total_send += data_sent;
  }
  return 0;
}

void send_packet(struct network_packet *client_data, u_int8_t socket_fd,
                 char *command)
{
  int x = 0;
  if ((x = send(socket_fd, client_data, sizeof(struct network_packet), 0)) !=
      sizeof(struct network_packet))
  {
    LOG(ERROR, "Sending %s Packets", command);
  }
}

off_t get_file_size(FILE *fp)
{
  struct stat st;

  if (fstat(fileno(fp), &st) == 0)
    return st.st_size;

  LOG(ERROR, "Error reading file");
  return -1;
}
