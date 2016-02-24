#ifndef node_permute_h
#define node_permute_h


void node_permute(int* parents, int n);
void node_permute_cleanup();
void permute_data(double* vec, int n);
int* permute_node_subset_data(double* vec, int* indices, int n, int sz);

#endif
