#include <arpa/inet.h>
#include <assert.h>
#include <cjson/cJSON.h>
#include <endian.h>
#include <errno.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 6729
#define BACKLOG 10
#define BUFFER_SIZE 4096
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define ANSI_RESET_ALL "\x1b[0m"
#define ANSI_COLOR_BLACK "\x1b[30m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_WHITE "\x1b[37m"

typedef enum LOG_LEVEL { INFO, DEBUG, ERROR } logll;
char *json_rendered_message(const char *sender, const char *message);

// handy tool i use to make make logs
void LOG(logll level, const char *format, ...) {
  va_list args;
  time_t now;
  struct tm *timeinfo;
  char timestamp[20];

  // get the time
  time(&now);
  // store the time in a timeinfo struct
  timeinfo = localtime(&now);
  // collect the relevant information from the timeinfo strcut
  strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

  // prefix timestamps to the logs
  switch (level) {
  case INFO:
    fprintf(stdout, ANSI_COLOR_GREEN "%s:[INFO]", timestamp);
    break;
  case DEBUG:
    fprintf(stdout, ANSI_COLOR_CYAN "%s:[DEBUG]", timestamp);
    break;
  case ERROR:
    fprintf(stderr, ANSI_COLOR_RED "%s:[ERROR]", timestamp);
    break;
  }

  va_start(args, format);
  switch (level) {
  case INFO:
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    break;
  case DEBUG:
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    break;
  case ERROR:
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    break;
  }
  va_end(args);
}

char *base64_encode(const unsigned char *input, int length) {
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // No newlines
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, length);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  char *buff = (char *)malloc(bptr->length + 1);
  if (buff == NULL) {
    perror("Failed to allocate memory");
    return NULL;
  }
  memcpy(buff, bptr->data, bptr->length);
  buff[bptr->length] = 0;

  BIO_free_all(b64);

  return buff;
}

unsigned char *sha1_hash(const char *input, size_t length) {
  unsigned char *hash = (unsigned char *)malloc(SHA_DIGEST_LENGTH);
  if (hash == NULL) {
    perror("Failed to allocate memory");
    return NULL;
  }
  SHA1((unsigned char *)input, length, hash);
  return hash;
}

// when the client is off we need update the pollfds list
void del_client_from_poll(struct pollfd **pollfds, int client_fd,
                          uint32_t *n_pollfds) {
  for (uint32_t i = 0; i < *n_pollfds; i++) {
    if ((*pollfds)[i].fd == client_fd) {
      // Shift remaining elements to the left
      for (uint32_t j = i; j < *n_pollfds - 1; j++) {
        (*pollfds)[j] = (*pollfds)[j + 1];
      }
      (*n_pollfds)--;
      break;
    }
  }
}

int send_frame(int client_fd, const void *data, size_t data_len, int is_text) {
  size_t header_len;
  size_t frame_size;
  char *frame;
  ssize_t bytes_sent;
  size_t total_sent = 0;
  char *json_data = {0};

  // Calculate header size based on payload length
  if (data_len <= 125) {
    header_len = 2;
  } else if (data_len <= 65535) {
    header_len = 4;
  } else {
    header_len = 10;
  }

  frame_size = header_len + data_len;
  frame = malloc(frame_size);
  if (frame == NULL) {
    LOG(ERROR, "Failed to allocate memory for frame");
    return -1;
  }

  // Frame construction (same as before)
  frame[0] = is_text ? 0x81 : 0x82;

  if (data_len <= 125) {
    frame[1] = data_len;
  } else if (data_len <= 65535) {
    frame[1] = 126;
    frame[2] = (data_len >> 8) & 0xFF;
    frame[3] = data_len & 0xFF;
  } else {
    frame[1] = 127;
    for (int i = 0; i < 8; i++) {
      frame[2 + i] = (data_len >> ((7 - i) * 8)) & 0xFF;
    }
  }

  // make a cJSON wrapper for the data being sent
  memcpy(&frame[header_len], data, data_len);

  // Modified sending loop with better error handling
  while (total_sent < frame_size) {
    bytes_sent = send(client_fd, frame + total_sent, frame_size - total_sent,
                      MSG_NOSIGNAL);
    if (bytes_sent == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Retry if would block
        continue;
      } else if (errno == EPIPE || errno == ECONNRESET) {
        // Connection closed by peer
        LOG(ERROR, "Client [%d] disconnected (connection closed by peer)",
            client_fd);
        free(frame);
        return -1;
      } else {
        LOG(ERROR, "Failed to send frame to client [%d]: %s", client_fd,
            strerror(errno));
        free(frame);
        return -1;
      }
    }
    total_sent += bytes_sent;
  }

  LOG(DEBUG, "Frame sent to client [%d]", client_fd);
  free(frame);
  return 0;
}

