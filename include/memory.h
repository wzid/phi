#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

// Safe memory allocation functions
void *s_malloc(size_t size);
void *s_calloc(size_t nmemb, size_t size);
void *s_realloc(void *ptr, size_t size);
void s_free(void *ptr);

#endif
