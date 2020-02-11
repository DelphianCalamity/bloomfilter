


#ifndef _MURMURHASH3_H_
#define _MURMURHASH3_H_

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;

#include <stdint.h>

void MurmurHash3_x64_128 ( const void * key, int len, uint32_t seed, void * out );

#endif // _MURMURHASH3_H_
