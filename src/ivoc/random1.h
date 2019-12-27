#ifndef random1_h
#define random1_h

#include "RNG.h"
#include "Random.h"

struct Object;

class Rand {
  public:
    Rand(unsigned long seed = 0, int size = 55, Object* obj = NULL);
    ~Rand();
    RNG* gen;
    Random* rand;
    int type_;  // can do special things with some kinds of RNG
    // double* looks like random variable that gets changed on every fadvance
    Object* obj_;
};

#endif