/**
 * broadcast_to_all_clients: relays the message to all the connected clients
 * @pollfds: a pointer to an array of struct pollfds
 * @message: void pointer holding the data to be sent to all the connected
 * clients
 * @client_fd: file descriptor for the current client sharing the message
 * @n_pollfds: unsigned 32 bit int that holds the current number of available
 * pollfds Return: 0 success, otherwise failure
 */
int broadcast_to_all_clients(struct pollfd **pollfds, void *message,
                             int client_fd, uint32_t *n_pollfds) {
  for (uint32_t i = 0; i < *n_pollfds; i++) {
    // Skip the sender and the server socket (fd == 1)
    if ((*pollfds)[i].fd != client_fd && (*pollfds)[i].fd != 1) {
      LOG(DEBUG, "Broadcasting message to client_fd [%d]", (*pollfds)[i].fd);
      LOG(DEBUG, "++++++++++++++++++++++++++++++++++++");
      LOG(DEBUG, "%s", message);
      LOG(DEBUG, "++++++++++++++++++++++++++++++++++++");

      // Send the frame and check for errors
      if (send_frame((*pollfds)[i].fd, message, strlen(message), 1) == -1) {
        LOG(ERROR,
            "Failed to send message to client_fd [%d]. Closing the connection.",
            (*pollfds)[i].fd);

        // Close the client socket and remove it from the pollfd array
        close((*pollfds)[i].fd);
        del_client_from_poll(pollfds, (*pollfds)[i].fd, n_pollfds);
        i--;
      }
    }
  }
  return 0;
}

ssize_t send_all(int fd, void *buffer, size_t len) {
  size_t total = 0;
  const char *data = buffer;
  while (total < len) {
    ssize_t sent = write(fd, data + total, len - total);
    if (sent < 0) {
      LOG(ERROR, "Somthing happened");
      return sent;
    }
    total += sent;
  }
  return total;
}

void broadcast_binaray_to_all_clients(struct pollfd *pollfds,
                                      const unsigned char *binary_data,
                                      uint32_t payload_len, int sender_fd,
                                      uint32_t n_pollfds) {
  for (uint32_t i = 0; i < n_pollfds; i++) {
    int client_fd = pollfds[i].fd;
    if (sender_fd == client_fd || client_fd == 3)
      continue;

    unsigned char frame_header[10];
    int header_len = 2;

    frame_header[0] = 0x82;
    if (payload_len <= 125) {
      frame_header[1] = payload_len;
    } else if (payload_len <= 65535) {
      frame_header[1] = 126;
      frame_header[2] = (payload_len >> 8) & 0xFF;
      frame_header[3] = payload_len & 0xFF;
      header_len += 2;
    } else {
      frame_header[1] = 127;
      u_int64_t payload_len_64 = htobe64((u_int64_t)payload_len);
      memcpy(&frame_header[2], &payload_len_64, 8);
      header_len += 8;
    }

    if (send_all(client_fd, frame_header, header_len) != header_len) {
      LOG(ERROR, "Failed to send the frame_header to client_fd => [%d]",
          client_fd);
      continue;
    }

    LOG(DEBUG, "bin data: %d", payload_len);
    if (send_all(client_fd, (void *)binary_data, payload_len) != payload_len) {
      LOG(ERROR, "Failed to send binary data to client_fd => [%d]", client_fd);
      continue;
    }
  }
}

