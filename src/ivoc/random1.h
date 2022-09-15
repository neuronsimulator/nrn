#pragma once

#include "RNG.h"
#include "Random.h"

struct Object;

class Rand {
  public:
    Rand();
    ~Rand();
    RNG_random123* gen;
    Random_random123* rand;
    int type_;  // can do special things with some kinds of RNG
    // double* looks like random variable that gets changed on every fadvance
    Object* obj_;
};
