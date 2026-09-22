#include "strutil.h"

int minha_strlen(const char *s) {
  int len = 0;
  while (s[len] != '\0')
    len++;
  return len;
}