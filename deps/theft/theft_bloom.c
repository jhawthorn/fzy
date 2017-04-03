#include <string.h>
#include <stdio.h>

#include "theft.h"
#include "theft_bloom.h"

#define DEFAULT_BLOOM_BITS 17
#define DEBUG_BLOOM_FILTER 0
#define LOG2_BITS_PER_BYTE 3
#define HASH_COUNT 4

static uint8_t get_bits_set_count(uint8_t counts[256], uint8_t byte);

/* Initialize a bloom filter. */
struct theft_bloom *theft_bloom_init(uint8_t bit_size2) {
    size_t sz = 1 << (bit_size2 - LOG2_BITS_PER_BYTE);
    struct theft_bloom *b = malloc(sizeof(struct theft_bloom) + sz);
    if (b) {
        b->size = sz;
        b->bit_count = bit_size2;
        memset(b->bits, 0, sz);
    }
    return b;
}

/* Hash data and mark it in the bloom filter. */
void theft_bloom_mark(struct theft_bloom *b, uint8_t *data, size_t data_size) {
    uint64_t hash = theft_hash_onepass(data, data_size);
    uint8_t bc = b->bit_count;
    uint64_t mask = (1 << bc) - 1;

    /* Use HASH_COUNT distinct slices of MASK bits as hashes for the bloom filter. */
    int bit_inc = (64 - bc) / HASH_COUNT;

    for (int i = 0; i < (64 - bc); i += bit_inc) {
        uint64_t v = (hash & (mask << i)) >> i;
        size_t offset = v / 8;
        uint8_t bit = 1 << (v & 0x07);
        b->bits[offset] |= bit;
    }
}

/* Check whether the data's hash is in the bloom filter. */
bool theft_bloom_check(struct theft_bloom *b, uint8_t *data, size_t data_size) {
    uint64_t hash = theft_hash_onepass(data, data_size);
    uint8_t bc = b->bit_count;
    uint64_t mask = (1 << bc) - 1;

    int bit_inc = (64 - bc) / HASH_COUNT;

    for (int i = 0; i < (64 - bc); i += bit_inc) {
        uint64_t v = (hash & (mask << i)) >> i;
        size_t offset = v / 8;
        uint8_t bit = 1 << (v & 0x07);
        if (0 == (b->bits[offset] & bit)) { return false; }
    }

    return true;
}

/* Free the bloom filter. */
void theft_bloom_free(struct theft_bloom *b) { free(b); }

/* Dump the bloom filter's contents. (Debugging.) */
void theft_bloom_dump(struct theft_bloom *b) {
    uint8_t counts[256];
    memset(counts, 0xFF, sizeof(counts));

    size_t total = 0;
    uint16_t row_total = 0;
    
    for (size_t i = 0; i < b->size; i++) {
        uint8_t count = get_bits_set_count(counts, b->bits[i]);
        total += count;
        row_total += count;
        #if DEBUG_BLOOM_FILTER > 1
        char c = (count == 0 ? '.' : '0' + count);
        printf("%c", c);
        if ((i & 63) == 0 || i == b->size - 1) {
            printf(" - %2.1f%%\n",
                (100 * row_total) / (64.0 * 8));
            row_total = 0;
        }
        #endif
    }

    #if DEBUG_BLOOM_FILTER
    printf(" -- bloom filter: %zd of %zd bits set (%2d%%)\n",
        total, 8*b->size, (int)((100.0 * total)/(8.0*b->size)));
    #endif

    /* If the total number of bits set is > the number of bytes
     * in the table (i.e., > 1/8 full) then warn the user. */
    if (total > b->size) {
        fprintf(stderr, "\nWARNING: bloom filter is %zd%% full, "
            "larger bloom_bits value recommended.\n",
            (size_t)((100 * total) / (8 * b->size)));
    }
}

/* Recommend a bloom filter size for a given number of trials. */
uint8_t theft_bloom_recommendation(int trials) {
    /* With a preferred priority of false positives under 0.1%,
     * the required number of bits m in the bloom filter is:
     *     m = -lg(0.001)/(lg(2)^2) == 14.378 ~= 14,
     * so we want an array with 1 << ceil(log2(14*trials)) cells.
     *
     * Note: The above formula is for the *ideal* number of hash
     * functions, but we're using a hardcoded count. It appears to work
     * well enough in practice, though, and this can be adjusted later
     * without impacting the API. */
    #define TRIAL_MULTIPILER 14
    uint8_t res = DEFAULT_BLOOM_BITS;

    const uint8_t min = THEFT_BLOOM_BITS_MIN - LOG2_BITS_PER_BYTE;
    const uint8_t max = THEFT_BLOOM_BITS_MAX - LOG2_BITS_PER_BYTE;

    for (uint8_t i = min; i < max; i++) {
        int32_t v = (1 << i);
        if (v > (TRIAL_MULTIPILER * trials)) {
            res = i + LOG2_BITS_PER_BYTE;
            break;
        }
    }

    #if DEBUG_BLOOM_FILTER
    size_t sz = 1 << (res - LOG2_BITS_PER_BYTE);
    printf("Using %zd bytes for bloom filter: %d trials -> bit_size2 %u\n",
        sizeof(struct theft_bloom) + sz, trials, res);
    #endif

    return res;
}

/* Check a byte->bits set table, and lazily populate it. */
static uint8_t get_bits_set_count(uint8_t counts[256], uint8_t byte) {
    uint8_t v = counts[byte];
    if (v != 0xFF) { return v; }
    uint8_t t = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (byte & (1 << i)) { t++; }
    }
    counts[byte] = t;
    return t;
}