void send_pong(int client_fd, const unsigned char *payload,
               size_t payload_len) {
  unsigned char header[10];
  size_t header_len;
  int index = 0;

  // Set FIN bit and opcode (0xA for Pong)
  header[index++] = 0x8A; // FIN bit set, opcode 0xA

  // Set payload length
  if (payload_len <= 125) {
    header[index++] = payload_len;
  } else if (payload_len <= 65535) {
    header[index++] = 126;
    header[index++] = (payload_len >> 8) & 0xFF;
    header[index++] = payload_len & 0xFF;
  } else {
    header[index++] = 127;
    for (int i = 0; i < 8; i++) {
      header[index++] = (payload_len >> ((7 - i) * 8)) & 0xFF;
    }
  }

  header_len = index;

  // Send the header completely
  if (send_all(client_fd, header, header_len) != header_len) {
    perror("Failed to send Pong header");
    return;
  }

  // Send the payload completely (if any)
  if (payload_len > 0 &&
      send_all(client_fd, payload, payload_len) != payload_len) {
    perror("Failed to send Pong payload");
    return;
  }

  LOG(DEBUG, "Pong frame sent to client [%d]", client_fd);
}

/**
 * read_n_bytes: helps in partial reading ensureing data i read completely
 * @fd: client file descriptor we are reading data from
 * @buff: where we are storing the data being read from the client_fd
 * @n: number of bytes we need to read from the file descriptor
 * Return: total number of bytes read from the client_fd, any negative value
 * indicates an error
 */
ssize_t read_n_bytes(int fd, unsigned char *buf, size_t n) {
  size_t total = 0;
  while (total < n) {
    ssize_t bytes = read(fd, buf + total, n - total);
    if (bytes < 0) {
      // If error is EAGAIN/EWOULDBLOCK, break out and return what we got.
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      return -1;
    } else if (bytes == 0) {
      // Connection closed.
      break;
    }
    total += bytes;
  }
  return total;
}

