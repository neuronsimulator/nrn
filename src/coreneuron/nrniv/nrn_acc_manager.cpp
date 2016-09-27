#include <queue>
#include <utility>

#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/netcon.h"
#include "coreneuron/nrniv/nrn_acc_manager.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/vrecitem.h"
#include "coreneuron/nrniv/profiler_interface.h"
#include "coreneuron/nrniv/cellorder.h"
#include "coreneuron/nrniv/cuda_profile.h"
#include "coreneuron/scopmath_core/newton_struct.h"

#ifdef _OPENACC
#include <openacc.h>
#endif

#ifdef CRAYPAT
#include <pat_api.h>
#endif

extern InterleaveInfo* interleave_info;
void copy_ivoc_vect_to_device(IvocVect*& iv, IvocVect*& div);

/* note: threads here are corresponding to global nrn_threads array */
void setup_nrnthreads_on_device(NrnThread* threads, int nthreads) {
#ifdef _OPENACC

    if (nthreads <= 0) {
        printf("\n Warning: No threads to copy on GPU! ");
        return;
    }

    /** @todo: currently only checking nvidia gpu */
    int num_gpus = acc_get_num_devices(acc_device_nvidia);
    if (num_gpus == 0) {
        printf("\n WARNING: Enabled GPU execution but couldn't find NVIDIA GPU! \n");
    }

    int i;
    NrnThread* d_threads;

    /* @todo: why dt is not setup at this moment? */
    for (i = 0; i < nthreads; i++) {
        (threads + i)->_dt = dt;
        /* this thread will be computed on GPU */
        (threads + i)->compute_gpu = 1;
    }

    /* -- copy NrnThread to device. this needs to be contigious vector because offset is used to
     * find
     * corresponding NrnThread using Point_process in NET_RECEIVE block
     */
    d_threads = (NrnThread*)acc_copyin(threads, sizeof(NrnThread) * nthreads);

    printf("\n --- Copying to Device! --- ");

    if (interleave_info == NULL) {
        printf("\n Warning: No permutation data? Required for linear algebra!");
    }

    /* pointers for data struct on device, starting with d_ */

    for (i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;      // NrnThread on host
        NrnThread* d_nt = d_threads + i;  // NrnThread on device

        if (nt->end <= 0) {
            // this is an empty thread
            continue;
        }

        double* d__data;  // nrn_threads->_data on device

        printf("\n -----------COPYING %d'th NrnThread TO DEVICE --------------- \n", i);
#if defined(CUDA_PROFILING)
        print_gpu_memory_usage();
#endif

        /* -- copy _data to device -- */

        /*copy all double data for thread */
        d__data = (double*)acc_copyin(nt->_data, nt->_ndata * sizeof(double));

        /* Here is the example of using OpenACC data enter/exit
         * Remember that we are not allowed to use nt->_data but we have to use:
         *      double *dtmp = nt->_data;  // now use dtmp!
                #pragma acc enter data copyin(dtmp[0:nt->_ndata]) async(nt->stream_id)
                #pragma acc wait(nt->stream_id)
         */

        /*update d_nt._data to point to device copy */
        acc_memcpy_to_device(&(d_nt->_data), &d__data, sizeof(double*));

        /* -- setup rhs, d, a, b, v, node_aread to point to device copy -- */
        double* dptr;

        /* for padding, we have to recompute ne */
        int ne = nrn_soa_padded_size(nt->end, 0);

        dptr = d__data + 0 * ne;
        acc_memcpy_to_device(&(d_nt->_actual_rhs), &(dptr), sizeof(double*));

        dptr = d__data + 1 * ne;
        acc_memcpy_to_device(&(d_nt->_actual_d), &(dptr), sizeof(double*));

        dptr = d__data + 2 * ne;
        acc_memcpy_to_device(&(d_nt->_actual_a), &(dptr), sizeof(double*));

        dptr = d__data + 3 * ne;
        acc_memcpy_to_device(&(d_nt->_actual_b), &(dptr), sizeof(double*));

        dptr = d__data + 4 * ne;
        acc_memcpy_to_device(&(d_nt->_actual_v), &(dptr), sizeof(double*));

        dptr = d__data + 5 * ne;
        acc_memcpy_to_device(&(d_nt->_actual_area), &(dptr), sizeof(double*));

        int* d_v_parent_index = (int*)acc_copyin(nt->_v_parent_index, nt->end * sizeof(int));
        acc_memcpy_to_device(&(d_nt->_v_parent_index), &(d_v_parent_index), sizeof(int*));

        /* nt._ml_list is used in NET_RECEIVE block and should have valid membrane list id*/
        Memb_list** d_ml_list =
            (Memb_list**)acc_copyin(nt->_ml_list, n_memb_func * sizeof(Memb_list*));
        acc_memcpy_to_device(&(d_nt->_ml_list), &(d_ml_list), sizeof(Memb_list**));

        /* -- copy NrnThreadMembList list ml to device -- */

        NrnThreadMembList* tml;
        NrnThreadMembList* d_tml;
        NrnThreadMembList* d_last_tml;

        Memb_list* d_ml;
        int first_tml = 1;
        size_t offset = 6 * ne;

        for (tml = nt->tml; tml; tml = tml->next) {
            /*copy tml to device*/
            /*QUESTIONS: does tml will point to NULL as in host ? : I assume so!*/
            d_tml = (NrnThreadMembList*)acc_copyin(tml, sizeof(NrnThreadMembList));

            /*first tml is pointed by nt */
            if (first_tml) {
                acc_memcpy_to_device(&(d_nt->tml), &d_tml, sizeof(NrnThreadMembList*));
                first_tml = 0;
            } else {
                /*rest of tml forms linked list */
                acc_memcpy_to_device(&(d_last_tml->next), &d_tml, sizeof(NrnThreadMembList*));
            }

            // book keeping for linked-list
            d_last_tml = d_tml;

            /* now for every tml, there is a ml. copy that and setup pointer */
            d_ml = (Memb_list*)acc_copyin(tml->ml, sizeof(Memb_list));
            acc_memcpy_to_device(&(d_tml->ml), &d_ml, sizeof(Memb_list*));

            /* setup nt._ml_list */
            acc_memcpy_to_device(&(d_ml_list[tml->index]), &d_ml, sizeof(Memb_list*));

            int type = tml->index;
            int n = tml->ml->nodecount;
            int szp = nrn_prop_param_size_[type];
            int szdp = nrn_prop_dparam_size_[type];
            int is_art = nrn_is_artificial_[type];
            int layout = nrn_mech_data_layout_[type];

            offset = nrn_soa_padded_size(offset, layout);

            dptr = d__data + offset;

            acc_memcpy_to_device(&(d_ml->data), &(dptr), sizeof(double*));

            offset += nrn_soa_padded_size(n, layout) * szp;

            if (!is_art) {
                int* d_nodeindices = (int*)acc_copyin(tml->ml->nodeindices, sizeof(int) * n);
                acc_memcpy_to_device(&(d_ml->nodeindices), &d_nodeindices, sizeof(int*));
            }

            if (szdp) {
                int pcnt = nrn_soa_padded_size(n, layout) * szdp;
                int* d_pdata = (int*)acc_copyin(tml->ml->pdata, sizeof(int) * pcnt);
                acc_memcpy_to_device(&(d_ml->pdata), &d_pdata, sizeof(int*));
            }

            int ts = memb_func[type].thread_size_;
            if (ts) {
                ThreadDatum* td =
                    (ThreadDatum*)acc_copyin(tml->ml->_thread, ts * sizeof(ThreadDatum));
                acc_memcpy_to_device(&(d_ml->_thread), &td, sizeof(ThreadDatum*));
            }

            NetReceiveBuffer_t *nrb, *d_nrb;
            int *d_weight_index, *d_pnt_index, *d_displ, *d_nrb_index;
            double *d_nrb_t, *d_nrb_flag;

            // net_receive buffer associated with mechanism
            nrb = tml->ml->_net_receive_buffer;

            // if net receive buffer exist for mechanism
            if (nrb) {
                d_nrb = (NetReceiveBuffer_t*)acc_copyin(nrb, sizeof(NetReceiveBuffer_t));
                acc_memcpy_to_device(&(d_ml->_net_receive_buffer), &d_nrb,
                                     sizeof(NetReceiveBuffer_t*));

                d_pnt_index = (int*)acc_copyin(nrb->_pnt_index, sizeof(int) * nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_pnt_index), &d_pnt_index, sizeof(int*));

                d_weight_index = (int*)acc_copyin(nrb->_weight_index, sizeof(int) * nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_weight_index), &d_weight_index, sizeof(int*));

                d_nrb_t = (double*)acc_copyin(nrb->_nrb_t, sizeof(double) * nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_nrb_t), &d_nrb_t, sizeof(double*));

                d_nrb_flag = (double*)acc_copyin(nrb->_nrb_flag, sizeof(double) * nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_nrb_flag), &d_nrb_flag, sizeof(double*));

                d_displ = (int*)acc_copyin(nrb->_displ, sizeof(int) * (nrb->_size + 1));
                acc_memcpy_to_device(&(d_nrb->_displ), &d_displ, sizeof(int*));

                d_nrb_index = (int*)acc_copyin(nrb->_nrb_index, sizeof(int) * (nrb->_size + 1));
                acc_memcpy_to_device(&(d_nrb->_nrb_index), &d_nrb_index, sizeof(int*));
            }

            /* copy NetSendBuffer_t on to GPU */
            NetSendBuffer_t* nsb;
            nsb = tml->ml->_net_send_buffer;

            if (nsb) {
                NetSendBuffer_t* d_nsb;
                int* d_iptr;
                double* d_dptr;

                d_nsb = (NetSendBuffer_t*)acc_copyin(nsb, sizeof(NetSendBuffer_t));
                acc_memcpy_to_device(&(d_ml->_net_send_buffer), &d_nsb, sizeof(NetSendBuffer_t*));

                d_iptr = (int*)acc_copyin(nsb->_sendtype, sizeof(int) * nsb->_size);
                acc_memcpy_to_device(&(d_nsb->_sendtype), &d_iptr, sizeof(int*));

                d_iptr = (int*)acc_copyin(nsb->_vdata_index, sizeof(int) * nsb->_size);
                acc_memcpy_to_device(&(d_nsb->_vdata_index), &d_iptr, sizeof(int*));

                d_iptr = (int*)acc_copyin(nsb->_pnt_index, sizeof(int) * nsb->_size);
                acc_memcpy_to_device(&(d_nsb->_pnt_index), &d_iptr, sizeof(int*));

                d_iptr = (int*)acc_copyin(nsb->_weight_index, sizeof(int) * nsb->_size);
                acc_memcpy_to_device(&(d_nsb->_weight_index), &d_iptr, sizeof(int*));

                d_dptr = (double*)acc_copyin(nsb->_nsb_t, sizeof(double) * nsb->_size);
                acc_memcpy_to_device(&(d_nsb->_nsb_t), &d_dptr, sizeof(double*));

                d_dptr = (double*)acc_copyin(nsb->_nsb_flag, sizeof(double) * nsb->_size);
                acc_memcpy_to_device(&(d_nsb->_nsb_flag), &d_dptr, sizeof(double*));
            }
        }

        if (nt->shadow_rhs_cnt) {
            double* d_shadow_ptr;

            int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);

            /* copy shadow_rhs to device and fix-up the pointer */
            d_shadow_ptr = (double*)acc_copyin(nt->_shadow_rhs, pcnt * sizeof(double));
            acc_memcpy_to_device(&(d_nt->_shadow_rhs), &d_shadow_ptr, sizeof(double*));

            /* copy shadow_d to device and fix-up the pointer */
            d_shadow_ptr = (double*)acc_copyin(nt->_shadow_d, pcnt * sizeof(double));
            acc_memcpy_to_device(&(d_nt->_shadow_d), &d_shadow_ptr, sizeof(double*));
        }

        if (nt->n_pntproc) {
            /* copy Point_processes array and fix the pointer to execute net_receive blocks on GPU
             */
            Point_process* pntptr =
                (Point_process*)acc_copyin(nt->pntprocs, nt->n_pntproc * sizeof(Point_process));
            acc_memcpy_to_device(&(d_nt->pntprocs), &pntptr, sizeof(Point_process*));
        }

        if (nt->n_weight) {
            /* copy weight vector used in NET_RECEIVE which is pointed by netcon.weight */
            double* d_weights = (double*)acc_copyin(nt->weights, sizeof(double) * nt->n_weight);
            acc_memcpy_to_device(&(d_nt->weights), &d_weights, sizeof(double*));
        }

        if (nt->_nvdata) {
            /* copy vdata which is setup in bbcore_read. This contains cuda allocated
             * nrnran123_State * */
            void** d_vdata = (void**)acc_copyin(nt->_vdata, sizeof(void*) * nt->_nvdata);
            acc_memcpy_to_device(&(d_nt->_vdata), &d_vdata, sizeof(void**));
        }

        if (nt->n_presyn) {
            /* copy presyn vector used for spike exchange, note we have added new PreSynHelper due
             * to issue
             * while updating PreSyn objects which has virtual base class. May be this is issue due
             * to
             * VTable and alignment */
            PreSynHelper* d_presyns_helper =
                (PreSynHelper*)acc_copyin(nt->presyns_helper, sizeof(PreSynHelper) * nt->n_presyn);
            acc_memcpy_to_device(&(d_nt->presyns_helper), &d_presyns_helper, sizeof(PreSynHelper*));
            PreSyn* d_presyns = (PreSyn*)acc_copyin(nt->presyns, sizeof(PreSyn) * nt->n_presyn);
            acc_memcpy_to_device(&(d_nt->presyns), &d_presyns, sizeof(PreSyn*));
        }

        if (nt->_net_send_buffer_size) {
            /* copy send_receive buffer */
            int* d_net_send_buffer =
                (int*)acc_copyin(nt->_net_send_buffer, sizeof(int) * nt->_net_send_buffer_size);
            acc_memcpy_to_device(&(d_nt->_net_send_buffer), &d_net_send_buffer, sizeof(int*));
        }

        if (nt->n_vecplay) {
            /* copy VecPlayContinuous instances */

            printf("\n Warning: VectorPlay used but NOT implemented on GPU! ");

            /** just empty containers */
            void** d_vecplay = (void**)acc_copyin(nt->_vecplay, sizeof(void*) * nt->n_vecplay);
            acc_memcpy_to_device(&(d_nt->_vecplay), &d_vecplay, sizeof(void**));

            for (int i = 0; i < nt->n_vecplay; i++) {
                VecPlayContinuous* vecplay_instance = (VecPlayContinuous*)nt->_vecplay[i];

                /** just VecPlayContinuous object */
                void* d_p = (void*)acc_copyin(vecplay_instance, sizeof(VecPlayContinuous));
                acc_memcpy_to_device(&(d_vecplay[i]), &d_p, sizeof(void*));

                VecPlayContinuous* d_vecplay_instance = (VecPlayContinuous*)d_p;

                /** copy y_, t_ and discon_indices_ */
                copy_ivoc_vect_to_device(vecplay_instance->y_, d_vecplay_instance->y_);
                copy_ivoc_vect_to_device(vecplay_instance->t_, d_vecplay_instance->t_);
                copy_ivoc_vect_to_device(vecplay_instance->discon_indices_,
                                         d_vecplay_instance->discon_indices_);

                /** copy PlayRecordEvent : todo: verify this */
                PlayRecordEvent* d_e_ =
                    (PlayRecordEvent*)acc_copyin(vecplay_instance->e_, sizeof(PlayRecordEvent));
                acc_memcpy_to_device(&(d_e_->plr_), &d_vecplay_instance,
                                     sizeof(VecPlayContinuous*));
                acc_memcpy_to_device(&(d_vecplay_instance->e_), &d_e_, sizeof(PlayRecordEvent*));

                /** copy pd_ : note that it's pointer inside ml->data and hence data itself is
                 * already on GPU */
                double* d_pd_ = (double*)acc_deviceptr(vecplay_instance->pd_);
                acc_memcpy_to_device(&(d_vecplay_instance->pd_), &d_pd_, sizeof(double*));
            }
        }

        if (nt->_permute) {
            if (use_interleave_permute == 1) {
                /* todo: not necessary to setup pointers, just copy it */
                InterleaveInfo* info = interleave_info + i;
                InterleaveInfo* d_info = (InterleaveInfo*)acc_copyin(info, sizeof(InterleaveInfo));
                int* d_ptr = NULL;

                d_ptr = (int*)acc_copyin(info->stride, sizeof(int) * (info->nstride + 1));
                acc_memcpy_to_device(&(d_info->stride), &d_ptr, sizeof(int*));

                d_ptr = (int*)acc_copyin(info->firstnode, sizeof(int) * nt->ncell);
                acc_memcpy_to_device(&(d_info->firstnode), &d_ptr, sizeof(int*));

                d_ptr = (int*)acc_copyin(info->lastnode, sizeof(int) * nt->ncell);
                acc_memcpy_to_device(&(d_info->lastnode), &d_ptr, sizeof(int*));

                d_ptr = (int*)acc_copyin(info->cellsize, sizeof(int) * nt->ncell);
                acc_memcpy_to_device(&(d_info->cellsize), &d_ptr, sizeof(int*));

            } else if (use_interleave_permute == 2) {
                /* todo: not necessary to setup pointers, just copy it */
                InterleaveInfo* info = interleave_info + i;
                InterleaveInfo* d_info = (InterleaveInfo*)acc_copyin(info, sizeof(InterleaveInfo));
                int* d_ptr = NULL;

                d_ptr = (int*)acc_copyin(info->stride, sizeof(int) * info->nstride);
                acc_memcpy_to_device(&(d_info->stride), &d_ptr, sizeof(int*));

                d_ptr = (int*)acc_copyin(info->firstnode, sizeof(int) * (info->nwarp + 1));
                acc_memcpy_to_device(&(d_info->firstnode), &d_ptr, sizeof(int*));

                d_ptr = (int*)acc_copyin(info->lastnode, sizeof(int) * (info->nwarp + 1));
                acc_memcpy_to_device(&(d_info->lastnode), &d_ptr, sizeof(int*));

                d_ptr = (int*)acc_copyin(info->stridedispl, sizeof(int) * (info->nwarp + 1));
                acc_memcpy_to_device(&(d_info->stridedispl), &d_ptr, sizeof(int*));

                d_ptr = (int*)acc_copyin(info->cellsize, sizeof(int) * info->nwarp);
                acc_memcpy_to_device(&(d_info->cellsize), &d_ptr, sizeof(int*));
            } else {
                printf("\n ERROR: only --cell_permute = [12] implemented");
                abort();
            }
        } else {
            printf("\n WARNING: NrnThread %d not permuted, error for linear algebra?", i);
        }

        printf("\n Compute thread on GPU? : %s, Stream : %d\n", (nt->compute_gpu) ? "Yes" : "No",
               nt->stream_id);
    }

    if (nrn_ion_global_map_size) {
        double** d_data =
            (double**)acc_copyin(nrn_ion_global_map, sizeof(double*) * nrn_ion_global_map_size);
        for (int j = 0; j < nrn_ion_global_map_size; j++) {
            if (nrn_ion_global_map[j]) {
                /* @todo: fix this constant size 3 :( */
                double* d_mechmap = (double*)acc_copyin(nrn_ion_global_map[j], 3 * sizeof(double));
                acc_memcpy_to_device(&(d_data[j]), &d_mechmap, sizeof(double*));
            }
        }
    }
