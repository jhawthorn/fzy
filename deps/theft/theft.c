#include <string.h>

#include "theft.h"
#include "theft_types_internal.h"
#include "theft_bloom.h"
#include "theft_mt.h"

/* Initialize a theft test runner.
 * BLOOM_BITS sets the size of the table used for detecting
 * combinations of arguments that have already been tested.
 * If 0, a default size will be chosen based on trial count.
 * (This will only be used if all property types have hash
 * callbacks defined.) The bloom filter can also be disabled
 * by setting BLOOM_BITS to THEFT_BLOOM_DISABLE.
 * 
 * Returns a NULL if malloc fails or BLOOM_BITS is out of bounds. */
struct theft *theft_init(uint8_t bloom_bits) {
    if ((bloom_bits != 0 && (bloom_bits < THEFT_BLOOM_BITS_MIN))
        || ((bloom_bits > THEFT_BLOOM_BITS_MAX) &&
            bloom_bits != THEFT_BLOOM_DISABLE)) {
        return NULL;
    }

    theft *t = malloc(sizeof(*t));
    if (t == NULL) { return NULL; }
    memset(t, 0, sizeof(*t));

    t->mt = theft_mt_init(DEFAULT_THEFT_SEED);
    if (t->mt == NULL) {
        free(t);
        return NULL;
    } else {
        t->out = stdout;
        t->requested_bloom_bits = bloom_bits;
        return t;
    }
}

/* (Re-)initialize the random number generator with a specific seed. */
void theft_set_seed(struct theft *t, uint64_t seed) {
    t->seed = seed;
    theft_mt_reset(t->mt, seed);
}

/* Get a random 64-bit integer from the test runner's PRNG. */
theft_seed theft_random(struct theft *t) {
    theft_seed ns = (theft_seed)theft_mt_random(t->mt);
    return ns;
}

/* Get a random double from the test runner's PRNG. */
double theft_random_double(struct theft *t) {
    return theft_mt_random_double(t->mt);
}

/* Change T's output stream handle to OUT. (Default: stdout.) */
void theft_set_output_stream(struct theft *t, FILE *out) { t->out = out; }

/* Check if all argument info structs have all required callbacks. */
static bool
check_all_args(struct theft_propfun_info *info, bool *all_hashable) {
    bool ah = true;
    for (int i = 0; i < info->arity; i++) {
        struct theft_type_info *ti = info->type_info[i];
        if (ti->alloc == NULL) { return false; }
        if (ti->hash == NULL) { ah = false; }
    }
    *all_hashable = ah;
    return true;
}

static theft_progress_callback_res
default_progress_cb(struct theft_trial_info *info, void *env) {
    (void)info;
    (void)env;
    return THEFT_PROGRESS_CONTINUE;
}

static void infer_arity(struct theft_propfun_info *info) {
    for (int i = 0; i < THEFT_MAX_ARITY; i++) {
        if (info->type_info[i] == NULL) {
            info->arity = i;
            break;
        }
    }
}

/* Run a series of randomized trials of a property function.
 *
 * Configuration is specified in CFG; many fields are optional.
 * See the type definition in `theft_types.h`. */
theft_run_res
theft_run(struct theft *t, struct theft_cfg *cfg) {
    if (t == NULL || cfg == NULL) {
        return THEFT_RUN_ERROR_BAD_ARGS;
    }

    struct theft_propfun_info info;
    memset(&info, 0, sizeof(info));
    info.name = cfg->name;
    info.fun = cfg->fun;
    memcpy(info.type_info, cfg->type_info, sizeof(info.type_info));
    info.always_seed_count = cfg->always_seed_count;
    info.always_seeds = cfg->always_seeds;

    if (cfg->seed) {
        theft_set_seed(t, cfg->seed);
    } else {
        theft_set_seed(t, DEFAULT_THEFT_SEED);
    }

    if (cfg->trials == 0) { cfg->trials = THEFT_DEF_TRIALS; }

    return theft_run_internal(t, &info, cfg->trials, cfg->progress_cb,
        cfg->env, cfg->report);
}

