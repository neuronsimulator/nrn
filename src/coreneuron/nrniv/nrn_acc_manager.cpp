#include "coreneuron/nrnoc/multicore.h"
#ifdef _OPENACC
#include<openacc.h>
#endif

#ifdef CRAYPAT
#include <pat_api.h>
#endif

void mech_state(NrnThread *_nt, Memb_list *ml, int type);

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
        
        /* you could use the pragma as : */
        /* Ben: not allowed to use nt->_data */
        //double *dtmp = nt->_data;
        //#pragma acc enter data copyin(dtmp[0:nt->_ndata]) async(nt->stream_id)
        //#pragma acc wait(nt->stream_id)
        //d__data = (double *)acc_deviceptr(nt->_data);

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
        nt->compute_gpu = 1;
        //nt->compute_gpu = 0;
        printf("\n Compute thread on GPU? : %s, Stream : %d", (nt->compute_gpu)? "Yes" : "No", nt->stream_id);
    }

#endif
}

void update_nrnthreads_on_host(NrnThread *threads, int nthreads) {

#ifdef _OPENACC
        printf("\n --- Copying to Host! --- \n");

    int i;
    
    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;
        
        /* -- copy data to host -- */

        int ne = nt->end;

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
          int type = tml->index;
          int n = ml->nodecount;
          int szp = nrn_prop_param_size_[type];
          int szdp = nrn_prop_dparam_size_[type];
          int is_art = nrn_is_artificial_[type];

          acc_update_self(ml->data, n*szp*sizeof(double));

          if (!is_art) {
              acc_update_self(ml->nodeindices, n*sizeof(int));
          }

          if (szdp) {
              acc_update_self(ml->pdata, n*szdp*sizeof(int));
          }

        }

        if(nt->shadow_rhs_cnt) {
            /* copy shadow_rhs to host */
            acc_update_self(nt->_shadow_rhs, nt->shadow_rhs_cnt*sizeof(double));
            /* copy shadow_d to host */
            acc_update_self(nt->_shadow_d, nt->shadow_rhs_cnt*sizeof(double));
        }
    }
#endif

}

void update_nrnthreads_on_device(NrnThread *threads, int nthreads) {

#ifdef _OPENACC
    printf("\n --- Copying to Device! --- \n");

    int i;
    
    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;
        
        if (!nt->compute_gpu)
          continue;

        /* -- copy data to device -- */

        int ne = nt->end;

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

          acc_update_device(ml->data, n*szp*sizeof(double));

          if (!is_art) {
              acc_update_device(ml->nodeindices, n*sizeof(int));
          }

          if (szdp) {
              acc_update_device(ml->pdata, n*szdp*sizeof(int));
          }

        }

        if(nt->shadow_rhs_cnt) {
            /* copy shadow_rhs to host */
            acc_update_device(nt->_shadow_rhs, nt->shadow_rhs_cnt*sizeof(double));
            /* copy shadow_d to host */
            acc_update_device(nt->_shadow_d, nt->shadow_rhs_cnt*sizeof(double));
        }
    }
#endif

}

#ifdef __cplusplus
extern "C" {
#endif

void update_matrix_from_gpu(NrnThread *_nt){
#ifdef _OPENACC
  if (!_nt->compute_gpu)
    return;

  printf("\n Copying matrix to GPU ... ");
  // RHS and D are contigious, copy them in one go!
  //acc_update_self(_nt->_actual_rhs, 2*_nt->end*sizeof(double));
  double *rhs = _nt->_actual_rhs;
  #pragma acc update host(rhs[0:2*_nt->end]) async(_nt->stream_id)
  #pragma acc wait(_nt->stream_id)
#endif
}

void update_matrix_to_gpu(NrnThread *_nt){
#ifdef _OPENACC
  if (!_nt->compute_gpu)
    return;

  printf("\n Copying voltage to GPU ... ");
  //acc_update_device(_nt->_actual_v, _nt->end*sizeof(double));
  double *v = _nt->_actual_v;
  #pragma acc update device(v[0:_nt->end]) async(_nt->stream_id)
  #pragma acc wait(_nt->stream_id)
#endif
}

#ifdef __cplusplus
}
#endif

void modify_data_on_device(NrnThread *threads, int nthreads) {

#ifdef _OPENACC
        printf("\n --- Modifying data on device! --- \n");
#endif

    int i, j;
#if 0    
    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;
        
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
        }    

        if(nt->shadow_rhs_cnt) {
            #pragma acc parallel loop present(nt[0:1])
            for (j = 0; j < nt->shadow_rhs_cnt; ++j)
            {
              nt->_shadow_rhs[j] += 0.1;
              nt->_shadow_d[j] += 0.1;
            }
        }
    }