#else
    (void)threads;
    (void)nthreads;
#endif
}

void copy_ivoc_vect_to_device(IvocVect*& iv, IvocVect*& div) {
#ifdef _OPENACC
    if (iv) {
        IvocVect* d_iv = (IvocVect*)acc_copyin(iv, sizeof(IvocVect));
        acc_memcpy_to_device(&div, &d_iv, sizeof(IvocVect*));

        size_t n = iv->size();
        if (n) {
            double* d_data = (double*)acc_copyin(iv->data(), sizeof(double) * n);
            acc_memcpy_to_device(&(d_iv->data_), &d_data, sizeof(double*));
        }
    }
#else
    (void)iv;
    (void)div;
#endif
}

void realloc_net_receive_buffer(NrnThread* nt, Memb_list* ml) {
    NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;
    if (!nrb) {
        return;
    }

#ifdef _OPENACC
    if (nt->compute_gpu) {
        // free existing vectors in buffers on gpu
        acc_delete(nrb->_pnt_index, nrb->_size * sizeof(int));
        acc_delete(nrb->_weight_index, nrb->_size * sizeof(int));
        acc_delete(nrb->_nrb_t, nrb->_size * sizeof(double));
        acc_delete(nrb->_nrb_flag, nrb->_size * sizeof(double));
        acc_delete(nrb->_displ, (nrb->_size + 1) * sizeof(int));
        acc_delete(nrb->_nrb_index, nrb->_size * sizeof(int));
    }
#endif

    // Reallocate host
    nrb->_size *= 2;
    nrb->_pnt_index = (int*)erealloc(nrb->_pnt_index, nrb->_size * sizeof(int));
    nrb->_weight_index = (int*)erealloc(nrb->_weight_index, nrb->_size * sizeof(int));
    nrb->_nrb_t = (double*)erealloc(nrb->_nrb_t, nrb->_size * sizeof(double));
    nrb->_nrb_flag = (double*)erealloc(nrb->_nrb_flag, nrb->_size * sizeof(double));
    nrb->_displ = (int*)erealloc(nrb->_displ, (nrb->_size + 1) * sizeof(int));
    nrb->_nrb_index = (int*)erealloc(nrb->_nrb_index, nrb->_size * sizeof(int));

#ifdef _OPENACC
    if (nt->compute_gpu) {
        int *d_weight_index, *d_pnt_index, *d_displ, *d_nrb_index;
        double *d_nrb_t, *d_nrb_flag;

        // update device copy
        acc_update_device(nrb, sizeof(NetReceiveBuffer_t));

        NetReceiveBuffer_t* d_nrb = (NetReceiveBuffer_t*)acc_deviceptr(nrb);

        // recopy the vectors in the buffer
        d_pnt_index = (int*)acc_copyin(nrb->_pnt_index, sizeof(int) * nrb->_size);
        acc_memcpy_to_device(&(d_nrb->_pnt_index), &d_pnt_index, sizeof(int*));

        d_weight_index = (int*)acc_copyin(nrb->_weight_index, sizeof(int) * nrb->_size);
        acc_memcpy_to_device(&(d_nrb->_weight_index), &d_weight_index, sizeof(int*));

        d_nrb_t = (double*)acc_copyin(nrb->_nrb_t, sizeof(double) * nrb->_size);
        acc_memcpy_to_device(&(d_nrb->_nrb_t), &d_nrb_t, sizeof(double*));

        d_nrb_flag = (double*)acc_copyin(nrb->_nrb_flag, sizeof(double) * nrb->_size);
        acc_memcpy_to_device(&(d_nrb->_nrb_flag), &d_nrb_flag, sizeof(double*));

        d_displ = (int*)acc_copyin(nrb->_displ, sizeof(int) * (nrb->_size + 1));
        acc_memcpy_to_device(&(d_nrb->_displ), &d_displ, sizeof(int*));

        d_nrb_index = (int*)acc_copyin(nrb->_nrb_index, sizeof(int) * nrb->_size);
        acc_memcpy_to_device(&(d_nrb->_nrb_index), &d_nrb_index, sizeof(int*));
    }
#endif
}

