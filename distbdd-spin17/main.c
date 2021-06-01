#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include "avl.h"
#include "sylvan.h"
//#include <localstore.h>
//#include <nodecache.h>
//#include <varchain.h>
//#include <wstealer.h>
#include <sys/time.h>
#include <popt.h>

#include <math.h>

#include "lace.h"

// TODO: clean up comments, whitespace, etc.


/* < bit structure of (MT)BDD nodes in Sylvan >*/
// mtbdd_complement = 0x8000000000000000LL;
// mtbdd_false      = 0;
// mtbdd_true       = 0x8000000000000000LL;
// mtbdd_invalid    = 0xffffffffffffffffLL;
// MTBDDs are assumed to have 40 bits for the pointer part. We'll put these 
// "metadata bits" from the distdd implementation directly left of these 40 bits.

/* < from bddnode.h > */
// bitmaps for BDD references with memory layout:
// - 3 bits metadata ("done" flag, "lock" flag, and "local" flag)
// - 1 bit for complement edges
// - 42 bits for storing the index in 'data'
#define distdd_bdd_metadata       ((BDD)0xE000000000000000) // 3 bits for metadata:
#define distdd_bdd_metadata_done  ((BDD)0x8000000000000000) // - 1 bit, "done" flag
#define distdd_bdd_metadata_lock  ((BDD)0x4000000000000000) // - 1 bit, "lock" flag
#define distdd_bdd_metadata_local ((BDD)0x2000000000000000) // - 1 bit, "local" flag
#define distdd_bdd_complement     ((BDD)0x1000000000000000) // 1 bit for complement edges
#define distdd_bdd_false          ((BDD)0x0000000000000000)
#define distdd_bdd_true           (distdd_bdd_false|distdd_bdd_complement)
#define distdd_bdd_invalid        ((BDD)0x0FFFFFFFFFFFFFFF) // 60 bits


#define distdd_bdd_strip_metadata(s) ((s)&~distdd_bdd_metadata)
#define distdd_bdd_strip_mark(s) ((s)&~distdd_bdd_complement)
//#define distdd_bdd_strip_mark_metadata(s) distdd_bdd_strip_mark(distdd_bdd_strip_metadata(s))

#define distdd_bdd_istrue(bdd) (distdd_bdd_strip_metadata(bdd) == distdd_bdd_true)
#define distdd_bdd_isfalse(bdd) (distdd_bdd_strip_metadata(bdd) == distdd_bdd_false)
//#define distdd_bdd_isconst(bdd) (distdd_bdd_isfalse(bdd) || distdd_bdd_istrue(bdd))
#define distdd_bdd_isnode(bdd) (!distdd_bdd_isfalse(bdd) && !distdd_bdd_istrue(bdd))
//#define distdd_bdd_islocal(bdd) ((bdd) & distdd_bdd_metadata_local)
//#define distdd_bdd_isdone(bdd) ((bdd) & distdd_bdd_metadata_done)

#define distdd_bdd_not(a) (((BDD)a)^distdd_bdd_complement)
#define distdd_bdd_hasmark(s) (((s)&distdd_bdd_complement) ? 1 : 0)

#define distdd_bdd_transfermark(from, to) ((to) ^ ((from) & distdd_bdd_complement))
//#define distdd_bdd_set_data(bdd, data) (((bdd) & 0xF00003FFFFFFFFFF) | (((uint64_t)((data) & 0x0000FFFF)) << 42))
//#define distdd_bdd_set_done(bdd) ((bdd) | distdd_bdd_metadata_done)
/* </ from bddnode.h > */


BDD
_distdd_to_sylvan(BDD a)
{
    return a;
    // TODO: move these "metadata" flags?
    // For now, assume no metadata
    if (a != distdd_bdd_strip_metadata(a)) {
        printf("WARNING: currently ignoring metadata\n");
        a = distdd_bdd_strip_metadata(a);
    }

    //if (a == distdd_bdd_invalid) return sylvan_invalid;
    //if (a == distdd_bdd_false) return sylvan_false;
    //if (a == distdd_bdd_true) return sylvan_true;

    // move complement flag
    if ((a & distdd_bdd_complement) != 0) { // if complement flag is set
        a &= ~distdd_bdd_complement; // set to 0
        a |= sylvan_complement; // set Sylvan complement flag instead
    }
    
    return a;
}

