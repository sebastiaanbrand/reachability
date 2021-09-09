/**
 * Awari
 *
 * TODO: explain more
 *
 * The transition relation implements the rules of the game as used in the
 * the 1990-1992 Computer Olympiads, described here:
 * http://web.cecs.pdx.edu/~bart/cs510games-summer2000/hw4/bart-awari-rules.html
 *
 *
 * TODO: Implement unit tests (sanity checks on BDDs produced
 * TODO: Create specialization of sylvan::Bdd class that let's us buffer the BDD to disk
 *
 * TODO: comment code
 * TODO: introduce coding standard: https://users.ece.cmu.edu/~eno/coding/CppCodingStandard.html#descriptive
 * TODO: introduce typedef for PitIdx and Stones
 * TODO: refactor into several files (classes):
 *             Auxiliary (printing and stuff),
 *             Awari Game,
 *             Search Algorithms (Awari *is a* TwoPlayerGame and should only expose Vars, relation, Start, Goal, SweepLines)
 * TODO: Rename var names like K, M,
 * TODO: Create program argument to turn all counting / reporting off (this slows down BDDs)
 *
 */

#include <algorithm>
#include <chrono>
#include <climits>
#include <initializer_list>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <locale>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdbool.h>
#include <vector>

#include <bvec_sylvan.h>
#include <sylvan_obj.hpp>
#include <sylvan_int.h>
#include <sylvan_table.h>


#ifdef HAVE_PROFILER
#include <gperftools/profiler.h>
#endif

using namespace sylvan;
using namespace std;
using namespace std::chrono;

// don't use for heap / malloced arrays
#define len(x) (sizeof(x) / sizeof(x[0]))

// SAVING (adapted from from LTSmin)
static void
dom_save(FILE* f, BddSet vars)
{
    // To be compatible w/ LTSmin output we need to specify the following:
    // 1. vectorsize = number of integers for each full state
    // 2. statebits* = number of bits for each state variable
    // 3. actionbits = number of bits for the action label
    // For 1 and 2 we'll just have a single state be 1 "n_vars"-bit integer.
    // We don't need 3.
    int vectorsize = 1;
    int statebits[] = {0}; statebits[0] = vars.size();
    int actionbits = 0;
    fwrite(&vectorsize, sizeof(int), 1, f);
    fwrite(statebits, sizeof(int), vectorsize, f);
    fwrite(&actionbits, sizeof(int), 1, f);
}

static void
set_save(FILE* f, BddSet vars, BDD set)
{
    // get state vars as array
    int k = vars.size();
    BDDVAR *proj = (BDDVAR *) malloc(k * sizeof(int));
    vars.toArray(proj);

    fwrite(&k, sizeof(int), 1, f);
    if (k != -1) fwrite(proj, sizeof(int), k, f);
    free(proj);

    LACE_ME;
    mtbdd_writer_tobinary(f, &set, 1);
}

static void
rel_save_proj(FILE* f, BddSet unprimed, BddSet primed)
{
    // get vars
    int r_k = unprimed.size(); // number of unprimed
    int w_k = primed.size(); // number of primed
    BDDVAR *r_proj = (BDDVAR *) malloc(r_k * sizeof(int)); // unprimed vars
    BDDVAR *w_proj = (BDDVAR *) malloc(w_k * sizeof(int)); // primed vars
    unprimed.toArray(r_proj);
    primed.toArray(w_proj);

    // get vars from relation
    fwrite(&r_k, sizeof(int), 1, f);
    fwrite(&w_k, sizeof(int), 1, f);
    fwrite(r_proj, sizeof(int), r_k, f);
    fwrite(w_proj, sizeof(int), w_k, f);

    free(r_proj);
    free(w_proj);
}

static void
rel_save(FILE* f, BDD rel)
{
    LACE_ME;
    mtbdd_writer_tobinary(f, &rel, 1);
}

// TODO: rename all these confusing varnames:
struct AwariSize {
    int                     M; //the number of pits (squares)
    int                     M2; //the number of pits on each side
    int                     K; //the number of seeds at initial state
    int                     K1; //the number of seeds per pit at initial state
    int                     K2; //the number of seeds divided by 2
    int                     logK;
};

Bdd OracleExists(const Bdd In) {
   vector<BDDVAR> order = {69, 47, 59, 71, 35, 57, 45, 83, 81, 73, 61, 49, 85, 37, 79, 95, 33, 91, 107, 103, 119, 93, 105, 67, 117, 55, 43, 115, 131, 129, 89, 109, 121, 97, 127, 133, 143, 141, 139, 145, 125, 137, 147, 23, 11, 13, 149, 113, 157, 151, 155, 153, 135, 123, 111, 1, 25, 77, 31, 99, 101, 75, 53, 29, 41, 65, 3, 21, 87, 51, 27, 63, 39, 9, 7, 5, 19, 17, 15};
   Bdd Out = In;
   for (BDDVAR v : order)
       Out = Out.ExistAbstract(BddSet(v));
   return Out;
}