/* Actually run the trials, with all arguments made explicit. */
static theft_run_res
theft_run_internal(struct theft *t, struct theft_propfun_info *info,
        int trials, theft_progress_cb *cb, void *env,
        struct theft_trial_report *r) {

    struct theft_trial_report fake_report;
    if (r == NULL) { r = &fake_report; }
    memset(r, 0, sizeof(*r));
    
    infer_arity(info);
    if (info->arity == 0) {
        return THEFT_RUN_ERROR_BAD_ARGS;
    }

    if (t == NULL || info == NULL || info->fun == NULL
        || info->arity == 0) {
        return THEFT_RUN_ERROR_BAD_ARGS;
    }
    
    bool all_hashable = false;
    if (!check_all_args(info, &all_hashable)) {
        return THEFT_RUN_ERROR_MISSING_CALLBACK;
    }

    if (cb == NULL) { cb = default_progress_cb; }

    /* If all arguments are hashable, then attempt to use
     * a bloom filter to avoid redundant checking. */
    if (all_hashable) {
        if (t->requested_bloom_bits == 0) {
            t->requested_bloom_bits = theft_bloom_recommendation(trials);
        }
        if (t->requested_bloom_bits != THEFT_BLOOM_DISABLE) {
            t->bloom = theft_bloom_init(t->requested_bloom_bits);
        }
    }
    
    theft_seed seed = t->seed;
    theft_seed initial_seed = t->seed;
    int always_seeds = info->always_seed_count;
    if (info->always_seeds == NULL) { always_seeds = 0; }

    void *args[THEFT_MAX_ARITY];
    
    theft_progress_callback_res cres = THEFT_PROGRESS_CONTINUE;

    for (int trial = 0; trial < trials; trial++) {
        memset(args, 0xFF, sizeof(args));
        if (cres == THEFT_PROGRESS_HALT) { break; }

        /* If any seeds to always run were specified, use those before
         * reverting to the specified starting seed. */
        if (trial < always_seeds) {
            seed = info->always_seeds[trial];
        } else if ((always_seeds > 0) && (trial == always_seeds)) {
            seed = initial_seed;
        }

        struct theft_trial_info ti = {
            .name = info->name,
            .trial = trial,
            .seed = seed,
            .arity = info->arity,
            .args = args
        };

        theft_set_seed(t, seed);
        all_gen_res_t gres = gen_all_args(t, info, seed, args, env);
        switch (gres) {
        case ALL_GEN_SKIP:
            /* skip generating these args */
            ti.status = THEFT_TRIAL_SKIP;
            r->skip++;
            cres = cb(&ti, env);
            break;
        case ALL_GEN_DUP:
            /* skip these args -- probably already tried */
            ti.status = THEFT_TRIAL_DUP;
            r->dup++;
            cres = cb(&ti, env);
            break;
        default:
        case ALL_GEN_ERROR:
            /* Error while generating args */
            ti.status = THEFT_TRIAL_ERROR;
            cres = cb(&ti, env);
            return THEFT_RUN_ERROR;
        case ALL_GEN_OK:
            /* (Extracted function to avoid deep nesting here.) */
            if (!run_trial(t, info, args, cb, env, r, &ti, &cres)) {
                return THEFT_RUN_ERROR;
            }
        }

        free_args(info, args, env);

        /* Restore last known seed and generate next. */
        theft_set_seed(t, seed);
        seed = theft_random(t);
    }

    if (r->fail > 0) {
        return THEFT_RUN_FAIL;
    } else {
        return THEFT_RUN_PASS;
    }
}

/* Now that arguments have been generated, run the trial and update
 * counters, call cb with results, etc. */
static bool run_trial(struct theft *t, struct theft_propfun_info *info,
        void **args, theft_progress_cb *cb, void *env,
        struct theft_trial_report *r, struct theft_trial_info *ti,
        theft_progress_callback_res *cres) {
    if (t->bloom) { mark_called(t, info, args, env); }
    theft_trial_res tres = call_fun(info, args);
    ti->status = tres;
    switch (tres) {
    case THEFT_TRIAL_PASS:
        r->pass++;
        *cres = cb(ti, env);
        break;
    case THEFT_TRIAL_FAIL:
        if (!attempt_to_shrink(t, info, args, env)) {
            ti->status = THEFT_TRIAL_ERROR;
            *cres = cb(ti, env);
            return false;
        }
        r->fail++;
        *cres = report_on_failure(t, info, ti, cb, env);
        break;
    case THEFT_TRIAL_SKIP:
        *cres = cb(ti, env);
        r->skip++;
        break;
    case THEFT_TRIAL_DUP:
        /* user callback should not return this; fall through */
    case THEFT_TRIAL_ERROR:
        *cres = cb(ti, env);
        free_args(info, args, env);
        return false;
    }
    return true;
}