typedef std::pair<int, int> NRB_P;

struct comp {
    bool operator()(const NRB_P& a, const NRB_P& b) {
        if (a.first == b.first) {
            return a.second > b.second;  // same instances in original net_receive order
        }
        return a.first > b.first;
    }
};

static void net_receive_buffer_order(NetReceiveBuffer_t* nrb) {
    if (nrb->_cnt == 0) {
        nrb->_displ_cnt = 0;
        return;
    }

    std::priority_queue<NRB_P, std::vector<NRB_P>, comp> nrbq;

    for (int i = 0; i < nrb->_cnt; ++i) {
        nrbq.push(NRB_P(nrb->_pnt_index[i], i));
    }

    int displ_cnt = 0;
    int index_cnt = 0;
    int last_instance_index = -1;
    nrb->_displ[0] = 0;

    while (!nrbq.empty()) {
        const NRB_P& p = nrbq.top();
        nrb->_nrb_index[index_cnt++] = p.second;
        if (p.first != last_instance_index) {
            ++displ_cnt;
        }
        nrb->_displ[displ_cnt] = index_cnt;
        last_instance_index = p.first;
        nrbq.pop();
    }
    nrb->_displ_cnt = displ_cnt;
}

/* when we execute NET_RECEIVE block on GPU, we provide the index of synapse instances
 * which we need to execute during the current timestep. In order to do this, we have
 * update NetReceiveBuffer_t object to GPU. When size of cpu buffer changes, we set
 * reallocated to true and hence need to reallocate buffer on GPU and then need to copy
 * entire buffer. If reallocated is 0, that means buffer size is not changed and hence
 * only need to copy _size elements to GPU.
 * Note: this is very preliminary implementation, optimisations will be done after first
 * functional version.
 */
