#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrniv/netcon.h"
#include "coreneuron/nrniv/nrn_acc_manager.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/vrecitem.h"
#include "coreneuron/nrniv/profiler_interface.h"
#include "coreneuron/nrniv/cellorder.h"

#ifdef _OPENACC
#include<openacc.h>
#endif

#ifdef CRAYPAT
#include <pat_api.h>
#endif

extern InterleaveInfo* interleave_info;
void copy_ivoc_vect_to_device(IvocVect *& iv, IvocVect *& div);

/* note: threads here are corresponding to global nrn_threads array */
void setup_nrnthreads_on_device(NrnThread *threads, int nthreads)  {

#ifdef _OPENACC

    if(nthreads <= 0 ) {
        printf("\n Warning: No threads to copy on GPU! ");
        return;
    }

    int i;
    NrnThread *d_threads;

    /* @todo: why dt is not setup at this moment? */
    for( i = 0; i < nthreads; i++) {
        (threads+i)->_dt = dt;
    }

    /* -- copy NrnThread to device. this needs to be contigious vector because offset is used to find
     * corresponding NrnThread using Point_process in NET_RECEIVE block
     */
    d_threads = (NrnThread *) acc_copyin(threads, sizeof(NrnThread)*nthreads);

    printf("\n --- Copying to Device! --- ");

    if(interleave_info == NULL) {
        printf("\n Warning: No permutation data? Required for linear algebra!");
    }

    /* pointers for data struct on device, starting with d_ */

    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;                  //NrnThread on host
        NrnThread *d_nt = d_threads + i;               //NrnThread on device

        if(nt->end <= 0) {
            //this is an empty thread
            continue;
        }

        /* this thread will be computed on GPU */
        nt->compute_gpu = 1;

        double *d__data;                // nrn_threads->_data on device

        printf("\n -----------COPYING %d'th NrnThread TO DEVICE --------------- \n", i);

        /* -- copy _data to device -- */

        /*copy all double data for thread */
        d__data = (double *) acc_copyin(nt->_data, nt->_ndata*sizeof(double));

        /* Here is the example of using OpenACC data enter/exit
         * Remember that we are not allowed to use nt->_data but we have to use:
         *      double *dtmp = nt->_data;  // now use dtmp!
                #pragma acc enter data copyin(dtmp[0:nt->_ndata]) async(nt->stream_id)
                #pragma acc wait(nt->stream_id)
         */

        /*update d_nt._data to point to device copy */
        acc_memcpy_to_device(&(d_nt->_data), &d__data, sizeof(double*));

        /* -- setup rhs, d, a, b, v, node_aread to point to device copy -- */
        double *dptr;

        /* for padding, we have to recompute ne */
        int ne = nrn_soa_padded_size(nt->end, 0);

        dptr = d__data + 0*ne;
        acc_memcpy_to_device(&(d_nt->_actual_rhs), &(dptr), sizeof(double*));

        dptr = d__data + 1*ne;
        acc_memcpy_to_device(&(d_nt->_actual_d), &(dptr), sizeof(double*));

        dptr = d__data + 2*ne;
        acc_memcpy_to_device(&(d_nt->_actual_a), &(dptr), sizeof(double*));

        dptr = d__data + 3*ne;
        acc_memcpy_to_device(&(d_nt->_actual_b), &(dptr), sizeof(double*));

        dptr = d__data + 4*ne;
        acc_memcpy_to_device(&(d_nt->_actual_v), &(dptr), sizeof(double*));

        dptr = d__data + 5*ne;
        acc_memcpy_to_device(&(d_nt->_actual_area), &(dptr), sizeof(double*));


        int *d_v_parent_index = (int *) acc_copyin(nt->_v_parent_index, nt->end*sizeof(int));
        acc_memcpy_to_device(&(d_nt->_v_parent_index), &(d_v_parent_index), sizeof(int*));

        /* nt._ml_list is used in NET_RECEIVE block and should have valid membrane list id*/
        Memb_list ** d_ml_list = (Memb_list**) acc_copyin(nt->_ml_list, n_memb_func * sizeof(Memb_list*));
        acc_memcpy_to_device(&(d_nt->_ml_list), &(d_ml_list), sizeof(Memb_list**));

        /* -- copy NrnThreadMembList list ml to device -- */

        NrnThreadMembList* tml;
        NrnThreadMembList* d_tml;
        NrnThreadMembList* d_last_tml;

        Memb_list * d_ml;
        int first_tml = 1;
        size_t offset = 6 * ne;

        for (tml = nt->tml; tml; tml = tml->next) {

            /*copy tml to device*/
            /*QUESTIONS: does tml will point to NULL as in host ? : I assume so!*/
            d_tml = (NrnThreadMembList *) acc_copyin(tml, sizeof(NrnThreadMembList));

            /*first tml is pointed by nt */
            if(first_tml) {
                acc_memcpy_to_device(&(d_nt->tml), &d_tml, sizeof(NrnThreadMembList*));
                first_tml = 0;
            } else {
            /*rest of tml forms linked list */
                acc_memcpy_to_device(&(d_last_tml->next), &d_tml, sizeof(NrnThreadMembList*));
            }

            //book keeping for linked-list
            d_last_tml = d_tml;

            /* now for every tml, there is a ml. copy that and setup pointer */
            d_ml = (Memb_list *) acc_copyin(tml->ml, sizeof(Memb_list));
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
                int * d_nodeindices = (int *) acc_copyin(tml->ml->nodeindices, sizeof(int)*n);
                acc_memcpy_to_device(&(d_ml->nodeindices), &d_nodeindices, sizeof(int*));
            }

            if (szdp) {
                int pcnt = nrn_soa_padded_size(n, layout)*szdp;
                int * d_pdata = (int *) acc_copyin(tml->ml->pdata, sizeof(int)*pcnt);
                acc_memcpy_to_device(&(d_ml->pdata), &d_pdata, sizeof(int*));
            }

            NetReceiveBuffer_t *nrb, *d_nrb;
            int *d_weight_index, *d_pnt_index;
            double *d_nrb_t;

            //net_receive buffer associated with mechanism
            nrb = tml->ml->_net_receive_buffer;

            // if net receive buffer exist for mechanism
            if(nrb) {

                d_nrb = (NetReceiveBuffer_t*) acc_copyin(nrb, sizeof(NetReceiveBuffer_t));
                acc_memcpy_to_device(&(d_ml->_net_receive_buffer), &d_nrb, sizeof(NetReceiveBuffer_t*));

                d_pnt_index = (int *) acc_copyin(nrb->_pnt_index, sizeof(int)*nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_pnt_index), &d_pnt_index, sizeof(int*));

                d_weight_index = (int *) acc_copyin(nrb->_weight_index, sizeof(int)*nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_weight_index), &d_weight_index, sizeof(int*));

                d_nrb_t = (double *) acc_copyin(nrb->_nrb_t, sizeof(double)*nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_nrb_t), &d_nrb_t, sizeof(double*));

                //0 means gpu copy updated with size of buffer on cpu
                nrb->reallocated = 0;
            }
        }

        if(nt->shadow_rhs_cnt) {
            double * d_shadow_ptr;

            int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);

            /* copy shadow_rhs to device and fix-up the pointer */
            d_shadow_ptr = (double *) acc_copyin(nt->_shadow_rhs, pcnt*sizeof(double));
            acc_memcpy_to_device(&(d_nt->_shadow_rhs), &d_shadow_ptr, sizeof(double*));

            /* copy shadow_d to device and fix-up the pointer */
            d_shadow_ptr = (double *) acc_copyin(nt->_shadow_d, pcnt*sizeof(double));
            acc_memcpy_to_device(&(d_nt->_shadow_d), &d_shadow_ptr, sizeof(double*));
        }

        if(nt->n_pntproc) {
            /* copy Point_processes array and fix the pointer to execute net_receive blocks on GPU */
            Point_process *pntptr = (Point_process*) acc_copyin(nt->pntprocs, nt->n_pntproc*sizeof(Point_process));
            acc_memcpy_to_device(&(d_nt->pntprocs), &pntptr, sizeof(Point_process*));
        }

        if(nt->n_weight) {
            /* copy weight vector used in NET_RECEIVE which is pointed by netcon.weight */
            double * d_weights = (double *) acc_copyin(nt->weights, sizeof(double)*nt->n_weight);
            acc_memcpy_to_device(&(d_nt->weights), &d_weights, sizeof(double*));
        }

        if(nt->_nvdata) {
            /* copy vdata which is setup in bbcore_read. This contains cuda allocated nrnran123_State * */
            void ** d_vdata = (void **) acc_copyin(nt->_vdata, sizeof(void *)*nt->_nvdata);
            acc_memcpy_to_device(&(d_nt->_vdata), &d_vdata, sizeof(void**));
        }

        if(nt->n_presyn) {
            /* copy presyn vector used for spike exchange */
            PreSyn *d_presyns = (PreSyn *) acc_copyin(nt->presyns, sizeof(PreSyn)*nt->n_presyn);
            acc_memcpy_to_device(&(d_nt->presyns), &d_presyns, sizeof(PreSyn*));
        }

        if(nt->_net_send_buffer_size) {
            /* copy send_receive buffer */
            int *d_net_send_buffer = (int *) acc_copyin(nt->_net_send_buffer, sizeof(int)*nt->_net_send_buffer_size);
            acc_memcpy_to_device(&(d_nt->_net_send_buffer), &d_net_send_buffer, sizeof(int*));
        }

        if(nt->n_vecplay) {

            /* copy VecPlayContinuous instances */

            printf("\n Warning: VectorPlay used but NOT implemented on GPU! ");

            /** just empty containers */
            void **d_vecplay = (void**) acc_copyin(nt->_vecplay, sizeof(void*)*nt->n_vecplay);
            acc_memcpy_to_device(&(d_nt->_vecplay), &d_vecplay, sizeof(void**));

            for(int i = 0; i < nt->n_vecplay; i++) {

                VecPlayContinuous*  vecplay_instance = (VecPlayContinuous*) nt->_vecplay[i];

                /** just VecPlayContinuous object */
                void *d_p = (void*) acc_copyin(vecplay_instance, sizeof(VecPlayContinuous));
                acc_memcpy_to_device(&(d_vecplay[i]), &d_p, sizeof(void*));

                VecPlayContinuous* d_vecplay_instance = (VecPlayContinuous*)d_p;

                /** copy y_, t_ and discon_indices_ */
                copy_ivoc_vect_to_device(vecplay_instance->y_, d_vecplay_instance->y_);
                copy_ivoc_vect_to_device(vecplay_instance->t_, d_vecplay_instance->t_);
                copy_ivoc_vect_to_device(vecplay_instance->discon_indices_, d_vecplay_instance->discon_indices_);

                /** copy PlayRecordEvent : todo: verify this */
                PlayRecordEvent* d_e_ = (PlayRecordEvent*) acc_copyin(vecplay_instance->e_, sizeof(PlayRecordEvent));
                acc_memcpy_to_device(&(d_e_->plr_), &d_vecplay_instance, sizeof(VecPlayContinuous*));
                acc_memcpy_to_device(&(d_vecplay_instance->e_), &d_e_, sizeof(PlayRecordEvent*));

                /** copy pd_ : note that it's pointer inside ml->data and hence data itself is already on GPU */
                double *d_pd_ = (double *) acc_deviceptr(vecplay_instance->pd_);
                acc_memcpy_to_device(&(d_vecplay_instance->pd_), &d_pd_, sizeof(double*));
            }
        }

        if(nt->_permute) {
            /* todo: not necessary to setup pointers, just copy it */
            InterleaveInfo& info = interleave_info[i];
            acc_copyin(info.stride, sizeof(int) * info.nstride+1);
            acc_copyin(info.firstnode, sizeof(int) * nt->ncell);
            acc_copyin(info.lastnode, sizeof(int) * nt->ncell);
            acc_copyin(info.cellsize, sizeof(int) * nt->ncell);

        } else {
            printf("\n WARNING: NrnThread %d not permuted, error for linear algebra?", i);
        }

        printf("\n Compute thread on GPU? : %s, Stream : %d", (nt->compute_gpu)? "Yes" : "No", nt->stream_id);
    }

    if(nrn_ion_global_map_size) {
        double **d_data  = (double **) acc_copyin(nrn_ion_global_map, sizeof(double*) * nrn_ion_global_map_size);
        for( int j = 0; j < nrn_ion_global_map_size; j++) {
            if(nrn_ion_global_map[j]) {
                /* @todo: fix this constant size 3 :( */
                double *d_mechmap = (double *) acc_copyin(nrn_ion_global_map[j], 3*sizeof(double));
                acc_memcpy_to_device(&(d_data[j]), &d_mechmap, sizeof(double*));
            }
        }
    }
