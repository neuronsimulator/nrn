/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once
namespace coreneuron {
namespace units {
/* NMODL translated MOD files get unit constants typically from
 * share/lib/nrnunits.lib. But there were other source files that hardcode
 * some of the constants. Here we gather a few modern units into a single place
 * (but, unfortunately, also in nrnunits.lib). Legacy units cannot be
 * gathered here because they can differ slightly from place to place.
 *
 * These come from https://physics.nist.gov/cuu/Constants/index.html.
 * Termed the "2018 CODATA recommended values", they became available
 * on 20 May 2019 and replace the 2014 CODATA set.
 *
 * See oc/hoc_init.c, nrnoc/eion.c, nrniv/kschan.h
 */
namespace detail {
constexpr double electron_charge{1.602176634e-19};  // coulomb exact
constexpr double avogadro_number{6.02214076e+23};   // exact
constexpr double boltzmann{1.380649e-23};           // joule/K exact
}  // namespace detail
constexpr double faraday{detail::electron_charge * detail::avogadro_number};  // 96485.33212...
                                                                              // coulomb/mol
constexpr double gasconstant{detail::boltzmann * detail::avogadro_number};    // 8.314462618...
                                                                              // joule/mol-K
}  // namespace units
}  // namespace coreneuron
