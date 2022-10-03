#pragma once
#include <iostream>


namespace nrn::test {
extern int MAX_PROCESSORS;
// generate a range starting from 1 and doubling until we reach the nof concurrent threads
inline auto make_available_threads_range() {
    auto nof_threads_range = std::vector<int>(1 + static_cast<int>(std::log2(MAX_PROCESSORS - 1)),
                                              1);
    std::generate(nof_threads_range.begin() + 1, nof_threads_range.end(), [n = 1]() mutable {
        return n *= 2;
    });
    return nof_threads_range;
}
}  // namespace nrn::test

// pass_cell_template is a string containing a template for a Cell with inserted pas mechanism
constexpr auto pass_cell_template = R"(
begintemplate PasCell
public init, topol, basic_shape, subsets, geom, biophys, geom_nseg, biophys_inhomo
public synlist, x, y, z, position, connect2target
public icl

public soma, dend
public all

objref synlist, icl

create soma, dend

proc init() {
  topol()
  subsets()
  geom()
  biophys()
  geom_nseg()
  synlist = new List()
  synapses()
  x = y = z = 0 // only change via position
  soma {icl = new IClamp(0.5)}
  icl.del = 1
  icl.dur = 3
  icl.amp = 0
}

proc topol() { local i
  connect dend(0), soma(1)
  basic_shape()
}
proc basic_shape() {
  soma {pt3dclear() pt3dadd(0, 0, 0, 1) pt3dadd(15, 0, 0, 1)}
  dend {pt3dclear() pt3dadd(15, 0, 0, 1) pt3dadd(90, 0, 0, 1)}
}

objref all
proc subsets() { local i
  objref all
  all = new SectionList()
    soma all.append()
    dend all.append()

}
proc geom() {
  forsec all {  }
   soma.L = 10
   dend.L = 1000
   soma.diam = 10
   dend.diam = 1
  dend {  }
}

proc geom_nseg() {
   dend { nseg = 100  }
}
proc biophys() {
  forsec all {
    Ra = 100
    cm = 1
    insert pas
      g_pas = 0.001
      e_pas = -65
  }
}
proc biophys_inhomo(){}
proc position() { local i
  soma for i = 0, n3d()-1 {
    pt3dchange(i, $1-x+x3d(i), $2-y+y3d(i), $3-z+z3d(i), diam3d(i))
  }
  x = $1  y = $2  z = $3
}
obfunc connect2target() { localobj nc //$o1 target point process, optional $o2 returned NetCon
  soma nc = new NetCon(&v(1), $o1)
  nc.threshold = 10
  if (numarg() == 2) { $o2 = nc } // for backward compatibility
  return nc
}
proc synapses() {}
endtemplate PasCell
)";

/**
 * @brief prun
 * requires a parallel context to be created before -> pc
 */
constexpr auto prun = R"(
load_file("nrngui.hoc")
load_file("stdlib.hoc")
func prun() {local runtime
	pc.set_maxstep(10)
	finitialize(-65)
	runtime=startsw()
	stdinit()
	batch_run(tstop, tstop)
	runtime=startsw()-runtime
	return runtime
}  
)";


// utility to create a given number of passive membrane cells (nof_cells)
std::string operator"" _pas_cells(unsigned long long nof_cells) {
    std::string cells = "ncell = " + std::to_string(nof_cells);
    cells += R"(
    objref cell[ncell]
    for i=0, ncell - 1 {
        cell[i] = new PasCell()
        cell[i].icl.amp = i/ncell
        cell[i].position(0, 100*i, 0)
    }
    )";
    return cells;
};
