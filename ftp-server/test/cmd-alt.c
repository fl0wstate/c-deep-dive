#include "../log.c"
#include <stdint.h>
#include <stdlib.h>

struct commands
{
  uint8_t id;
  uint8_t commands_len;
  char **command_data;
};

static char special_commands[4][10] = {"py", "help", "exit", "run"};

struct commands *data_retrived(char *argv)
{
  // allocate memory to a new data struct
  struct commands *cmd = (struct commands *)malloc(sizeof(struct commands));
  uint8_t start = -1;
  cmd->id = start;
  cmd->commands_len = 0;
  cmd->command_data = NULL;

  char *token;
  char *saved_pointer;
  uint8_t i, j;
  for (i = 0;; i++, argv = NULL)
  {
    token = strtok_r(argv, " \n\t", &saved_pointer);
    if (token == NULL)
      break;
    if (cmd->id == start)
    {
      for (j = 0; j < sizeof(special_commands) / sizeof(special_commands[0]);
           j++)
        // comparison fails but still passes
        if (!strcmp(token, special_commands[j]))
        {
          cmd->id = j;
          LOG(INFO, "The first command is special");
          break;
        }
    }
    else
    {
      cmd->commands_len++;

      char **commands = (char **)malloc(cmd->commands_len * sizeof(char *));
      if (cmd->commands_len > 1)
      {
        // you will need to allocate more memory to the rest of the remaining
        // arguments
        memcpy(commands, cmd->command_data,
               (cmd->commands_len) * sizeof(char *));
      }
      char *command = (char *)malloc((strlen(token) + 1) * sizeof(char));
      int i;
      for (i = 0; *(command + i) = *(token + i) == ':' ? ' ' : *(token + i);
           i++)
        ;

      // add to the temp  to the temps array of arguments
      *(commands + cmd->commands_len - 1) = command;

      cmd->command_data = commands;
    }
  }
  return cmd;
}

int main()
{
  struct commands *actual_arguments;
  //                command to detect:  command arguments
  char data[100] = "python exits runs run py exit run:py";

  actual_arguments = data_retrived(data);

  LOG(INFO, "Number of commands passed :-> %d", actual_arguments->commands_len);
  LOG(INFO, "Special command index: %d", actual_arguments->id);
  for (int i = 0; i < actual_arguments->commands_len; i++)
  {
    LOG(INFO, "%s", actual_arguments->command_data[i]);
  }

  LOG(INFO, "End");
  LOG(INFO, "size of uint8_t data type: %u", sizeof(uint8_t));
  return EXIT_SUCCESS;
}