// *************************************************************************
int receive_text_frame(int client_fd, struct pollfd **pollfds,
                       uint32_t *n_pollfds) {
  size_t allocated = BUFFER_SIZE;
  unsigned char *message_buffer = malloc(allocated);
  if (!message_buffer) {
    LOG(ERROR, "Malloc failed for message buffer");
    return -1;
  }

  size_t total_payload = 0;
  int finished = 0;
  int initial_opcode = -1;
  const int poll_timeout_ms = 1000;

  while (!finished) {
    struct pollfd pfd = {.fd = client_fd, .events = POLLIN};
    int poll_ret = poll(&pfd, 1, poll_timeout_ms);
    if (poll_ret == 0) {
      LOG(DEBUG, "Client [%d] idle; no data available", client_fd);
      free(message_buffer);
      return 0; // Idle client, no action needed
    } else if (poll_ret < 0) {
      LOG(ERROR, "Poll error for client [%d]: %s", client_fd, strerror(errno));
      free(message_buffer);
      close(client_fd);
      del_client_from_poll(pollfds, client_fd, n_pollfds);
      return -1;
    } else if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
      LOG(DEBUG,
          "Client [%d] disconnected or error detected by poll (revents: %d)",
          client_fd, pfd.revents);
      free(message_buffer);
      close(client_fd);
      del_client_from_poll(pollfds, client_fd, n_pollfds);
      return -1;
    }

    unsigned char header[14] = {0};
    ssize_t header_bytes = read_n_bytes(client_fd, header, 2);
    if (header_bytes == 0) {
      LOG(DEBUG, "Client [%d] disconnected while reading header", client_fd);
      free(message_buffer);
      close(client_fd);
      del_client_from_poll(pollfds, client_fd, n_pollfds);
      return -1;
    } else if (header_bytes < 0) {
      LOG(ERROR, "Failed to read header from client [%d]: %s", client_fd,
          strerror(errno));
      free(message_buffer);
      close(client_fd);
      del_client_from_poll(pollfds, client_fd, n_pollfds);
      return -1;
    }

    unsigned char fin = (header[0] & 0x80) != 0;
    unsigned char opcode = header[0] & 0x0F;
    unsigned char masked = (header[1] & 0x80) != 0;
    unsigned char payload_len_field = header[1] & 0x7F;
    int header_size = 2;
    uint64_t payload_len = 0;

    if (opcode == 0x09) {
      LOG(DEBUG, "Ping payload active from client [%d]", client_fd);
      if (payload_len_field == 126) {
        if (read_n_bytes(client_fd, header + header_size, 2) != 2) {
          LOG(ERROR, "Failed to read extended payload for ping from client: %d",
              client_fd);
          free(message_buffer);
          return -1;
        }
        payload_len = (header[2] << 8) | header[3];
        header_size += 2;
      } else if (payload_len_field == 127) {
        LOG(ERROR,
            "64-bit payload length not supported for ping from client: %d",
            client_fd);
        free(message_buffer);
        return -1;
      } else {
        payload_len = payload_len_field;
      }
      if (masked) {
        if (read_n_bytes(client_fd, header + header_size, 4) != 4) {
          LOG(ERROR, "Failed to read mask for ping from client: %d", client_fd);
          free(message_buffer);
          return -1;
        }
        header_size += 4;
      }
      unsigned char *ping_payload = malloc(payload_len);
      if (!ping_payload) {
        LOG(ERROR, "Failed to allocate memory for ping_payload");
        free(message_buffer);
        return -1;
      }
      LOG(DEBUG, "Reading %llu bytes for ping payload",
          (unsigned long long)payload_len);
      if (read_n_bytes(client_fd, ping_payload, payload_len) != payload_len) {
        LOG(ERROR, "Failed to read full ping payload from client: %d",
            client_fd);
        free(ping_payload);
        free(message_buffer);
        return -1;
      }
      if (masked) {
        unsigned char *mask = header + (header_size - 4);
        for (uint32_t i = 0; i < payload_len; i++) {
          ping_payload[i] ^= mask[i % 4];
        }
      }
      send_pong(client_fd, ping_payload, payload_len);
      LOG(DEBUG, "Sent a pong payload to client: %d", client_fd);
      free(ping_payload);
      // Continue waiting for further frames (ping frames do not complete a
      // message)
      continue;
    }

    if (opcode == 0x08) {
      LOG(DEBUG, "Client [%d] sent a close frame", client_fd);
      free(message_buffer);
      close(client_fd);
      del_client_from_poll(pollfds, client_fd, n_pollfds);
      return -1;
    }

    if (opcode != 0x00 && initial_opcode == -1) {
      if (opcode != 0x01 && opcode != 0x02) {
        LOG(ERROR, "Unsupported opcode [%d] from client [%d]", opcode,
            client_fd);
        free(message_buffer);
        return -1;
      }
      initial_opcode = opcode;
    }

    if (payload_len_field == 126) {
      if (read_n_bytes(client_fd, header + header_size, 2) != 2) {
        LOG(ERROR, "Incomplete 16-bit payload length from client [%d]",
            client_fd);
        free(message_buffer);
        close(client_fd);
        del_client_from_poll(pollfds, client_fd, n_pollfds);
        return -1;
      }
      payload_len = (header[2] << 8) | header[3];
      header_size += 2;
    } else if (payload_len_field == 127) {
      if (read_n_bytes(client_fd, header + header_size, 8) != 8) {
        LOG(ERROR, "Incomplete 64-bit payload length from client [%d]",
            client_fd);
        free(message_buffer);
        close(client_fd);
        del_client_from_poll(pollfds, client_fd, n_pollfds);
        return -1;
      }
      uint64_t len = 0;
      memcpy(&len, header + header_size, 8);
      payload_len = be64toh(len);
      header_size += 8;
    } else {
      payload_len = payload_len_field;
    }

    if (masked) {
      if (read_n_bytes(client_fd, header + header_size, 4) != 4) {
        LOG(ERROR, "Failed to read mask from client [%d]", client_fd);
        free(message_buffer);
        close(client_fd);
        del_client_from_poll(pollfds, client_fd, n_pollfds);
        return -1;
      }
      header_size += 4;
    }

    if (total_payload + payload_len > allocated) {
      allocated = total_payload + payload_len;
      unsigned char *temp = realloc(message_buffer, allocated);
      if (!temp) {
        LOG(ERROR, "Memory reallocation failed");
        free(message_buffer);
        return -1;
      }
      message_buffer = temp;
    }

    if (read_n_bytes(client_fd, message_buffer + total_payload, payload_len) !=
        payload_len) {
      LOG(ERROR, "Failed to read payload from client [%d]", client_fd);
      free(message_buffer);
      close(client_fd);
      del_client_from_poll(pollfds, client_fd, n_pollfds);
      return -1;
    }

    if (masked) {
      unsigned char *mask = header + (header_size - 4);
      for (uint64_t i = 0; i < payload_len; i++) {
        message_buffer[total_payload + i] ^= mask[i % 4];
      }
    }

    total_payload += payload_len;
    finished = fin;
  }

  if (initial_opcode == 0x01) {
    unsigned char *temp = realloc(message_buffer, total_payload + 1);
    if (!temp) {
      LOG(ERROR, "Memory reallocation failed for null termination");
      free(message_buffer);
      return -1;
    }
    message_buffer = temp;
    message_buffer[total_payload] = '\0';
    LOG(DEBUG, "Received text from client [%d]: %s", client_fd, message_buffer);
    broadcast_to_all_clients(pollfds, message_buffer, client_fd, n_pollfds);
  } else if (initial_opcode == 0x02) {
    LOG(DEBUG, "Received binary data from client [%d] of length %zu", client_fd,
        total_payload);
    broadcast_binaray_to_all_clients(*pollfds, message_buffer, total_payload,
                                     client_fd, *n_pollfds);
  }
  free(message_buffer);
  return 0;
}
// *************************************************************************