#endif
}

void copy_ivoc_vect_to_device(IvocVect *& iv, IvocVect *& div) {
#ifdef _OPENACC
    if(iv) {
        IvocVect *d_iv = (IvocVect *) acc_copyin(iv, sizeof(IvocVect));
        acc_memcpy_to_device(&div, &d_iv, sizeof(IvocVect*));

        size_t n = iv->size();
        if(n) {
            double *d_data = (double *)acc_copyin(iv->data(), sizeof(double)*n);
            acc_memcpy_to_device(&(d_iv->data_), &d_data, sizeof(double*));
        }
    }
#endif
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
void update_net_receive_buffer(NrnThread *nt) {

#ifdef _OPENACC

    if (!nt->compute_gpu)
        return;

    NrnThreadMembList* tml;
    Memb_list *ml, *d_ml;
    NetReceiveBuffer_t *nrb, *d_nrb;
    int *d_weight_index, *d_pnt_index;
    double *d_nrb_t;

    for (tml = nt->tml; tml; tml = tml->next) {

        //net_receive buffer to copy
        nrb = tml->ml->_net_receive_buffer;

        // if net receive buffer exist for mechanism
        if(nrb) {

            d_nrb = (NetReceiveBuffer_t *) acc_deviceptr(nrb);
            d_ml = (Memb_list *) acc_deviceptr(tml->ml);

            // reallocated is true when buffer on cpu is changed
            if(nrb->reallocated) {

                /* free existing vectors in buffers on gpu */
                acc_free( acc_deviceptr(nrb->_pnt_index) );
                acc_free( acc_deviceptr(nrb->_weight_index) );
                acc_free( acc_deviceptr(nrb->_nrb_t) );

                /* update device copy */
                acc_update_device(nrb, sizeof(NetReceiveBuffer_t));

                /* recopy the vectors in the buffer */
                d_pnt_index = (int *) acc_copyin(nrb->_pnt_index, sizeof(int)*nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_pnt_index), &d_pnt_index, sizeof(int*));

                d_weight_index = (int *) acc_copyin(nrb->_weight_index, sizeof(int)*nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_weight_index), &d_weight_index, sizeof(int*));

                d_nrb_t = (double *) acc_copyin(nrb->_nrb_t, sizeof(double)*nrb->_size);
                acc_memcpy_to_device(&(d_nrb->_nrb_t), &d_nrb_t, sizeof(double*));

                /* gpu copy updated */
                nrb->reallocated = 0;

            } else {

                //note that dont update nrb otherwise we loose pointers

                /* update scalar elements */
                acc_update_device(&nrb->_cnt, sizeof(int));
                acc_update_device(&nrb->_size, sizeof(int));
                acc_update_device(&nrb->_pnt_offset, sizeof(int));

                /*@todo confirm with Michael that we don't need to _size but only
                 * _cnt elements to GPU
                 */
                acc_update_device(nrb->_pnt_index, sizeof(int)*nrb->_cnt);
                acc_update_device(nrb->_weight_index, sizeof(int)*nrb->_cnt);
                acc_update_device(nrb->_nrb_t, sizeof(double)*nrb->_cnt);
            }
        }
    }
