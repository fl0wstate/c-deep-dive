#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define PORT 7000
#define BUFFER_SIZE 1024

uv_loop_t *loop;
uv_tcp_t client;
uv_connect_t connect_req;

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}

void on_write(uv_write_t *req, int status)
{
  if (status)
  {
    fprintf(stderr, "Write error: %s\n", uv_strerror(status));
  }
  free(req);
}

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
  if (nread > 0)
  {
    printf("Server response: %.*s\n", (int)nread, buf->base);
  }
  else if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
    }
    uv_close((uv_handle_t *)stream, NULL);
  }

  if (buf->base)
  {
    free(buf->base);
  }
}

void on_connect(uv_connect_t *req, int status)
{
  if (status < 0)
  {
    fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
    return;
  }

  printf("Connected to server!\n");

  uv_read_start((uv_stream_t *)&client, alloc_buffer, on_read);

  // Send a message to the server
  char message[BUFFER_SIZE];
  snprintf(message, sizeof(message), "Hello, Server!\n");

  uv_write_t *write_req = malloc(sizeof(uv_write_t));
  uv_buf_t wrbuf = uv_buf_init(message, strlen(message));
  uv_write(write_req, (uv_stream_t *)&client, &wrbuf, 1, on_write);
}

int main()
{
  loop = uv_default_loop();

  uv_tcp_init(loop, &client);

  struct sockaddr_in dest;
  uv_ip4_addr("127.0.0.1", PORT, &dest);

  uv_tcp_connect(&connect_req, &client, (const struct sockaddr *)&dest,
                 on_connect);

  return uv_run(loop, UV_RUN_DEFAULT);
}