/**
 * handle_handshake - Handle the initial handshake with the client
 * @client_fd: interger value of the client file descriptor
 * Return: void(this needs to be fixed in order to handle failed websocket
 * handshakes)
 */
void handle_handshake(int client_fd) {
  char buffer[BUFFER_SIZE];
  int client_read = read(client_fd, buffer, BUFFER_SIZE);

  if (client_read <= 0) {
    perror("Failed to read handshake request");
    close(client_fd);
    return;
  }

  // ensure the key is present
  char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
  if (!key_start) {
    fprintf(stderr, "Missing websocket key...closing the connection\n");
    close(client_fd);
    return;
  }

  // read the key
  key_start += strlen("Sec-WebSocket-Key: ");
  char key[256];
  sscanf(key_start, "%255s", key);
  // printf("Sec-WebSocket-Key: %s\n", key);

  // Generate Sec-WebSocket-Accept
  char concatenated[512];
  snprintf(concatenated, sizeof(concatenated), "%s%s", key, GUID);
  concatenated[sizeof concatenated - 1] = '\0';
  // printf("key + GUID: %s%s\n", key, GUID);
  unsigned char *hash = sha1_hash(concatenated, strlen(concatenated));

  // still look into the issue where hash in null
  if (hash == NULL) {
    LOG(ERROR, "Failed to generate the hash");
    close(client_fd);
    return;
  }

  // generate a key to send to the client
  char *accept_key = base64_encode(hash, SHA_DIGEST_LENGTH);
  // avoid memory leak!!!
  if (!accept_key) {
    close(client_fd);
    return;
  }
  // still an issue
  // free(hash);

  // setting up the handshake response header
  char response[BUFFER_SIZE];
  snprintf(response, BUFFER_SIZE,
           "HTTP/1.1 101 Switching Protocols\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Accept: %s\r\n\r\n",
           accept_key);

  free(accept_key);
  if (send(client_fd, response, strlen(response), 0) == -1) {
    fprintf(stderr, "Failed to send the handshake\n");
    close(client_fd);
    return;
  }

  fprintf(stdout, "Handshake was succefull\n");
  // done setting up the handshake, websocket is now ready
}