#endif
}

void update_nrnthreads_on_host(NrnThread *threads, int nthreads) {

#ifdef _OPENACC

    printf("\n --- Copying to Host! --- \n");

    int i;
    NetReceiveBuffer_t *nrb;

    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;

        if ( nt->compute_gpu && (nt->end > 0) ) {

            /* -- copy data to host -- */

            int ne = nrn_soa_padded_size(nt->end, 0);

            acc_update_self(nt->_actual_rhs, ne*sizeof(double));
            acc_update_self(nt->_actual_d, ne*sizeof(double));
            acc_update_self(nt->_actual_a, ne*sizeof(double));
            acc_update_self(nt->_actual_b, ne*sizeof(double));
            acc_update_self(nt->_actual_v, ne*sizeof(double));
            acc_update_self(nt->_actual_area, ne*sizeof(double));

            /* @todo: nt._ml_list[tml->index] = tml->ml; */

            /* -- copy NrnThreadMembList list ml to host -- */
            NrnThreadMembList* tml;
            for (tml = nt->tml; tml; tml = tml->next) {

                Memb_list *ml = tml->ml;

                acc_update_self(&tml->index, sizeof(int));
                acc_update_self(&ml->nodecount, sizeof(int));

                int type = tml->index;
                int n = ml->nodecount;
                int szp = nrn_prop_param_size_[type];
                int szdp = nrn_prop_dparam_size_[type];
                int is_art = nrn_is_artificial_[type];
                int layout = nrn_mech_data_layout_[type];

                int pcnt = nrn_soa_padded_size(n, layout) * szp;

                acc_update_self(ml->data, pcnt*sizeof(double));

                if (!is_art) {
                    acc_update_self(ml->nodeindices, n*sizeof(int));
                }

                if (szdp) {
                    int pcnt = nrn_soa_padded_size(n, layout) * szdp;
                    acc_update_self(ml->pdata, pcnt*sizeof(int));
                }

                nrb = tml->ml->_net_receive_buffer;

                if(nrb) {

                    acc_update_self(&nrb->_cnt, sizeof(int));
                    acc_update_self(&nrb->_size, sizeof(int));
                    acc_update_self(&nrb->_pnt_offset, sizeof(int));

                    acc_update_self(nrb->_pnt_index, sizeof(int)*nrb->_size);
                    acc_update_self(nrb->_weight_index, sizeof(int)*nrb->_size);
                }

            }

            if(nt->shadow_rhs_cnt) {
                int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);
                /* copy shadow_rhs to host */
                acc_update_self(nt->_shadow_rhs, pcnt*sizeof(double));
                /* copy shadow_d to host */
                acc_update_self(nt->_shadow_d, pcnt*sizeof(double));
            }

            if(nt->n_pntproc) {
                acc_update_self(nt->pntprocs, nt->n_pntproc*sizeof(Point_process));
            }

            if(nt->n_weight) {
                acc_update_self(nt->weights, sizeof(double)*nt->n_weight);
            }

            /* dont update vdata, its pointer array
               if(nt->_nvdata) {
               acc_update_self(nt->_vdata, sizeof(double)*nt->_nvdata);
               }
             */
        }
    }
