/// @file main.cpp
/// Executable codes for the tree search.

#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <NTL/RR.h>

#include "converter.h"

/// The structure holding the parameters of the current experiment
struct {
    int TaskSize    = 24;   ///< Task size (number of Knapsack items);
    int ElementSize = 64;   ///< Knapsack item size in bits;
    int ProcCount   = 8;    ///< ProcessorCount to emulate/use;
    int IterCount   = 100;  ///< Number of iterations to run;
    int RelativeTargetWeight   = -1;  ///< Relative target weight of knapsack vector;
    } cfg;

/// Experiment Start Time
time_t rawtime;
/// Experiment Start Time (local)
struct tm* timeinfo;

/// Experiment duration
clock_t clck;
/// Experiment duration (msec)
float clck_msec;

/// Current processor number
int ProcRank = 0;

/// Generate a big random number
NTL::ZZ BigRandom(int bits);

/// Print help info to the Console
void PrintHelp();
/// Print argument error to the Console
void PrintError(char* arg);

/// Calculate the weight of the tree branch rooted on the node with given mask
/// @param ts   Task Size (Knapsack Vector length);
/// @param mask Packing vector of the subtree root;
/// @return Node count for this subtree;
NTL::ZZ WeighBranch(int ts, NTL::vec_GF2 mask);

