#include "../ftp.h"
#include <pthread.h>
#include <stdio.h>

void *print_thread(void *array)
{
  char **str = (char **)array;
  while (*str != NULL)
  {
    printf("%s\n", *str);
    str++;
  }
  return NULL;
}

/* it has some issues:
 * - thread is still active
 * - main thread might finish executing before thread1
 */
int main(int ac, char **arg)
{
  if (ac < 2)
  {
    LOG(ERROR, "less args provided: %s : arguments1 ...", arg[0]);
    return (-1);
  }

  pthread_t thread1;

  pthread_create(&thread1, NULL, print_thread, (void *)arg);

  /* gracefully kill a thread...*/
  pthread_cancel(thread1);

  return (0);
}
