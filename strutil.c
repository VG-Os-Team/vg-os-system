#include "strutil.h"

int minha_strlen(const char *s) {
  int len = 0;
  while (s[len] != '\0')
    len++;
  return len;
}

void *minha_memset(void *dst, int val, unsigned int n) {
  unsigned char *ptr = (unsigned char *)dst;
  unsigned char byte = (unsigned char)val;
  for (unsigned int i = 0; i < n; i++) {
    ptr[i] = byte;
  }
  return dst;
}

void *memset(void *dst, int val, unsigned int n) {
  return minha_memset(dst, val, n);
}