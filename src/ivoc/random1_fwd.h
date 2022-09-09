#pragma once
struct Object;
class Random;
class RNG;
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
