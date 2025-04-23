#include "ftp.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>

// newtork_presentation to host network presentation from big edian to small
// edian
struct network_packet *
network_to_host_presentation(struct network_packet *network_presentation)
{
  // make a new host_presentation_packet
  struct network_packet *host_presentation_packet =
      (struct network_packet *)malloc(sizeof(struct network_packet));

  // initalize it
  memset(host_presentation_packet, 0, sizeof(struct network_packet));
  // fill it with the network_presentation_packet
  host_presentation_packet->command_id =
      ntohs(network_presentation->command_id);
  host_presentation_packet->command_type =
      ntohs(network_presentation->command_type);
  host_presentation_packet->command_len =
      ntohs(network_presentation->command_len);
  // return then host_presentation_packet

  memcpy(host_presentation_packet->command_buffer,
         network_presentation->command_buffer, BUFFSIZE);

  return host_presentation_packet;
}

// host to network_presentation for edianess check over the network(big edian)
struct network_packet *
host_to_network_presentation(struct network_packet *host_presentation)
{

  // make a new network_presentation_packet
  struct network_packet *network_presentation_packet =
      (struct network_packet *)malloc(sizeof(struct network_packet));

  // initalize it
  memset(network_presentation_packet, 0, sizeof(struct network_packet));

  // fill it with host_presentation_packet
  network_presentation_packet->command_id =
      ntohs(host_presentation->command_id);
  network_presentation_packet->command_id =
      ntohs(host_presentation->command_id);
  network_presentation_packet->command_id =
      ntohs(host_presentation->command_id);
  network_presentation_packet->command_id =
      ntohs(host_presentation->command_id);
  memcpy(network_presentation_packet->command_buffer,
         host_presentation->command_buffer, BUFFSIZE);

  return network_presentation_packet;
}

// handling the case on how to store the client information
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

// printing packet
void print_packet(struct network_packet *packet, u_int8_t packet_type)
{
  if (packet_type)
    LOG(INFO, "HOST PACKET");
  else
    LOG(INFO, "NETWORK PACKET");

  LOG(DEBUG, "Connection id: \t%d", packet->connection_id);
  LOG(DEBUG, "Command id: \t%d", packet->command_id);
  LOG(DEBUG, "Command type: \t%d", packet->command_type);
  LOG(DEBUG, "Command len: \t%d", packet->command_len);
  LOG(DEBUG, "Command buffer: \t%s", packet->command_buffer);
}

// handling termination of a commands
void terminate_connection(struct network_packet *return_packet,
                          struct network_packet *recieved_packet,
                          u_int8_t socket_fd)
{
  int x;
  recieved_packet->command_type = TERM;
  recieved_packet = host_to_network_presentation(return_packet);
  if ((x = send(socket_fd, recieved_packet, sizeof(struct network_packet),
                0)) != sizeof(struct network_packet))
    LOG(ERROR, "Sending termination packet error");
}

void end_of_transfer(struct network_packet *return_packet,
                     struct network_packet *recieved_packet, u_int8_t socket_fd)
{
  int x;
  recieved_packet->command_type = EOT;
  recieved_packet = host_to_network_presentation(return_packet);
  if ((x = send(socket_fd, recieved_packet, sizeof(struct network_packet),
                0)) != sizeof(struct network_packet))
    LOG(ERROR, "Sending end of transfer packet error");
}
