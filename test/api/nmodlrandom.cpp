// Exercises the public NMODL RANDOM wrappers for density mechanisms and point
// processes. The generated mechanisms provide one RANDOM variable each.
#include <array>
#include <iostream>

#include "neuronapi.h"

using std::cerr;
using std::endl;

extern "C" void _randomdensity_reg();
extern "C" void _randompoint_reg();
extern "C" void modl_reg() {
    _randomdensity_reg();
    _randompoint_reg();
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        cerr << "FAIL: " << message << endl;
    }
    return condition;
}

static bool set_seq(Object* random, double sequence) {
    nrn_double_push(sequence);
    nrn_method_call(random, nrn_method_symbol(random, "set_seq"), 1);
    Object* returned = nrn_object_pop();
    bool const ok = returned == random;
    nrn_object_unref(returned);
    return ok;
}

static double get_seq(Object* random) {
    nrn_method_call(random, nrn_method_symbol(random, "get_seq"), 0);
    return nrn_double_pop();
}

int main() {
    static std::array<const char*, 4> argv = {"nmodlrandom", "-nogui", "-nopython", nullptr};
    nrn_init(3, argv.data());

    bool ok = true;
    Section* cable = nrn_section_new("cable");
    nrn_nseg_set(cable, 3);
    Symbol* mechanism = nrn_symbol("randomdensity");
    Symbol* density_rng = nrn_symbol("rng_randomdensity");
    ok &= check(mechanism && density_rng, "density RANDOM symbols registered");
    nrn_mechanism_insert(cable, mechanism);

    Object* density_left = nrn_segment_nmodlrandom_get(cable, 1.0 / 6.0, density_rng);
    Object* density_right = nrn_segment_nmodlrandom_get(cable, 5.0 / 6.0, density_rng);
    ok &= check(density_left && density_right, "density RANDOM instances wrapped");
    ok &= check(set_seq(density_left, 17), "density setter returns its wrapper");
    ok &= check(set_seq(density_right, 29), "second density setter returns its wrapper");
    ok &= check(get_seq(density_left) == 17, "density wrapper addresses the requested segment");
    ok &= check(get_seq(density_right) == 29, "density RANDOM states remain independent");

    // Public wrappers return retained objects. Releasing one wrapper must not
    // affect the mechanism-owned RANDOM state, which a fresh wrapper can read.
    nrn_object_unref(density_left);
    density_left = nrn_segment_nmodlrandom_get(cable, 1.0 / 6.0, density_rng);
    ok &= check(density_left && get_seq(density_left) == 17,
                "density RANDOM survives wrapper release and rewrap");

    Section* empty = nrn_section_new("empty");
    ok &= check(!nrn_segment_nmodlrandom_get(nullptr, 0.5, density_rng), "null section rejected");
    ok &= check(!nrn_segment_nmodlrandom_get(cable, 0.5, nullptr), "null density symbol rejected");
    ok &= check(!nrn_segment_nmodlrandom_get(cable, 0.5, nrn_symbol("v")),
                "non-RANDOM density symbol rejected");
    ok &= check(!nrn_segment_nmodlrandom_get(cable, -0.1, density_rng),
                "out-of-range density position rejected");
    ok &= check(!nrn_segment_nmodlrandom_get(empty, 0.5, density_rng),
                "absent density mechanism rejected");

    nrn_section_push(cable);
    nrn_double_push(0.5);
    Object* point = nrn_object_new(nrn_symbol("RandomPoint"), 1);
    nrn_section_pop();
    Symbol* point_rng = nrn_method_symbol(point, "rng");
    Object* point_random = nrn_pntproc_nmodlrandom_get(point, point_rng);
    ok &= check(point_random, "point-process RANDOM instance wrapped");
    ok &= check(set_seq(point_random, 41), "point-process setter returns its wrapper");
    ok &= check(get_seq(point_random) == 41, "point-process wrapper preserves setter state");
    nrn_object_unref(point_random);
    point_random = nrn_pntproc_nmodlrandom_get(point, point_rng);
    ok &= check(point_random && get_seq(point_random) == 41,
                "point-process RANDOM survives wrapper release and rewrap");

    nrn_double_push(1);
    Object* vector = nrn_object_new(nrn_symbol("Vector"), 1);
    Object* unlocated = nrn_object_new(nrn_symbol("RandomPoint"), 0);
    ok &= check(!nrn_pntproc_nmodlrandom_get(nullptr, point_rng), "null point process rejected");
    ok &= check(!nrn_pntproc_nmodlrandom_get(point, nullptr), "null point symbol rejected");
    ok &= check(!nrn_pntproc_nmodlrandom_get(vector, point_rng), "non-point object rejected");
    ok &= check(!nrn_pntproc_nmodlrandom_get(point, density_rng),
                "RANDOM symbol from another mechanism rejected");
    ok &= check(!nrn_pntproc_nmodlrandom_get(unlocated, nrn_method_symbol(unlocated, "rng")),
                "unlocated point process rejected");

    nrn_object_unref(density_left);
    nrn_object_unref(density_right);
    nrn_object_unref(point_random);
    nrn_object_unref(point);
    nrn_object_unref(vector);
    nrn_object_unref(unlocated);
    return ok ? 0 : 1;
}
