#ifndef BASE_CHECK_H_
#define BASE_CHECK_H_

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                       \
  do {                                                         \
    if (!(condition)) {                                        \
      fprintf(stderr, "CHECK failed: %s\n", #condition);       \
      abort();                                                 \
    }                                                          \
  } while (0)

#define DCHECK(condition) CHECK(condition)

#endif  // BASE_CHECK_H_