// Smart existential quantification by selecting local optimum
Bdd SmartExists(const Bdd In, const BddSet &Vars, int n) {
    Bdd Set = In;
    BddSet vars = Vars;

    while (!vars.isEmpty()) {

        // try all vars and store results as pairs (var, new-size) in vec
        typedef pair<BddSet, size_t> my_pair;
        vector<my_pair> vec;
        for (BddSet x = vars; !x.isEmpty(); x = x.Next()) {
            BddSet var(Bdd(x.TopVar()));
            size_t count = Set.ExistAbstract(var).NodeCount();
            vec.push_back({var, count});
        }

        // sort vec according to new-size
        sort(vec.begin(), vec.end(), [](const my_pair &a, const my_pair &b) -> bool
                {
                    return a.second < b.second;
                });

        // quantify the best n variables away
        int i = 0;
        for (my_pair &p : vec) {
            Set = Set.ExistAbstract(p.first);
            vars.remove(p.first.TopVar());
            if (++i == n) break;
        }
    }
    return Set;
}

// Prettify printing of large numbers
class MyNumPunct : public numpunct<char>
{
protected:
    virtual char_type do_decimal_point() const      { return ',';   }
    virtual char do_thousands_sep() const           { return '.';   }
    virtual string do_grouping() const              { return "\03"; }
};

void WriteToFile(const Bdd *Final, string name, int count)
{
    LACE_ME;
    cout << "Writing "<< name << "...." << endl;
    FILE *f = fopen(name.c_str(), "w");
    assert(f != NULL && "cannot open file");
    BDD array[count];
    for (int i = 0; i < count; i++)
        array[i] = Final[i].GetBDD();
    mtbdd_writer_tobinary(f, array, count);
    fclose(f);
}

void WriteToFile(const Bdd Final, string name) { WriteToFile(&Final, name, 1); }

void ReadFromFile(Bdd *Final, string name, int count) {
    LACE_ME;
    cout << "Reading "<< name <<"...." << endl;
    FILE *f = fopen(name.c_str(), "r");
    assert(f != NULL && "cannot open file");
    BDD Final_[count];
    mtbdd_reader_frombinary(f, Final_, count);
    fclose(f);
    for (int i = 0; i < count; i++) {
        Final[i] = Bdd(Final_[i]);
    }
}

Bdd ReadFromFile(string name) { Bdd Res; ReadFromFile(&Res, name, 1); return Res; }

/* Obtain current wallclock time */
static double
wctime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

size_t choose(size_t n, size_t k) {
    size_t res = 1;
    if (k > n - k) k = n - k; // Since C(n, k) = C(n, n-k)
    for (size_t i = 0; i < k; ++i) {  // [n * (n-1) *---* (n-k+1)] / [k * (k-1) *----* 1]
        res *= (n - i);
        res /= (i + 1);
    }
    return res;
}

VOID_TASK_0(gc_start)
{
    // nodes_size = 24 *
    // cache_size = 36 *
    double nodes_size = 1 * llmsset_get_size(nodes) / (1024 * 1024 * 1024);
    double cache_size = 1 * cache_getsize() / (1024 * 1024 * 1024);
    cout << endl << "(GC) Starting garbage collection ...\t\t"<< fixed << setprecision(1) << setw(3) << setfill(' ') << nodes_size
         << "GB / "<< fixed << setprecision(1) << setw (3) << setfill(' ') << cache_size <<"GB\n";
}

static size_t MaxTable = 0;
VOID_TASK_0(gc_end)
{
    size_t tot, filled;
    sylvan_table_usage(&filled, &tot);
    MaxTable = max(MaxTable, filled);
    // nodes_size = 24 *
    // cache_size = 36 *
    double nodes_size = 1 * llmsset_get_size(nodes) / (1024 * 1024 * 1024);
    double cache_size = 1 * cache_getsize() / (1024 * 1024 * 1024);
    cout << "(GC) Garbage collection done (" << setprecision(0) << setw (15) << setfill(' ') << filled
         << ")\t"<< fixed << setprecision(1) << setw(3) << setfill(' ') << nodes_size
         << "GB / "<< fixed << setprecision(1) << setw(3) << setfill(' ') << cache_size <<"GB\n";
}

void sylvan_gc_no_resize() {
    LACE_ME;
    sylvan_clear_cache();
    sylvan_clear_and_mark();
    sylvan_rehash_all();
}

static size_t MaxNodes = 0;
static double start_time = 0;
void PrintLevel (Bdd& Visited, BddSet &Vars, int level) {
    cout << "["<< fixed  << setprecision(2) << setfill(' ') << setw(8) << wctime() - start_time <<"]  ";
    size_t NodeCount = Visited.NodeCount();
    MaxNodes = max(MaxNodes, NodeCount);
    cout << "Search Level " << fixed << setprecision(0) << setw(3) << setfill(' ') << level <<
            ": Visited has " << defaultfloat << setprecision(0) << fixed << setfill(' ') << setw (20)
            << Visited.SatCount(Vars) << " boards and " << setw (20)
            << NodeCount << " BDD nodes" << endl;
}

