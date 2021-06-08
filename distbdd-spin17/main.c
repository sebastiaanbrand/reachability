#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/time.h>
#include <popt.h>
#include <math.h>

#include "avl.h"
#include "sylvan.h"
#include "lace.h"
#include "relnext.h"


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define strip_comp_mark(dd) ((dd)&~sylvan_complement)
#define transfer_comp_mark(from, to) ((to) ^ ((from) & sylvan_complement))

/*
static double wctime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}
*/


struct bdd_ser {
	BDD bdd;
	uint64_t assigned;
	uint64_t fix;
};

typedef struct set {
	BDD bdd;
	BDD variables; // all (unprimed) variables in the set
} *set_t;

typedef struct relation {
	BDD bdd;
	BDD variables; // all variables (primed and unprimed) in the relation
    BDDSET primed;
    BDDSET unprimed;
} *rel_t;

MTBDDMAP xp_to_x; // map from primed vars to unprimed vars

set_t states;
rel_t *next; // each partition of the transition relation
int next_count; // number of partitions of the transition relation



static size_t ser_done = 0;
static int vector_size; // size of vector
static int statebits, actionbits; // number of bits for state, number of bits for action
static int bits_per_integer; // number of bits per integer in the vector
const char *filename;


/**
 * AVL tree for rebuilding the loaded BDDs
 */
AVL(ser_reversed, struct bdd_ser) {
    return left->assigned - right->assigned;
}
static avl_node_t *ser_reversed_set = NULL;



BDD serialize_get_reversed(uint64_t value) {

    if (!sylvan_isnode(value)) {
        return value;
    }

    struct bdd_ser s, *ss;
    s.assigned = strip_comp_mark(value);
    ss = ser_reversed_search(ser_reversed_set, &s);
    assert(ss != NULL);
    
    return transfer_comp_mark(value, ss->bdd);
}



void serialize_fromfile(FILE *in) {
    size_t count;
    if (fread(&count, sizeof(size_t), 1, in) != 1) {
        printf("serialize_fromfile: file format error, giving up\n");
        exit(EXIT_FAILURE);
    }

    //unsigned long i; 
    for (unsigned long i = 1; i <= count; i++) {
        uint64_t a, b;
        
        // read node from file
        if ((fread(&a, sizeof(uint64_t), 1, in) != 1) ||
                (fread(&b, sizeof(uint64_t), 1, in) != 1)) {
                printf("serialize_fromfile: file format error, giving up\n");
                exit(EXIT_FAILURE);
        }
        
        // extract all components from the node
        BDD node_high = a & 0x800000ffffffffff;
        BDD node_low = b & 0x000000ffffffffff;
        BDDVAR node_level = (BDDVAR)(b >> 40);

        // some sanity check on the read data
        assert((a & ~0x800000ffffffffff) == 0 && "read invalid bits");
        if (node_level > 1000) {
            printf("a = [%016lx], b = [%016lx] ", a, b);
            printf("var: %d (%x)", node_level, node_level);
            printf("\n");
            exit(1);
        }

        // look up if low and/or high have already been inserted previously (?)
        BDD low = serialize_get_reversed(node_low);
        BDD high = serialize_get_reversed(node_high);

        struct bdd_ser s;
        s.bdd = sylvan_makenode(node_level, low, high); //bdd_makenode_local(node_level, low, high);
        s.assigned = ++ser_done;
        
        ser_reversed_insert(&ser_reversed_set, &s);
    }
}


static rel_t rel_load(FILE* f) {
    serialize_fromfile(f);

    // read info from file
    size_t rel_bdd, rel_vars;
    if ((fread(&rel_bdd, sizeof(size_t), 1, f) != 1) ||
            (fread(&rel_vars, sizeof(size_t), 1, f) != 1)) {
            printf("Invalid input file\n");
          exit(EXIT_FAILURE);
    }

    // get states
    rel_t rel = (rel_t)malloc(sizeof(struct relation));
    rel->bdd = serialize_get_reversed(rel_bdd);

    // get and process variables
    rel->variables = serialize_get_reversed(rel_vars);

    return rel;
}


