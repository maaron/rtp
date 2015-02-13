#pragma once

#define bit(n) ( 1 << n )
#define bit_mask(n) ( bit(n) - 1 )
#define bit_set(x, n) ( x |= bit(n) )
#define bit_get(x, n) ( (x & bit(n)) == bit(n) )
#define bf_mask(start, length) ( bit_mask(length) << (start) )
#define bf_prep(x, start, length) ( ((x) & bit_mask(length)) << (start) )
#define bf_get(x, start, length) ( ((x) >> (start)) & bit_mask(length) )
#define bf_set(y, x, start, length) ( y = ((y) & ~bf_mask(start, length)) | bf_prep(x, start, length) )