static void free_args(struct theft_propfun_info *info,
        void **args, void *env) {
    for (int i = 0; i < info->arity; i++) {
        theft_free_cb *fcb = info->type_info[i]->free;
        if (fcb && args[i] != THEFT_SKIP) {
            fcb(args[i], env);
        }
    }
}

void theft_free(struct theft *t) {
    if (t->bloom) {
        theft_bloom_dump(t->bloom);
        theft_bloom_free(t->bloom);
        t->bloom = NULL;
    }
    theft_mt_free(t->mt);
    free(t);
}

/* Actually call the property function. Its number of arguments is not
 * constrained by the typedef, but will be defined at the call site
 * here. (If info->arity is wrong, it will probably crash.) */
static theft_trial_res
call_fun(struct theft_propfun_info *info, void **args) {
    theft_trial_res res = THEFT_TRIAL_ERROR;
    switch (info->arity) {
    case 1:
        res = info->fun(args[0]);
        break;
    case 2:
        res = info->fun(args[0], args[1]);
        break;
    case 3:
        res = info->fun(args[0], args[1], args[2]);
        break;
    case 4:
        res = info->fun(args[0], args[1], args[2], args[3]);
        break;
    case 5:
        res = info->fun(args[0], args[1], args[2], args[3], args[4]);
        break;
    case 6:
        res = info->fun(args[0], args[1], args[2], args[3], args[4],
            args[5]);
        break;
    case 7:
        res = info->fun(args[0], args[1], args[2], args[3], args[4],
            args[5], args[6]);
        break;
    case 8:
        res = info->fun(args[0], args[1], args[2], args[3], args[4],
            args[5], args[6], args[7]);
        break;
    case 9:
        res = info->fun(args[0], args[1], args[2], args[3], args[4],
            args[5], args[6], args[7], args[8]);
        break;
    case 10:
        res = info->fun(args[0], args[1], args[2], args[3], args[4],
            args[5], args[6], args[7], args[8], args[9]);
        break;
    /* ... */
    default:
        return THEFT_TRIAL_ERROR;
    }
    return res;
}

/* Attempt to instantiate arguments, starting with the current seed. */
static all_gen_res_t
gen_all_args(theft *t, struct theft_propfun_info *info,
        theft_seed seed, void *args[THEFT_MAX_ARITY], void *env) {
    for (int i = 0; i < info->arity; i++) {
        struct theft_type_info *ti = info->type_info[i];
        void *p = ti->alloc(t, seed, env);
        if (p == THEFT_SKIP || p == THEFT_ERROR) {
            for (int j = 0; j < i; j++) {
                ti->free(args[j], env);
            }
            if (p == THEFT_SKIP) {
                return ALL_GEN_SKIP;
            } else {
                return ALL_GEN_ERROR;
            }
        } else {
            args[i] = p;
        }
        seed = theft_random(t);
    }

    /* check bloom filter */
    if (t->bloom && check_called(t, info, args, env)) {
        return ALL_GEN_DUP;
    }
    
    return ALL_GEN_OK;
}

/* Attempt to simplify all arguments, breadth first. Continue as long as
 * progress is made, i.e., until a local minima is reached. */
static bool
attempt_to_shrink(theft *t, struct theft_propfun_info *info,
        void *args[], void *env) {
    bool progress = false;
    do {
        progress = false;
        for (int ai = 0; ai < info->arity; ai++) {
            struct theft_type_info *ti = info->type_info[ai];
            if (ti->shrink) {
                /* attempt to simplify this argument by one step */
                shrink_res rres = attempt_to_shrink_arg(t, info, args, env, ai);
                switch (rres) {
                case SHRINK_OK:
                    progress = true;
                    break;
                case SHRINK_DEAD_END:
                    break;
                default:
                case SHRINK_ERROR:
                    return false;
                }
            }
        }
    } while (progress);

    return true;
}

