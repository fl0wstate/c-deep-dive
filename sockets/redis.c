#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// create a generic redis function handler
/**
 * redis_cli_command_handler: handle the redis-cli function in c manner
 * @c: pointer that holds the state of the connection establshed
 * @argc: argument count passed
 * @argv: pointer to an array of constant chars(array of string)
 * Return: 0 success, negative int indicates failure
 */
int redis_cli_command_handler(redisContext *c, int argc, const char **argv) {

  int index = 0;
  redisReply *reply;
  // create an array of each argument length
  size_t *argvlen = malloc(argc * sizeof(size_t));

  if (!argvlen) {
    fprintf(stderr, "Error during memory allocation\n");
    return -1;
  }

  // fill up the array with the correct argv length
  for (; index < argc; index++)
    argvlen[index] = strlen(argv[index]);

  // handle the command
  reply = redisCommandArgv(c, argc, argv, argvlen);

  free(argvlen);

  if (reply == NULL || c->err) {
    fprintf(stderr, "Failed to execute the redisCommandArgv\n ERROR: %s\n",
            c->errstr);
    return -2;
  }

  // handle the reply
  switch (reply->type) {
  case REDIS_REPLY_STRING:
    fprintf(stdout, "-> %s\n", reply->str);
    break;
  case REDIS_REPLY_INTEGER:
    fprintf(stdout, "-> %lld\n", reply->integer);
    break;
  case REDIS_REPLY_STATUS:
    fprintf(stdout, "-> %s\n", reply->str);
    break;
  case REDIS_REPLY_ERROR:
    fprintf(stderr, "-> %s\n", reply->str);
    break;
  case REDIS_REPLY_ARRAY:
    fprintf(stdout, "array len -> %zu\n", reply->elements);
    for (size_t i = 0; i < reply->elements; i++)
      fprintf(stdout, "-> %s\n", (char *)reply->element[i]);
    break;
  case REDIS_REPLY_NIL:
    fprintf(stdout, "-> %s\n", reply->str);
    break;
  default:
    fprintf(stderr,
            "somthing happened that's either an error or not handled\n");
  }
  freeReplyObject(reply);

  return (0);
}
// redis on C simple right
int main(void) {
  // context
  redisContext *c;
  // reply holder
  redisReply *reply;
  int status = 0;

  // make a simple connection to the database
  c = redisConnect("127.0.0.1", 6379);
  if (c->err) {
    fprintf(stderr, "%s\n", c->errstr);
    return -1;
  }

  // connection was established well, send commands
  // simple ping command
  reply = redisCommand(c, "PING %s", "hello redis");
  fprintf(stdout, "%s\n", reply->str);
  freeReplyObject(reply);

  // pushing to a list
  reply = redisCommand(c, "DEL myslist");
  freeReplyObject(reply);
  for (unsigned int j = 0; j < 10; j++) {
    char buf[64] = {0};

    snprintf(buf, 15, "%u", j);
    reply = redisCommand(c, "LPUSH myslist element-%s", buf);
    freeReplyObject(reply);
  }

  // reading from the redis list
  reply = redisCommand(c, "LRANGE myslist 0 -1");
  if (reply->type == REDIS_REPLY_ARRAY) {
    // you need to loop over each of the elements and get the string value
    for (unsigned int j = 0; j < reply->elements; j++) {
      fprintf(stdout, "@ index [%d] = %s\n", j, reply->element[j]->str);
    }
  }
  freeReplyObject(reply);

  // the above methods are super repetitive and not the efficient way to do it
  int command_len = 3;
  const char *command[] = {"SET", "HELLO", "WORLD"};
  reply = redisCommandArgv(c, command_len, command, NULL);

  if (reply == NULL || c->err) {
    fprintf(stderr, "failed to execute the redisCommandArgv\n ERROR: %s\n",
            c->errstr);
    return -1;
  }
  freeReplyObject(reply);

  // now lets go even deeper where we can be able to create an generic function
  // that will be able to handle all the redis-cli funciton in a similar way in
  printf("====================================================================="
         "======\n");
  const char *set_cmd[] = {"SET", "post", "TPOT"};
  const char *get_cmd[] = {"GET", "post"};
  const char *lpush_cmd[] = {"RPUSH", "mylist", "hello", "world",
                             "we",    "are",    "so",    "back"};

  if ((status = redis_cli_command_handler(c, 3, set_cmd)) != 0)
    fprintf(stderr, "you have an error in the command\n");

  if ((status = redis_cli_command_handler(c, 2, get_cmd)) != 0)
    fprintf(stderr, "you have an error in the command\n");

  if ((status = redis_cli_command_handler(c, 8, lpush_cmd)) != 0)
    fprintf(stderr, "you have an error in the command\n");
  printf("====================================================================="
         "======\n");

  // next up is how to handle pub-subs in hiredis api, will have to use it with
  // asysnchronous approach, either poll or libevent;

  // C clean up the conenction
  redisFree(c);
  return 0;
}
