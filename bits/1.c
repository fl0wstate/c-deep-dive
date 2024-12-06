#include "macros.c"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

size_t super_strlen(const char *str);
/**
 * main - the entry point for you to become super cracked in C
 * return Starting...
 */
int main(void) {
  const char *string = "Hello world";
  const char *str = NULL;
  uint8_t i = 0;
  uint32_t bit_array[] = {0};
  printf("%zu \n", super_strlen(string));
  printf("%zu \n", super_strlen("Yippy...."));
  size_t ret = super_strlen(str);
  if (ret == -1) {
    fprintf(stderr, "It appears you have an empty string\n");
  }

  SET_BIT(4, bit_array);
  SET_BIT(9, bit_array);
  SET_BIT(7, bit_array);
  SET_BIT(1, bit_array);
  SET_BIT(5, bit_array);
  SET_BIT(0, bit_array);
  SET_BIT(9, bit_array);

  CLEAR_BIT(0, bit_array);
  for (; i < BITARRY_LEN; i++) {
    if (CHECK_BIT(i, bit_array) != 0) {
      printf("1");
    }
    printf("0");
  }
  printf("\n");
  return (0);
}

size_t super_strlen(const char *str) {
  if (!str) {
    return (-1);
  }

  /* Please ensure that the string is of length > 8 */
  // cast to an array of 64bit size
  uint64_t *copy_str = (uint64_t *)str;

  while (1) {
    /* Pointer to the first memory chunk to inspect */
    uint64_t first_64bit_chunck = *copy_str;

    /* This will cause an overflow on the original bits that were there
     * highlighting a null terminator is present */
    uint64_t overflow_trap_detector =
        first_64bit_chunck + (uint64_t)0x7efefefefefefeff;

    /* Pin points which chunck of memory contains the null terminator */
    first_64bit_chunck = (~first_64bit_chunck) ^ overflow_trap_detector;

    /* Validate no NULL terminator was found proceeds to the next iteration */
    if ((first_64bit_chunck & (uint64_t)0x8101010101010100) == 0) {
      ++copy_str;
      continue;
    }

    uint8_t i = 0;
    /* Copy the updated position( last position after finding a chunck with a
     * NULL ) */
    uint64_t tail_chunck = *copy_str;

    /* This ensures each chunck(1byte == 8 bits) doesn't have NULL*/
    while ((tail_chunck & 0xFF) != 0 && i < 8) {
      ++i;
      /* Isolate each byte after inspection */
      tail_chunck = tail_chunck >> 8;
    }
    return (char *)copy_str - str + i;
  }
}