int
toint(uint8_t *e, int l)
{
    int c = 0;
    for (int i = 0; i < l; i++)
        c |= e[i] << i;
    return c;
}


VOID_TASK_4(print_board, void *,context, BDDVAR *,VA, uint8_t *,e, int, l) {
    AwariSize *A = (AwariSize *)context;
    int offset = 1 + A->logK;                        // player + score
    int lastP2  = offset + (A->M - 1)*A->logK;
    int firstP2 = offset + A->M2*A->logK;
    for (int i = lastP2; i >= firstP2; i-=A->logK)
        printf("%2d, ", toint(&e[i], A->logK));
    printf("\tplayer: %d\n", e[0]);
    for (int i = offset; i < firstP2; i+=A->logK)
        printf("%2d, ", toint(&e[i], A->logK));
    printf("\tscore: %d\n\n", toint(&e[1], A->logK));
    (void) VA;
    (void) l;
}
#define print_board TASK(print_board)


class Awari : public AwariSize
{
public:

    // all variables according to each pit (square).
    vector<Bvec>            Pits[2];
    vector<Bvec>            Pits2[2]; // different variable order
    Bvec                    Scor[2];
    Bdd                     Turn[2];

    // BDD variables:
    BddSet                  Vars[2];        // unprimed / primed
    BddSet                  PitVars[2];
    BddSet                  RelationVars; // unprimed & primed  (interleaved)
    BddMap                  Pit2BitInterleaved;

    // BDDs representing sets of boards (domain is Vars[0]):
    Bdd                     InitialBoard;
    Bdd                     WinningBoards;

    // BDD representing the move relation(domain is RelationVars):
    Bdd                     MoveRelation;

    Awari(int m, int k) {
        assert ((m & 1) == 0 && "M should be even");
        assert (k % m == 0 && "K should be divisible by M");

        Cache[0] = new Bdd*[k + 1];
        Cache[1] = new Bdd*[k + 1];
        for (int i = 0; i <= k; i++) Cache[0][i] = new Bdd[m];
        for (int i = 0; i <= k; i++) Cache[1][i] = new Bdd[m];

        M = m;
        K = k;
        K1 = K / M;
        K2 = K / 2;
        M2 = M / 2;
        logK = ceil(log2(K));

        InitializeVars();
        CreateInitialBoard();

        double start = wctime();
        CreateRelation();
        cout << "MoveRelation took " << fixed << setprecision(2) << wctime() - start <<" sec and has a total of  "<< MoveRelation.NodeCount() <<" BDD nodes" << endl;

        //CreateWiningboards();
    };

    Awari() : Awari(12, 48) {};

    Bdd PlayerMoveRelation(bool p) {
        return MoveRelation & (p ? Turn[0] : !Turn[0]);
    }

    void InitializeVars(){
        BDDVAR var = 0;  // we'll just increment this, assigning even to unprimed and odd to primed vars

        Turn[0] = Bdd::bddVar(var++);
        Turn[1] = Bdd::bddVar(var++);

        // initialize score variable
        for (int i = 0; i < logK; i++) {
            Scor[0].push_back(var++);
            Scor[1].push_back(var++);
        }

        // initialize pit variables (consecutively)
        int varOld = var;
        Pits[0].resize(M);
        Pits[1].resize(M);
        for (int p = 0; p < M; p++) {
            for (int i = 0; i < logK; i++) {
                Pits[0][p].push_back(var++);
                Pits[1][p].push_back(var++);
            }
        }
        // initialize pit variables (bit-interleaved)
        Pits2[0].resize(M);
        Pits2[1].resize(M);
        var = varOld;
        for (int i = 0; i < logK; i++) {
            for (int p = 0; p < M; p++) {
                Pits2[0][p].push_back(var++);
                Pits2[1][p].push_back(var++);
            }
        }

        // combine all variables
        assert (var != 0);
        for (int v = var - 1; v >= 0; v--) { // this order is faster
            RelationVars.add(v);
            Vars[v & 1].add(v);
        }

        for (int p = 0; p < M; p++) {
            for (int i = 0; i < logK; i++) {
                uint32_t var = Pits[0][p][i].TopVar();
                Pit2BitInterleaved.put(Pits2[0][p][i].TopVar(), Bdd::bddVar(var));
            }
        }

        for (int i = 0; i < M; i++) {
            for (int j = 0; j < logK; j++) {
                for (int b = 0 ; b <= 1; b++) {
                    uint32_t var = Pits[b][i][j].TopVar();
                    PitVars[b].add(var);
                }
            }
        }
    }

    Bdd CreateBoard(bool primed, bool turn, size_t score, vector<int> pits) {
        Bdd res = Bdd::bddOne();
        assert (pits.size() > 0); // repeat the last argument provided
        for (int p = M - 1; p >= 0; p--) {
            int i = min(pits.size()-1, (size_t)p);
            res &= Pits[primed][p] == pits[i];
        }
        res &= Scor[primed] == score;
        res &= turn ? Turn[primed] : !Turn[primed];
        return res;
    }

