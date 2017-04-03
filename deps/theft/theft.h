#ifndef THEFT_H
#define THEFT_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Version 0.2.0 */
#define THEFT_VERSION_MAJOR 0
#define THEFT_VERSION_MINOR 2
#define THEFT_VERSION_PATCH 0

/* A property can have at most this many arguments. */
#define THEFT_MAX_ARITY 10

#include "theft_types.h"

/* Default number of trials to run. */
#define THEFT_DEF_TRIALS 100

/* Min and max bits used to determine bloom filter size.
 * (A larger value uses more memory, but reduces the odds of an
 * untested argument combination being falsely skipped.) */
#define THEFT_BLOOM_BITS_MIN 13 /* 1 KB */
#define THEFT_BLOOM_BITS_MAX 33 /* 1 GB */

/* Initialize a theft test runner.
 * BLOOM_BITS sets the size of the table used for detecting
 * combinations of arguments that have already been tested.
 * If 0, a default size will be chosen based on trial count.
 * (This will only be used if all property types have hash
 * callbacks defined.) The bloom filter can also be disabled
 * by setting BLOOM_BITS to THEFT_BLOOM_DISABLE.
 * 
 * Returns a NULL if malloc fails or BLOOM_BITS is out of bounds. */
struct theft *theft_init(uint8_t bloom_bits);

/* Free a property-test runner. */
void theft_free(struct theft *t);

/* (Re-)initialize the random number generator with a specific seed. */
void theft_set_seed(struct theft *t, uint64_t seed);

/* Get a random 64-bit integer from the test runner's PRNG. */
theft_hash theft_random(struct theft *t);

/* Get a random double from the test runner's PRNG. */
double theft_random_double(struct theft *t);

/* Change T's output stream handle to OUT. (Default: stdout.) */
void theft_set_output_stream(struct theft *t, FILE *out);

/* Run a series of randomized trials of a property function.
 *
 * Configuration is specified in CFG; many fields are optional.
 * See the type definition in `theft_types.h`. */
theft_run_res
theft_run(struct theft *t, struct theft_cfg *cfg);

/* Hash a buffer in one pass. (Wraps the below functions.) */
theft_hash theft_hash_onepass(uint8_t *data, size_t bytes);

/* Init/reset a hasher for incremental hashing.
 * Returns true, or false if you gave it a NULL pointer. */
void theft_hash_init(struct theft_hasher *h);

/* Sink more data into an incremental hash. */
void theft_hash_sink(struct theft_hasher *h, uint8_t *data, size_t bytes);

/* Finish hashing and get the result. */
theft_hash theft_hash_done(struct theft_hasher *h);

#endif
