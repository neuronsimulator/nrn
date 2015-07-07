#include "coreneuron/nrnoc/multicore.h"
#ifdef _OPENACC
#include<openacc.h>
#endif

/* note: threads here are corresponding to global nrn_threads array */
void setup_nrnthreads_on_device(NrnThread *threads, int nthreads)  {

#ifdef _OPENACC

    int i;
    
    printf("\n --- Copying to Device! --- ");

    /* pointers for data struct on device, starting with d_ */

    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;
        NrnThread *d_nt;                //NrnThread on device
        
        double *d__data;                // nrn_threads->_data on device
        
        printf("\n -----------COPYING %d'th NrnThread TO DEVICE --------------- \n", i);

        /* -- copy NrnThread to device -- */
        d_nt = (NrnThread *) acc_copyin(nt, sizeof(NrnThread));

        /* -- copy _data to device -- */

        /*copy all double data for thread */
        d__data = (double *) acc_copyin(nt->_data, nt->_ndata*sizeof(double));

        /*update d_nt._data to point to device copy */
        acc_memcpy_to_device(&(d_nt->_data), &d__data, sizeof(double*));

        /* -- setup rhs, d, a, b, v, node_aread to point to device copy -- */
        double *dptr;

        dptr = d__data + 0*nt->end;
        acc_memcpy_to_device(&(d_nt->_actual_rhs), &(dptr), sizeof(double*));

        dptr = d__data + 1*nt->end;
        acc_memcpy_to_device(&(d_nt->_actual_d), &(dptr), sizeof(double*));

        dptr = d__data + 2*nt->end;
        acc_memcpy_to_device(&(d_nt->_actual_a), &(dptr), sizeof(double*));

        dptr = d__data + 3*nt->end;
        acc_memcpy_to_device(&(d_nt->_actual_b), &(dptr), sizeof(double*));

        dptr = d__data + 4*nt->end;
        acc_memcpy_to_device(&(d_nt->_actual_v), &(dptr), sizeof(double*));

        dptr = d__data + 5*nt->end;
        acc_memcpy_to_device(&(d_nt->_actual_area), &(dptr), sizeof(double*));

        /* @todo: nt._ml_list[tml->index] = tml->ml; */


        /* -- copy NrnThreadMembList list ml to device -- */

        NrnThreadMembList* tml;
        NrnThreadMembList* d_tml;
        NrnThreadMembList* d_last_tml;

        Memb_list * d_ml;
        int first_tml = 1;
        size_t offset = 6 * nt->end;

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


            dptr = d__data+offset;

            acc_memcpy_to_device(&(d_ml->data), &(dptr), sizeof(double*));
            
            int type = tml->index;
            int n = tml->ml->nodecount;
            int szp = nrn_prop_param_size_[type];
            int szdp = nrn_prop_dparam_size_[type];
            int is_art = nrn_is_artificial_[type];

            offset += n*szp;

            if (!is_art) {
                int * d_nodeindices = (int *) acc_copyin(tml->ml->nodeindices, sizeof(int)*n);
                acc_memcpy_to_device(&(d_ml->nodeindices), &d_nodeindices, sizeof(int*));
            }

            if (szdp) {
                int * d_pdata = (int *) acc_copyin(tml->ml->pdata, sizeof(int)*n*szdp);
                acc_memcpy_to_device(&(d_ml->pdata), &d_pdata, sizeof(int*));
            }

        }

        if(nt->shadow_rhs_cnt) {
            double * d_shadow_ptr;

            /* copy shadow_rhs to device and fix-up the pointer */
            d_shadow_ptr = (double *) acc_copyin(nt->_shadow_rhs, nt->shadow_rhs_cnt*sizeof(double));
            acc_memcpy_to_device(&(d_nt->_shadow_rhs), &d_shadow_ptr, sizeof(double*));

            /* copy shadow_d to device and fix-up the pointer */
            d_shadow_ptr = (double *) acc_copyin(nt->_shadow_d, nt->shadow_rhs_cnt*sizeof(double));
            acc_memcpy_to_device(&(d_nt->_shadow_d), &d_shadow_ptr, sizeof(double*));
        }
    }
#endif
}

void update_nrnthreads_on_host(NrnThread *nt) {

#ifdef _OPENACC
        printf("\n --- Copying to Host! --- ");
#endif

}
