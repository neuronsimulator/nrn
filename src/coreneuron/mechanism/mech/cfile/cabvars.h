/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

namespace coreneuron {

extern void capacitance_reg(void), _passive_reg(void),
#if EXTRACELLULAR
    extracell_reg_(void),
#endif
    _stim_reg(void), _hh_reg(void), _netstim_reg(void), _expsyn_reg(void), _exp2syn_reg(void),
    _svclmp_reg(void);

static void (*mechanism[])(void) = {/* type will start at 3 */
                                    capacitance_reg,
                                    _passive_reg,
#if EXTRACELLULAR
                                    /* extracellular requires special handling and must be type 5 */
                                    extracell_reg_,
#endif
                                    _stim_reg,
                                    _hh_reg,
                                    _expsyn_reg,
                                    _netstim_reg,
                                    _exp2syn_reg,
                                    _svclmp_reg,
                                    0};

}  // namespace coreneuron