#endif

}

void update_nrnthreads_on_device(NrnThread *threads, int nthreads) {

#ifdef _OPENACC

    printf("\n --- Copying to Device! --- \n");

    int i;
    NetReceiveBuffer_t *nrb;

    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;

        if ( nt->compute_gpu && (nt->end > 0) ) {

            /* -- copy data to device -- */

            int ne = nrn_soa_padded_size(nt->end, 0);

            acc_update_device(nt->_actual_rhs, ne*sizeof(double));
            acc_update_device(nt->_actual_d, ne*sizeof(double));
            acc_update_device(nt->_actual_a, ne*sizeof(double));
            acc_update_device(nt->_actual_b, ne*sizeof(double));
            acc_update_device(nt->_actual_v, ne*sizeof(double));
            acc_update_device(nt->_actual_area, ne*sizeof(double));

            /* @todo: nt._ml_list[tml->index] = tml->ml; */

            /* -- copy NrnThreadMembList list ml to host -- */
            NrnThreadMembList* tml;
            for (tml = nt->tml; tml; tml = tml->next) {

                Memb_list *ml = tml->ml;
                int type = tml->index;
                int n = ml->nodecount;
                int szp = nrn_prop_param_size_[type];
                int szdp = nrn_prop_dparam_size_[type];
                int is_art = nrn_is_artificial_[type];
                int layout = nrn_mech_data_layout_[type];

                int pcnt = nrn_soa_padded_size(n, layout) * szp;

                acc_update_device(ml->data, pcnt*sizeof(double));

                if (!is_art) {
                    acc_update_device(ml->nodeindices, n*sizeof(int));
                }

                if (szdp) {
                    int pcnt = nrn_soa_padded_size(n, layout) * szdp;
                    acc_update_device(ml->pdata, pcnt*sizeof(int));
                }

                nrb = tml->ml->_net_receive_buffer;

                if(nrb) {
                    acc_update_device(&nrb->_cnt, sizeof(int));
                    acc_update_device(&nrb->_size, sizeof(int));
                    acc_update_device(&nrb->_pnt_offset, sizeof(int));

                    acc_update_device(nrb->_pnt_index, sizeof(int)*nrb->_size);
                    acc_update_device(nrb->_weight_index, sizeof(int)*nrb->_size);
                }
            }

            if(nt->shadow_rhs_cnt) {
                int pcnt = nrn_soa_padded_size(nt->shadow_rhs_cnt, 0);
                /* copy shadow_rhs to host */
                acc_update_device(nt->_shadow_rhs, pcnt*sizeof(double));
                /* copy shadow_d to host */
                acc_update_device(nt->_shadow_d, pcnt*sizeof(double));
            }

            if(nt->n_pntproc) {
                acc_update_device(nt->pntprocs, nt->n_pntproc*sizeof(Point_process));
            }

            if(nt->n_weight) {
                acc_update_device(nt->weights, sizeof(double)*nt->n_weight);
            }

            /* don't and don't update vdata, its pointer array
               if(nt->_nvdata) {
               acc_update_device(nt->_vdata, sizeof(double)*nt->_nvdata);
               }
             */
        }
    }
