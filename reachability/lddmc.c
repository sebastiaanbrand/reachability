#include <argp.h>
#include <inttypes.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <libgen.h>

#ifdef HAVE_PROFILER
#include <gperftools/profiler.h>
#endif

#include <sylvan_int.h>

#include "getrss.h"
#include "cache_op_ids.h"

/* Configuration (via argp) */
static int report_levels = 0; // report states at start of every level
static int report_table = 0; // report table size at end of every level
static int report_nodes = 0; // report number of nodes of LDDs
static int strategy = 0; // 0 = BFS, 1 = PAR, 2 = SAT, 3 = CHAINING, 4 = REC
static int check_deadlocks = 0; // set to 1 to check for deadlocks on-the-fly
static int merge_relations = 0; // merge relations to 1 relation
static int print_transition_matrix = 0; // print transition relation matrix
static int workers = 0; // autodetect
static char* model_filename = NULL; // filename of model
static char* out_filename = NULL; // filename of output
static char* stats_filename = NULL; // filename of csv stats output file
#ifdef HAVE_PROFILER
static char* profile_filename = NULL; // filename for profiling
#endif

typedef enum strats {
    strat_bfs,
    strat_par,
    strat_sat,
    strat_chaining,
    strat_rec,
    num_strats
} strategy_t;