    void CreateInitialBoard(){
        InitialBoard = CreateBoard(false, 0, 0, {K1});
        //InitialBoard.Support().PrintDot(stdout);
    }

    // how many ways can place m bars between a sequence of n stars
    Bdd **Cache[2];
    Bdd StarsAndBars(int k, int m, bool b) {
        if (!Cache[b][k][m-1].isZero()) return Cache[b][k][m-1];
        if (m == 1) {
            Cache[b][k][m-1] = Pits[b][0] == k;
        } else {
            for (int i = 0; i <= k; i++) {
                Cache[b][k][m-1] |= (Pits[b][m-1] == (k - i)) & StarsAndBars(i, m-1, b);
            }
        }
        return Cache[b][k][m-1];
    }

    Bdd StarsAndBars2(int k, int m, bool b) {
        Bdd Carry;
        Bvec Sum(logK, 0);
        for (int j = m - 1; j >= 0; j--)
            Sum = Bvec::bvec_add(&Carry, Sum, Pits2[b][j]);
        Bdd SnB = (Sum == k) & !Carry;
        return SnB.Compose(Pit2BitInterleaved);
    }

//    Bdd Deadlocks;                  // Deadlocks are always grand slams
    Bdd Won[2];                     // Immediately won states by Player 0/1
    Bdd SlamWinScore[2];            // winning scores for Player  0 / 1
    vector<Bdd> NrStones[2];        // all boards with i stones
    Bdd Max;
    void CreateWiningboards() {
        double start = wctime();
        NrStones[0].resize(K + 1);
        NrStones[1].resize(K + 1);
        for (int i = 0; i <= K; i++) {
            for (int b = 0; b <= 1; b++)
                NrStones[b][i] = StarsAndBars2(i, M, b);
            assert((size_t)NrStones[0][i].SatCount(PitVars[0]) == choose(i + M - 1, M - 1));
            Max |= NrStones[0][i];
            Bvec Vi  = Bvec(logK, i);
            Bvec Vk  = Bvec(logK, K);
            Bvec Vk2 = Bvec(logK, K2);
            Bdd C;
            Bvec Scor0_Vi = Bvec::bvec_add(&C, Scor[0], Vi);
            SlamWinScore[0] |= (!Turn[0]) & NrStones[0][i] & (Scor0_Vi > Vk2) & (Scor[0] <= Vk) & !C; // Grand slam P0, win P0
            SlamWinScore[0] |= ( Turn[0]) & NrStones[0][i] & (Scor[0]  > Vk2) & (Scor[0] <= Vk); // Grand slam P1, win P0
            SlamWinScore[1] |= (!Turn[0]) & NrStones[0][i] & (Scor0_Vi < Vk2) & (Scor[0] <= Vk) & !C; // Grand slam P0, win P1
            SlamWinScore[1] |= ( Turn[0]) & NrStones[0][i] & (Scor[0]  < Vk2) & (Scor[0] <= Vk); // Grand slam P1, win P1
        }
        cout << "SlamWinScore[0] took "<< fixed << setprecision(2) << wctime() - start<< " sec and has "<< SlamWinScore[0].NodeCount() <<" BDD nodes" << endl;

        start = wctime();
//        string DeadlockName = "Deadlock-"+ MoveRelation.GetShaHash() +".bdd";
        string WonName = "Won-"+ MoveRelation.GetShaHash() +".bdd";
        if (access( WonName.c_str(), F_OK ) != -1) {
            ReadFromFile(Won, WonName, 2);
        } else {
            Bdd Deadlocks = !OracleExists(MoveRelation);// !SmartExists(MoveRelation, Vars[1], 1);
            Won[0] = Deadlocks & SlamWinScore[0];
            Won[1] = Deadlocks & SlamWinScore[1];
            WriteToFile(Won, "Won-"+ MoveRelation.GetShaHash() +".bdd", 2);
        }
        cout << "Won has "<< Won[0].NodeCount() <<" BDD nodes" << endl;
    }

    int mod(int x,int N)    { return (x % N + N) %N; } // C semantics of % is "remainder," not modulus.
    int turn(int pit)       { return pit >= M2; }
    int last(int o, int s)  { return (o + s) % M; }

    int sown(int o, int d, int s) {
        if(o == d) return 0;
        int m = s / (M-1); // round down
        return m + (mod(d - o, M) <= (s % (M-1)));
    }

