#ifndef random_h_INCLUDED
#define random_h_INCLUDED

#include <time.h>

static u32 fast_random_seed;

void random_init();
i32 random_i32();
f32 random_f32();

void fast_random_init();
u32 fast_random_u32();
f32 fast_random_f32();

#ifdef CSM_CORE_IMPLEMENTATION

inline void fast_random_init() {
    fast_random_seed = time(NULL);
}

inline u32 fast_random_u32() {
	uint32_t x = fast_random_seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return fast_random_seed = x;

    //fast_random_seed = (214013 * fast_random_seed + 2531011);
    //return (fast_random_seed >> 16) & 0x7FFF;
}

inline f32 fast_random_f32() {
	return (f32)fast_random_u32() / (f32)RAND_MAX;
}

void random_init() {
	srand(time(NULL));
}

i32 random_i32(i32 max) {
	return rand() / (RAND_MAX / max);
}

f32 random_f32() {
	return (f32)rand() / (f32)RAND_MAX;
}

#endif

#endif
