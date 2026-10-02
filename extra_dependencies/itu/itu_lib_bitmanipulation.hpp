#ifndef ITU_LIB_BITMANIPULATION_HPP
#define ITU_LIB_BITMANIPULATION_HPP

#ifndef ITU_UNITY_BUILD
#include <SDL3/SDL_stdinc.h> // fixed-sized integers
#endif

inline Uint32 hibit(Uint8 n)
{
    n |= (n >>  1u);
    n |= (n >>  2u);
    n |= (n >>  4u);
    return n-(n >> 1);
}

inline Uint32 hibit(Uint16 n) {
    n |= (n >>  1u);
    n |= (n >>  2u);
    n |= (n >>  4u);
    n |= (n >>  8u);
    return n-(n >> 1);
}

inline Uint32 hibit(Uint32 n) {
    n |= (n >>  1u);
    n |= (n >>  2u);
    n |= (n >>  4u);
    n |= (n >>  8u);
    n |= (n >> 16u);
    return n-(n >> 1);
}

inline Uint32 hibit(Uint64 n) {
    n |= (n >>  1u);
    n |= (n >>  2u);
    n |= (n >>  4u);
    n |= (n >>  8u);
    n |= (n >> 16u);
    n |= (n >> 32u);
    return n-(n >> 1);
}

#endif // ITU_LIB_BITMANIPULATION_HPP