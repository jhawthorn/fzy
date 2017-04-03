#include "theft.h"

/* Fowler/Noll/Vo hash, 64-bit FNV-1a.
 * This hashing algorithm is in the public domain.
 * For more details, see: http://www.isthe.com/chongo/tech/comp/fnv/. */
static const uint64_t fnv64_prime = 1099511628211L;
static const uint64_t fnv64_offset_basis = 14695981039346656037UL;

/* Init a hasher for incremental hashing. */
void theft_hash_init(struct theft_hasher *h) {
    h->accum = fnv64_offset_basis;
}

/* Sink more data into an incremental hash. */
void theft_hash_sink(struct theft_hasher *h, uint8_t *data, size_t bytes) {
    if (h == NULL || data == NULL) { return; }
    uint64_t a = h->accum;
    for (size_t i = 0; i < bytes; i++) {
        a = (a ^ data[i]) * fnv64_prime;
    }
    h->accum = a;
}

/* Finish hashing and get the result. */
theft_hash theft_hash_done(struct theft_hasher *h) {
    theft_hash res = h->accum;
    theft_hash_init(h);                /* reset */
    return res;
}

/* Hash a buffer in one pass. (Wraps the above functions.) */
theft_hash theft_hash_onepass(uint8_t *data, size_t bytes) {
    struct theft_hasher h;
    theft_hash_init(&h);
    theft_hash_sink(&h, data, bytes);
    return theft_hash_done(&h);
}