static void set_load(FILE* f) {
    serialize_fromfile(f);

    // read info from file
    size_t set_bdd, set_vector_size, set_state_vars;
    if ((fread(&set_bdd, sizeof(size_t), 1, f) != 1) ||
            (fread(&set_vector_size, sizeof(size_t), 1, f) != 1) ||
            (fread(&set_state_vars, sizeof(size_t), 1, f) != 1)) {
            fprintf(stderr, "Invalid input file!\n");
            exit(EXIT_FAILURE);
    }

    // get states
    states = (set_t)malloc(sizeof(struct set));
    states->bdd = serialize_get_reversed(set_bdd);

    // get and process variables
    states->variables = serialize_get_reversed(set_state_vars);
}


/**
 * Extends a transition relation to the domain of all given vars.
 */
#define extend_relation(rel, vars) RUN(extend_relation, rel, vars)
TASK_2(BDD, extend_relation, MTBDD, relation, MTBDD, variables)
{
    // first determine which state BDD variables are in rel
    int has[statebits]; // totalbits
    for (int i = 0; i < statebits; i++) has[i] = 0; // totalbits
    MTBDD s = variables;
    while (!sylvan_set_isempty(s)) { //  === (s != mtbdd_true);
        uint32_t v = sylvan_set_first(s);
        if (v/2 >= (unsigned)statebits) break; // action labels // total bits
        has[v/2] = 1;
        s = sylvan_set_next(s);
        //printf("s = %lx\n", s);
    }

    // create "s=s'" for all variables not in rel
    BDD eq = sylvan_true;
    for (int i = statebits - 1; i>=0; i--) { // totalbits
        if (has[i]) continue;
        BDD low = sylvan_makenode(2*i+1, eq, sylvan_false);
        bdd_refs_push(low);
        BDD high = sylvan_makenode(2*i+1, sylvan_false, eq);
        bdd_refs_pop(1);
        eq = sylvan_makenode(2*i, low, high);
    }

    bdd_refs_push(eq);
    BDD result = sylvan_and(relation, eq);
    bdd_refs_pop(1);

    return result;
}

/**
 * Computes union the list of input bdds
 */
#define big_union(first, count) RUN(big_union, first, count)
TASK_2(BDD, big_union, int, first, int, count)
{
    if (count == 1) return next[first]->bdd;

    bdd_refs_spawn(SPAWN(big_union, first, count/2));
    BDD right = bdd_refs_push(CALL(big_union, first+count/2, count-count/2));
    BDD left = bdd_refs_push(bdd_refs_sync(SYNC(big_union)));
    BDD result = sylvan_or(left, right);
    bdd_refs_pop(2);
    return result;
}

/**
 *  Merge all relations to one big transition relation
 */
void merge_relations() {
    //BDD newvars = sylvan_set_empty();
    BDD primed   = sylvan_set_empty();
    BDD unprimed = sylvan_set_empty();
    xp_to_x = mtbdd_map_empty();
    bdd_refs_pushptr(&primed);
    bdd_refs_pushptr(&unprimed);
    bdd_refs_pushptr(&xp_to_x);
    for (int i = statebits - 1; i >= 0; i--) {
        primed = mtbdd_set_add(primed, i*2+1);
        unprimed = mtbdd_set_add(unprimed, i*2);
        xp_to_x = mtbdd_map_add(xp_to_x, (i*2+1), sylvan_ithvar(i*2));
    }

    BDD newvars = mtbdd_set_union(primed, unprimed);

    printf("Extending transition relations to full domain...\n");

    printf("%d/%d", 0, next_count); fflush(stdout);
    for (int i = 0; i < next_count; i++) {
        next[i]->bdd = extend_relation(next[i]->bdd, next[i]->variables);
        next[i]->variables = newvars;
        printf("\r%d/%d", i+1, next_count); fflush(stdout);
    }
    printf("\n");



    bdd_refs_popptr(3);


    printf("Taking union of all transition relations.\n");
    next[0]->bdd = big_union(0, next_count);
    next[0]->variables = newvars;
    next[0]->primed = primed;
    next[0]->unprimed = unprimed;

    for (int i=1; i<next_count; i++) {
        next[i]->bdd = sylvan_false;
        next[i]->variables = sylvan_true;
    }
    next_count = 1;
}



