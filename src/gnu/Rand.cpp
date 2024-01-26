#include "Rand.hpp"

#include "NrnRandom123RNG.hpp"
#include "Normal.h"

Rand::Rand(std::uint32_t id1, std::uint32_t id2, Object* obj) {
    // printf("Rand\n");
    gen = new NrnRandom123(id1, id2);
    rand = new Normal(0., 1., gen);
    type_ = 0;
    obj_ = obj;
}

Rand::~Rand() {
    // printf("~Rand\n");
    delete gen;
    delete rand;
}