/* argp configuration */
static struct argp_option options[] =
{
    {"workers", 'w', "<workers>", 0, "Number of workers (default=0: autodetect)", 0},
    {"strategy", 's', "<bfs|par|sat|chaining|rec>", 0, "Strategy for reachability (default=bfs)", 0},
#ifdef HAVE_PROFILER
    {"profiler", 'p', "<filename>", 0, "Filename for profiling", 0},
#endif
    {"deadlocks", 3, 0, 0, "Check for deadlocks", 1},
    {"count-nodes", 5, 0, 0, "Report #nodes for LDDs", 1},
    {"count-states", 1, 0, 0, "Report #states at each level", 1},
    {"count-table", 2, 0, 0, "Report table usage at each level", 1},
    {"merge-relations", 6, 0, 0, "Merge transition relations into one transition relation", 1},
    {"print-matrix", 4, 0, 0, "Print transition matrix", 1},
    {"statsfile", 7, "FILENAME", 0, "Write stats to given filename (or append if exists)", 0},
    {0, 0, 0, 0, 0, 0}
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
    switch (key) {
    case 'w':
        workers = atoi(arg);
        break;
    case 's':
        if (strcmp(arg, "bfs")==0) strategy = strat_bfs;
        else if (strcmp(arg, "par")==0) strategy = strat_par;
        else if (strcmp(arg, "sat")==0) strategy = strat_sat;
        else if (strcmp(arg, "chaining")==0) strategy = strat_chaining;
        else if (strcmp(arg, "rec")==0) strategy = strat_rec;
        else argp_usage(state);
        break;
    case 4:
        print_transition_matrix = 1;
        break;
    case 3:
        check_deadlocks = 1;
        break;
    case 1:
        report_levels = 1;
        break;
    case 2:
        report_table = 1;
        break;
    case 5:
        report_nodes = 1;
        break;
    case 6:
        merge_relations = 1;
        break;
    case 7:
        stats_filename = arg;
        break;
#ifdef HAVE_PROFILER
    case 'p':
        profile_filename = arg;
        break;
#endif
    case ARGP_KEY_ARG:
        if (state->arg_num == 0) model_filename = arg;
        if (state->arg_num == 1) out_filename = arg;
        if (state->arg_num >= 2) argp_usage(state);
        break; 
    case ARGP_KEY_END:
        if (state->arg_num < 1) argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, "<model> [<output-bdd>]", 0, 0, 0, 0 };

/**
 * Types (set and relation)
 */
typedef struct set
{
    MDD dd;
} *set_t;

typedef struct relation
{
    MDD dd;
    MDD meta; // for relprod
    int r_k, w_k, *r_proj, *w_proj;
    int firstvar; // for saturation/chaining
    MDD topmeta; // for saturation
} *rel_t;

static int vector_size; // size of vector in integers
static int next_count; // number of partitions of the transition relation
static rel_t *next; // each partition of the transition relation

typedef struct stats {
    double reach_time;
    double merge_rel_time;
    double final_states;
    size_t final_nodecount;
    size_t peaknodes;
} stats_t;
stats_t stats = {0};

/**
 * Obtain current wallclock time
 */
static double
wctime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

static double t_start;
#define INFO(s, ...) fprintf(stdout, "[% 8.2f] " s, wctime()-t_start, ##__VA_ARGS__)
#define Abort(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "Abort at line %d!\n", __LINE__); exit(-1); }

/**
 * Load a set from file
 */
static set_t
set_load(FILE* f)
{
    set_t set = (set_t)malloc(sizeof(struct set));

    /* read projection (actually we don't support projection) */
    int k;
    if (fread(&k, sizeof(int), 1, f) != 1) Abort("Invalid input file!\n");
    if (k != -1) Abort("Invalid input file!\n"); // only support full vector

    /* read dd */
    lddmc_serialize_fromfile(f);
    size_t dd;
    if (fread(&dd, sizeof(size_t), 1, f) != 1) Abort("Invalid input file!\n");
    set->dd = lddmc_serialize_get_reversed(dd);
    lddmc_protect(&set->dd);

    return set;
}

/**
 * Save a set to file
 */
static void
set_save(FILE* f, set_t set)
{
    int k = -1;
    fwrite(&k, sizeof(int), 1, f);
    size_t dd = lddmc_serialize_add(set->dd);
    lddmc_serialize_tofile(f);
    fwrite(&dd, sizeof(size_t), 1, f);
}

/**
 * Load a relation from file
 */
#define rel_load_proj(f) RUN(rel_load_proj, f)
TASK_1(rel_t, rel_load_proj, FILE*, f)
{
    int r_k, w_k;
    if (fread(&r_k, sizeof(int), 1, f) != 1) Abort("Invalid file format.");
    if (fread(&w_k, sizeof(int), 1, f) != 1) Abort("Invalid file format.");

    rel_t rel = (rel_t)malloc(sizeof(struct relation));
    rel->r_k = r_k;
    rel->w_k = w_k;
    rel->r_proj = (int*)malloc(sizeof(int[rel->r_k]));
    rel->w_proj = (int*)malloc(sizeof(int[rel->w_k]));

    if (fread(rel->r_proj, sizeof(int), rel->r_k, f) != (size_t)rel->r_k) Abort("Invalid file format.");
    if (fread(rel->w_proj, sizeof(int), rel->w_k, f) != (size_t)rel->w_k) Abort("Invalid file format.");

    int *r_proj = rel->r_proj;
    int *w_proj = rel->w_proj;

    rel->firstvar = -1;

    /* Compute the meta */
    uint32_t meta[vector_size*2+2];
    memset(meta, 0, sizeof(uint32_t[vector_size*2+2]));
    int r_i=0, w_i=0, i=0, j=0;
    for (;;) {
        int type = 0;
        if (r_i < r_k && r_proj[r_i] == i) {
            r_i++;
            type += 1; // read
        }
        if (w_i < w_k && w_proj[w_i] == i) {
            w_i++;
            type += 2; // write
        }
        if (type == 0) meta[j++] = 0;
        else if (type == 1) { meta[j++] = 3; }
        else if (type == 2) { meta[j++] = 4; }
        else if (type == 3) { meta[j++] = 1; meta[j++] = 2; }
        if (type != 0 && rel->firstvar == -1) rel->firstvar = i;
        if (r_i == r_k && w_i == w_k) {
            meta[j++] = 5; // action label
            meta[j++] = (uint32_t)-1;
            break;
        }
        i++;
    }

    rel->meta = lddmc_cube((uint32_t*)meta, j);
    lddmc_protect(&rel->meta);
    if (rel->firstvar != -1) {
        rel->topmeta = lddmc_cube((uint32_t*)meta+rel->firstvar, j-rel->firstvar);
        lddmc_protect(&rel->topmeta);
    }
    rel->dd = lddmc_false;
    lddmc_protect(&rel->dd);

    return rel;
}

#define rel_load(f, rel) RUN(rel_load, f, rel)
VOID_TASK_2(rel_load, FILE*, f, rel_t, rel)
{
    lddmc_serialize_fromfile(f);
    size_t dd;
    if (fread(&dd, sizeof(size_t), 1, f) != 1) Abort("Invalid input file!");
    rel->dd = lddmc_serialize_get_reversed(dd);
}

/**
 * Save a relation to file
 */
static void
rel_save_proj(FILE* f, rel_t rel)
{
    fwrite(&rel->r_k, sizeof(int), 1, f);
    fwrite(&rel->w_k, sizeof(int), 1, f);
    fwrite(rel->r_proj, sizeof(int), rel->r_k, f);
    fwrite(rel->w_proj, sizeof(int), rel->w_k, f);
}

static void
rel_save(FILE* f, rel_t rel)
{
    size_t dd = lddmc_serialize_add(rel->dd);
    lddmc_serialize_tofile(f);
    fwrite(&dd, sizeof(size_t), 1, f);
}

/**
 * Clone a set
 */
static set_t
set_clone(set_t source)
{
    set_t set = (set_t)malloc(sizeof(struct set));
    set->dd = source->dd;
    lddmc_protect(&set->dd);
    return set;
}

static char*
to_h(double size, char *buf)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    for (;size>1024;size/=1024) i++;
    sprintf(buf, "%.*f %s", i, size, units[i]);
    return buf;
}

