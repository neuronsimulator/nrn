/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include <functional>

#include "coreneuron/coreneuron.hpp"
#include "coreneuron/nrnconf.h"
#include "coreneuron/apps/corenrn_parameters.hpp"
#include "coreneuron/sim/multicore.hpp"
#include "coreneuron/mpi/nrnmpi.h"
#include "coreneuron/sim/fast_imem.hpp"
#include "coreneuron/gpu/nrn_acc_manager.hpp"
#include "coreneuron/io/reports/nrnreport.hpp"
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/netpar.hpp"
#include "coreneuron/network/partrans.hpp"
#include "coreneuron/utils/nrnoc_aux.hpp"
#include "coreneuron/utils/progressbar/progressbar.hpp"
#include "coreneuron/utils/profile/profiler_interface.h"
#include "coreneuron/io/nrn2core_direct.h"

namespace coreneuron {

extern corenrn_parameters corenrn_param;
static void* nrn_fixed_step_thread(NrnThread*);
static void* nrn_fixed_step_group_thread(NrnThread*, int, int, int&);


namespace {

class ProgressBar final {
    progressbar* pbar;
    int current_step = 0;
    bool show;
    constexpr static int progressbar_update_steps = 5;

  public:
    ProgressBar(int nsteps)
        : show(nrnmpi_myid == 0 && !corenrn_param.is_quiet()) {
        if (show) {
            printf("\n");
            pbar = progressbar_new("psolve", nsteps);
        }
    }

    void update(int step, double time) {
        current_step = step;
        if (show && (current_step % progressbar_update_steps) == 0) {
            progressbar_update(pbar, current_step, time);
        }
    }

    void step(double time) {
        update(current_step + 1, time);
    }

    ~ProgressBar() {
        if (show) {
            progressbar_finish(pbar);
        }
    }
};

}  // unnamed namespace


void dt2thread(double adt) { /* copied from nrnoc/fadvance.c */
    if (adt != nrn_threads[0]._dt) {
        for (int i = 0; i < nrn_nthread; ++i) {
            NrnThread* nt = nrn_threads + i;
            nt->_t = t;
            nt->_dt = dt;
            if (secondorder) {
                nt->cj = 2.0 / dt;
            } else {
                nt->cj = 1.0 / dt;
            }
        }
    }
}

void nrn_fixed_step_minimal() { /* not so minimal anymore with gap junctions */
    if (t != nrn_threads->_t) {
        dt2thread(-1.);
    } else {
        dt2thread(dt);
    }
    nrn_thread_table_check();
    nrn_multithread_job(nrn_fixed_step_thread);
    if (nrn_have_gaps) {
        nrnmpi_v_transfer();
        nrn_multithread_job(nrn_fixed_step_lastpart);
    }
#if NRNMPI
    if (nrn_threads[0]._stop_stepping) {
        nrn_spike_exchange(nrn_threads);
    }
#endif

#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
    {
        Instrumentor::phase p("flush_reports");
        nrn_flush_reports(nrn_threads[0]._t);
    }
#endif
    t = nrn_threads[0]._t;
}

/* better cache efficiency since a thread can do an entire minimum delay
integration interval before joining
*/
/// --> Coreneuron


void nrn_fixed_single_steps_minimal(int total_sim_steps, double tstop) {
    ProgressBar progress_bar(total_sim_steps);
#if NRNMPI
    double updated_tstop = tstop - dt;
    nrn_assert(nrn_threads->_t <= tstop);
    // It may very well be the case that we do not advance at all
    while (nrn_threads->_t <= updated_tstop) {
#else
    double updated_tstop = tstop - .5 * dt;
    while (nrn_threads->_t < updated_tstop) {
#endif
        nrn_fixed_step_minimal();
        if (stoprun) {
            break;
        }
        progress_bar.step(nrn_threads[0]._t);
    }
}


void nrn_fixed_step_group_minimal(int total_sim_steps) {
    dt2thread(dt);
    nrn_thread_table_check();
    int step_group_n = total_sim_steps;
    int step_group_begin = 0;
    int step_group_end = 0;

    ProgressBar progress_bar(step_group_n);

    while (step_group_end < step_group_n) {
        nrn_multithread_job(nrn_fixed_step_group_thread,
                            step_group_n,
                            step_group_begin,
                            step_group_end);
#if NRNMPI
        nrn_spike_exchange(nrn_threads);
#endif

#if defined(ENABLE_BIN_REPORTS) || defined(ENABLE_SONATA_REPORTS)
        {
            Instrumentor::phase p("flush_reports");
            nrn_flush_reports(nrn_threads[0]._t);
        }
#endif
        if (stoprun) {
            break;
        }
        step_group_begin = step_group_end;
        progress_bar.update(step_group_end, nrn_threads[0]._t);
    }
    t = nrn_threads[0]._t;
}

static void* nrn_fixed_step_group_thread(NrnThread* nth,
                                         int step_group_max,
                                         int step_group_begin,
                                         int& step_group_end) {
    nth->_stop_stepping = 0;
    for (int i = step_group_begin; i < step_group_max; ++i) {
        nrn_fixed_step_thread(nth);
        if (nth->_stop_stepping) {
            if (nth->id == 0) {
                step_group_end = i + 1;
            }
            nth->_stop_stepping = 0;
            return nullptr;
        }
    }
    if (nth->id == 0) {
        step_group_end = step_group_max;
    }
    return nullptr;
}

void update(NrnThread* _nt) {
    double* vec_v = &(VEC_V(0));
    double* vec_rhs = &(VEC_RHS(0));
    int i2 = _nt->end;
#if defined(_OPENACC)
    int stream_id = _nt->stream_id;
#endif

    /* do not need to worry about linmod or extracellular*/
    if (secondorder) {
        // clang-format off

        #pragma acc parallel loop present(          \
            vec_v[0:i2], vec_rhs[0:i2])             \
            if (_nt->compute_gpu) async(stream_id)
        // clang-format on
        for (int i = 0; i < i2; ++i) {
            vec_v[i] += 2. * vec_rhs[i];
        }
    } else {
        // clang-format off

        #pragma acc parallel loop present(              \
                vec_v[0:i2], vec_rhs[0:i2])             \
                if (_nt->compute_gpu) async(stream_id)
        // clang-format on
        for (int i = 0; i < i2; ++i) {
            vec_v[i] += vec_rhs[i];
        }
    }

