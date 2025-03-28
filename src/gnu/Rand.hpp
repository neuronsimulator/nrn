#pragma once

#include "RNG.h"
#include "Random.h"

struct Object;

/* type_:
 * 0: unused
 * 1: unused
 * 2: MCellRan4
 * 3: unused
 * 4: Random123
 */
class Rand {
  public:
    Rand(unsigned long seed = 0, int size = 55, Object* obj = nullptr);
    ~Rand();
    RNG* gen;
    Random* rand;
    int type_;  // can do special things with some kinds of RNG
    // double* looks like random variable that gets changed on every fadvance
    Object* obj_;
};