static void
print_memory_usage(void)
{
    char buf[32];
    to_h(getCurrentRSS(), buf);
    INFO("Memory usage: %s\n", buf);
}

static void
write_stats()
{
    FILE *fp = fopen(stats_filename, "a");
    // write header if file is empty
    fseek (fp, 0, SEEK_END);
        long size = ftell(fp);
        if (size == 0)
            fprintf(fp, "%s\n", "benchmark, strategy, merg_rels, workers, reach_time, merge_time, final_states, final_nodecount, peaknodes");
    // append stats of this run
    char* benchname = basename((char*)model_filename);
    fprintf(fp, "%s, %d, %d, %d, %f, %f, %0.0f, %ld, %ld\n",
            benchname,
            strategy,
            merge_relations,
            lace_workers(),
            stats.reach_time,
            stats.merge_rel_time,
            stats.final_states,
            stats.final_nodecount,
            stats.peaknodes);
    fclose(fp);
}

/**
 * Get the first variable of the transition relation
 */
static int
get_first(MDD meta)
{
    uint32_t val = lddmc_getvalue(meta);
    if (val != 0) return 0;
    return 1+get_first(lddmc_follow(meta, val));
}

/**
 * Print a single example of a set to stdout
 */
static void
print_example(MDD example)
{
    if (example != lddmc_false) {
        uint32_t vec[vector_size];
        lddmc_sat_one(example, vec, vector_size);

        printf("[");
        for (int i=0; i<vector_size; i++) {
            if (i>0) printf(",");
            printf("%" PRIu32, vec[i]);
        }
        printf("]");
    }
}

static void
print_matrix(size_t size, MDD meta)
{
    if (size == 0) return;
    uint32_t val = lddmc_getvalue(meta);
    if (val == 1) {
        printf("+");
        print_matrix(size-1, lddmc_follow(lddmc_follow(meta, 1), 2));
    } else {
        if (val == (uint32_t)-1) printf("-");
        else if (val == 0) printf("-");
        else if (val == 3) printf("r");
        else if (val == 4) printf("w");
        print_matrix(size-1, lddmc_follow(meta, val));
    }
}

/**
 * Implement parallel strategy (that performs the relprod operations in parallel)
 */
TASK_5(MDD, go_par, MDD, cur, MDD, visited, size_t, from, size_t, len, MDD*, deadlocks)
{
    if (len == 1) {
        // Calculate NEW successors (not in visited)
        MDD succ = lddmc_relprod(cur, next[from]->dd, next[from]->meta);
        lddmc_refs_push(succ);
        if (deadlocks) {
            // check which MDDs in deadlocks do not have a successor in this relation
            MDD anc = lddmc_relprev(succ, next[from]->dd, next[from]->meta, cur);
            lddmc_refs_push(anc);
            *deadlocks = lddmc_minus(*deadlocks, anc);
            lddmc_refs_pop(1);
        }
        MDD result = lddmc_minus(succ, visited);
        lddmc_refs_pop(1);
        return result;
    } else if (deadlocks != NULL) {
        MDD deadlocks_left = *deadlocks;
        MDD deadlocks_right = *deadlocks;
        lddmc_refs_pushptr(&deadlocks_left);
        lddmc_refs_pushptr(&deadlocks_right);

        // Recursively compute left+right
        lddmc_refs_spawn(SPAWN(go_par, cur, visited, from, len/2, &deadlocks_left));
        MDD right = CALL(go_par, cur, visited, from+len/2, len-len/2, &deadlocks_right);
        lddmc_refs_push(right);
        MDD left = lddmc_refs_sync(SYNC(go_par));
        lddmc_refs_push(left);

        // Merge results of left+right
        MDD result = lddmc_union(left, right);
        lddmc_refs_pop(2);

        // Intersect deadlock sets
        lddmc_refs_push(result);
        *deadlocks = lddmc_intersect(deadlocks_left, deadlocks_right);
        lddmc_refs_pop(1);
        lddmc_refs_popptr(2);

        // Return result
        return result;
    } else {
        // Recursively compute left+right
        lddmc_refs_spawn(SPAWN(go_par, cur, visited, from, len/2, NULL));
        MDD right = CALL(go_par, cur, visited, from+len/2, len-len/2, NULL);
        lddmc_refs_push(right);
        MDD left = lddmc_refs_sync(SYNC(go_par));
        lddmc_refs_push(left);

        // Merge results of left+right
        MDD result = lddmc_union(left, right);
        lddmc_refs_pop(2);

        // Return result
        return result;
    }
}

