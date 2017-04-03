/* 
   A C-program for MT19937-64 (2004/9/29 version).
   Coded by Takuji Nishimura and Makoto Matsumoto.

   This is a 64-bit version of Mersenne Twister pseudorandom number
   generator.

   Before using, initialize the state by using init_genrand64(seed)  
   or init_by_array64(init_key, key_length).

   Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   References:
   T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
     ACM Transactions on Modeling and 
     Computer Simulation 10. (2000) 348--357.
   M. Matsumoto and T. Nishimura,
     ``Mersenne Twister: a 623-dimensionally equidistributed
       uniform pseudorandom number generator''
     ACM Transactions on Modeling and 
     Computer Simulation 8. (Jan. 1998) 3--30.

   Any feedback is very welcome.
   http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
*/

/* The code has been modified to store internal state in heap/stack
 * allocated memory, rather than statically allocated memory, to allow
 * multiple instances running in the same address space. */

#include <stdlib.h>
#include <stdio.h>
#include "theft_mt.h"

#define NN THEFT_MT_PARAM_N
#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */

static uint64_t genrand64_int64(struct theft_mt *r);

/* Heap-allocate a mersenne twister struct. */
struct theft_mt *theft_mt_init(uint64_t seed) {
    struct theft_mt *mt = malloc(sizeof(struct theft_mt));
    if (mt == NULL) { return NULL; }
    theft_mt_reset(mt, seed);
    return mt;
}

/* Free a heap-allocated mersenne twister struct. */
void theft_mt_free(struct theft_mt *mt) {
    free(mt);
}

/* initializes mt[NN] with a seed */
void theft_mt_reset(struct theft_mt *mt, uint64_t seed)
{
    mt->mt[0] = seed;
    uint16_t mti = 0;
    for (mti=1; mti<NN; mti++) {
        mt->mt[mti] = (6364136223846793005ULL *
            (mt->mt[mti-1] ^ (mt->mt[mti-1] >> 62)) + mti);
    }
    mt->mti = mti;
}

/* Get a 64-bit random number. */
uint64_t theft_mt_random(struct theft_mt *mt) {
    return genrand64_int64(mt);
}

/* Generate a random number on [0,1]-real-interval. */
double theft_mt_random_double(struct theft_mt *mt)
{
    return (genrand64_int64(mt) >> 11) * (1.0/9007199254740991.0);
}

/* generates a random number on [0, 2^64-1]-interval */
static uint64_t genrand64_int64(struct theft_mt *r)
{
    int i;
    uint64_t x;
    static uint64_t mag01[2]={0ULL, MATRIX_A};

    if (r->mti >= NN) { /* generate NN words at one time */

        /* if init has not been called, */
        /* a default initial seed is used */
        if (r->mti == NN+1)
            theft_mt_reset(r, 5489ULL);

        for (i=0;i<NN-MM;i++) {
            x = (r->mt[i]&UM)|(r->mt[i+1]&LM);
            r->mt[i] = r->mt[i+MM] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        for (;i<NN-1;i++) {
            x = (r->mt[i]&UM)|(r->mt[i+1]&LM);
            r->mt[i] = r->mt[i+(MM-NN)] ^ (x>>1) ^ mag01[(int)(x&1ULL)];
        }
        x = (r->mt[NN-1]&UM)|(r->mt[0]&LM);
        r->mt[NN-1] = r->mt[MM-1] ^ (x>>1) ^ mag01[(int)(x&1ULL)];

        r->mti = 0;
    }
  
    x = r->mt[r->mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}