#define handle_errror(fd, msg)                                                 \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "Error thrown by this %s func with the following code: %s\n",      \
            strerror(fd), msg);                                                \
    close(fd);                                                                 \
  } while (0);

// this is where we are going to setup the server function using pollfds

// create the server
int create_server(uint32_t port) {
  int server_fd;
  struct sockaddr_in server_addr = {0};
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("Failed to create the server socket");
    return (-1);
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    handle_errror(server_fd, "Bind");
    return (-1);
  }

  LOG(INFO, "Server listening on port %d", port);
  return (server_fd);
}

// initialze the pollfds
void init_pollfd(struct pollfd **pollfds, uint32_t *n_pollfds, int server_fd,
                 uint32_t *max_pollfds) {
  (*pollfds) = calloc(*n_pollfds + 1, sizeof(pollfds));

  if (!pollfds) {
    LOG(ERROR, "Memory allocation failed! at %d in %s", __LINE__,
        __FILE_NAME__);
    exit(EXIT_FAILURE);
  }

  (*pollfds)[0].fd = server_fd;
  (*pollfds)[0].events = POLLIN;
  (*n_pollfds) = 1;
  LOG(INFO, "Poll file descriptors created succrfully");
}

// clean up the pollfds
void clean_up_pollfd(struct pollfd **pollfds, uint32_t *n_pollfds) {
  for (uint32_t i = 0; i < *n_pollfds; i++) {
    close((*pollfds)[i].fd);
  }
  if (*pollfds)
    free(*pollfds);
  *pollfds = NULL;
  *n_pollfds = 0;
}

// add_clients_to_poll
int add_clients_to_poll(struct pollfd *pollfds[], uint32_t new_client_fd,
                        uint32_t *n_pollfds, uint32_t *max_pollfds) {
  uint32_t status = *n_pollfds;
  if (*n_pollfds == *max_pollfds) {
    *max_pollfds *= 2; /* Double the size of the max number of connection to
                          be made to the server*/
    *pollfds = realloc((void *)*pollfds, (sizeof(**pollfds) * (*max_pollfds)));
    if (!pollfds) {
      perror("realloc failed");
      return (-1);
    }
  }
  (*pollfds)[*n_pollfds].fd = new_client_fd;
  (*pollfds)[*n_pollfds].events = POLLIN;
  (*n_pollfds)++;
  if (status == *n_pollfds) {
    LOG(ERROR, "Failed to add client to pollfds");
    return (-1);
  }
  return (0);
}

/**
 * json_rendered_message: used to rnder the message being sent to the client in
 * form of json
 * @sender: char pointer to th user name
 * @message: pointer to the data being sent over to the clients
 * Return: pointer to a string of json data to be sent over
 */
char *json_rendered_message(const char *sender, const char *message) {
  // TODO: ensure you have handled all the errors that may occur
  cJSON *root = cJSON_CreateObject();

  cJSON_AddItemToObject(root, "username", cJSON_CreateString(sender));
  cJSON_AddItemToObject(root, "message", cJSON_CreateString(message));

  char *rendered_json_message = cJSON_Print(root);
  cJSON_Delete(root);
  return rendered_json_message;
}