int read_model() {
    if (filename == NULL) {
        fprintf(stderr, "No model has been given!\n");
        return -1;
    }

    FILE *f = fopen(filename, "r");

    if (f == NULL) {
        fprintf(stderr, "Cannot open file '%s'!\n", filename);
        return -1;
    }

    printf("Initializing %s!\n", filename);
    
    if ((fread(&vector_size, sizeof(int), 1, f) != 1) ||
            (fread(&statebits, sizeof(int), 1, f) != 1) ||
            (fread(&actionbits, sizeof(int), 1, f) != 1)) {
            fprintf(stderr, "Invalid input file!\n");
            exit(EXIT_FAILURE);
    }
    
    bits_per_integer = statebits;
    statebits *= vector_size;

	printf("vector size: %d\n", vector_size);
	printf("bits per integer: %d\n", bits_per_integer);
	printf("number of BDD variables: %d\n", vector_size * bits_per_integer);
	printf("Loading initial states: \n");

	
    set_load(f);
    printf("Initial states loaded... \n");

    // read transitions
    if (fread(&next_count, sizeof(int), 1, f) != 1) {
        fprintf(stderr, "Invalid input file!\n");
        exit(EXIT_FAILURE);
    }


    printf("Loading transition relations... \n");

    next = (rel_t*)malloc(sizeof(rel_t) * next_count);

    int i; for (i = 0; i < next_count; i++) {
        next[i] = rel_load(f);
    }

    // done reading from file..
    fclose(f);

    // TODO: parameterize this
    merge_relations();

	printf("%d integers per state, %d bits per integer, %d transition groups\n", vector_size, bits_per_integer, next_count);
	printf("BDD nodes:\n");

    
	printf("Initial states: %lu BDD nodes", sylvan_nodecount(states->bdd)); //bdd_nodecount(states->bdd)
    //printf(" (%.0f states)", mtbdd_satcount(states->bdd, states->variables));
    printf(" (%ld variables)", sylvan_set_count(states->variables));
    printf("\n");
    f = fopen("initial_states.dot", "w");
    sylvan_fprintdot(f, states->bdd);
    fclose(f);

    f = fopen("transition0.dot", "w");
    sylvan_fprintdot(f, next[0]->bdd);
    fclose(f);

	for (i = 0; i < next_count; i++) {
		printf("Transition %d: %lu BDD nodes\n", i, sylvan_nodecount(next[i]->bdd)); // bdd_nodecount(next[i]->bdd)
	}

    return 1;
}

void reachability(BDD initial, BDD R)
{
    // compute the closure of input states under the transition relation
    // (assume single merged transition relation for now)
    BDD prev = sylvan_false;
    BDD visited = initial;
    BDD successors = sylvan_false;
    int k = 0;
    printf("it %d, nodecount = %ld ", k, sylvan_nodecount(visited)); fflush(stdout);
    printf("(%'0.0f states)\n", sylvan_satcount(visited, states->variables));
    while (prev != visited) {
        prev = visited;
        successors = my_relnext(visited, R, next[0]->unprimed, next[0]->primed, xp_to_x);
        visited = sylvan_or(visited, successors); //mtbdd_set_union(visited, successors); // in bddmc.c 'sylvan_or' is used
        printf("it %d, nodecount = %ld ", ++k, sylvan_nodecount(visited)); fflush(stdout);
        printf("(%'0.0f states)\n", sylvan_satcount(visited, states->variables));
    }
}