void update_net_receive_buffer(NrnThread* nt) {
    NrnThreadMembList* tml;

    for (tml = nt->tml; tml; tml = tml->next) {
        // net_receive buffer to copy
        NetReceiveBuffer_t* nrb = tml->ml->_net_receive_buffer;

        // if net receive buffer exist for mechanism
        if (nrb && nrb->_cnt) {
            // instance order to avoid race. setup _displ and _nrb_index
            net_receive_buffer_order(nrb);

#ifdef _OPENACC
            if (nt->compute_gpu) {
                // note that dont update nrb otherwise we loose pointers

                /* update scalar elements */
                acc_update_device(&nrb->_cnt, sizeof(int));
                acc_update_device(&nrb->_displ_cnt, sizeof(int));

                acc_update_device(nrb->_pnt_index, sizeof(int) * nrb->_cnt);
                acc_update_device(nrb->_weight_index, sizeof(int) * nrb->_cnt);
                acc_update_device(nrb->_nrb_t, sizeof(double) * nrb->_cnt);
                acc_update_device(nrb->_nrb_flag, sizeof(double) * nrb->_cnt);
                acc_update_device(nrb->_displ, sizeof(int) * (nrb->_displ_cnt + 1));
                acc_update_device(nrb->_nrb_index, sizeof(int) * nrb->_cnt);
            }
#endif
        }
    }
}

