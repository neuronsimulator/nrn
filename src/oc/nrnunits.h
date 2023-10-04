#ifndef nrnunits_modern_h
#define nrnunits_modern_h

/**
 NMODL translated MOD files get unit constants typically from
 share/lib/nrnunits.lib. But there were other source files that
 hardcode some of the constants. Here we gather a few modern units into
 a single place (but, unfortunately, also in nrnunits.lib).

 These come from https://physics.nist.gov/cuu/Constants/index.html.
 Termed the "2018 CODATA recommended values", they became available
 on 20 May 2019 and replace the 2014 CODATA set.

 See oc/hoc_init.c, nrnoc/eion.c, nrniv/kschan.h
**/


constexpr double _electron_charge_codata2018 = 1.602176634e-19; /* coulomb exact*/
constexpr double _avogadro_number_codata2018 = 6.02214076e+23;  /* exact */
constexpr double _boltzmann_codata2018 = 1.380649e-23;          /* joule/K exact */
constexpr double _faraday_codata2018 = _electron_charge_codata2018 *
                                       _avogadro_number_codata2018; /* 96485.33212... coulomb/mol */
constexpr double _gasconstant_codata2018 =
    _boltzmann_codata2018 * _avogadro_number_codata2018; /* 8.314462618... joule/mol-K */

/* e/k in K/millivolt */
constexpr double _e_over_k_codata2018 = .001 * _electron_charge_codata2018 /
                                        _boltzmann_codata2018; /* 11.604518... K/mV */

#endif /* nrnunits_modern_h */
