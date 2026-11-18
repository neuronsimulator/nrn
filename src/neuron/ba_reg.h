#pragma once

// Third argument of hoc_reg_ba. Before is 10 and After is 20. The block kind
// is Breakpoint 1, Solve 2, Initial 3, or Step 4. Only five sums are
// implemented: 11, 22, 13, 23, and 14. The BEFORE_* macros in membfunc.h are
// the bamech_[] slots after hoc_reg_ba translates this argument.
struct BAType {
    enum { Breakpoint = 1, Solve = 2, Initial = 3, Step = 4, Before = 10, After = 20 };
};
