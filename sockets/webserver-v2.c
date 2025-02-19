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
    fprintf(stderr, ANSI_COLOR_RED "%s:[ERROR] [Line: %d]", timestamp,
            __LINE__);
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

    if (write(client_fd, frame_header, header_len) != header_len) {
      LOG(ERROR, "Failed to send the frame_header to client_fd => [%d]",
          client_fd);
      continue;
    }

    if (write(client_fd, binary_data, payload_len) != payload_len) {
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

  // Send the header
  if (send(client_fd, header, header_len, 0) == -1) {
    perror("Failed to send Pong header");
    return;
  }

  // Send the payload (if any)
  if (payload_len > 0 && send(client_fd, payload, payload_len, 0) == -1) {
    perror("Failed to send Pong payload");
    return;
  }

  LOG(DEBUG, "Pong frame sent to client [%d]", client_fd);
}

/**
 * receive_text_frame: reads the clients data to a buffer and then broadcasts
the data
 * to all the connected clients in the poll
 * @client_fd: the client file descriptor
 * @pollfds: pointer to a struct holding all the available connected clients in
the poll array
 * @n_pollfds: pointer to the number of clients in the pollfd array
 * Return: 0 success, -1 failure
 */
int receive_text_frame(int client_fd, struct pollfd **pollfds,
                       uint32_t *n_pollfds) {
  unsigned char buffer[BUFFER_SIZE] = {0};
  int bytes_read = read(client_fd, buffer, sizeof(buffer));

  if (bytes_read <= 0) {
    if (bytes_read == 0) {
      LOG(DEBUG, "Client [%d] disconnected", client_fd);
    } else {
      LOG(ERROR, "Failed to read from client [%d]: %s", client_fd,
          strerror(errno));
    }
    close(client_fd);
    del_client_from_poll(pollfds, client_fd,
                         n_pollfds); // Remove client from pollfd array
    return -1;                       // Indicate failure
  }

  unsigned char fin = (buffer[0] & 0x80) != 0;
  unsigned char opcode = buffer[0] & 0x0F;
  unsigned char masked = (buffer[1] & 0x80) != 0;
  unsigned char payload_len = buffer[1] & 0x7F;

  LOG(DEBUG, "OPCODE: %d", opcode);
  // Handle Ping frame (opcode 0x09)
  if (opcode == 0x09) {
    LOG(DEBUG, "Received ping from client [%d]", client_fd);
    int mask_offset = 2;
    int data_offset = masked ? 6 : 2;

    if (payload_len == 126) {
      payload_len = (buffer[2] << 8) | buffer[3];
      mask_offset = 4;
      data_offset = masked ? 8 : 4;
    } else if (payload_len == 127) {
      LOG(ERROR, "64-bit payload length not supported for client [%d]",
          client_fd);
      return -1; // Indicate failure
    }

    unsigned char payload[BUFFER_SIZE] = {0};
    if (payload_len > 0) {
      memcpy(payload, &buffer[data_offset], payload_len);
      if (masked) {
        unsigned char *mask = &buffer[mask_offset];
        for (int i = 0; i < payload_len; i++) {
          payload[i] ^= mask[i % 4];
        }
      }
    }
    // send back the exact same ping payload
    send_pong(client_fd, payload, payload_len);
    LOG(DEBUG, "Pong sent to client [%d]", client_fd);
    return (0);
  }

  // Handle Close frame (opcode 0x08)
  if (opcode == 0x08) {
    LOG(DEBUG, "Client [%d] sent a close frame", client_fd);
    close(client_fd);
    del_client_from_poll(pollfds, client_fd,
                         n_pollfds); // Remove client from pollfd array
    return -1;                       // Indicate failure
  }

  // Handle Text frame (opcode 0x01) or Binary frame (opcode 0x02)
  if (opcode == 0x01) { // Text frame
    int mask_offset = 2;
    int data_offset = masked ? 6 : 2; // Masking key is 4 bytes
    uint64_t payload_len_64 = 0;

    // Handle extended payload lengths
    if (payload_len == 126) {
      payload_len = (buffer[2] << 8) | buffer[3];
      mask_offset = 4;
      data_offset = masked ? 8 : 4;
    } else if (payload_len == 127) {
      if (recv(client_fd, &payload_len_64, 8, 0) != 8) {
        LOG(ERROR,
            "Failed to read the next 8 bits of information in the header from "
            "client: %d",
            client_fd);
      }
      if (payload_len_64 > UINT32_MAX) {
        LOG(ERROR, "Payload length too large: %llu for client [%d]",
            (unsigned long long)payload_len_64, client_fd);
        return -1;
      }

      payload_len = (uint32_t)be64toh(payload_len_64);
      mask_offset = 10;
      data_offset = masked ? 14 : 10;
    }

    if (payload_len >= BUFFER_SIZE) {
      LOG(ERROR, "Payload too large for buffer");
      close(client_fd);
      del_client_from_poll(pollfds, client_fd,
                           n_pollfds); // Remove client from pollfd array
      return -1;
    }

    if (masked) {
      unsigned char *mask = &buffer[mask_offset];
      unsigned char *payload = &buffer[data_offset];
      char message[BUFFER_SIZE] = {0};

      // Unmask the payload
      for (int i = 0; i < payload_len; i++) {
        message[i] = payload[i] ^ mask[i % 4];
      }
      message[payload_len] = '\0'; // Null-terminate the message

      LOG(DEBUG, "Received text from client [%d]: %s", client_fd, message);

      // Broadcast or process as text
      broadcast_to_all_clients(pollfds, message, client_fd, n_pollfds);
    }
  } else if (opcode == 0x02) { // Binary frame
    int mask_offset = 2;
    int data_offset = masked ? 6 : 2; // Masking key is 4 bytes
    uint64_t payload_len_64 = 0;

    // Handle extended payload lengths
    if (payload_len == 126) {
      payload_len = (buffer[2] << 8) | buffer[3];
      mask_offset = 4;
      data_offset = masked ? 8 : 4;
    } else if (payload_len == 127) {
      if (recv(client_fd, &payload_len_64, 8, 0) != 8) {
        LOG(ERROR,
            "Failed to read the next 8 bits of information in the header from "
            "client: %d",
            client_fd);
      }
      if (payload_len_64 > UINT32_MAX) {
        LOG(ERROR, "Payload length too large: %llu for client [%d]",
            (unsigned long long)payload_len_64, client_fd);
        return -1;
      }
      payload_len = (uint32_t)be64toh(payload_len_64);
      mask_offset = 10;
      data_offset = masked ? 14 : 10;
    }

    LOG(DEBUG, "payload_length: %d:    BUFFER_SIZE: %d", payload_len,
        BUFFER_SIZE);
    if (payload_len >= BUFFER_SIZE) {
      LOG(ERROR, "Payload too large for buffer");
      return -1;
    }

    if (masked) {
      unsigned char *mask = &buffer[mask_offset];
      unsigned char *binary_data = &buffer[data_offset];

      // Unmask the payload
      for (int i = 0; i < payload_len; i++) {
        binary_data[i] ^= mask[i % 4];
      }

      LOG(DEBUG, "Received binary data from client [%d], length: %d", client_fd,
          payload_len);
      // Here you might want to process or broadcast binary data differently:
      // For example, if you need to save it:
      FILE *file = fopen("binary_output.bin", "wb");
      fwrite(binary_data, 1, payload_len, file);
      fclose(file);
      // Or you might broadcast it differently:
      broadcast_binaray_to_all_clients(*pollfds, binary_data, payload_len,
                                       client_fd, *n_pollfds);
    }
  }
  return 0; // Success
}

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