#endif

    #ifdef CRAYPAT
      PAT_record(PAT_STATE_ON);
    #endif

    for( i = 0; i < nthreads; i++) {

        NrnThread * nt = threads + i;

        NrnThreadMembList* tml;
        for (tml = nt->tml; tml; tml = tml->next) {

          if(tml->index == 125 && tml->ml->nodecount > 0)
          {
            printf("\n\n ------------- State for %d nodes ------------- \n\n", tml->ml->nodecount);
            Memb_list *ml = tml->ml;
 
            mech_state(nt, ml, tml->index);
          }
       }
    }

    #ifdef CRAYPAT
      PAT_record(PAT_STATE_OFF);
    #endif
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


void dump_nt_to_file(char *filename, NrnThread *threads, int nthreads) {
 
    FILE *hFile;
    int i, j;
    
    NrnThreadMembList* tml;


    for( i = 0; i < nthreads; i++) {

      char fname[1024]; 
      sprintf(fname, "%s%d.dat", filename, i);
      hFile = fopen(fname, "w");

      NrnThread * nt = threads + i;
        

      long int offset;
      int nmech = 0;


      int ne = nt->end;
      fprintf(hFile, "%d\n", nt->_ndata);
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
      }

      if(nt->shadow_rhs_cnt) {
        write_darray_to_file(hFile, nt->_shadow_rhs, nt->shadow_rhs_cnt);
        write_darray_to_file(hFile, nt->_shadow_d, nt->shadow_rhs_cnt);
      }

      fclose(hFile);

    }
}

void finalize_data_on_device(NrnThread *, int nthreads) {
   #ifdef _OPENACC
        acc_shutdown ( acc_device_default);
    #endif
}


void mech_state(NrnThread *_nt, Memb_list *ml, int type)
{
    double _v, v;
    int i, j;

    int *_ni = ml->nodeindices;
    int _cntml = ml->nodecount;
    double * restrict _p = ml->data;
    int * restrict _ppvar = ml->pdata;
    double * restrict _vec_v = _nt->_actual_v;
    double * restrict _nt_data = _nt->_data;
    int _num_compartment = _nt->end;



    const double _lqt = 2.952882641412121 ;
    const double dt=_nt->_dt;

    printf("The number of mechanisms to be solved is: %d\n", _cntml);

    int szp = nrn_prop_param_size_[type];
    int szdp = nrn_prop_dparam_size_[type];

#pragma acc parallel loop present(_ni[0:_cntml], _nt_data[0:_nt->_ndata], _p[ml->nodecount*szp], _ppvar[0:_cntml*szdp], _vec_v[0:_num_compartment])

    for (int _iml = 0; _iml < _cntml; ++_iml)
    {
        int _nd_idx = _ni[_iml];
        _v = _vec_v[_nd_idx];
        v=_v;

        _p[3*_cntml + _iml] = _nt_data[_ppvar[0*_cntml + _iml]];
        double _lmAlpha , _lmBeta , _lmInf , _lmTau , _lhAlpha , _lhBeta , _lhInf , _lhTau , _llv=0.0 , _lqt=0.0 ;
        _llv=_v;
        _lmAlpha = ( 0.182 * ( _llv - - 32.0 ) ) / ( 1.0 - ( exp ( - ( _llv - - 32.0 ) / 6.0 ) ) ) ;
        _lmBeta = ( 0.124 * ( - _llv - 32.0 ) ) / ( 1.0 - ( exp ( - ( - _llv - 32.0 ) / 6.0 ) ) ) ;
        _lmInf = _lmAlpha / ( _lmAlpha + _lmBeta ) ;
        _lmTau = ( 1.0 / ( _lmAlpha + _lmBeta ) ) / _lqt ;
        _p[1*_cntml + _iml] = _p[1*_cntml + _iml] + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / _lmTau)))*(- ( ( ( _lmInf ) ) / _lmTau ) / ( ( ( ( - 1.0) ) ) / _lmTau ) - _p[1*_cntml + _iml]) ;
        _lhAlpha = ( - 0.015 * ( _llv - - 60.0 ) ) / ( 1.0 - ( exp ( ( _llv - - 60.0 ) / 6.0 ) ) ) ;
        _lhBeta = ( - 0.015 * ( - _llv - 60.0 ) ) / ( 1.0 - ( exp ( ( - _llv - 60.0 ) / 6.0 ) ) ) ;
        _lhInf = _lhAlpha / ( _lhAlpha + _lhBeta ) ;
        _lhTau = ( 1.0 / ( _lhAlpha + _lhBeta ) ) / _lqt ;
        _p[2*_cntml + _iml] = _p[2*_cntml + _iml] + (1. - exp(dt*(( ( ( - 1.0 ) ) ) / _lhTau)))*(- ( ( ( _lhInf ) ) / _lhTau ) / ( ( ( ( - 1.0) ) ) / _lhTau ) - _p[2*_cntml + _iml]) ;
   }
}
