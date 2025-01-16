#include <arpa/inet.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define GUIDX "b8c4ba98-1aba-417e-b0f7-1faf0d2c6280"

// ping resoponse
char *ping(void)
{
  while (1)
  {
    sleep(2);
    return ("PING...");
  }
}
// Function to perform Base64 encoding
char *base64_encode(const unsigned char *input, int length)
{
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
  if (buff == NULL)
  {
    perror("Failed to allocate memory");
    return NULL;
  }
  memcpy(buff, bptr->data, bptr->length);
  buff[bptr->length] = 0;

  BIO_free_all(b64);

  return buff;
}

// Function to compute SHA1 hash
unsigned char *sha1_hash(const char *input, size_t length)
{
  unsigned char *hash = (unsigned char *)malloc(SHA_DIGEST_LENGTH);
  if (hash == NULL)
  {
    perror("Failed to allocate memory");
    return NULL;
  }
  SHA1((unsigned char *)input, length, hash);
  return hash;
}

/* https://youtu.be/5tBmkxpeTyE */
void send_text_frame(int client_fd, const char *message)
{
  size_t message_len = strlen(message);
  size_t frame_size = 2 + message_len;
  char *frame = malloc(frame_size);

  if (frame == NULL)
  {
    perror("Failed to allocate memory");
    return;
  }

  frame[0] = 0x81;
  frame[1] = message_len;

  memcpy(&frame[2], message, message_len);

  if (send(client_fd, frame, frame_size, 0) == -1)
  {
    perror("Failed to send message");
  }
  free(frame);
}
void receive_text_frame(int client_fd)
{
  unsigned char buffer[BUFFER_SIZE];
  int bytes_read = read(client_fd, buffer, sizeof(buffer));

  if (bytes_read <= 0)
  {
    // Your existing error handling...
    close(client_fd);
    return;
  }

  unsigned char fin = (buffer[0] & 0x80) != 0;
  unsigned char opcode = buffer[0] & 0x0F;
  unsigned char masked = (buffer[1] & 0x80) != 0;
  unsigned char payload_len = buffer[1] & 0x7F;

  int mask_offset = 2;
  int data_offset = masked ? 6 : 2; // Masking key is 4 bytes

  // Handle extended payload lengths if needed
  if (payload_len == 126)
  {
    // 16-bit length
    payload_len = (buffer[2] << 8) | buffer[3];
    mask_offset = 4;
    data_offset = masked ? 8 : 4;
  }
  else if (payload_len == 127)
  {
    // 64-bit length (not handling this case for simplicity)
    fprintf(stderr, "64-bit payload length not supported\n");
    return;
  }

  if (masked)
  {
    unsigned char *mask = &buffer[mask_offset];
    unsigned char *payload = &buffer[data_offset];
    char message[BUFFER_SIZE];

    // Unmask the payload
    for (int i = 0; i < payload_len; i++)
    {
      message[i] = payload[i] ^ mask[i % 4];
    }

    message[payload_len] = '\0';

    printf("Received: %s\n", message);
    send_text_frame(client_fd, message);
  }
}

int main()
{
  int server_fd, client_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Create and bind socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    perror("Socket failed");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("Bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 3) < 0)
  {
    perror("Listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("WebSocket Server is listening on port %d\n", PORT);

  // Accept client connection
  while (1)
  {
    if ((client_fd = accept(server_fd, (struct sockaddr *)&address,
                            (socklen_t *)&addrlen)) < 0)
    {
      perror("Accept failed");
      continue;
    }

    printf("Connection established with %s:%d\n", inet_ntoa(address.sin_addr),
           ntohs(address.sin_port));

    // Read handshake request
    int valread = read(client_fd, buffer, BUFFER_SIZE);
    if (valread <= 0)
    {
      perror("Failed to read handshake request");
      close(client_fd);
      continue;
    }
    printf("Handshake request:\n%s\n", buffer);

    // Extract Sec-WebSocket-Key
    char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
    if (!key_start)
    {
      printf("No Sec-WebSocket-Key found.\n");
      close(client_fd);
      continue;
    }
    key_start += strlen("Sec-WebSocket-Key: ");
    char key[256];
    sscanf(key_start, "%s", key);
    printf("Sec-WebSocket-Key: %s\n", key);

    // Generate Sec-WebSocket-Accept
    char concatenated[256];
    snprintf(concatenated, sizeof(concatenated), "%s%s", key, GUID);
    printf("key + GUID: %s%s\n", key, GUID);
    unsigned char *hash = sha1_hash(concatenated, strlen(concatenated));
    if (hash == NULL)
    {
      close(client_fd);
      continue;
    }
    char *accept_key = base64_encode(hash, SHA_DIGEST_LENGTH);
    free(hash);
    if (accept_key == NULL)
    {
      close(client_fd);
      continue;
    }

    // Send handshake response
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);

    free(accept_key);

    if (send(client_fd, response, strlen(response), 0) == -1)
    {
      perror("Failed to send handshake response");
      close(client_fd);
      continue;
    }
    printf("Handshake response sent.\n");

    // Send a welcome message
    send_text_frame(client_fd, "Welcome to the C WebSocket Server!");
    printf("client file descriptor: %d\n", client_fd);

    // Receive and echo messages
    while (1)
    {
      receive_text_frame(client_fd);
    }

    // Close client socket
    close(client_fd);
  }

  // Close server socket
  close(server_fd);

  return 0;
}