int main(int argc, char* argv[])
{
    /* Print the boilerplate */
    printf("National Research Nuclear University \"MEPhI\"\n"
           "(Moscow Engineering Physics Institute)\n\n"
           "=== EXACT ALGORITHMS FOR THE KNAPSACK PROBLEM ===\n"
           "======== ALGORITHM #2: TREE SEARCH ==============\n\n");

    if(argc <= 1) {PrintHelp(); return(0);}

    /* Read the input arguments */
    int mode = 0;
    for(int a=1; a<argc; a++)
    {
        if(mode == 1) {cfg.TaskSize             = atoi(argv[a]); mode = 0; continue;}
        if(mode == 2) {cfg.ElementSize          = atoi(argv[a]); mode = 0; continue;}
        if(mode == 3) {cfg.ProcCount            = atoi(argv[a]); mode = 0; continue;}
        if(mode == 4) {cfg.IterCount            = atoi(argv[a]); mode = 0; continue;}
        if(mode == 5) {cfg.RelativeTargetWeight = atoi(argv[a]); mode = 0; continue;}

        if(!strcmp(argv[a],"-n")) {mode = 1; continue;}
        if(!strcmp(argv[a],"-m")) {mode = 2; continue;}
        if(!strcmp(argv[a],"-p")) {mode = 3; continue;}
        if(!strcmp(argv[a],"-i")) {mode = 4; continue;}
        if(!strcmp(argv[a],"-r")) {mode = 5; continue;}

        PrintError(argv[a]);
        return(-1);
    }
    if(mode != 0) {PrintError(argv[argc-1]); return(-1);}

    /* Initialize the pseudorandom number generator */
    srand(clock() * time(NULL));

    /* Print the experiment parameters */
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    printf("Experiment parameters:\n"
           "---> Task size:       %i;\n"
           "---> Element size:    %i;\n"
           "---> Processor Count: %i;\n"
           "---> Iteration Count: %i;\n"
           "---> Fixed relative target weight, %: %i;\n"
           "---> Codebase Date:   17-10-2021;\n"     // TODO: Codebase date!!!
           "---> Experiment Date: %02i-%02i-%4i;\n",
           cfg.TaskSize,
           cfg.ElementSize,
           cfg.ProcCount,
           cfg.IterCount,
           cfg.RelativeTargetWeight,
           timeinfo->tm_mday,
           timeinfo->tm_mon+1,
           timeinfo->tm_year+1900);

    #ifdef _DEBUG
        printf("Experiment build: Debug;\n\n" );
    #else
        #ifdef _RELEASE
            printf("Experiment build: Release;\n\n" );
        #else
            printf("Experiment build: Unknown;\n\n" );
        #endif
    #endif


    /* Definition of the Knapsack Problem */
    NTL::vec_ZZ  knp;   //< The Knapsack vector (item weights);
    NTL::vec_GF2 pck;   //< The packing vector (1 = include item; 0 = don't);
    NTL::ZZ      w;     //< Target weight;

    /* Per-processor data */
    NTL::ZZ c;           //< Buffer for the current packing weight;

    /* Performance counters */
    NTL::ZZ solutions_total;    //< Counter for found solutions;
    NTL::ZZ nodes_total;        //< Counter for checked nodes (packings);

    /* Initialize the Knapsack Problem Instance */
    knp.SetLength(cfg.TaskSize,NTL::ZZ(0));  //< Initialize the Knapsack vector;
    pck.SetLength(cfg.TaskSize,NTL::GF2(0)); //< Initialize a packing vector;

    /// @section Linearization
    /// First, we have to do some precalculations for the linearization
    /// routines. The functions used here split the original tree into 8
    /// subtrees. Whenever tree size gets too small, it needs to split into
    /// 4 or 2 subtrees, too. The order of subtrees is known, so is their size.
    /// Therefore, at preliminary stage we calculate where each of the trees
    /// starts and build the static Domain Size Cache with this data.
    ///
    /// As soon as the initialization is done the pair of functions
    /// GetLiteralStringByNumber() and SetMaskByLiteralString() may be
    /// used to translate packing numbers into packing vectors, and
    /// and GetLiteralStringByMask() and GetNumberByLiteralString()
    /// to translate packing vectors into their respective numbers;
    DomainType* lit = (DomainType*)malloc((unsigned long)((cfg.TaskSize/3+3) *
                      sizeof(DomainType)));
    InitializeDomainSizeCache(cfg.TaskSize);

    /* Format the output table header */
    printf("ITER   |");
    printf("RELW, %%|");
#ifdef _DEBUG
    printf("Weight     |");
#endif
    for(int j=0; j<cfg.ProcCount; j++) printf("Time,ms|");
    printf("\n");
    printf("-------x");
    printf("-------x"); for(int j=0; j<cfg.ProcCount; j++) printf("-------x");
    printf("\n");

    for(int iter = 0; iter < cfg.IterCount; iter++)
    {
        /* Randomize the Knapsack Problem Instance */
        for(int j=0; j<cfg.TaskSize; j++)
            knp[j] = BigRandom(cfg.ElementSize - ceil( log(cfg.TaskSize)/log(2) ));

        /* Get the sum of all Knapsack elements */
        NTL::ZZ sum_ai = NTL::ZZ(0);
        for(int i=0; i<cfg.TaskSize;i++)
            NTL::add(sum_ai, sum_ai, knp.get(i));

        NTL::ZZ relw;
        if((cfg.RelativeTargetWeight < 0) || (cfg.RelativeTargetWeight > 100))
        {
#ifdef _DEBUG
            if(iter == 0)
            {
                printf("Wrong RelativeTargetWeight, using random weight;\n" );
            }                
#endif
            
            /* Generate a proper non-trivial target weight */
            w = 0;
            while((w<=0) || (w>=sum_ai))
                w = BigRandom(cfg.ElementSize);

            /* Calculate the relative target weight */
            relw = w * 100 / sum_ai;
        }
        else
        {
            // Set relative target weight and calculate target weight
            relw = NTL::ZZ(cfg.RelativeTargetWeight);            
            w = relw * sum_ai / 100;
        }

        /* Initialize an iteration */
        printf("I:%5i| ", iter);
        printf("%6i| ", NTL::to_uint(relw));
#ifdef _DEBUG
        printf("%10u| ", NTL::to_uint(w));
#endif
        solutions_total = 0;
        nodes_total = 0;

        /* Start the algorithm for each of the processors */
        for(ProcRank=0; ProcRank < cfg.ProcCount; ProcRank++)
        {
            /* Start the experiment clock */
            clck = clock();

            /// @section LoadBalancing Load Balancing
            /// Here we must split the task into (n) subtasks for each of the
            /// processors. Our input data is the number of processors (P) and
            /// the number of the current processor (ProcRank). We must generate
            /// and return the (ProcRank)-th subtask. Here we need to provide
            /// the first and the last packing number in the work area.

            /* Count the nodes in subtasks */
            NTL::ZZ InitialFragSize;
            NTL::power(InitialFragSize, 2, cfg.TaskSize);
            InitialFragSize /= cfg.ProcCount;

            /* Calculate the number of the first packing to check */
            NTL::ZZ frag_start; frag_start = ProcRank * InitialFragSize;
            /* Calculate the number of the last packing to check */
            NTL::ZZ frag_end;   frag_end = frag_start + InitialFragSize - 1;

            /* Set the current node to the start of the work area */
            NTL::ZZ CurrentNode = NTL::ZZ(frag_start);
            NTL::ZZ current_pool = NTL::ZZ(InitialFragSize);

            /* Buffer for storing the node count in a branch */
            NTL::ZZ branch_size = NTL::ZZ(0);

            /* Reset weight buffer */
            c = 0;

            /* Start the search */
            while (CurrentNode <= frag_end)
            {
                /// @section PackingVectorGeneration In this variation of the
                /// algorithm we every time construct the packing vector
                /// by its number, thus always invoking the linearization
                /// routine. This is clearly suboptimal.
                GetLiteralStringByNumber(cfg.TaskSize, lit, CurrentNode);
                SetMaskByLiteralString(cfg.TaskSize, &pck, lit);

                /// @section GettingPackingWeight Getting the Packing Weight
                /// Here we must calculate the weight (c) of the packing (pck)
                /// we have just generated. Here we can choose to use addition
                /// or multiplication modulo m, as well as calculate the weight
                /// (c) from scratch or using the previous value.
                c = 0;
                for (int i = cfg.TaskSize-1; i>=0; i--)
                    if(pck.get(i)==1)
                    {
                        NTL::add(c, c, knp.get(i));
                        //NTL::mul(c, c, knp.get(i));
                        //NTL::AddMod(c,c,k.knp.get(i));
                        //NTL::MulMod(c,c,k.knp.get(i));
                    };

                /// @section NodeProcessing Processing a Node
                /// Here we do everything we need to with the current packing.
                /// The order of the operations matter: the most common outcome
                /// should be the first to avoid excessive ifs. If the node is
                /// deficient (the weight is too low) then we just advance to
                /// the next node. If the node is excessive (too much weight)
                /// we determine the size of the branch we cut and advance the
                /// current node number by as much. If the node is a solution,
                /// we do the same, but also any solution processing we need.
                //nodes_total++;
                if(c < w)
                {
                    CurrentNode++;
                    current_pool--;
                }
                else if(c > w)
                {
                    branch_size = WeighBranch(cfg.TaskSize, pck);
                    current_pool -= branch_size;
                    CurrentNode += branch_size;
                }
                else if(c == w)
                {
                    solutions_total++;
                    branch_size = WeighBranch(cfg.TaskSize, pck);
                    current_pool -= branch_size;
                    CurrentNode += branch_size;
                }

            }

            /* Stop the timer */
            clck = clock() - clck;
            clck_msec = ((float)clck) / CLOCKS_PER_SEC * 1000.0;
            printf("%6.0f| ", clck_msec);
        }

        /* Finalize an iteration */
        printf("\n");
    }

    return(0);
}

NTL::ZZ BigRandom(int bits)
{
    NTL::ZZ ret; ret = 0;
    for(int a = 0; a<bits; a++)
        if(rand() >= floor(RAND_MAX / 2)) ret = ret * 2 + 1;
        else ret = ret * 2;
    return(ret);
}

void PrintHelp()
{
    printf("Command line switches:\n"
           "   -n [number]: Set task size (number of items);                    def:  24\n"
           "   -m [number]: Set element size (in bits);                         def:  64\n"
           "   -p [number]: Set processor count;                                def:   8\n"
           "   -i [number]: Set iterations count;                               def: 100\n"
           "   -r [number]: Set relative target weight of knapsack vector, %;   undef\n");
    return;
}

void PrintError(char* arg)
{
    printf("Invalid argument: %s;\n", arg);
    return;
}

NTL::ZZ WeighBranch(int ts, NTL::vec_GF2 mask)
{
	NTL::ZZ ret; ret = 1;
	for (int k = 0; mask[ts - k - 1] != NTL::GF2(1);)
	{
		ret *= 2;
		k++;
		if (k == ts) break;
	}
	return ret;
}
