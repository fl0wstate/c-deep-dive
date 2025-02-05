#include <stdio.h>

#define max(a, b)                                                              \
  _Generic((a), int: max_int, float: max_float, double: max_double)((a), (b))

int max_int(int a, int b) { return a > b ? a : b; }
float max_float(float a, float b) { return a > b ? a : b; }
double max_double(double a, double b) { return a > b ? a : b; }

int main(void)
{
  printf("%d\n", max(44, 9));
  printf("%f\n", max(44.5, 5.33));
  printf("%lf\n", max(44.5, 45.5));
  return 0;
}