/**
 * Implementation of the PAR strategy
 */
VOID_TASK_1(par, set_t, set)
{
    /* Prepare variables */
    MDD visited = set->dd;
    MDD front = visited;
    lddmc_refs_pushptr(&visited);
    lddmc_refs_pushptr(&front);

    int iteration = 1;
    do {
        if (check_deadlocks) {
            // compute successors in parallel
            MDD deadlocks = front;
            lddmc_refs_pushptr(&deadlocks);
            front = CALL(go_par, front, visited, 0, next_count, &deadlocks);
            lddmc_refs_popptr(1);

            if (deadlocks != lddmc_false) {
                INFO("Found %'0.0f deadlock states... ", lddmc_satcount_cached(deadlocks));
                printf("example: ");
                print_example(deadlocks);
                printf("\n");
                check_deadlocks = 0;
            }
        } else {
            // compute successors in parallel
            front = CALL(go_par, front, visited, 0, next_count, NULL);
        }

        // visited = visited + front
        visited = lddmc_union(visited, front);

        INFO("Level %d done", iteration);
        if (report_levels) {
            printf(", %'0.0f states explored", lddmc_satcount_cached(visited));
        }
        if (report_table) {
            size_t filled, total;
            sylvan_table_usage(&filled, &total);
            printf(", table: %0.1f%% full (%'zu nodes)", 100.0*(double)filled/total, filled);
        }
        char buf[32];
        to_h(getCurrentRSS(), buf);
        printf(", rss=%s.\n", buf);
        iteration++;
    } while (front != lddmc_false);

    set->dd = visited;
    lddmc_refs_popptr(2);
}

/**
 * Implement sequential strategy (that performs the relprod operations one by one)
 */
TASK_5(MDD, go_bfs, MDD, cur, MDD, visited, size_t, from, size_t, len, MDD*, deadlocks)
{
    if (len == 1) {
        // Calculate NEW successors (not in visited)
        MDD succ = lddmc_relprod(cur, next[from]->dd, next[from]->meta);
        lddmc_refs_push(succ);
        if (deadlocks) {
            // check which MDDs in deadlocks do not have a successor in this relation
            MDD anc = lddmc_relprev(succ, next[from]->dd, next[from]->meta, cur);
            lddmc_refs_push(anc);
            *deadlocks = lddmc_minus(*deadlocks, anc);
            lddmc_refs_pop(1);
        }
        MDD result = lddmc_minus(succ, visited);
        lddmc_refs_pop(1);
        return result;
    } else if (deadlocks != NULL) {
        MDD deadlocks_left = *deadlocks;
        MDD deadlocks_right = *deadlocks;
        lddmc_refs_pushptr(&deadlocks_left);
        lddmc_refs_pushptr(&deadlocks_right);

        // Recursively compute left+right
        MDD left = CALL(go_par, cur, visited, from, len/2, &deadlocks_left);
        lddmc_refs_push(left);
        MDD right = CALL(go_par, cur, visited, from+len/2, len-len/2, &deadlocks_right);
        lddmc_refs_push(right);

        // Merge results of left+right
        MDD result = lddmc_union(left, right);
        lddmc_refs_pop(2);

        // Intersect deadlock sets
        lddmc_refs_push(result);
        *deadlocks = lddmc_intersect(deadlocks_left, deadlocks_right);
        lddmc_refs_pop(1);
        lddmc_refs_popptr(2);

        // Return result
        return result;
    } else {
        // Recursively compute left+right
        MDD left = CALL(go_par, cur, visited, from, len/2, NULL);
        lddmc_refs_push(left);
        MDD right = CALL(go_par, cur, visited, from+len/2, len-len/2, NULL);
        lddmc_refs_push(right);

        // Merge results of left+right
        MDD result = lddmc_union(left, right);
        lddmc_refs_pop(2);

        // Return result
        return result;
    }
}

