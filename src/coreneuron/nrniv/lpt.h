#ifndef lpt_h
#define lpt_h

#include <vector>

std::vector<size_t>* lpt(size_t nbag, std::vector<size_t>& pieces, double* bal = NULL);

double load_balance(std::vector<size_t>&);
#endif