void update_net_send_buffer_on_host(NrnThread* nt, NetSendBuffer_t* nsb) {
#ifdef _OPENACC
    if (!nt->compute_gpu)
        return;

    if (nsb->_cnt) {
        acc_update_self(nsb->_sendtype, sizeof(int) * nsb->_cnt);
        acc_update_self(nsb->_vdata_index, sizeof(int) * nsb->_cnt);
        acc_update_self(nsb->_pnt_index, sizeof(int) * nsb->_cnt);
        acc_update_self(nsb->_weight_index, sizeof(int) * nsb->_cnt);
        acc_update_self(nsb->_nsb_t, sizeof(double) * nsb->_cnt);
        acc_update_self(nsb->_nsb_flag, sizeof(double) * nsb->_cnt);
    }
#else
    (void)nt;
    (void)nsb;
#endif
}

void update_nrnthreads_on_host(NrnThread* threads, int nthreads) {
#ifdef _OPENACC

    printf("\n --- Copying to Host! --- \n");

    int i;
    NetReceiveBuffer_t* nrb;

    for (i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;

        if (nt->compute_gpu && (nt->end > 0)) {
            /* -- copy data to host -- */

            int ne = nrn_soa_padded_size(nt->end, 0);

            acc_update_self(nt->_actual_rhs, ne * sizeof(double));
            acc_update_self(nt->_actual_d, ne * sizeof(double));
            acc_update_self(nt->_actual_a, ne * sizeof(double));
            acc_update_self(nt->_actual_b, ne * sizeof(double));
            acc_update_self(nt->_actual_v, ne * sizeof(double));
            acc_update_self(nt->_actual_area, ne * sizeof(double));

            /* @todo: nt._ml_list[tml->index] = tml->ml; */

            /* -- copy NrnThreadMembList list ml to host -- */
            NrnThreadMembList* tml;
            for (tml = nt->tml; tml; tml = tml->next) {
                Memb_list* ml = tml->ml;

                acc_update_self(&tml->index, sizeof(int));
                acc_update_self(&ml->nodecount, sizeof(int));

                int type = tml->index;
                int n = ml->nodecount;
                int szp = nrn_prop_param_size_[type];
                int szdp = nrn_prop_dparam_size_[type];
                int is_art = nrn_is_artificial_[type];
                int layout = nrn_mech_data_layout_[type];

                int pcnt = nrn_soa_padded_size(n, layout) * szp;

                acc_update_self(ml->data, pcnt * sizeof(double));

                if (!is_art) {
                    acc_update_self(ml->nodeindices, n * sizeof(int));
                }

                if (szdp) {
                    int pcnt = nrn_soa_padded_size(n, layout) * szdp;
                    acc_update_self(ml->pdata, pcnt * sizeof(int));
                }

                nrb = tml->ml->_net_receive_buffer;

                if (nrb) {
                    acc_update_self(&nrb->_cnt, sizeof(int));
                    acc_update_self(&nrb->_size, sizeof(int));
                    acc_update_self(&nrb->_pnt_offset, sizeof(int));
                    acc_update_self(&nrb->_displ_cnt, sizeof(int));

                    acc_update_self(nrb->_pnt_index, sizeof(int) * nrb->_size);
                    acc_update_self(nrb->_weight_index, sizeof(int) * nrb->_size);
                    acc_update_self(nrb->_displ, sizeof(int) * (nrb->_size + 1));
                    acc_update_self(nrb->_nrb_index, sizeof(int) * nrb->_size);
                }
            }

            if (nt->shadow_rhs_cnt) {
                int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);
                /* copy shadow_rhs to host */
                acc_update_self(nt->_shadow_rhs, pcnt * sizeof(double));
                /* copy shadow_d to host */
                acc_update_self(nt->_shadow_d, pcnt * sizeof(double));
            }

            if (nt->n_pntproc) {
                acc_update_self(nt->pntprocs, nt->n_pntproc * sizeof(Point_process));
            }

            if (nt->n_weight) {
                acc_update_self(nt->weights, sizeof(double) * nt->n_weight);
            }

            /* dont update vdata, its pointer array
               if(nt->_nvdata) {
               acc_update_self(nt->_vdata, sizeof(double)*nt->_nvdata);
               }
             */
        }
    }