/* BFS strategy, sequential strategy (but operations are parallelized by Sylvan) */
VOID_TASK_1(bfs, set_t, set)
{
    /* Prepare variables */
    MDD visited = set->dd;
    MDD front = visited;
    lddmc_refs_pushptr(&visited);
    lddmc_refs_pushptr(&front);

    int iteration = 1;
    do {
        if (check_deadlocks) {
            // compute successors
            MDD deadlocks = front;
            lddmc_refs_pushptr(&deadlocks);
            front = CALL(go_bfs, front, visited, 0, next_count, &deadlocks);
            lddmc_refs_popptr(1);

            if (deadlocks != lddmc_false) {
                INFO("Found %'0.0f deadlock states... ", lddmc_satcount_cached(deadlocks));
                printf("example: ");
                print_example(deadlocks);
                printf("\n");
                check_deadlocks = 0;
            }
        } else {
            // compute successors
            front = CALL(go_bfs, front, visited, 0, next_count, NULL);
        }

        // visited = visited + front
        visited = lddmc_union(visited, front);

        INFO("Level %d done", iteration);
        if (report_levels) {
            printf(", %'0.0f states explored", lddmc_satcount_cached(visited));
        }
        if (report_table) {
            size_t filled, total;
            sylvan_table_usage(&filled, &total);
            printf(", table: %0.1f%% full (%'zu nodes)", 100.0*(double)filled/total, filled);
        }
        char buf[32];
        to_h(getCurrentRSS(), buf);
        printf(", rss=%s.\n", buf);
        iteration++;
    } while (front != lddmc_false);

    set->dd = visited;
    lddmc_refs_popptr(2);
}

/**
 * Implementation of (parallel) saturation
 * (assumes relations are ordered on first variable)
 */
TASK_3(MDD, go_sat, MDD, set, int, idx, int, depth)
{
    /* Terminal cases */
    if (set == lddmc_false) return lddmc_false;
    if (idx == next_count) return set;

    /* Consult the cache */
    MDD result;
    const MDD _set = set;
    if (cache_get3(CACHE_LDD_SAT, _set, idx, 0, &result)) return result;
    lddmc_refs_pushptr(&_set);

    /**
     * Possible improvement: cache more things (like intermediate results?)
     *   and chain-apply more of the current level before going deeper?
     */

    /* Check if the relation should be applied */
    const int var = next[idx]->firstvar;
    assert(depth <= var);
    if (depth == var) {
        /* Count the number of relations starting here */
        int n = 1;
        while ((idx + n) < next_count && var == next[idx + n]->firstvar) n++;
        /*
         * Compute until fixpoint:
         * - SAT deeper
         * - chain-apply all current level once
         */
        MDD prev = lddmc_false;
        lddmc_refs_pushptr(&set);
        lddmc_refs_pushptr(&prev);
        while (prev != set) {
            prev = set;
            // SAT deeper
            set = CALL(go_sat, set, idx + n, depth);
            // chain-apply all current level once
            for (int i=0; i<n; i++) {
                set = lddmc_relprod_union(set, next[idx+i]->dd, next[idx+i]->topmeta, set);
            }
        }
        lddmc_refs_popptr(2);
        result = set;
    } else {
        /* Recursive computation */
        lddmc_refs_spawn(SPAWN(go_sat, lddmc_getright(set), idx, depth));
        MDD down = lddmc_refs_push(CALL(go_sat, lddmc_getdown(set), idx, depth+1));
        MDD right = lddmc_refs_sync(SYNC(go_sat));
        lddmc_refs_pop(1);
        result = lddmc_makenode(lddmc_getvalue(set), down, right);
    }

    /* Store in cache */
    cache_put3(CACHE_LDD_SAT, _set, idx, 0, result);
    lddmc_refs_popptr(1);
    return result;
}

/**
 * Wrapper for the Saturation strategy
 */
VOID_TASK_1(sat, set_t, set)
{
    set->dd = CALL(go_sat, set->dd, 0, 0);
}

/**
 * Implementation of the Chaining strategy (does not support deadlock detection)
 */
