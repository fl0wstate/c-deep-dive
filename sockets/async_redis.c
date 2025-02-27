#include <hiredis/adapters/poll.h>
#include <hiredis/async.h>
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <stdio.h>

static int exit_loop = 0;

void connectCallback(const redisAsyncContext *ac, int status) {
  if (status != REDIS_OK) {
    fprintf(stderr, "Error: %s\n", ac->errstr);
    exit_loop = 1;
    return;
  }
  // otherwise the connecton was established
  fprintf(stdout, "DEBUG: Connection established...\n");
}

void disconnectCallback(const redisAsyncContext *ac, int status) {
  // update the status
  exit_loop = 1;

  if (status != REDIS_OK) {
    fprintf(stderr, "Error while disconnecting: %s\n", ac->errstr);
    return;
  }
  // everything happend correctly
  fprintf(stdout, "dissconnected succefully\n");
}

void generic_callback(redisAsyncContext *ac, void *r, void *privatedata) {
  redisReply *reply = r;

  if (reply == NULL || ac->err) {
    fprintf(stderr, "Error: %s", ac->errstr);
    return;
  }

  fprintf(stdout, "%s\n", (char *)privatedata);

  // handle the reply type accordingly
  switch (reply->type) {
  case REDIS_REPLY_INTEGER:
    fprintf(stdout, "%lld\n", reply->integer);
    break;
  case REDIS_REPLY_ERROR:
    fprintf(stderr, "Error: %s\n", reply->str);
    break;
  case REDIS_REPLY_STRING:
    fprintf(stdout, "%s\n", reply->str);
    break;
  case REDIS_REPLY_ARRAY:
    fprintf(stdout, "array len -> %zu\n", reply->elements);
    for (size_t i = 0; i < reply->elements; i++)
      fprintf(stdout, "-> %s\n", (char *)reply->element[i]->str);
    break;
  case REDIS_REPLY_STATUS:
    fprintf(stdout, "-> %s\n", reply->str);
    break;
  default:
    fprintf(stderr,
            "somthing happened that's either an error or not handled\n");
  }

  redisAsyncDisconnect(ac);
}
// once you are done doing all the connection and data exchange that's when
// you will need to close the connection
int main(void) {

  // you have now created an async context for connections
  redisAsyncContext *ac = redisAsyncConnect("127.0.0.1", 6379);

  if (!ac || ac->err) {
    fprintf(stderr, "Error: %s\n", ac->errstr);
    // redisAsyncFree(ac);
    return (1);
  }

  // adds the client to the poll array
  redisPollAttach(ac);

  // handle connection
  redisAsyncSetConnectCallback(ac, connectCallback);
  // handle dissconnection
  redisAsyncSetDisconnectCallback(ac, disconnectCallback);
  // handle how to send commands
  redisAsyncCommand(ac, NULL, NULL, "SET py %s", "python");
  // redisAsyncCommand(ac, getCallback, "test-1", "GET py");

  const char *set[] = {"SET", "py", "PYTHON"};
  const char *get[] = {"GET", "py"};
  const char *lpush_cmd[] = {"RPUSH", "mylist", "hello", "world",
                             "we",    "are",    "so",    "back"};
  const char *get_myList[] = {"LRANGE", "mylist", "0", "-1"};
  redisAsyncCommandArgv(ac, generic_callback, "Setting Up a command", 3, set,
                        NULL);
  redisAsyncCommandArgv(ac, generic_callback, "Multiple commands", 8, lpush_cmd,
                        NULL);
  redisAsyncCommandArgv(ac, generic_callback, "Multiple command", 2, get, NULL);
  redisAsyncCommandArgv(ac, generic_callback, "generic func to handle", 4,
                        get_myList, NULL);

  // enter the while loop
  while (!exit_loop) {
    redisPollTick(ac, 0.1);
  }

  fprintf(stdout, "Hello world\n");
  return (0);
}
