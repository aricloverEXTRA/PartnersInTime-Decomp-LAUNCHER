#ifndef PIT_MSL_ARRAY_H
#define PIT_MSL_ARRAY_H

#include <nitro.h>

typedef void *(*MslArrayDestructor)(void *element);

#ifdef __cplusplus
extern "C" {
#endif

/* The constructor stores the element count one word before array.
 * header is the distance from the allocation base to the first element. */
void MSL_DeleteArray(void *array, u32 size, u32 header, MslArrayDestructor destroy);

#ifdef __cplusplus
}
#endif
#endif