VOID_TASK_1(chaining, set_t, set)
{
    MDD visited = set->dd;
    MDD front = visited;
    MDD succ = sylvan_false;

    lddmc_refs_pushptr(&visited);
    lddmc_refs_pushptr(&front);
    lddmc_refs_pushptr(&succ);

    int iteration = 1;
    do {
        // calculate successors in parallel
        for (int i=0; i<next_count; i++) {
            succ = lddmc_relprod(front, next[i]->dd, next[i]->meta);
            front = lddmc_union(front, succ);
            succ = lddmc_false; // reset, for gc
        }

        // front = front - visited
        // visited = visited + front
        front = lddmc_minus(front, visited);
        visited = lddmc_union(visited, front);

        INFO("Level %d done", iteration);
        if (report_levels) {
            printf(", %'0.0f states explored", lddmc_satcount_cached(visited));
        }
        if (report_table) {
            size_t filled, total;
            sylvan_table_usage(&filled, &total);
            printf(", table: %0.1f%% full (%'zu nodes)", 100.0*(double)filled/total, filled);
        }
        char buf[32];
        to_h(getCurrentRSS(), buf);
        printf(", rss=%s.\n", buf);
        iteration++;
    } while (front != lddmc_false);

    set->dd = visited;
    lddmc_refs_popptr(3);
}

static MDD
get_next_meta(MDD meta)
{
    /* Step over a read (1) and a write (2) in the meta MDD */
    mddnode_t n;

    n = LDD_GETNODE(meta);
    assert(mddnode_getvalue(n) == 1);
    meta = mddnode_getdown(n);

    n = LDD_GETNODE(meta);
    assert(mddnode_getvalue(n) == 2);
    meta = mddnode_getdown(n);

    return meta;
}


/**
 * Implementation of recursive reachability algorithm for a single global
 * relation.
 */
TASK_3(MDD, go_rec, MDD, set, MDD, rel, MDD, meta)
{
    /* Terminal cases */
    if (set == lddmc_false) return lddmc_false; // empty.R* = empty
    if (rel == lddmc_false) return set; // s.empty* = s.(empty union I)^+ = s
    if (set == lddmc_true || rel == lddmc_true) return lddmc_true;
    // all.r* = all, s.all* = all (if s is not empty)

    /* Assert assumptions about rel */
    assert(lddmc_getvalue(meta) == 1);
    assert(!lddmc_iscopy(rel));

    /* We can skip nodes if Rel requires a "read" which is not in S */
    //if (!match_ldds(&set, &rel)) return lddmc_false;

    /* Consult cache */
    int cachenow = 1;
    if (cachenow) {
        MDD res;
        if (cache_get3(CACHE_LDD_REACH, set, rel, 0, &res)) return res;
    }
    
    /* Protect relevant MDDs */
    MDD next_meta = get_next_meta(meta); // Q: protect this?
    MDD prev      = lddmc_false;    lddmc_refs_pushptr(&prev);
    MDD itr_r     = lddmc_false;    lddmc_refs_pushptr(&itr_r);
    MDD itr_w     = lddmc_false;    lddmc_refs_pushptr(&itr_w);
    MDD rel_ij    = lddmc_false;    lddmc_refs_pushptr(&rel_ij);
    MDD set_i     = lddmc_false;    lddmc_refs_pushptr(&set_i);
    MDD _set      = set;            lddmc_refs_pushptr(&_set);

    /* Loop until reachable set has converged */
    while (_set != prev) {
        prev = _set;

        // Iterate over over all reads (i) of 'rel'
        for (itr_r = rel;  itr_r != lddmc_false; itr_r = lddmc_getright(itr_r)) {

            uint32_t i = lddmc_getvalue(itr_r);
            set_i  = lddmc_follow(_set, i);

            // Iterate over all writes (j) corresponding to reading 'i'
            for (itr_w = lddmc_getdown(itr_r); itr_w != lddmc_false; itr_w = lddmc_getright(itr_w)) {

                uint32_t j = lddmc_getvalue(itr_w);
                rel_ij = lddmc_getdown(itr_w); // equiv to following i then j from rel
                
                if (i == j) {
                    set_i = CALL(go_rec, set_i, rel_ij, next_meta);
                }
                else {
                    MDD succ_j = lddmc_relprod(set_i, rel_ij, next_meta);

                    // extend succ_j and add to 'set'
                    MDD set_j_ext = lddmc_makenode(j, succ_j, lddmc_false);
                    _set = lddmc_union(_set, set_j_ext);
                } 
            }

            // extend set_i and add to 'set'
            MDD set_i_ext = lddmc_makenode(i, set_i, lddmc_false);
            _set = lddmc_union(_set, set_i_ext);
        }
    }

    lddmc_refs_popptr(6);

    /* Put in cache */
    if (cachenow)
        cache_put3(CACHE_LDD_REACH, set, rel, 0, _set);

    return _set;
}

