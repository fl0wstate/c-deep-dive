#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#define on_error(...)                                                          \
  {                                                                            \
    fprintf(stderr, "[ERROR] %s : %s\n", __VA_ARGS__);                         \
    fflush(stderr);                                                            \
  }

#define LOG(...)                                                               \
  {                                                                            \
    fprintf(stdout, __VA_ARGS__);                                              \
    fflush(stdout);                                                            \
  }

uv_loop_t *loop;
struct sockaddr_in addr;

void on_close_connection(uv_handle_t *handle)
{
  free(handle);
  LOG("[INFO] Connection closed\n");
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
}
/*
void echo_write(uv_write_t *req, int status)
{
  if (status)
  {
    fprintf(stderr, "Write error %s\n", uv_strerror(status));
  }
  free(req);
}
*/

/* TODO: Try to figure out how to broadcast the text from one client to all the
 * others as well */
void read_cb(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
  if (nread < 0)
  {
    if (nread == UV_EOF)
    {
      LOG("[INFO] Disconnected...");
      uv_close((uv_handle_t *)client, on_close_connection);
    }
  }
  else if (nread > 0)
  {
    /* This is where you will need to start reading the data to stdout in the
     * server side */
    write(1, buf->base, buf->len);
  }
  free(buf->base);
}
/*
void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf)
{
  if (nread < 0)
  {
    if (nread != UV_EOF)
    {
      fprintf(stderr, "Read error %s\n", uv_err_name(nread));
      uv_close((uv_handle_t *)client, NULL);
    }
  }
  else if (nread > 0)
  {
    uv_write_t *req = (uv_write_t *)malloc(sizeof(uv_write_t));
    uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
    uv_write(req, client, &wrbuf, 1, echo_write);
  }

   Always free the buffer for each new read ends
  if (buf->base)
  {
    free(buf->base);
  }
}
*/

void on_new_connection(uv_stream_t *server, int status)
{
  printf("[INFO] Connection established\n");
  if (status < 0)
    on_error("New connection error", uv_strerror(status));

  /* Setting up the TCP connection for the incomming client connections */
  uv_tcp_t *client = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  /* thing that will be running inside the loop */
  uv_tcp_init(loop, client);
  /* Accept the connection */
  if (uv_accept(server, (uv_stream_t *)client) == 0)
  {
    /* Reading starts, alloc_buffer needs to set up the buffer for reading data
     * echo_read is used to ensure that all the data that is read
     */
    uv_read_start((uv_stream_t *)client, alloc_buffer, read_cb);
  }
  else
    uv_close((uv_handle_t *)client, on_close_connection);
}

int main()
{
  loop = uv_default_loop();

  uv_tcp_t server;
  /* thing that will be running inside the loop */
  uv_tcp_init(loop, &server);

  /* does all the network translation for you */
  uv_ip4_addr("127.0.0.1", 7000, &addr);

  /* Binds the ip */
  int br = uv_tcp_bind(&server, (const struct sockaddr *)&addr, 0);
  if (br)
    on_error("Bind", uv_strerror(br));

  /* This if (true) {
  } else {
  }ll be the basic start point for you to make the connection with thte
   * client
   * on_new_connection will be the handler for all the communication that is
   * going to happend between the server and the client
   */
  int r = uv_listen((uv_stream_t *)&server, 128, on_new_connection);
  if (r)
    on_error("Listen", uv_strerror(r));
  return uv_run(loop, UV_RUN_DEFAULT);
}
