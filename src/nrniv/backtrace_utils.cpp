#include "backward.hpp"

/*
 * small wrapper for backward-cpp to C
 */

extern "C" {

void backward_wrapper() {
    backward::StackTrace st; st.load_here(12);
    backward::Printer p; p.print(st);
}

}