#endif

}

#ifdef __cplusplus
extern "C" {
#endif

void update_matrix_from_gpu(NrnThread *_nt){
#ifdef _OPENACC
    if ( _nt->compute_gpu && (_nt->end > 0) ) {

        /* before copying, make sure all computations in the stream are completed */
        #pragma acc wait(_nt->stream_id)

        /* openacc routine doesn't allow asyn, use pragma */
        // acc_update_self(_nt->_actual_rhs, 2*_nt->end*sizeof(double));

        /* RHS and D are contigious, copy them in one go!
         * NOTE: in pragma you have to give actual pointer like below and not nt->rhs...
         */
        double *rhs = _nt->_actual_rhs;
        int ne = nrn_soa_padded_size(_nt->end, 0);

        #pragma acc update host(rhs[0:2*ne]) async(_nt->stream_id)
        #pragma acc wait(_nt->stream_id)
    }
#endif
}

void update_matrix_to_gpu(NrnThread *_nt){
#ifdef _OPENACC
    if ( _nt->compute_gpu && (_nt->end > 0) ) {

        /* before copying, make sure all computations in the stream are completed */
        #pragma acc wait(_nt->stream_id)

        /* while discussion with Michael we found that RHS is also needed on
         * gpu because nrn_cap_jacob uses rhs which is being updated on GPU
         */
        //printf("\n Copying voltage to GPU ... ");
        double *v = _nt->_actual_v;
        double *rhs = _nt->_actual_rhs;
        int ne = nrn_soa_padded_size(_nt->end, 0);

        #pragma acc update device(v[0:ne]) async(_nt->stream_id)
        #pragma acc update device(rhs[0:ne]) async(_nt->stream_id)
        #pragma acc wait(_nt->stream_id)
    }
#endif
}

#ifdef __cplusplus
}
#endif

