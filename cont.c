#include <stdint.h>
#include <stdio.h>
/*
 * This is a simple explanation of how the C continue keyword
 * is used to skip the rest of the code and jump back to the next
 * iteration in line
 */
int main(void)
{
  uint8_t x = 0;
  uint8_t i = 0;
  char str[] = "Hello World";

  int len = sizeof(str) / sizeof(str[0]);
  printf("Len of the array: %d\n", len);

  while (i < len)
  {
    i++;
    if (str[i] == 'o')
    {
      x++;
      printf("You have skipped me %c", str[i]);
      continue;
    }
    printf("character = %c\n", str[i]);
  }
  printf("%d\n", x);
}

/* Bits is all you need to know... */
uint64_t juicyNumber = 0x7efefefefefefeff;
uint64_t magicNumber = 0x8101010101010100;
