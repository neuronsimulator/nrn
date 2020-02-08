/*
Copyright (c) 2019, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef fast_imem_h
#define fast_imem_h

#include "coreneuron/sim/multicore.hpp"

namespace coreneuron {

/* Bool global variable to define if the fast_imem
 * calculations should be enabled.
 */
extern bool nrn_use_fast_imem;

/* Free memory allocated for the fast current membrane calculation.
 * Found in src/nrnoc/multicore.c in NEURON.
 */
void fast_imem_free();

/* fast_imem_alloc() wrapper.
 * Found in src/nrnoc/multicore.c in NEURON.
 */
void nrn_fast_imem_alloc();

/* Calculate the new values of rhs array at every timestep.
 * Found in src/nrnoc/fadvance.c in NEURON.
 */
void nrn_calc_fast_imem(NrnThread* _nt);

}  // namespace coreneuron
#endif //fast_imem_h