    void CreateRelation() {
        MoveRelation = Bdd::bddZero();
        Bvec zer0(logK, 0);

        // sowing the i square
        for (int o = 0; o < M; o++) {                      // (o)rigin pit
            for (int s = 1; s <= K; s++) {                 // (s)tones, number of
                Bdd rel = Bdd::bddOne();
                Bdd Carry;
                rel &= turn(o) ? Turn[0] & (!Turn[1]) : (!Turn[0]) & Turn[1];

                // sowing to destianation d
                Bdd CanTake = Bdd::bddOne();
                Bvec NewScore = Scor[0];
                for (int d = M - 1; d >= 0; d--) {        // (d)estination pits, in taking order (and dd-favored order!)
                    if (d == o) {
                        rel &= Pits[0][d] == s;
                        rel &= Pits[1][d] == zer0;
                    } else {                              // taking from d
                        Bvec NewStones = Bvec::bvec_add(&Carry, Pits[0][d], Bvec(logK, sown(o, d, s)));
                       //Bvec NewStones = Pits[0][d] + Bvec(logK, sown(o, d, s));
                        if (turn(o) != turn(d) && last(o, s) >= d && !CanTake.isZero()) {
                            CanTake  &= (NewStones == 2) | (NewStones == 3);
                            if (turn(o) == 0)
                                NewScore = Bvec::bvec_add(&Carry, NewScore, Bvec::bvec_ite(CanTake, NewStones, zer0));
                                //NewScore += Bvec::bvec_ite(CanTake, NewStones, zer0);
                            NewStones = Bvec::bvec_ite(CanTake, zer0, NewStones);
                        }
                        rel &= Pits[1][d] == NewStones;   // P_d' := P_d + saw
                    }
                }
                rel &= Scor[1] == NewScore;

                // prune illegal moves
                Bdd illegal = Bdd::bddOne();
                for (int d = M - 1; d >= 0; d--) {
                    if (turn(o) != turn(d)) illegal &= Pits[1][d] == 0;
                }
                rel &= !illegal;

                MoveRelation |= rel & !Carry;
            }
        }
    }

    void PrintBoard(Bdd &b, string name){
        LACE_ME;
        cout<<"print board: "<<name<<endl;
        //sylvan_enum(b.GetBDD(), Vars[0].GetBDD(), print_board, (AwariSize *)this);
    }

    void WriteAsLTSminOutput(FILE *f)
    {
        // 1. write domain info
        dom_save(f, Vars[0]);

        // 2. write initial states
        set_save(f, Vars[0], InitialBoard.GetBDD());

        // 3. write number of transition relation groups
        int nGrps = 1; fwrite(&nGrps, sizeof(int), 1, f);

        // 4. write rel_proj
        rel_save_proj(f, Vars[0], Vars[1]);

        // 5. write rel
        rel_save(f, MoveRelation.GetBDD());
    }
};

Bdd BfsForward(Awari &game, Bdd Seed) {
    int level = 0;
    Bdd Visited = Seed;
    Bdd Next = Seed;
    while (Next != Bdd::bddZero()) {
        Next  = Next.RelNext(game.MoveRelation, game.RelationVars);
        Next -= Visited;
        Visited |= Next;

        PrintLevel (Visited, game.Vars[0], level++);
    }
    return Visited;
}

Bdd BfsBackward(Awari &game, Bdd Seed, Bdd Universe) {
    int level = 0;
    Bdd Visited = Seed;
    Bdd Next = Visited;
    while (Next != Bdd::bddZero()) {
        Next  = Next.RelPrev(game.MoveRelation, game.RelationVars) & Universe;
        Next -= Visited;
        Visited |= Next;

        PrintLevel (Visited, game.Vars[0], level++);
    }
    return Visited;
}

Bdd SweepForward(Awari &game, Bdd Seed) {
    int level = 0;
    int L = game.K + 1;
    Bdd Visited[L];

    Visited[0] = Seed;

    Bdd Below;
    for (int k = 0; k < L; k++)
        Below |= game.NrStones[0][k];

    for (int l = L-1; l >= 0; l--) {

        Bdd MoveNP = game.MoveRelation & game.NrStones[1][l];

        cout << endl << "SWEEPLINE with "<< l <<" stones has "<< Visited[l].SatCount(game.Vars[0]) <<" and "<< Visited[l].NodeCount() <<" BDD nodes" << endl;
        Bdd Next = Visited[l];
        while (Next != Bdd::bddZero()) {
            Next  = Next.RelNext(MoveNP, game.RelationVars);
            Next -= Visited[l];
            Visited[l] |= Next;

            PrintLevel (Visited[l], game.Vars[0], level++);
        }

        Next  = Visited[l].RelNext(game.MoveRelation & Below, game.RelationVars);
        for (int k = l + 1; k < L; k++) {
            Visited[k] |= Next & game.NrStones[0][k];
        }
        Below -= game.NrStones[0][l];
    }
    return Visited[0];
}