#else
    (void)threads;
    (void)nthreads;
#endif
}

void update_nrnthreads_on_device(NrnThread* threads, int nthreads) {
#ifdef _OPENACC

    printf("\n --- Copying to Device! --- \n");

    int i;
    NetReceiveBuffer_t* nrb;

    for (i = 0; i < nthreads; i++) {
        NrnThread* nt = threads + i;

        if (nt->compute_gpu && (nt->end > 0)) {
            /* -- copy data to device -- */

            int ne = nrn_soa_padded_size(nt->end, 0);

            acc_update_device(nt->_actual_rhs, ne * sizeof(double));
            acc_update_device(nt->_actual_d, ne * sizeof(double));
            acc_update_device(nt->_actual_a, ne * sizeof(double));
            acc_update_device(nt->_actual_b, ne * sizeof(double));
            acc_update_device(nt->_actual_v, ne * sizeof(double));
            acc_update_device(nt->_actual_area, ne * sizeof(double));

            /* @todo: nt._ml_list[tml->index] = tml->ml; */

            /* -- copy NrnThreadMembList list ml to host -- */
            NrnThreadMembList* tml;
            for (tml = nt->tml; tml; tml = tml->next) {
                Memb_list* ml = tml->ml;
                int type = tml->index;
                int n = ml->nodecount;
                int szp = nrn_prop_param_size_[type];
                int szdp = nrn_prop_dparam_size_[type];
                int is_art = nrn_is_artificial_[type];
                int layout = nrn_mech_data_layout_[type];

                int pcnt = nrn_soa_padded_size(n, layout) * szp;

                acc_update_device(ml->data, pcnt * sizeof(double));

                if (!is_art) {
                    acc_update_device(ml->nodeindices, n * sizeof(int));
                }

                if (szdp) {
                    int pcnt = nrn_soa_padded_size(n, layout) * szdp;
                    acc_update_device(ml->pdata, pcnt * sizeof(int));
                }

                nrb = tml->ml->_net_receive_buffer;

                if (nrb) {
                    acc_update_device(&nrb->_cnt, sizeof(int));
                    acc_update_device(&nrb->_size, sizeof(int));
                    acc_update_device(&nrb->_pnt_offset, sizeof(int));
                    acc_update_device(&nrb->_displ_cnt, sizeof(int));

                    acc_update_device(nrb->_pnt_index, sizeof(int) * nrb->_size);
                    acc_update_device(nrb->_weight_index, sizeof(int) * nrb->_size);
                    acc_update_device(nrb->_displ, sizeof(int) * (nrb->_size + 1));
                    acc_update_device(nrb->_nrb_index, sizeof(int) * nrb->_size);
                }
            }

            if (nt->shadow_rhs_cnt) {
                int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);
                /* copy shadow_rhs to host */
                acc_update_device(nt->_shadow_rhs, pcnt * sizeof(double));
                /* copy shadow_d to host */
                acc_update_device(nt->_shadow_d, pcnt * sizeof(double));
            }

            if (nt->n_pntproc) {
                acc_update_device(nt->pntprocs, nt->n_pntproc * sizeof(Point_process));
            }

            if (nt->n_weight) {
                acc_update_device(nt->weights, sizeof(double) * nt->n_weight);
            }

            /* don't and don't update vdata, its pointer array
               if(nt->_nvdata) {
               acc_update_device(nt->_vdata, sizeof(double)*nt->_nvdata);
               }
             */
        }
    }
