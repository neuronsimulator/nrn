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

class Rand_random123 {
  public:
    Rand_random123() { gen = new RNG_random123(); }
    ~Rand_random123() { delete gen; delete rand;}
    RNG_random123* gen;
    Random_random123* rand;
    int type_;  // can do special things with some kinds of RNG
    // double* looks like random variable that gets changed on every fadvance
    Object* obj_;
};

#endif