VOID_TASK_1(rec, set_t, set)
{
    if (next_count != 1) Abort("Strategy rec requires merge-relations");
    set->dd = CALL(go_rec, set->dd, next[0]->dd, next[0]->meta);
}

void
print_meta(MDD meta)
{
    while(meta != lddmc_true && meta != lddmc_false) {
        uint32_t val = lddmc_getvalue(meta);
        printf("%d, ", val);
        meta = lddmc_follow(meta, val);
    }
    if (meta == lddmc_true) printf("True");
    else if (meta == lddmc_false) printf("False");
    printf("\n");
}

void
assert_meta_full_domain(MDD meta)
{
    uint32_t val;
    int counter = 0;
    // for every variable we want read (1) followed by write (2)
    while (counter < vector_size) {
        // read (1)
        val = lddmc_getvalue(meta);
        assert(val == 1);
        meta = lddmc_follow(meta, val);
        // write (2)
        val = lddmc_getvalue(meta);
        assert(val == 2);
        meta = lddmc_follow(meta, val);
        counter++;
    }
    // everything else until terminal should either be 5 or -1
    while (meta != lddmc_true) {
        val = lddmc_getvalue(meta);
        assert(val == 5 || val == (uint32_t)-1);
        meta = lddmc_follow(meta, val);
        assert(meta != lddmc_false);
    }
}


#define big_union(first, count) RUN(big_union, first, count)
TASK_2(MDD, big_union, int, first, int, count)
{
    // TODO: maybe do this function in parallel like in bddmc
    MDD res = next[first]->dd;
    lddmc_protect(&res);
    for (int i = first+1; i < count; i++) {
        res = lddmc_union(res, next[i]->dd);
    }
    lddmc_unprotect(&res);
    return res;
}

VOID_TASK_0(gc_start)
{
    char buf[32];
    to_h(getCurrentRSS(), buf);
    INFO("(GC) Starting garbage collection... (rss: %s)\n", buf);
}

VOID_TASK_0(gc_end)
{
    char buf[32];
    to_h(getCurrentRSS(), buf);
    INFO("(GC) Garbage collection done.       (rss: %s)\n", buf);
}

void
print_h(double size)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    for (;size>1024;size/=1024) i++;
    printf("%.*f %s", i, size, units[i]);
}

