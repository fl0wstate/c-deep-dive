#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  cJSON *root = cJSON_CreateObject();

  cJSON_AddItemToObject(root, "name", cJSON_CreateString("flowstate"));
  cJSON_AddItemToObject(root, "message", cJSON_CreateString("system test one"));

  char *render = cJSON_Print(root);

  fprintf(stdout, "%s\n", render);

  free(render);
  cJSON_Delete(root);

  return EXIT_SUCCESS;
}