    // update_matrix_to_gpu(_nt);

    if (_nt->tml) {
        assert(_nt->tml->index == CAP);
        nrn_cur_capacitance(_nt, _nt->tml->ml, _nt->tml->index);
    }
    if (nrn_use_fast_imem) {
        nrn_calc_fast_imem(_nt);
    }
}

void nonvint(NrnThread* _nt) {
    if (nrn_have_gaps) {
        Instrumentor::phase p("gap-v-transfer");
        nrnthread_v_transfer(_nt);
    }
    errno = 0;

    Instrumentor::phase_begin("state-update");
    for (auto tml = _nt->tml; tml; tml = tml->next)
        if (corenrn.get_memb_func(tml->index).state) {
            mod_f_t s = corenrn.get_memb_func(tml->index).state;
            std::string ss("state-");
            ss += nrn_get_mechname(tml->index);
            {
                Instrumentor::phase p(ss.c_str());
                (*s)(_nt, tml->ml, tml->index);
            }
#ifdef DEBUG
            if (errno) {
                hoc_warning("errno set during calculation of states", nullptr);
            }
#endif
        }
    Instrumentor::phase_end("state-update");
}

void nrn_ba(NrnThread* nt, int bat) {
    for (auto tbl = nt->tbl[bat]; tbl; tbl = tbl->next) {
        mod_f_t f = tbl->bam->f;
        int type = tbl->bam->type;
        Memb_list* ml = tbl->ml;
        (*f)(nt, ml, type);
    }
}

void nrncore2nrn_send_init() {
    if (nrn2core_trajectory_values_ == nullptr) {
        // standalone execution : no callbacks
        return;
    }
    // if per time step transfer, need to call nrn_record_init() in NEURON.
    // if storing full trajectories in CoreNEURON, need to initialize
    // vsize for all the trajectory requests.
    (*nrn2core_trajectory_values_)(-1, 0, nullptr, 0.0);
    for (int tid = 0; tid < nrn_nthread; ++tid) {
        NrnThread& nt = nrn_threads[tid];
        if (nt.trajec_requests) {
            nt.trajec_requests->vsize = 0;
        }
    }
}