#else
    (void)threads;
    (void)nthreads;
#endif
}

#ifdef __cplusplus
extern "C" {
#endif

void update_matrix_from_gpu(NrnThread* _nt) {
#ifdef _OPENACC
    if (_nt->compute_gpu && (_nt->end > 0)) {
/* before copying, make sure all computations in the stream are completed */
#pragma acc wait(_nt->stream_id)

        /* openacc routine doesn't allow asyn, use pragma */
        // acc_update_self(_nt->_actual_rhs, 2*_nt->end*sizeof(double));

        /* RHS and D are contigious, copy them in one go!
         * NOTE: in pragma you have to give actual pointer like below and not nt->rhs...
         */
        double* rhs = _nt->_actual_rhs;
        int ne = nrn_soa_padded_size(_nt->end, 0);

#pragma acc update host(rhs[0 : 2 * ne]) async(_nt->stream_id)
#pragma acc wait(_nt->stream_id)
    }
#else
    (void)_nt;
#endif
}

void update_matrix_to_gpu(NrnThread* _nt) {
#ifdef _OPENACC
    if (_nt->compute_gpu && (_nt->end > 0)) {
/* before copying, make sure all computations in the stream are completed */
#pragma acc wait(_nt->stream_id)

        /* while discussion with Michael we found that RHS is also needed on
         * gpu because nrn_cap_jacob uses rhs which is being updated on GPU
         */
        // printf("\n Copying voltage to GPU ... ");
        double* v = _nt->_actual_v;
        double* rhs = _nt->_actual_rhs;
        int ne = nrn_soa_padded_size(_nt->end, 0);

#pragma acc update device(v[0 : ne]) async(_nt->stream_id)
#pragma acc update device(rhs[0 : ne]) async(_nt->stream_id)
#pragma acc wait(_nt->stream_id)
    }
#else
    (void)_nt;
#endif
}

#ifdef __cplusplus
}
#endif

void finalize_data_on_device() {
    /*@todo: when we have used random123 on gpu and we do this finalize,
    I am seeing cuCtxDestroy returned CUDA_ERROR_INVALID_CONTEXT error.
    THis might be due to the fact that the cuda apis (e.g. free is not
    called yet for Ramdom123 data / streams etc. So handle this better!
    */
    return;

#ifdef _OPENACC
    acc_shutdown(acc_device_default);
#endif
}

