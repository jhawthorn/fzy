#ifndef THEFT_BLOOM_H
#define THEFT_BLOOM_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct theft_bloom {
    uint8_t bit_count;
    size_t size;
    uint8_t bits[];
};

/* Initialize a bloom filter. */
struct theft_bloom *theft_bloom_init(uint8_t bit_size2);

/* Hash data and mark it in the bloom filter. */
void theft_bloom_mark(struct theft_bloom *b, uint8_t *data, size_t data_size);

/* Check whether the data's hash is in the bloom filter. */
bool theft_bloom_check(struct theft_bloom *b, uint8_t *data, size_t data_size);

/* Free the bloom filter. */
void theft_bloom_free(struct theft_bloom *b);

/* Dump the bloom filter's contents. (Debugging.) */
void theft_bloom_dump(struct theft_bloom *b);

/* Recommend a bloom filter size for a given number of trials. */
uint8_t theft_bloom_recommendation(int trials);


#endif
