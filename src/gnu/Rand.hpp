#pragma once

#include "RNG.h"
#include "Random.h"

struct Object;

class Rand {
  public:
    Rand(unsigned long seed = 0, int size = 55, Object* obj = nullptr);
    ~Rand();
    RNG* gen;
    Random* rand;
    Object* obj_;
};