// temp wrapper for debugging
int change_counter = 0;
BDD
distdd_to_sylvan(BDD a)
{
    BDD res = _distdd_to_sylvan(a);
    if (res != a) {
        printf("changed %lx -> %lx (%d total)\n", a, res, ++change_counter);
    }
    return res;
}


BDD
sylvan_to_distdd(BDD a)
{
    if (a == sylvan_false) return distdd_bdd_false;
    if (a == sylvan_invalid) return distdd_bdd_invalid;
    if (a == sylvan_true) return distdd_bdd_true;
    if (a == sylvan_complement) return distdd_bdd_complement;

    return a;
}

/* < from bdd.h > */
struct bdd_ser {
	BDD bdd;
	uint64_t assigned;
	uint64_t fix;
};

typedef struct set {
	BDD bdd;
	BDD variables; // all variables in the set (used by satcount)
	//varchain_t *varchain;
	//varchain_t **vararray;
	//uint64_t varcount;
} *set_t;

typedef struct relation {
	BDD bdd;
	BDD variables; // all variables in the relation (used by relprod)
	//varchain_t *varchain;
	//varchain_t **vararray;
	//uint64_t varcount;
} *rel_t;

set_t states;
rel_t *next; // each partition of the transition relation
int next_count; // number of partitions of the transition relation
/* </ from bdd.h */


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))


static size_t ser_done = 0;
static int vector_size; // size of vector
static int statebits, actionbits; // number of bits for state, number of bits for action
static int bits_per_integer; // number of bits per integer in the vector
const char *filename;




AVL(ser_reversed, struct bdd_ser) {
    return left->assigned - right->assigned;
}


static avl_node_t *ser_reversed_set = NULL;

/*
static double wctime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}
*/


/* returns a DistDD BDD */
BDD serialize_get_reversed(uint64_t value) {
    //printf("value: %lx\n", value);
    
    if (!distdd_bdd_isnode(value)) {
        
        return value;
    }

    struct bdd_ser s, *ss;
    s.assigned = distdd_bdd_strip_mark(value);
    ss = ser_reversed_search(ser_reversed_set, &s);
    assert(ss != NULL);
    
    return distdd_bdd_transfermark(value, ss->bdd);
}

//static int counter = 0;



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
        printf("a = [%016lx], b = [%016lx] ", a, b);
        if (node_level > 1000) {
            printf("var: %d (%x)", node_level, node_level);
        }
        printf("\n");
        
        
        
        // THIS is the thing which messes up the bit structure it seems
        // we changed the position of the 'comp' bit, which must be fixed...
        //if ((node_high & distdd_bdd_metadata_done) != 0) {
        //    node_high &= ~distdd_bdd_metadata_done;
        //    node_high |= distdd_bdd_complement;
        //}
        
        BDD low = serialize_get_reversed(node_low);
        BDD high = serialize_get_reversed(node_high);

        // change to Sylvan bit structure after reading
        low = distdd_to_sylvan(low);
        high = distdd_to_sylvan(high);
        
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
    rel->bdd = distdd_to_sylvan(rel->bdd);

    // get and process variables
    rel->variables = serialize_get_reversed(rel_vars); //CALL_SUPPORT(serialize_get_reversed(rel_vars));
    //rel->varchain = bdd_to_chain(rel->variables); 
    //rel->vararray = chain_to_array(rel->varchain);
    //rel->varcount = chain_count(rel->varchain);
    // TODO: figure out what the 3 lines above are supposed to do
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
    states->variables = serialize_get_reversed(set_state_vars); //CALL_SUPPORT(serialize_get_reversed(set_state_vars));
    printf("variables: %ld <- serialize_get_reversed(%ld)\n", states->variables,  set_state_vars);
    //states->varchain = bdd_to_chain(states->variables); 
    //states->vararray = chain_to_array(states->varchain);
    //states->varcount = chain_count(states->varchain);
    // TODO: figure out what the 3 lines above are supposed to do
}



