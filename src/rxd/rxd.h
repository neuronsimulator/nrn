#include <pthread.h>

typedef void (*fptr)(void);

typedef struct {
	Grid_node* grid;
	int idx;
} ReactSet;

typedef struct {
	ReactSet* onset;
	ReactSet* offset;
} ReactGridData;

typedef struct {
        double* copyFrom;
        long copyTo;
} AdiLineData;

typedef struct {
    int start, stop;
    AdiLineData* vals;
    double* state;
    Grid_node g;
    int sizej;
    AdiLineData (*dg_adi_dir)(Grid_node, double, int, int, double const *, double*);
    double* scratchpad;
} AdiGridData;

extern void set_num_threads(int);
extern int get_num_threads(void);
extern int dg_adi_vol(Grid_node g);
extern int dg_adi_tort(Grid_node g);
extern void dg_transfer_data(AdiLineData * const, double* const, int const, int const, int const);
extern void run_threaded_dg_adi(AdiGridData*, pthread_t*, const int, const int, Grid_node, double*, AdiLineData*, AdiLineData (*dg_adi_dir)(Grid_node, double, int, int, double const *, double*), const int n);

extern ReactGridData* create_threaded_reactions(void);
extern void* do_reactions(void*);