void modify_data_on_device(NrnThread *threads, int nthreads) {

#ifdef _OPENACC
        printf("\n --- Modifying data on device! --- \n");
#endif

    /* Note: even if we have added padding for memory alignment, starting pointer
     * and their count remains same. Hence below code is still valid even if
     * we don't consider padding related information.
     */

    int i, j;
    NetReceiveBuffer_t *nrb;

    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;

        if ( nt->compute_gpu && (nt->end > 0) ) {

            /* -- modify data on device -- */

            int ne = nt->end;

            #pragma acc parallel loop present(nt[0:1])
            for (j = 0; j < ne; ++j)
            {
                nt->_actual_rhs[j] += 0.1;
                nt->_actual_d[j] += 0.1;
                nt->_actual_a[j] += 0.1;
                nt->_actual_b[j] += 0.1;
                nt->_actual_v[j] += 0.1;
                nt->_actual_area[j] += 0.1;
            }

            /* @todo: nt._ml_list[tml->index] = tml->ml; */

            NrnThreadMembList* tml;
            for (tml = nt->tml; tml; tml = tml->next) {

                Memb_list *ml = tml->ml;
                int type = tml->index;
                int n = ml->nodecount;
                int szp = nrn_prop_param_size_[type];
                int szdp = nrn_prop_dparam_size_[type];
                int is_art = nrn_is_artificial_[type];

                #pragma acc parallel loop present(ml[0:1])
                for (j = 0; j < n*szp; ++j)
                {
                    ml->data[j] += 0.1;
                }


                if (!is_art) {
                    #pragma acc parallel loop present(ml[0:1])
                    for (j = 0; j < n; ++j)
                    {
                        ml->nodeindices[j] += 1;
                    }
                }

                if (szdp) {
                    #pragma acc parallel loop present(ml[0:1])
                    for (j = 0; j < n*szdp; ++j)
                    {
                        ml->pdata[j] += 1;
                    }
                }

                nrb = ml->_net_receive_buffer;

                if(nrb) {
                    #pragma acc parallel loop present(nrb[0:1])
                    for (j = 0; j < nrb->_size; ++j)
                    {
                        nrb->_pnt_index[j] += 1;
                        nrb->_weight_index[j] +=1;
                    }
                }
            }

            if(nt->shadow_rhs_cnt) {
                #pragma acc parallel loop present(nt[0:1])
                for (j = 0; j < nt->shadow_rhs_cnt; ++j)
                {
                    nt->_shadow_rhs[j] += 0.1;
                    nt->_shadow_d[j] += 0.1;
                }
            }

            if(nt->n_pntproc) {
                #pragma acc parallel loop present(nt[0:1])
                for (j = 0; j < nt->n_pntproc; ++j)
                {
                    nt->pntprocs[j]._type +=1;
                    nt->pntprocs[j]._i_instance +=1;
                    nt->pntprocs[j]._tid +=1;
                }
            }

            if(nt->n_weight) {
                #pragma acc parallel loop present(nt[0:1])
                for (j = 0; j < nt->n_weight; ++j)
                {
                    nt->weights[j] += 0.1;
                }
            }

            /* dont update vdata, its pointer array
               if(nt->_nvdata) {
               #pragma acc parallel loop present(nt[0:1])
               for (j = 0; j < nt->_nvdata; ++j)
               {
               nt->_vdata[j] += 0.1;
               }
               }
             */
        }
    }
}