void nrn_newtonspace_copyto_device(NewtonSpace* ns) {
#ifdef _OPENACC
    if (nrn_threads[0].compute_gpu == 0) {
        return;
    }

    int n = ns->n * ns->n_instance;
    // actually, the values of double do not matter, only the  pointers.
    NewtonSpace* d_ns = (NewtonSpace*)acc_copyin(ns, sizeof(NewtonSpace));

    double* pd;
    int* pint;
    double** ppd;

    pd = (double*)acc_copyin(ns->delta_x, n * sizeof(double));
    acc_memcpy_to_device(&(d_ns->delta_x), &pd, sizeof(double*));

    pd = (double*)acc_copyin(ns->high_value, n * sizeof(double));
    acc_memcpy_to_device(&(d_ns->high_value), &pd, sizeof(double*));

    pd = (double*)acc_copyin(ns->low_value, n * sizeof(double));
    acc_memcpy_to_device(&(d_ns->low_value), &pd, sizeof(double*));

    pd = (double*)acc_copyin(ns->rowmax, n * sizeof(double));
    acc_memcpy_to_device(&(d_ns->rowmax), &pd, sizeof(double*));

    pint = (int*)acc_copyin(ns->perm, n * sizeof(int));
    acc_memcpy_to_device(&(d_ns->perm), &pint, sizeof(int*));

    ppd = (double**)acc_copyin(ns->jacobian, ns->n * sizeof(double*));
    acc_memcpy_to_device(&(d_ns->jacobian), &ppd, sizeof(double**));

    // the actual jacobian doubles were allocated as a single array
    double* d_jacdat = (double*)acc_copyin(ns->jacobian[0], ns->n * n * sizeof(double));

    for (int i = 0; i < ns->n; ++i) {
        pd = d_jacdat + i * n;
        acc_memcpy_to_device(&(ppd[i]), &pd, sizeof(double*));
    }
#endif
}

void nrn_sparseobj_copyto_device(SparseObj* so) {
#ifdef _OPENACC
    if (nrn_threads[0].compute_gpu == 0) {
        return;
    }

    unsigned n1 = so->neqn + 1;
    SparseObj* d_so = (SparseObj*)acc_copyin(so, sizeof(SparseObj));
    // only pointer fields in SparseObj that need setting up are
    //   rowst, diag, rhs, ngetcall, coef_list
    // only pointer fields in Elm that need setting up are
    //   r_down, c_right, value
    // do not care about the Elm* ptr value, just the space.

    Elm** ppelm;
    Elm* pelm;
    unsigned* pu;
    double* pd;
    double** ppd;

    Elm** d_rowst = (Elm**)acc_copyin(so->rowst, n1 * sizeof(Elm*));
    acc_memcpy_to_device(&(d_so->rowst), &d_rowst, sizeof(Elm**));

    Elm** d_diag = (Elm**)acc_copyin(so->diag, n1 * sizeof(Elm*));
    acc_memcpy_to_device(&(d_so->diag), &d_diag, sizeof(Elm**));

    pu = (unsigned*)acc_copyin(so->ngetcall, so->_cntml_padded * sizeof(unsigned));
    acc_memcpy_to_device(&(d_so->ngetcall), &pu, sizeof(Elm**));

    pd = (double*)acc_copyin(so->rhs, n1 * so->_cntml_padded * sizeof(double));
    acc_memcpy_to_device(&(d_so->rhs), &pd, sizeof(double*));

    double** d_coef_list =
        (double**)acc_copyin(so->coef_list, so->coef_list_size * sizeof(double*));
    acc_memcpy_to_device(&(d_so->coef_list), &d_coef_list, sizeof(double**));

    // Fill in relevant Elm pointer values

    for (unsigned irow = 1; irow < n1; ++irow) {
        for (Elm* elm = so->rowst[irow]; elm; elm = elm->c_right) {
            Elm* pelm = (Elm*)acc_copyin(elm, sizeof(Elm));

            if (elm == so->rowst[irow]) {
                acc_memcpy_to_device(&(d_rowst[irow]), &pelm, sizeof(Elm*));
            } else {
                Elm* d_e = (Elm*)acc_deviceptr(elm->c_left);
                acc_memcpy_to_device(&(pelm->c_left), &d_e, sizeof(Elm*));
            }

            if (elm->col == elm->row) {
                acc_memcpy_to_device(&(d_diag[irow]), &pelm, sizeof(Elm*));
            }

            if (irow > 1) {
                if (elm->r_up) {
                    Elm* d_e = (Elm*)acc_deviceptr(elm->r_up);
                    acc_memcpy_to_device(&(pelm->r_up), &d_e, sizeof(Elm*));
                }
            }

            pd = (double*)acc_copyin(elm->value, so->_cntml_padded * sizeof(double));
            acc_memcpy_to_device(&(pelm->value), &pd, sizeof(double*));
        }
    }

    // visit all the Elm again and fill in pelm->r_down and pelm->c_left
    for (unsigned irow = 1; irow < n1; ++irow) {
        for (Elm* elm = so->rowst[irow]; elm; elm = elm->c_right) {
            pelm = (Elm*)acc_deviceptr(elm);
            if (elm->r_down) {
                Elm* d_e = (Elm*)acc_deviceptr(elm->r_down);
                acc_memcpy_to_device(&(pelm->r_down), &d_e, sizeof(Elm*));
            }
            if (elm->c_right) {
                Elm* d_e = (Elm*)acc_deviceptr(elm->c_right);
                acc_memcpy_to_device(&(pelm->c_right), &d_e, sizeof(Elm*));
            }
        }
    }

    // Fill in the d_so->coef_list
    for (unsigned i = 0; i < so->coef_list_size; ++i) {
        pd = (double*)acc_deviceptr(so->coef_list[i]);
        acc_memcpy_to_device(&(d_coef_list[i]), &pd, sizeof(double*));
    }
#endif
}
