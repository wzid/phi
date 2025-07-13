#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *s_malloc(size_t size) {
    if (size == 0) {
        fprintf(stderr, "Warning: malloc called with size 0\n");
        return NULL;
    }
    
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Fatal error: malloc failed to allocate %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    
    return ptr;
}

void *s_calloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) {
        fprintf(stderr, "Warning: calloc called with zero size\n");
        return NULL;
    }
    
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        fprintf(stderr, "Fatal error: calloc failed to allocate %zu * %zu bytes\n", nmemb, size);
        exit(EXIT_FAILURE);
    }
    
    return ptr;
}

void *s_realloc(void *ptr, size_t size) {
    if (size == 0) {
        s_free(ptr);
        return NULL;
    }
    
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "Fatal error: realloc failed to allocate %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    
    return new_ptr;
}

void s_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}