void write_iarray_to_file(FILE *hFile, int *data, int n) {
    int i;

    for(i=0; i<n; i++) {
        fprintf(hFile, "%d\n", data[i]);
    }
    fprintf(hFile, "---\n");
}

void write_darray_to_file(FILE *hFile, double *data, int n) {
    int i;

    for(i=0; i<n; i++) {
        fprintf(hFile, "%lf\n", data[i]);
    }
    fprintf(hFile, "---\n");
}

void write_nodeindex_to_file(int gid, int id, int *data, int n) {
    int i;
    FILE *hFile;
    char filename[4096];

    sprintf(filename, "%d.%d.index", gid, id);
    hFile = fopen(filename, "w");

    for(i=0; i<n; i++) {
        fprintf(hFile, "%d\n", data[i]);
    }

    fclose(hFile);
}


void write_pntprocs_to_file(FILE *hFile, Point_process *pnt, int n) {
    int i;

    fprintf(hFile, "--pntprocs--\n");
    for(i=0; i<n; i++) {
        fprintf(hFile, "--%d %d %d--\n", pnt[i]._type, pnt[i]._i_instance, pnt[i]._tid);
    }
    fprintf(hFile, "---\n");
}


void dump_nt_to_file(const char *filename, NrnThread *threads, int nthreads) {

    FILE *hFile;
    int i;

    NrnThreadMembList* tml;
    NetReceiveBuffer_t* nrb;

    #pragma acc wait
    //copy back all gpu data
    update_nrnthreads_on_host(threads, nthreads);

    for( i = 0; i < nthreads; i++) {

      NrnThread * nt = threads + i;

      if(nt->end <= 0) {
         //this is an empty thread
         continue;
      }

      char fname[1024];
      sprintf(fname, "%s%d.dat", filename, i);
      hFile = fopen(fname, "w");

      long int offset;
      int nmech = 0;

      fprintf(hFile, "%lu\n", nt->_ndata);
      write_darray_to_file(hFile, nt->_data, nt->_ndata);

      fprintf(hFile, "%d\n", nt->end);
      for (tml = nt->tml; tml; tml = tml->next, nmech++);
      fprintf(hFile, "%d\n", nmech);

      offset = nt->end*6;

      for (tml = nt->tml; tml; tml = tml->next) {
        int type = tml->index;
        int is_art = nrn_is_artificial_[type];
        Memb_list* ml = tml->ml;
        int n = ml->nodecount;
        int szp = nrn_prop_param_size_[type];
        int szdp = nrn_prop_dparam_size_[type];

        fprintf(hFile, "%d %d %d %d %d %ld\n", type, is_art, n, szp, szdp, offset);
        offset += n*szp;

        if (!is_art) {
            write_iarray_to_file(hFile, ml->nodeindices, ml->nodecount);
//            write_nodeindex_to_file(gid, type, ml->nodeindices, ml->nodecount);
        }

        if (szdp) {
            write_iarray_to_file(hFile, ml->pdata, n*szdp);
        }

        nrb = ml->_net_receive_buffer;
        if(nrb) {
            write_iarray_to_file(hFile, nrb->_pnt_index, nrb->_size);
            write_iarray_to_file(hFile, nrb->_weight_index, nrb->_size);
        }
      }

      if(nt->shadow_rhs_cnt) {
        write_darray_to_file(hFile, nt->_shadow_rhs, nt->shadow_rhs_cnt);
        write_darray_to_file(hFile, nt->_shadow_d, nt->shadow_rhs_cnt);
      }

      if(nt->n_weight)
          write_darray_to_file(hFile, nt->weights, nt->n_weight);

      /* vdata is tricky as it involves pointer to the separately allocated arrays in bbcore_read
      if(nt->_nvdata)
          write_darray_to_file(hFile, nt->_vdata, nt->_nvdata);
      */

      if(nt->n_pntproc) {
          write_pntprocs_to_file(hFile, nt->pntprocs, nt->n_pntproc);
      }

      fclose(hFile);

    }
}

void finalize_data_on_device() {

    /*@todo: when we have used random123 on gpu and we do this finalize,
    I am seeing cuCtxDestroy returned CUDA_ERROR_INVALID_CONTEXT error.
    THis might be due to the fact that the cuda apis (e.g. free is not
    called yet for Ramdom123 data / streams etc. So handle this better!
    */
    return;

   #ifdef _OPENACC
        acc_shutdown ( acc_device_default);
    #endif
}

void device_data_update_test(NrnThread *threads, int nthreads) {

    dump_nt_to_file("pre", threads, nthreads);

    modify_data_on_device(threads, nthreads);
    update_nrnthreads_on_host(threads, nthreads);

    dump_nt_to_file("post", threads, nthreads);
}