int main(int argc, char *argv[]) 
{

    int flag_help = -1;
    int flag_qp = -1;
    int cachesize = 23;
    int chunksize = 8;
    int granularity = 8;
    int tablesize = 23;

    poptContext con;
	struct poptOption optiontable[] = {
		{ "cachesize", 'c', POPT_ARG_INT, &cachesize, 'c', "Sets the size of the memoization table to 1<<N entries. Defaults to 23. Maximum of 32.", NULL },
		{ "chunksize", 'e', POPT_ARG_INT, &chunksize, 'e', "When quadratic probing is used, the chunk size determines the number of buckets obtained per probe. Maximum of 4096.", NULL },
		{ "granularity", 'g', POPT_ARG_INT, &granularity, 'g', "Only use the memoization table every 1/N BDD levels. Defaults to 8.", NULL },
		{ "help", 'h', POPT_ARG_NONE, &flag_help, 'h', "Display available options", NULL },
		{ "quadratic-probing", 'q', POPT_ARG_NONE, &flag_qp, 'q', "Use quadratic probing instead of linear probing as collision strategy for the BDD table. This prevents clustering issues, but it hits performance in distributed runs.", NULL },
		{ "tablesize", 't', POPT_ARG_INT, &tablesize, 't', "Sets the size of the BDD table to 1<<N nodes. Defaults to 23. Maximum of 32.", NULL },
		{NULL, 0, 0, NULL, 0, NULL, NULL}
	};


    con = poptGetContext("distdd", argc, (const char **)argv, optiontable, 0);
    poptSetOtherOptionHelp(con, "[OPTIONS..] <model.bdd>");
    
    if (argc < 2) {
        poptPrintUsage(con, stderr, 0);
        exit(1);
    }


    char c;  
    while ((c = poptGetNextOpt(con)) > 0) {  
        switch (c) {
            case 'h':
                poptPrintHelp(con, stdout, 0);
                return 0;
        }
    }


    // make sure that: 4 <= cachegran
    granularity = MAX(granularity, 4);
    printf("using a cache granularity of %d\n", granularity);
    
    // make sure that 1 <= chunksize <= 4096
    chunksize = MAX(chunksize, 1);
    chunksize = MIN(chunksize, 4096);

	if (flag_qp > 0) {
        printf("using quadratic probing in the BDD table with chunk size %d\n", chunksize);
    }
    else {
        printf("using linear probing in the BDD table\n");
    }


    // make sure that: 23 <= cachesize <= 32
    cachesize = MAX(cachesize, 23);
    cachesize = MIN(cachesize, 32);

    uint64_t cacheentries = (((uint64_t)1) << cachesize);
    uint64_t cachemem = (40 * cacheentries) / (1048576);
    printf("using a memoization table of 2^%d = %lu entries (~%lu MB) per thread\n", cachesize, cacheentries, cachemem);


    // make sure that: 23 <= tablesize <= 32
    tablesize = MAX(tablesize, 23);
    tablesize = MIN(tablesize, 32);

    uint64_t tableentries = (((uint64_t)1) << tablesize);
    uint64_t tablemem = (24 * tableentries) / (1048576);
    printf("using a BDD table of 2^%d = %lu entries (~%lu MB) per thread\n", tablesize, tableentries, tablemem);


    //bdd_init(granularity, tableentries, cacheentries, flag_qp, chunksize);
	printf("init sylvan and lace\n");
	lace_start(1, 0);
	sylvan_set_granularity(granularity);
    sylvan_set_sizes(tableentries, tableentries, cacheentries, cacheentries);
    sylvan_init_package();
    sylvan_init_bdd();


    // read model file from disk
    filename = poptGetArg(con);
    read_model();

    // perform reachability
    printf("Doing reachability\n");
    reachability(states->bdd, next[0]->bdd);

    //printf("PAR Time: %f\n", parout.time);

    /*

    // show statistics
    nodecache_show_stats();
    ws_statistics();

    // determine the size of the state space
    ws_comp_out_t satout = COMPUTE_SATCOUNT(parout.output, 0, 0);

    if (MYTHREAD == 0) {
        printf("Final states: %llu states\n", satout.output);
        printf("Final states: %llu BDD nodes\n", bdd_nodecount(parout.output));
    }
    */
   	printf("stopping lace\n");
    
    lace_stop();
	printf("lace stopped\n");
    return 0;
}

