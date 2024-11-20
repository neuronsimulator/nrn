#include "Rand.hpp"

#include "NrnRandom123RNG.hpp"
#include "Normal.h"

Rand::Rand(unsigned long seed, int size, Object* obj) {
    // printf("Rand\n");
    gen = new NrnRandom123(seed, size);
    rand = new Normal(0., 1., gen);
    type_ = 4;
    obj_ = obj;
}

Rand::~Rand() {
    // printf("~Rand\n");
    delete gen;
    delete rand;
}

