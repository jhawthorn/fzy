#ifndef THEFT_TYPES_H
#define THEFT_TYPES_H

/* A pseudo-random number/seed, used to generate instances. */
typedef uint64_t theft_seed;

/* A hash of an instance. */
typedef uint64_t theft_hash;

/* These are opaque, as far as the API is concerned. */
struct theft_bloom;             /* bloom filter */
struct theft_mt;                /* mersenne twister PRNG */

/* Struct for property-testing state. */
struct theft {
    FILE *out;
    theft_seed seed;
    uint8_t requested_bloom_bits;
    struct theft_bloom *bloom;  /* bloom filter */
    struct theft_mt *mt;        /* random number generator */
};

/* Special sentinel values returned instead of instance pointers. */
#define THEFT_SKIP ((void *)-1)
#define THEFT_ERROR ((void *)-2)
#define THEFT_DEAD_END ((void *)-1)
#define THEFT_NO_MORE_TACTICS ((void *)-3)

/* Explicitly disable using the bloom filter.
 * Note that if you do this, you must be sure your simplify function
 * *always* returns a simpler value, or it will loop forever. */
#define THEFT_BLOOM_DISABLE ((uint8_t)-1)

/* Allocate and return an instance of the type, based on a known
 * pseudo-random number seed. To get additional seeds, use
 * theft_random(t); this stream of numbers will be deterministic, so if
 * the alloc callback is constructed appropriately, an identical
 * instance can be constructed later from the same initial seed.
 * 
 * Returns a pointer to the instance, THEFT_ERROR, or THEFT_SKIP. */
typedef void *(theft_alloc_cb)(struct theft *t, theft_seed seed, void *env);

/* Free an instance. */
typedef void (theft_free_cb)(void *instance, void *env);

/* Hash an instance. Used to skip combinations of arguments which
 * have probably already been checked. */
typedef theft_hash (theft_hash_cb)(void *instance, void *env);

/* Attempt to shrink an instance to a simpler instance.
 * 
 * For a given INSTANCE, there are likely to be multiple ways in which
 * it can be simplified. For example, a list of unsigned ints could have
 * the first element decremented, divided by 2, or dropped. This
 * callback should return a pointer to a freshly allocated, simplified
 * instance, or should return THEFT_DEAD_END to indicate that the
 * instance cannot be simplified further by this method.
 *
 * These tactics will be lazily explored breadth-first, to
 * try to find simpler versions of arguments that cause the
 * property to no longer hold.
 *
 * If this callback is NULL, it is equivalent to always returning
 * THEFT_NO_MORE_TACTICS. */
typedef void *(theft_shrink_cb)(void *instance, uint32_t tactic, void *env);

/* Print INSTANCE to output stream F.
 * Used for displaying counter-examples. Can be NULL. */
typedef void (theft_print_cb)(FILE *f, void *instance, void *env);

/* Result from a single trial. */
typedef enum {
    THEFT_TRIAL_PASS,           /* property held */
    THEFT_TRIAL_FAIL,           /* property contradicted */
    THEFT_TRIAL_SKIP,           /* user requested skip; N/A */
    THEFT_TRIAL_DUP,            /* args probably already tried */
    THEFT_TRIAL_ERROR,          /* unrecoverable error, halt */
} theft_trial_res;

/* A test property function. Arguments must match the types specified by
 * theft_cfg.type_info, or the result will be undefined. For example, a
 * propfun `prop_foo(A x, B y, C z)` must have a type_info array of
 * `{ info_A, info_B, info_C }`.
 * 
 * Should return:
 *     THEFT_TRIAL_PASS if the property holds,
 *     THEFT_TRIAL_FAIL if a counter-example is found,
 *     THEFT_TRIAL_SKIP if the combination of args isn't applicable,
 *  or THEFT_TRIAL_ERROR if the whole run should be halted. */
typedef theft_trial_res (theft_propfun)( /* arguments unconstrained */ );

/* Callbacks used for testing with random instances of a type.
 * For more information, see comments on their typedefs. */
struct theft_type_info {
    /* Required: */
    theft_alloc_cb *alloc;      /* gen random instance from seed */

    /* Optional, but recommended: */
    theft_free_cb *free;        /* free instance */
    theft_hash_cb *hash;        /* instance -> hash */
    theft_shrink_cb *shrink;    /* shrink instance */
    theft_print_cb *print;      /* fprintf instance */
};

/* Result from an individual trial. */
struct theft_trial_info {
    const char *name;           /* property name */
    int trial;                  /* N'th trial */
    theft_seed seed;            /* Seed used */
    theft_trial_res status;     /* Run status */
    uint8_t arity;              /* Number of arguments */
    void **args;                /* Arguments used */
};

/* Whether to keep running trials after N failures/skips/etc. */
typedef enum {
    THEFT_PROGRESS_CONTINUE,    /* keep running trials */
    THEFT_PROGRESS_HALT,        /* no need to continue */
} theft_progress_callback_res;

/* Handle test results.
 * Can be used to halt after too many failures, print '.' after
 * every N trials, etc. */
typedef theft_progress_callback_res
(theft_progress_cb)(struct theft_trial_info *info, void *env);

/* Result from a trial run. */
typedef enum {
    THEFT_RUN_PASS = 0,             /* no failures */
    THEFT_RUN_FAIL = 1,             /* 1 or more failures */
    THEFT_RUN_ERROR = 2,            /* an error occurred */
    THEFT_RUN_ERROR_BAD_ARGS = -1,  /* API misuse */
    /* Missing required callback for 1 or more types */
    THEFT_RUN_ERROR_MISSING_CALLBACK = -2,
} theft_run_res;

/* Optional report from a trial run; same meanings as theft_trial_res. */
struct theft_trial_report {
    size_t pass;
    size_t fail;
    size_t skip;
    size_t dup;
};

/* Configuration struct for a theft test.
 * In C99, this struct can be specified as a literal, like this:
 * 
 *     struct theft_cfg cfg = {
 *         .name = "example",
 *         .fun = prop_fun,
 *         .type_info = { type_arg_a, type_arg_b },
 *         .seed = 0x7he5eed,
 *     };
 *
 * and omitted fields will be set to defaults.
 * */
struct theft_cfg {
    /* Property function under test, and info about its arguments.
     * The function is called with as many arguments are there
     * are values in TYPE_INFO, so it can crash if that is wrong. */
    theft_propfun *fun;
    struct theft_type_info *type_info[THEFT_MAX_ARITY];

    /* -- All fields after this point are optional. -- */

    /* Property name, displayed in test runner output. */
    const char *name;

    /* Array of seeds to always run, and its length.
     * Can be used for regression tests. */
    int always_seed_count;      /* number of seeds */
    theft_seed *always_seeds;   /* seeds to always run */

    /* Number of trials to run. Defaults to THEFT_DEF_TRIALS. */
    int trials;

    /* Progress callback, used to display progress in
     * slow-running tests, halt early under certain conditions, etc. */
    theft_progress_cb *progress_cb;

    /* Environment pointer. This is completely opaque to theft itself,
     * but will be passed along to all callbacks. */
    void *env;

    /* Struct to populate with more detailed test results. */
    struct theft_trial_report *report;

    /* Seed for the random number generator. */
    theft_seed seed;
};

/* Internal state for incremental hashing. */
struct theft_hasher {
    theft_hash accum;
};

#endif
