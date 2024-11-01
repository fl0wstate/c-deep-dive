#ifndef __MY_MACRO__
#define __MY_MACRO__

#include <limits.h>
/* Bit array where you can use to make changes to a particular  end */
/* Check for the system bit field */
#define BITARRY_LEN ((sizeof(unsigned)) * (CHAR_BIT))
/* Set a bit at a particular index in the bit array */
#define SET_BIT(K, BITARRY)                                                    \
  (BITARRY[(K) / (BITARRY_LEN)] |= (1 << ((K) % (BITARRY_LEN))))
#define CLEAR_BIT(K, BITARRY)                                                  \
  (BITARRY[(K) / (BITARRY_LEN)] &= ~(1 << ((K) % (BITARRY_LEN))))
#define CHECK_BIT(K, BITARRY)                                                  \
  (BITARRY[(K) / (BITARRY_LEN)] & (1 << ((K) % (BITARRY_LEN))))
#endif
