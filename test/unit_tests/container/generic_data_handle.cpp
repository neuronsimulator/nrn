#include "neuron/container/generic_data_handle.hpp"
#include "neuron/model_data.hpp"

// Pull `Datum`.
#include "neuron/cache/model_data.hpp"

#include <catch2/catch.hpp>

using namespace neuron::container;


TEST_CASE("POINTER raw pointer") {
    double* v = (double*) malloc(sizeof(double));
    double* w = (double*) malloc(sizeof(double));

    double* const v_copy = v;
    double* const w_copy = w;

    // generic_data_handle v_gdh(v); // Not implemented.
    Datum v_gdh;
    v_gdh = v;
    REQUIRE(v_gdh.get<double*>() == v);

    // Attempt 1a: Assign `w` via `generic_data_handle.get<double*>()`. This is
    // bound to fail, because the pointer is returned by value. Hence this
    // assigns to an rvalue.
    // v_gdh.get<double*>() = w;
    //
    // Results in a straight up compilation error:
    //     test/unit_tests/container/generic_data_handle.cpp:23:21: error: lvalue required as left
    //     operand of assignment
    //     23 |   v_gdh.get<double*>() = w;
    //        |   ~~~~~~~~~~~~~~~~~~^~
    //
    // Attempt 1b: Store the rvalue.
    double* tmp = v_gdh.get<double*>();
    tmp = w;

    REQUIRE(v_gdh.get<double*>() == v);
    REQUIRE(v_gdh.get<double*>() != w);

    // Assign via `literal_value`. This will store the address referred to by `w`
    // in `v_gdh`.
    v_gdh.literal_value<double*>() = w;
    REQUIRE(v == v_copy);
    REQUIRE(v_gdh.get<double*>() == w);
    REQUIRE(v_gdh.literal_value<double*>() == w);

    free(v_copy);
    free(w_copy);
}

// Since we'll be testing code that requires the "upgrading"
// of raw pointer, i.e. the ability to find the pointer within a
// soa container. We need to use "known" soa containers, such as
// the ones in `neuron::model()`.

TEST_CASE("POINTER data_handle") {
    auto& model = neuron::model();
    auto& container = model.node_data();

    // These two object create a row each and keep it alive.
    auto row1_owner = Node::owning_handle{container};
    auto row2_owner = Node::owning_handle{container};

    // We'd like a data_handle to the both voltages.
    auto v1 = container.get_handle<Node::field::Voltage>(row1_owner.id(), 0);
    auto v2 = container.get_handle<Node::field::Voltage>(row2_owner.id(), 0);

    // Initialize with the wrong sign.
    *v1 = -1.0;
    *v2 = -2.0;

    double* const v1_ptr = &(*v1);
    double* const v2_ptr = &(*v2);

    // Let's fixup the sign, to prove the pointers are correct:
    *v1_ptr = 1.0;
    *v2_ptr = 2.0;

    REQUIRE(*v1 == 1.0);
    REQUIRE(*v2 == 2.0);

    // We set up everything such that the `soa` containers have two voltages.
    // We'll now study what happens to a Datum that refers to one of these.  In
    // generated code this datum is called `_ppval[0]`. The
    Datum* _ppval = (Datum*) malloc(sizeof(Datum));

    // Important: `v1_ptr` has type `double *`. Here, it's just
    // a pointer. The "promotion" to data_handle happens inside
    // the assignment operator.
    _ppval[0] = v1_ptr;

    REQUIRE(_ppval[0].get<double*>() == v1_ptr);
    REQUIRE(*_ppval[0].get<double*>() == *v1);

    // Time to shuffle the container.
    container.apply_reverse_permutation(std::vector<int>{1, 0});

    // Check that it shuffled correctly.
    REQUIRE(*v1 == 1.0);
    REQUIRE(*v1_ptr == 2.0);
    REQUIRE(*v2 == 2.0);
    REQUIRE(*v2_ptr == 1.0);

    // Check _ppval[0] is aware of the permutation.
    REQUIRE(*_ppval[0].get<double*>() == 1.0);
    REQUIRE(_ppval[0].get<double*>() == v2_ptr);

    // This is the code generated that's causing #2458. Since, `_ppval` refers to
    // a voltage and voltages are stored in `soa` containers, this "datum" is a
    // data_handle; not a raw pointer.
    REQUIRE_THROWS(_ppval[0].literal_value<void*>());

    // This is still a compiler error:
    // _ppval[0].get<double*>() = nullptr;

    free(_ppval);
}