/*
BDD extend_relation(BDD relation, BDDSET variables) {
    // first determine which state BDD variables are in rel
    int *has = (int*)alloca(statebits * sizeof(int));
    int i; for (i = 0; i < statebits; i++) has[i] = 0;
    BDDSET s = variables;
    while (bdd_strip_metadata(s) == bdd_true) {
        BDDVAR v = bdd_var(s);
        if (v/2 >= (unsigned)statebits) break; // action labels
        has[v/2] = 1;
        s = bdd_high(s);
    }
    
    // then determine the extension to the transition relation
    BDD eq = bdd_true;
    for (i = statebits - 1; i >= 0; i--) {
        if (has[i]) continue;
        BDD low = bdd_makenode(2*i+1, eq, bdd_true);
        BDD high = bdd_makenode(2*i+1, bdd_false, eq);
        eq = bdd_makenode(2*i, low, high);
    }
    
    // finally extend the transition relation
    return CALL_AND(relation, eq, 0);
}
*/

// extend relations from bddmc.c example
#define extend_relation(rel, vars) RUN(extend_relation, rel, vars)
TASK_2(BDD, extend_relation, MTBDD, relation, MTBDD, variables)
{
    /* first determine which state BDD variables are in rel */
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

    /* create "s=s'" for all variables not in rel */
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


/*
BDD big_union(int first, int count) {
    // terminal case
    if (count == 1) return next[first]->bdd;
    
    // recursive computation
    BDD left = big_union(first, count/2);
    BDD right = big_union(first+count/2, count-count/2);
    
    // return the union of left and right
    return bdd_or(left, right);
}
*/

// big union from bddmc.c
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

/*
void merge_relations() {
    BDD prime_variables = sylvan_true; //bdd_true;
    
    int i; for (i = statebits - 1; i >= 0; i--) {
        prime_variables = bdd_set_add(prime_variables, i*2+1);
    }

    
    for (i = 0; i < next_count; i++) {
        next[i]->bdd = extend_relation(next[i]->bdd, next[i]->variables);
        next[i]->variables = prime_variables;
        //next[i]->varchain = bdd_to_chain(next[i]->variables); 
        //next[i]->vararray = chain_to_array(next[i]->varchain);
        //next[i]->varcount = chain_count(next[i]->varchain);
    }
    
    next[0]->bdd = big_union(0, next_count);
    next_count = 1;
    
}
*/


// merge relations from bddmc.c example
void merge_relations() {
    /* merge all relations to one big transition relation */
    BDD newvars = sylvan_set_empty();
    bdd_refs_pushptr(&newvars);
    for (int i = statebits - 1; i >= 0; i--) { //totalbits
        newvars = sylvan_set_add(newvars, i*2+1);
        newvars = sylvan_set_add(newvars, i*2);
    }

    printf("Extending transition relations to full domain...\n");

    printf("%d/%d", 0, next_count); fflush(stdout);
    for (int i = 0; i < next_count; i++) {
        next[i]->bdd = extend_relation(next[i]->bdd, next[i]->variables);
        next[i]->variables = newvars;
        printf("\r%d/%d", i+1, next_count); fflush(stdout);
    }
    printf("\n");



    bdd_refs_popptr(1);


    printf("Taking union of all transition relations.\n");
    next[0]->bdd = big_union(0, next_count);

    for (int i=1; i<next_count; i++) {
        next[i]->bdd = sylvan_false;
        next[i]->variables = sylvan_true;
    }
    next_count = 1;



    printf("BDD nodes:\n");
    printf("Initial states: %zu BDD nodes\n", sylvan_nodecount(states->bdd));
    for (int i=0; i<next_count; i++) {
        printf("Transition %d: %zu BDD nodes\n", i, sylvan_nodecount(next[i]->bdd));
    }
}



// TODO: have this function use lace ?
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

    
	printf("Initial states: %lu BDD nodes\n", sylvan_nodecount(states->bdd)); //bdd_nodecount(states->bdd)
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
    //ws_comp_out_t parout = COMPUTE_PAR(states->bdd);

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