/* Simplify an argument by trying all of its simplification tactics, in
 * order, and checking whether the property still fails. If it passes,
 * then revert the simplification and try another tactic.
 *
 * If the bloom filter is being used (i.e., if all arguments have hash
 * callbacks defined), then use it to skip over areas of the state
 * space that have probably already been tried. */
static shrink_res
attempt_to_shrink_arg(theft *t, struct theft_propfun_info *info,
        void *args[], void *env, int ai) {
    struct theft_type_info *ti = info->type_info[ai];

    for (uint32_t tactic = 0; tactic < THEFT_MAX_TACTICS; tactic++) {
        void *cur = args[ai];
        void *nv = ti->shrink(cur, tactic, env);
        if (nv == THEFT_NO_MORE_TACTICS) {
            return SHRINK_DEAD_END;
        } else if (nv == THEFT_ERROR) {
            return SHRINK_ERROR;
        } else if (nv == THEFT_DEAD_END) {
            continue;   /* try next tactic */
        }
        
        args[ai] = nv;
        if (t->bloom) {
            if (check_called(t, info, args, env)) {
                /* probably redundant */
                if (ti->free) { ti->free(nv, env); }
                args[ai] = cur;
                continue;
            } else {
                mark_called(t, info, args, env);
            }
        }
        theft_trial_res res = call_fun(info, args);
        
        switch (res) {
        case THEFT_TRIAL_PASS:
        case THEFT_TRIAL_SKIP:
            /* revert */
            args[ai] = cur;
            if (ti->free) { ti->free(nv, env); }
            break;
        case THEFT_TRIAL_FAIL:
            if (ti->free) { ti->free(cur, env); }
            return SHRINK_OK;
        case THEFT_TRIAL_DUP:  /* user callback should not return this */
        case THEFT_TRIAL_ERROR:
            return SHRINK_ERROR;
        }
    }
    (void)t;
    return SHRINK_DEAD_END;
}

/* Populate a buffer with hashes of all the arguments. */
static void get_arg_hash_buffer(theft_hash *buffer,
        struct theft_propfun_info *info, void **args, void *env) {
    for (int i = 0; i < info->arity; i++) {
        buffer[i] = info->type_info[i]->hash(args[i], env);
    }    
}

/* Mark the tuple of argument instances as called in the bloom filter. */
static void mark_called(theft *t, struct theft_propfun_info *info,
        void **args, void *env) {
    theft_hash buffer[THEFT_MAX_ARITY];
    get_arg_hash_buffer(buffer, info, args, env);
    theft_bloom_mark(t->bloom, (uint8_t *)buffer,
        info->arity * sizeof(theft_hash));
}

/* Check if this combination of argument instances has been called. */
static bool check_called(theft *t, struct theft_propfun_info *info,
        void **args, void *env) {
    theft_hash buffer[THEFT_MAX_ARITY];
    get_arg_hash_buffer(buffer, info, args, env);
    return theft_bloom_check(t->bloom, (uint8_t *)buffer,
        info->arity * sizeof(theft_hash));
}

/* Print info about a failure. */
static theft_progress_callback_res report_on_failure(theft *t,
        struct theft_propfun_info *info,
        struct theft_trial_info *ti, theft_progress_cb *cb, void *env) {
    static theft_progress_callback_res cres;

    int arity = info->arity;
    fprintf(t->out, "\n\n -- Counter-Example: %s\n",
        info->name ? info-> name : "");
    fprintf(t->out, "    Trial %u, Seed 0x%016llx\n", ti->trial,
        (uint64_t)ti->seed);
    for (int i = 0; i < arity; i++) {
        theft_print_cb *print = info->type_info[i]->print;
        if (print) {
            fprintf(t->out, "    Argument %d:\n", i);
            print(t->out, ti->args[i], env);
            fprintf(t->out, "\n");
        }
   }

    cres = cb(ti, env);
    return cres;
}
