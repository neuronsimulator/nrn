/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"

void nrn_finitialize(int setv, double v) {
	int i;
	NrnThread* _nt;

	t = 0.;
	dt2thread(-1.);
	nrn_thread_table_check();
	clear_event_queue();
	nrn_spike_exchange_init();
#if VECTORIZE
	nrn_play_init(); /* Vector.play */
        ///Play events should be executed before initializing events
	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The play events at t=0 */
	}
	if (setv) {
          for (_nt = nrn_threads; _nt < nrn_threads + nrn_nthread; ++_nt) {
            for (i=0; i < _nt->end; ++i) {
			VEC_V(i) = v;
            }
          }
        }
	for (i=0; i < nrn_nthread; ++i) {
		nrn_ba(nrn_threads + i, BEFORE_INITIAL);
	}
	/* the INITIAL blocks are ordered so that mechanisms that write
	   concentrations are after ions and before mechanisms that read
	   concentrations.
	*/
	/* the memblist list in NrnThread is already so ordered */
	for (i=0; i < nrn_nthread; ++i) {
		NrnThread* nt = nrn_threads + i;
		NrnThreadMembList* tml;
		for (tml = nt->tml; tml; tml = tml->next) {
			mod_f_t s = memb_func[tml->index].initialize;
			if (s) {
				(*s)(nt, tml->ml, tml->index);
			}
		}
	}
#endif

	init_net_events();
	for (i = 0; i < nrn_nthread; ++i) {
		nrn_ba(nrn_threads + i, AFTER_INITIAL);
	}
	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The INITIAL sent events at t=0 */
	}
        for (i=0; i < nrn_nthread; ++i) {
            setup_tree_matrix_minimal(nrn_threads + i);
        }
	for (i=0; i < nrn_nthread; ++i) {
		nrn_deliver_events(nrn_threads + i); /* The record events at t=0 */
	}
#if NRNMPI
	nrn_spike_exchange(nrn_threads);
#endif
}
