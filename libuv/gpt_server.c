#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define PORT 7000

uv_loop_t *loop;

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

void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
  if (nread > 0)
  {
    printf("Server received: %.*s", (int)nread, buf->base);

    // Echo back the message
    uv_write_t *write_req = malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
    uv_write(write_req, client, &wrbuf, 1, on_write);
  }
  else if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      fprintf(stderr, "Read error: %s\n", uv_strerror(nread));
    }
    uv_close((uv_handle_t *)client, NULL);
  }

  if (buf->base)
  {
    free(buf->base);
  }
}

void on_new_connection(uv_stream_t *server, int status)
{
  if (status < 0)
  {
    fprintf(stderr, "New connection error: %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, client);

  if (uv_accept(server, (uv_stream_t *)client) == 0)
  {
    uv_read_start((uv_stream_t *)client, alloc_buffer, on_read);
  }
  else
  {
    uv_close((uv_handle_t *)client, NULL);
  }
}

int main()
{
  loop = uv_default_loop();

  uv_tcp_t server;
  uv_tcp_init(loop, &server);

  struct sockaddr_in addr;
  uv_ip4_addr("0.0.0.0", PORT, &addr);

  uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);
  int r = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
  if (r)
  {
    fprintf(stderr, "Listen error: %s\n", uv_strerror(r));
    return 1;
  }

  printf("Server listening on port %d\n", PORT);
  return uv_run(loop, UV_RUN_DEFAULT);
}
