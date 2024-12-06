#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

size_t super_strlen(const char *str)
{
  if (!str)
  {
    return (size_t)(-1); // Return -1 if the input string is NULL
  }

  // Make a copy of the str
  uint64_t *copy_str = (uint64_t *)str;

  while (1)
  {
    /* Copy over the first element in the array */
    uint64_t first_element = *copy_str;
    printf("first_element (raw 64-bit chunk): 0x%016lx\n", first_element);

    uint64_t value_added = first_element + (uint64_t)0x7efefefefefefeff;
    printf("value_added (after adding constant): 0x%016lx\n", value_added);

    first_element = (~first_element) ^ value_added;
    printf("first_element (after negation and XOR): 0x%016lx\n", first_element);

    if ((first_element & (uint64_t)0x8101010101010100) == 0)
    {
      printf("No null byte found in this chunk. Moving to the next 64-bit "
             "chunk.\n\n");
      ++copy_str;
      continue;
    }

    uint8_t i = 0;
    uint64_t tail_element = *copy_str;

    while ((tail_element & 0xFF) != 0 && i < 8)
    {
      printf("Checking byte %d in tail_element: 0x%02x\n", i,
             (uint8_t)(tail_element & 0xFF));
      ++i;
      tail_element = tail_element >> 8;
    }

    printf("Null byte found after %d bytes.\n", i);
    return (char *)copy_str - str + i;
  }
}

int main()
{
  const char *test_str = "Hello, world!";
  size_t length = super_strlen(test_str);
  printf("String length: %zu\n", length);
  return 0;
}