Bdd SweepBackward(Awari &Game, Bdd Seed, Bdd Universe) {
    double start = wctime();
    int L = Game.K + 1;
    Bdd Final;
    Bdd Visited[L];
    Bdd Moves[2];
    Bdd MoreStones;
    for (int l = 0; l < L; l++) MoreStones |= Game.NrStones[0][l];

    for (int l = 0; l < L; l++) {
        Visited[l] |= (Seed & Game.NrStones[0][l]) & Universe;
        cout << endl << "["<< wctime() - start <<"] SWEEPLINE with "<< setw(2) << l <<" stones has "<< 0/*Visited[l].SatCount(game.Vars[0])*/ <<" boards and "<< Visited[l].NodeCount() <<" BDD nodes" << endl;

        Bdd MovesNP = Game.MoveRelation & Game.NrStones[0][l];
        Bdd Old, Temp;
        //int level = 0;
        while (Visited[l] != Old) {
            Old = Visited[l];
            Visited[l] |= Visited[l].RelPrev(MovesNP, Game.RelationVars) & Universe;
            //PrintLevel(Visited[l], game.Vars[0], level++);
        }
        if (l == L-1) break;

        MoreStones -= Game.NrStones[0][l];
        Old   =    Visited[l] .RelPrev(Game.MoveRelation & MoreStones, Game.RelationVars);
        for (int k = l+1; k < L; k++) {
            Temp = Old & Game.NrStones[0][k];
            Visited[k] |= Temp & Universe;
            Old -= Temp;
        }
        assert(Old.isZero());

        Final |= Visited[l];
        cout << endl << "FINAL BDD has "<< Final.SatCount(Game.Vars[0]) <<" boards and "<< Final.NodeCount() <<" BDD nodes" << endl;
    }

    return Final;
}

Bdd ForallExistsRelPrev(Awari &Game, const Bdd &Final, int player) {
    Bdd Res;
    Bdd    Exist = Final.RelPrev(Game.PlayerMoveRelation(player), Game.RelationVars);
    Bdd    Exits = Final.RelNext(Game.PlayerMoveRelation(player), Game.RelationVars) - Final;
    Res =  Exist - Exits.RelPrev(Game.PlayerMoveRelation(player), Game.RelationVars);
//    Bdd Test = !(!Final).RelPrev(Game.PlayerMoveRelation(1), Game.RelationVars);
//    assert((Exist & Test) == Res);
    return Res;
}

Bdd ForcedCycles(Awari &Game, double start, Bdd Seed) {
    Bdd Old;
    Bdd Forced = Seed;
    int level = 0;
    cout << "[       ] Searching forced cycles ...   ";
    while (Old != Forced) {
        Old = Forced;
        Forced &= ForallExistsRelPrev(Game, Forced, 1) | !Game.Turn[0];
        Forced &= Forced.RelPrev(Game.PlayerMoveRelation(0), Game.RelationVars) | Game.Turn[0];
//        PrintLevel(Final, Game.Vars[0], level++);
        cout << level++ <<",  "<< flush;
    }
    cout << "."<< endl;
    cout << "["<<setprecision(2) <<  wctime() - start <<"] Forced cycles BDD has " << setprecision(0) << Forced.SatCount(Game.Vars[0]) <<" boards and "<< Forced.NodeCount() <<" BDD nodes" << endl;

    return Forced;
}

Bdd Retrograde(Awari &Game, Bdd Seed) {
    double start = wctime();
    Bdd Visited = Seed;
    int level = 0;
    Bdd Next[2] = { Visited & !Game.Turn[0], Visited & Game.Turn[0] };
    while (!Next[1].isZero() || !Next[0].isZero()) {
        Bdd Temp = Next[1].RelPrev(Game.PlayerMoveRelation(0), Game.RelationVars);
        Next[0] |= Temp - Visited;
        Visited |= Next[0];

        Bdd    Exist = Next[0].RelPrev(Game.PlayerMoveRelation(1), Game.RelationVars) - Visited;
        Bdd    Exits =   Exist.RelNext(Game.PlayerMoveRelation(1), Game.RelationVars) - Visited;
        Next[1] = Exist -   Exits.RelPrev(Game.PlayerMoveRelation(1), Game.RelationVars);
        Visited |= Next[1];
        Next[0] = Bdd::bddZero();
        cout << "["<< setprecision(2) << wctime() - start <<"] Round "<< level++ <<" BDD has "<< setprecision(0) << /*Final.SatCount(Game.Vars[0])*/0 <<" boards and "<< Visited.NodeCount() <<" BDD nodes (writing to disk)" << endl;
    }
    return Visited;
}

class Lazy {
    Bdd B;

public:
    int idx;
    Lazy(Bdd &b, size_t s) : B(b), idx(s) { assert(b != sylvan_false); }

    void ToDisk() {
        if (B.GetBDD() == sylvan_false) return;
        WriteToFile (&B, "RetrogradeSweepline-"+ (string)(idx<=9?"0":"") + to_string(idx) +".bdd", 1);
        B = Bdd(sylvan_false);
    }

    Bdd GetOnce() {
        if (B.GetBDD() == sylvan_false)
            return ReadFromFile("RetrogradeSweepline-"+ (string)(idx<=9?"0":"") + to_string(idx) +".bdd");
        return B;
    }
};

class Stack
{
public:
    list<Lazy>   m_data;
//    list<Lazy>::iterator begin() { return m_data.begin(); }
//    list<Lazy>::iterator end()   { return m_data.begin(); }

    Stack(bool disk) : to_disk(disk), size(0) { }

