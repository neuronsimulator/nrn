#include "Rand.hpp"

#include "ACG.h"
#include "Normal.h"

Rand::Rand(unsigned long seed, int size, Object* obj) {
    // printf("Rand\n");
    gen = new ACG(seed, size);
    rand = new Normal(0., 1., gen);
    type_ = 0;
    obj_ = obj;
}

Rand::~Rand() {
    // printf("~Rand\n");
    delete gen;
    delete rand;
}

