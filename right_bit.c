#include <stdio.h>

/*
 * Getting the rigth most set bit of negative numbers
 * link to the article
 * [bit](https://dotkay.github.io/2018/03/03/checking-rightmost-set-bit/)
 */
int right_most_bit(int x) {
  int count = 0;

  x = x & (-x);
  while (x != 0) {
    x >>= 1;
    count++;
  }
  return count;
}

/*
 * Getting the right most set bit position in a bit vector
 */
int right_most_set_bit(int x) {
  int count = 0;

  while (x) {
    x >>= 1;
    count++;
  }

  return count;
}

/*
 * This will help out in detecting out the right most bit
 */
int main(void) {
  int x = -2;
  int y = -8;

  printf("%d\n", right_most_bit(x));
  printf("%d\n", right_most_bit(y));
  printf("%d\n", right_most_set_bit(8));

  return (0);
}