void nrncore2nrn_send_values(NrnThread* nth) {
    if (nrn2core_trajectory_values_ == nullptr) {
        // standalone execution : no callbacks
        return;
    }

    TrajectoryRequests* tr = nth->trajec_requests;
    if (tr) {
        if (tr->varrays) {  // full trajectories into Vector data
            int vs = tr->vsize++;
            // make sure we do not overflow the `varrays` buffers
            assert(vs < tr->bsize);

            // clang-format off

            #pragma acc parallel loop present(tr[0:1]) if(nth->compute_gpu) async(nth->stream_id)
            // clang-format on
            for (int i = 0; i < tr->n_trajec; ++i) {
                tr->varrays[i][vs] = *tr->gather[i];
            }
        } else if (tr->scatter) {  // scatter to NEURON and notify each step.
            nrn_assert(nrn2core_trajectory_values_);
            // Note that this is rather inefficient: we generate one `acc update
            // self` call for each `double` value (voltage, membrane current,
            // mechanism property, ...) that is being recorded, even though in most
            // cases these values will actually fall in a small number of contiguous
            // ranges in memory. A better solution, if the performance of this
            // branch becomes limiting, might be to offload this loop to the
            // device and populate some `scatter_values` array there and copy it
            // back with a single transfer. Note that the `async` clause here
            // should guarantee that correct values are reported even of
            // mechanism data that is updated in `nrn_state`. See also:
            // https://github.com/BlueBrain/CoreNeuron/issues/611
            for (int i = 0; i < tr->n_trajec; ++i) {
                double* gather_i = tr->gather[i];
                // clang-format off

                #pragma acc update self(gather_i[0:1]) if(nth->compute_gpu) async(nth->stream_id)
            }
            #pragma acc wait(nth->stream_id)
            // clang-format on
            for (int i = 0; i < tr->n_trajec; ++i) {
                *(tr->scatter[i]) = *(tr->gather[i]);
            }
            (*nrn2core_trajectory_values_)(nth->id, tr->n_pr, tr->vpr, nth->_t);
        }
    }
}

static void* nrn_fixed_step_thread(NrnThread* nth) {
    /* check thresholds and deliver all (including binqueue)
       events up to t+dt/2 */
    Instrumentor::phase_begin("timestep");

    {
        Instrumentor::phase p("deliver_events");
        deliver_net_events(nth);
    }

    nth->_t += .5 * nth->_dt;

    if (nth->ncell) {
#if defined(_OPENACC)
        int stream_id = nth->stream_id;
        /*@todo: do we need to update nth->_t on GPU: Yes (Michael, but can launch kernel) */
        // clang-format off

        #pragma acc update device(nth->_t) if (nth->compute_gpu) async(stream_id)
        #pragma acc wait(stream_id)
// clang-format on
#endif
        fixed_play_continuous(nth);

        {
            Instrumentor::phase p("setup_tree_matrix");
            setup_tree_matrix_minimal(nth);
        }

        {
            Instrumentor::phase p("matrix-solver");
            nrn_solve_minimal(nth);
        }

        {
            Instrumentor::phase p("second_order_cur");
            second_order_cur(nth, secondorder);
        }

        {
            Instrumentor::phase p("update");
            update(nth);
        }
    }
    if (!nrn_have_gaps) {
        nrn_fixed_step_lastpart(nth);
    }
    Instrumentor::phase_end("timestep");
    return nullptr;
}

void* nrn_fixed_step_lastpart(NrnThread* nth) {
    nth->_t += .5 * nth->_dt;

    if (nth->ncell) {
        /*@todo: do we need to update nth->_t on GPU */
        // clang-format off

        #pragma acc update device(nth->_t) if (nth->compute_gpu) async(nth->stream_id)
        #pragma acc wait(nth->stream_id)
        // clang-format on

        fixed_play_continuous(nth);
        nonvint(nth);
        nrncore2nrn_send_values(nth);
        nrn_ba(nth, AFTER_SOLVE);
        nrn_ba(nth, BEFORE_STEP);
    } else {
        nrncore2nrn_send_values(nth);
    }

    {
        Instrumentor::phase p("deliver_events");
        nrn_deliver_events(nth); /* up to but not past texit */
    }

    return nullptr;
}
}  // namespace coreneuron