int
main(int argc, char **argv)
{
    /**
     * Parse command line, set locale, set startup time for INFO messages.
     */
    argp_parse(&argp, argc, argv, 0, 0, 0);
    setlocale(LC_NUMERIC, "en_US.utf-8");
    t_start = wctime();


    /**
     * Initialize Lace.
     *
     * First: setup with given number of workers (0 for autodetect) and some large size task queue.
     * Second: start all worker threads with default settings.
     * Third: setup local variables using the LACE_ME macro.
     */
    lace_start(workers, 1000000);

    /**
     * Initialize Sylvan.
     *
     * First: set memory limits
     * - 2 GB memory, nodes table twice as big as cache, initial size halved 6x
     *   (that means it takes 6 garbage collections to get to the maximum nodes&cache size)
     * Second: initialize package and subpackages
     * Third: add hooks to report garbage collection
     */

    size_t max = 16LL<<30;
    if (max > getMaxMemory()) max = getMaxMemory()/10*9;
    printf("Setting Sylvan main tables memory to ");
    print_h(max);
    printf(" max.\n");

    sylvan_set_limits(max, 1, 16);
    sylvan_init_package();
    sylvan_init_ldd();
    sylvan_gc_hook_pregc(TASK(gc_start));
    sylvan_gc_hook_postgc(TASK(gc_end));

    /**
     * Read the model from file
     */

    FILE *f = fopen(model_filename, "r");
    if (f == NULL) {
        Abort("Cannot open file '%s'!\n", model_filename);
        return -1;
    }

    /* Read domain data */
    if (fread(&vector_size, sizeof(int), 1, f) != 1) Abort("Invalid input file!\n");

    /* Read initial state */
    set_t initial = set_load(f);

    /* Read number of transition relations */
    if (fread(&next_count, sizeof(int), 1, f) != 1) Abort("Invalid input file!\n");
    next = (rel_t*)malloc(sizeof(rel_t) * next_count);

    /* Read transition relations */
    for (int i=0; i<next_count; i++) next[i] = rel_load_proj(f);
    for (int i=0; i<next_count; i++) rel_load(f, next[i]);

    /* We ignore the reachable states and action labels that are stored after the relations */

    /* Close the file */
    fclose(f);

    /**
     * Pre-processing and some statistics reporting
     */

    if (strategy == strat_sat || strategy == strat_chaining) {
        // for SAT and CHAINING, sort the transition relations (gnome sort because I like gnomes)
        int i = 1, j = 2;
        rel_t t;
        while (i < next_count) {
            rel_t *p = &next[i], *q = p-1;
            if ((*q)->firstvar > (*p)->firstvar) {
                t = *q;
                *q = *p;
                *p = t;
                if (--i) continue;
            }
            i = j++;
        }
    }

    INFO("Read file '%s'\n", model_filename);
    INFO("%d integers per state, %d transition groups\n", vector_size, next_count);

    if (print_transition_matrix) {
        for (int i=0; i<next_count; i++) {
            INFO("");
            print_matrix(vector_size, next[i]->meta);
            printf(" (%d)\n", get_first(next[i]->meta));
        }
    }

    if (merge_relations) {
        double t1 = wctime();

        INFO("Asserting transition relations to cover full domain.\n");
        for (int i = 0; i < next_count; i++) {
            assert_meta_full_domain(next[i]->meta);
        }

        // Q: can we just leave the 5's and -1's when taking these unions?
        INFO("Taking union of all transition relations.\n");
        next[0]->dd = big_union(0, next_count);
        assert(next[0]->firstvar == 0);
        INFO("Rel nodes after union: %'zu\n", lddmc_nodecount(next[0]->dd));

        for (int i=1; i<next_count; i++) {
            next[i]->dd = lddmc_false;
            next[i]->meta = lddmc_true;
            next[i]->firstvar = 0;
        }
        next_count = 1;
        double t2 = wctime();
        stats.merge_rel_time = t2-t1;
    }

    set_t states = set_clone(initial);

#ifdef HAVE_PROFILER
    if (profile_filename != NULL) ProfilerStart(profile_filename);
#endif

    if (strategy == strat_bfs) {
        double t1 = wctime();
        RUN(bfs, states);
        double t2 = wctime();
        stats.reach_time = t2-t1;
        INFO("BFS Time: %f\n", stats.reach_time);
    } else if (strategy == strat_par) {
        double t1 = wctime();
        RUN(par, states);
        double t2 = wctime();
        stats.reach_time = t2-t1;
        INFO("PAR Time: %f\n", stats.reach_time);
    } else if (strategy == strat_sat) {
        double t1 = wctime();
        RUN(sat, states);
        double t2 = wctime();
        stats.reach_time = t2-t1;
        INFO("SAT Time: %f\n", stats.reach_time);
    } else if (strategy == strat_chaining) {
        double t1 = wctime();
        RUN(chaining, states);
        double t2 = wctime();
        stats.reach_time = t2-t1;
        INFO("CHAINING Time: %f\n", stats.reach_time);
    } else if (strategy == strat_rec) {
        double t1 = wctime();
        RUN(rec, states);
        double t2 = wctime();
        stats.reach_time = t2-t1;
        INFO("REC Time: %f\n", stats.reach_time);
    } else {
        Abort("Invalid strategy set?!\n");
    }

#ifdef HAVE_PROFILER
    if (profile_filename != NULL) ProfilerStop();
#endif

    // Now we just have states
    stats.final_states = lddmc_satcount_cached(states->dd);
    INFO("Final states: %'0.0f states\n", stats.final_states);
    if (report_nodes) {
        stats.final_nodecount = lddmc_nodecount(states->dd);
        INFO("Final states: %'zu MDD nodes\n", stats.final_nodecount);
    }

    if (out_filename != NULL) {
        INFO("Writing to %s.\n", out_filename);

        // Create LDD file
        FILE *f = fopen(out_filename, "w");
        lddmc_serialize_reset();

        // Write domain...
        fwrite(&vector_size, sizeof(int), 1, f);

        // Write initial state...
        set_save(f, initial);

        // Write number of transitions
        fwrite(&next_count, sizeof(int), 1, f);

        // Write transitions
        for (int i=0; i<next_count; i++) rel_save_proj(f, next[i]);
        for (int i=0; i<next_count; i++) rel_save(f, next[i]);

        // Write reachable states
        int has_reachable = 1;
        fwrite(&has_reachable, sizeof(int), 1, f);
        set_save(f, states);

        // Write action labels
        fclose(f);
    }

    print_memory_usage();
    sylvan_stats_report(stdout);

    lace_stop();

    if (stats_filename != NULL) {
        INFO("Writing stats to %s\n", stats_filename);
        write_stats();
    }

    return 0;
}