    void Push(Bdd &b) {
        if (to_disk && size > 0) {
            m_data.back().ToDisk();
        }
        m_data.push_back(Lazy(b, size++));
        if (m_data.size() > 18) {
            if (!to_disk) {
                Lazy L = m_data.front();
                Bdd B = L.GetOnce();
                WriteToFile(&B, "RetrogradeSweepline-" + (string) (L.idx <= 9 ? "0" : "") + to_string(L.idx) + ".bdd", 1);
            }
            m_data.pop_front();
        }
    }

    Bdd Top() {
        return m_data.back().GetOnce();
    }
private:
    bool        to_disk;
    size_t         size;
};

Bdd RetroSweep(Awari &Game, Bdd Seed, bool disk) {
    double start = wctime();
    int L = Game.K + 1;
    Stack Lines(disk);

    for (int l = 0; l < L; l++) {
        if (l == Game.K-1) continue;
        cout << endl << "["<< setprecision(2) << wctime() - start <<"] Starting sweepline with "<< setprecision(0)<< setw(2) << l <<" stones  ..." << endl;

        Bdd SweepLine;
        Bdd NoEscape = Game.NrStones[0][l] & Game.Turn[0];
        for (Lazy &L : Lines.m_data) {
            Bdd Temp = L.GetOnce();
            SweepLine |=   Temp .RelPrev(Game.PlayerMoveRelation(0), Game.RelationVars) & Game.NrStones[0][l];
            NoEscape -=  (!Temp).RelPrev(Game.PlayerMoveRelation(1) & Game.NrStones[1][L.idx], Game.RelationVars);;
        }

        {
            Bdd OnlyEscp = NoEscape - Game.NrStones[0][l].RelPrev(Game.PlayerMoveRelation(1), Game.RelationVars);
            Bdd    Next  = OnlyEscp.RelNext(Game.PlayerMoveRelation(1), Game.RelationVars);
            OnlyEscp    &= Next    .RelPrev(Game.PlayerMoveRelation(1), Game.RelationVars);
            SweepLine   |= OnlyEscp;
            cout << "["<<setprecision(2) <<  wctime() - start <<"] NoEscape / OnlyEscape BDDs have "<< NoEscape.NodeCount() <<" / "<< OnlyEscp.NodeCount() <<" BDD nodes" << endl;
        }

        // Add forced cycles
        if (l <= 6) {
            Bdd WinScore = Game.Scor[0] > Bvec::bvec_con(Game.logK, Game.K2 - l/2);
                WinScore &= Game.Scor[0] <= Bvec(Game.logK, Game.K);
            SweepLine |= ForcedCycles(Game, start, Game.NrStones[0][l] & WinScore & (NoEscape | !Game.Turn[0]));
        }
        SweepLine |= Seed & Game.NrStones[0][l];
        cout << "["<< setprecision(2) << wctime() - start <<"] sweep line seed "<< setprecision(0) << setw(2) << l <<" stones has "<< 0/*Visited[l].SatCount(game.Vars[0])*/ <<" boards and "<< SweepLine.NodeCount() <<" BDD nodes" << endl;

        sylvan_gc_no_resize();

        assert(SweepLine <= Game.NrStones[0][l]);
        // Retrograde in this sweep line
        int level = 0;
        Bdd Next[2] = { SweepLine & !Game.Turn[0], SweepLine & Game.Turn[0] };
        while (!Next[1].isZero() || !Next[0].isZero()) {
            Bdd Temp = Next[1].RelPrev(Game.PlayerMoveRelation(0), Game.RelationVars);
            Next[0] |= (Temp & Game.NrStones[0][l]) - SweepLine;
            SweepLine |= Next[0];
            cout << level++ << flush;

            Bdd    Exist = Next[0].RelPrev(Game.PlayerMoveRelation(1), Game.RelationVars) & NoEscape;
            Bdd    Exits =   Exist.RelNext(Game.PlayerMoveRelation(1), Game.RelationVars) - SweepLine;
            Temp = Exist -   Exits.RelPrev(Game.PlayerMoveRelation(1), Game.RelationVars);
            Next[1] = Temp - SweepLine;
            SweepLine |= Next[1];
            Next[0] = Bdd::bddZero();
            cout << ",  "<< flush;
        }
        cout << "." << endl;

        cout << "["<< setprecision(2) << wctime() - start <<"] FINAL BDD has "<< setprecision(0) << /*Final.SatCount(Game.Vars[0])*/0 <<" boards and "<< SweepLine.NodeCount() <<" BDD nodes" << endl;
        Lines.Push(SweepLine);
    }

    for (Lazy &L : Lines.m_data) {
        L.ToDisk();
    }

    return Lines.Top();
}

