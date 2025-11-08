#ifndef DESCALE_COMMON_H
#define DESCALE_COMMON_H


#ifdef _WIN32
    #include <malloc.h>
#else
    #define _POSIX_C_SOURCE 200112L
#endif
#include <stddef.h>
#include <stdlib.h>

#define DSMAX(a, b) ((a) > (b) ? (a) : (b))
#define DSMIN(a, b) ((a) > (b) ? (b) : (a))


static inline int ceil_n(int x, int n)
{
    return (x + (n - 1)) & ~(n - 1);
}


static inline int floor_n(int x, int n)
{
    return x & ~(n - 1);
}


static inline void descale_aligned_malloc(void **pptr, size_t size, size_t alignment)
{
#ifdef _WIN32
    *pptr = _aligned_malloc(size, alignment);
#else
    int err = posix_memalign(pptr, alignment, size);
    if (err)
        *pptr = NULL;
#endif
}


static inline void descale_aligned_free(void *ptr)
{
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}


#endif  // DESCALE_COMMON_H