// accept connections
int accept_connections(int server_fd, struct pollfd **pollfds,
                       uint32_t *n_pollfds, uint32_t *max_pollfds) {
  uint32_t client_fd;
  int status;
  char msg_buffer[BUFFER_SIZE];

  /* accept connections */
  if ((client_fd = accept(server_fd, NULL, NULL)) < 0) {
    LOG(ERROR, "Failed to accept connection: %s\n", strerror(client_fd));
    return -1;
  }

  /* collect the address comming in from the clients */
  if (add_clients_to_poll(pollfds, client_fd, n_pollfds, max_pollfds) < 0) {
    LOG(ERROR, "Failed to add client to pollfds");
    return (-1);
  }

  LOG(DEBUG, "Accepted new connection from: [%d]", client_fd);
  /* set the buffer for writting the messages */

  // change the protocol to websockets communication
  handle_handshake(client_fd);

  memset(&msg_buffer, 0, BUFFER_SIZE);
  sprintf(msg_buffer,
          "You have been connected to the server with the file descriptor: "
          "[%d]",
          client_fd);
  // char *data =
  //    json_rendered_message("System", "You are now connected to the
  //    server!!");

  /* send the message */
  // send_frame(client_fd, (void *)&msg_buffer, strlen(msg_buffer), 1);
  LOG(DEBUG, "Message sent to the client [%d]", client_fd);
  return (0);
}

// receive_text_frame

int main(void) {
  struct pollfd *pollfds;
  uint32_t n_pollfds = 0;
  uint32_t max_pollfds = 12;
  uint32_t status = 0;
  int server_fd = 0, is_active = 0;

  signal(SIGPIPE, SIG_IGN);
  server_fd = create_server(PORT);

  if (server_fd < 0)
    exit(EXIT_FAILURE);

  status = listen(server_fd, BACKLOG);
  if (status < 0)
    handle_errror(server_fd, "Listen...");

  // to understand poll -> https://youtu.be/O-yMs3T0APU
  init_pollfd(&pollfds, &n_pollfds, server_fd, &max_pollfds);

  while (1) {
    is_active = poll(pollfds, n_pollfds, 2000);

    if (is_active < 0) {
      LOG(ERROR, "Poll failed with the following error: %s",
          strerror(is_active));
      break;
    } else if (is_active == 0) {
      LOG(DEBUG, "Waiting for connections...");
      continue;
    } else {
      // loop over every memember of the pollfds list
      for (uint32_t i = 0; i < n_pollfds; i++) {
        LOG(DEBUG, "%d = %d", i, n_pollfds);
        if (pollfds[i].revents & POLLIN) {
          if (pollfds[i].fd == server_fd) {
            // accept connections: if it's a server fd allow it to accept
            // connections from other clients
            LOG(DEBUG,
                "We have a new connection checking if it's the server_fd");
            if (accept_connections(server_fd, &pollfds, &n_pollfds,
                                   &max_pollfds) < 0) {
              LOG(ERROR, "Failed to accept connection...");
              break;
            }
          } else {
            // this is a client handle the connection: basically read data from
            // the client
            LOG(DEBUG, "Reading data from the client [%d]", pollfds[i].fd);
            if (receive_text_frame(pollfds[i].fd, &pollfds, &n_pollfds) < 0) {
              LOG(ERROR, "Client [%d] removed from pollfd array",
                  pollfds[i].fd);
            }
          }
        }
      }
    }
    // instead of kicking out people in the chat make sure that you ping them
    // with some information about the current number of people logged in
    LOG(DEBUG, "Current connections = %d", n_pollfds);
    if (n_pollfds <= 1) {
      LOG(DEBUG, "No active connections...");
      LOG(DEBUG, "Closing the server....");
      close(server_fd);
      break;
    }
  }
  LOG(DEBUG, "Cleaning up the pollfds...");
  clean_up_pollfd(&pollfds, &n_pollfds);
  return (0);
}