Bdd Successors(Awari &Game, vector<int> board, bool turn, size_t score) {
    Bdd Succs;
    for (int p = 0; p < Game.M; p++) {
        if (board[p] == 0) continue;
        if ((p >= Game.M2) != turn) continue;

        vector<int> succ(board);
        succ[p] = 0;
        int o = p;
        for (int i = 0; i < board[p]; i++) {
             o = (o + (o + 1 == p ? 2 : 1) ) % Game.M;
             succ[o] += 1;
        }
        int taken = 0;
        if ((o >= Game.M2) != turn) {
            int last = Game.M2 * !turn;
            for (int l = o; l >= last; l--) {
                if (succ[l] != 2 && succ[l] != 3) break;
                taken += succ[l];
                succ[l] = 0;
            }
        }
        bool legal = 0;
        for (int o = 0; o < Game.M; o++) {
            legal |= (p >= Game.M2) != (o >= Game.M2) && succ[o] != 0;
        }
        if (!legal) continue;

        Succs |= Game.CreateBoard(false, 1 - turn, score + taken, succ);
    }
    return Succs;
}

static int COUNTB = 0;
void Test(Awari &Game, vector<int> &board) {
    Bdd B = Game.CreateBoard(false, 0, 0, board);
    Bdd S = Successors(Game, board, 0, 0);
    Bdd R = B.RelNext(Game.MoveRelation, Game.RelationVars);
    //Bdd All = B & S;
    if (S != R) {
        Game.PrintBoard(B, "start");
        Game.PrintBoard(S, "Successors");
        cout << "-------------------- =!= ----------------------" << endl;
        Game.PrintBoard(R, "Relation");
        exit(1);
    }
    if (++COUNTB % 5000 == 0) {
        cout << "|" <<flush;
        sylvan_gc_no_resize();
    }
}


// Test all boards with less than 4 stonesi n each pit:
int bitsPerPit = 2;
void Test(Awari &Game) {
    vector<int> board(Game.M);

    for (uint64_t c = 0; c < 1ULL<< Game.M * bitsPerPit; c++) {
        uint64_t c2 = c;
        uint64_t mask = (1ULL << bitsPerPit) - 1;
        for (int p = 0; p < Game.M; p++) {
            board[p] = (c2 >>= 2) & mask;
        }
        Test(Game, board);
    }
    cout << "Test 1 DONE" << endl;
}

// test all board with k stones in total
vector<int> board(12);
void Test2(Awari &Game, int m, int k) {
    if (m == 1) {
        board[0] = k;
        Test(Game, board);
    } else {
        for (int i = 0; i <= k; i++) {
            board[m-1] = k - i;
            Test2(Game, m - 1, k);
        }
    }
    cout << "Test 2 DONE" << endl;
}

void usage() {
    cerr << "Provide a valid argument: "<< endl <<
                    "\t r/f/b \t= Retrograde / Forward / Backward, "<< endl <<
                    "\t s     \t= Sweepline, "<< endl <<
                    "\t d     \t= swap to Disk " << endl <<
                    "\t u     \t= Unit tests" << endl << endl;
    exit(1);
}

int main(int argc, const char *argv[])
{
    int ms[] = {2,  4,  6,  8, 10, 12};
    int ks[] = {8, 16, 24, 32, 40, 48};

    for (int i = 0; i < len(ms); i++) {
        int m = ms[i];
        int k = ks[i];
    
        cout.imbue(locale( locale::classic(), new MyNumPunct ) );

        // Init Lace
        lace_init(1, 0); // auto-detect number of workers, use a 1,000,000 size task queue
        lace_startup(0, NULL, NULL); // auto-detect program stack, do not use a callback for startup

        sylvan_set_sizes(1ULL<<23, 1ULL<<23, 1ULL<<21, 1UL<<21);
        //sylvan_set_sizes(1ULL<<23, 1ULL<<31, 1ULL<<21, 1ULL<<29);
        // total_size =  node_table_size * 24 + cache_table_size * 36
        //sylvan_set_sizes(1LL<<26, 1LL<<29, 1LL<<23, 1LL<<25);

        sylvan_init_package();
        sylvan_set_granularity(3); // granularity 3 is decent value for this small problem - 1 means "use cache for every operation"
        sylvan_init_bdd();

        // Before and after garbage collection, call gc_start and gc_end
        sylvan_gc_hook_pregc(TASK(gc_start));
        sylvan_gc_hook_postgc(TASK(gc_end));

        sylvan_gc_hook_main(TASK(sylvan_gc_normal_resize));

        char algo = 'r';    // default to retrograde
        bool disk = false;
        bool unit = false;
        bool sweep = true;
        for (int i = 1; i < argc; i++) {
            if (strlen(argv[i]) > 1) usage();
            switch (argv[i][0]) {
            case 'r': case 'f': case 'b':
                algo = argv[i][0];  break;
            case 's': sweep = true; break;
            case 'd': disk  = true; break;
            case 'u': unit  = true; break;
            default: usage();
            }
        }

        printf("Awari(M = %d, K = %d)\n", m, k);
        Awari Game = Awari(m, k);

        if (unit) {
            Test(Game);
            Test2(Game, Game.M, 10);
            exit(0);
        }

        char filename[256];
        sprintf(filename, "awari.%d.%d.bdd", m, k);
        // Write relation and states to file (see bottom of this file)
        FILE *fp = fopen(filename, "w");
        Game.WriteAsLTSminOutput(fp);
        fclose(fp);
        printf("Written to %s\n", filename);
    }

    return 0;
}



