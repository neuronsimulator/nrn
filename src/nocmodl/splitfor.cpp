#include <../../nrnconf.h>

#include "splitfor.h"

static bool split_cur_;
static bool split_solve_;

// return true if any splitfor code should be generated
bool splitfor() {
    static bool called;
    if (!called) {
        called = true;
        // if there is a breakpoint block with multiple currents and assigns
        // only range variables. No function calls.
        if (brkpnt_exists) {
            split_cur_ = true;
        }

        // think about split_solve_ later
        split_solve_ = false;
    }
    return split_cur_ || split_solve_;
}

//based on copies of code from noccout.cpp

void splitfor_current() {
    if (!split_cur_) { return; }
}

void splitfor_cur() {
    if (!split_cur_) { return; }
}

void splitfor_solve() {
    if (!split_solve_) { return; }
}


