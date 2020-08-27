#include "nrncore_io.h"
#include "nrncore_write/data/cell_group.h"
#include "nrncore_write/callbacks/nrncore_callbacks.h"

#include <cstdlib>
#include "nrnmpi.h"
#include "section.h"
#include "hocdec.h"
#include "ocfile.h" // for idDirExist and makePath
#include "nrnran123.h" // globalindex written to globals.dat
#include "cvodeobj.h"
#include "netcvode.h" // for nrnbbcore_vecplay_write
#include "vrecitem.h" // for nrnbbcore_vecplay_write
#include <fstream>
#include <sstream>
#include "nrnsection_mapping.h"

extern "C" {
extern short* nrn_is_artificial_;
extern int* bbcore_dparam_size;
extern bbcore_write_t* nrn_bbcore_write_;
extern NetCvode* net_cvode_instance;
extern void (*nrnthread_v_transfer_)(NrnThread*);
}

int chkpnt;
const char *bbcore_write_version = "1.2";

std::string get_write_path(){
    std::string path("."); // default path
    if (ifarg(1)) {
        path = hoc_gargstr(1);
        if (nrnmpi_myid == 0) {
            if (!isDirExist(path)) {
                if (!makePath(path)) {
                    hoc_execerror(path.c_str(), "directory did not exist and makePath for it failed");
                }
            }
        }
        #ifdef NRNMPI
        nrnmpi_barrier();
        #endif
    }
    return path;
}

std::string get_filename(const std::string& path, std::string file_name){
    std::string fname(path+'/'+file_name);
    nrn_assert(fname.size() < 1024);
    return fname;
}


void write_memb_mech_types(const char* fname) {
    if (nrnmpi_myid > 0) { return; } // only rank 0 writes this file
    std::ofstream fs(fname);
    if (!fs.good()) {
        hoc_execerror("nrnbbcore_write write_mem_mech_types could not open for writing: %s\n", fname);
    }
    write_memb_mech_types_direct(fs);
}


// format is name value
// with last line of 0 0
//In case of an array, the line is name[num] with num lines following with
// one value per line.  Values are %.20g format.
void write_globals(const char* fname) {

    if (nrnmpi_myid > 0) { return; } // only rank 0 writes this file

    FILE* f = fopen(fname, "w");
    if (!f) {
        hoc_execerror("nrnbbcore_write write_globals could not open for writing: %s\n", fname);
    }

    fprintf(f, "%s\n", bbcore_write_version);
    const char* name;
    int size; // 0 means scalar, is 0 will still allocated one element for val.
    double* val; // Allocated by new in get_global_item, must be delete [] here.
    for (void* sp = NULL;
         (sp = get_global_dbl_item(sp, name, size, val)) != NULL;) {
        if (size) {
            fprintf(f, "%s[%d]\n", name, size);
            for (int i=0; i < size; ++i) {
                fprintf(f, "%.20g\n", val[i]);
            }
        }else{
            fprintf(f, "%s %.20g\n", name, val[0]);
        }
        delete [] val;
    }
    fprintf(f, "0 0\n");
    fprintf(f, "secondorder %d\n", secondorder);
    fprintf(f, "Random123_globalindex %d\n", nrnran123_get_globalindex());

    fclose(f);
}


void write_nrnthread(const char* path, NrnThread& nt, CellGroup& cg) {
    char fname[1000];
    if (cg.n_output <= 0) { return; }
    assert(cg.group_id >= 0);
    nrn_assert(snprintf(fname, 1000, "%s/%d_1.dat", path, cg.group_id) < 1000);
    FILE* f = fopen(fname, "wb");
    if (!f) {
        hoc_execerror("nrnbbcore_write write_nrnthread could not open for writing:", fname);
    }
    fprintf(f, "%s\n", bbcore_write_version);

    //nrnthread_dat1(int tid, int& n_presyn, int& n_netcon, int*& output_gid, int*& netcon_srcgid);
    fprintf(f, "%d npresyn\n", cg.n_presyn);
    fprintf(f, "%d nnetcon\n", cg.n_netcon);
    writeint(cg.output_gid, cg.n_presyn);
    writeint(cg.netcon_srcgid, cg.n_netcon);

    if (cg.output_gid) {delete [] cg.output_gid; cg.output_gid = NULL; }
    if (cg.netcon_srcgid) {delete [] cg.netcon_srcgid; cg.netcon_srcgid = NULL; }
    fclose(f);

    nrn_assert(snprintf(fname, 1000, "%s/%d_2.dat", path, cg.group_id) < 1000);
    f = fopen(fname, "w");
    if (!f) {
        hoc_execerror("nrnbbcore_write write_nrnthread could not open for writing:", fname);
    }

    fprintf(f, "%s\n", bbcore_write_version);

    // sizes and total data count
    int ngid, n_real_gid, nnode, ndiam, nmech, *tml_index, *ml_nodecount, nidata,
            nvdata, nweight;
    nrnthread_dat2_1(nt.id, ngid, n_real_gid, nnode, ndiam,
                     nmech, tml_index, ml_nodecount, nidata, nvdata, nweight);

    fprintf(f, "%d ngid\n", ngid);
    fprintf(f, "%d n_real_gid\n", n_real_gid);
    fprintf(f, "%d nnode\n", nnode);
    fprintf(f, "%d ndiam\n", ndiam);
    fprintf(f, "%d nmech\n", nmech);

    for (int i=0; i < nmech; ++i) {
        fprintf(f, "%d\n", tml_index[i]);
        fprintf(f, "%d\n", ml_nodecount[i]);
    }
    delete [] tml_index;
    delete [] ml_nodecount;

    fprintf(f, "%d nidata\n", 0);
    fprintf(f, "%d nvdata\n", nvdata);
    fprintf(f, "%d nweight\n", nweight);

    // data
    int *v_parent_index=NULL; double *a=NULL, *b=NULL, *area=NULL, *v=NULL, *diamvec=NULL;
    nrnthread_dat2_2(nt.id, v_parent_index, a, b, area, v, diamvec);
    assert(cg.n_real_output == nt.ncell);
    writeint(nt._v_parent_index, nt.end);
    writedbl(nt._actual_a, nt.end);
    writedbl(nt._actual_b, nt.end);
    writedbl(nt._actual_area, nt.end);
    writedbl(nt._actual_v, nt.end);
    if (cg.ndiam) {
        writedbl(diamvec, nt.end);
        delete [] diamvec;
    }

    // mechanism data
    int dsz_inst = 0;
    MlWithArt& mla = cg.mlwithart;
    for (size_t i = 0; i < mla.size(); ++i) {
        int type = mla[i].first;
        int *nodeindices=NULL, *pdata=NULL; double* data=NULL;
        nrnthread_dat2_mech(nt.id, i, dsz_inst, nodeindices, data, pdata);
        Memb_list* ml = mla[i].second;
        int n = ml->nodecount;
        int sz = nrn_prop_param_size_[type];
        if (nodeindices) {
            writeint(nodeindices, n);
        }
        writedbl(data, n * sz);
        if (nrn_is_artificial_[type]) {
            delete [] data;
        }
        sz = bbcore_dparam_size[type];
        if (pdata) {
            ++dsz_inst;
            writeint(pdata, n * sz);
            delete [] pdata;
        }
    }

    int *output_vindex, *netcon_pnttype, *netcon_pntindex;
    double *output_threshold, *weights, *delays;
    nrnthread_dat2_3(nt.id, nweight, output_vindex, output_threshold,
                     netcon_pnttype, netcon_pntindex, weights, delays);
    writeint(output_vindex, cg.n_presyn);
    writedbl(output_threshold, cg.n_real_output);
    delete [] output_threshold;

    // connections
    int n = cg.n_netcon;
//printf("n_netcon=%d nweight=%d\n", n, nweight);
    writeint(netcon_pnttype, n);
    delete [] netcon_pnttype;
    writeint(netcon_pntindex, n);
    delete [] netcon_pntindex;
    writedbl(weights, nweight);
    delete [] weights;
    writedbl(delays, n);
    delete [] delays;

    // special handling for BBCOREPOINTER
    // how many mechanisms require it
    nrnthread_dat2_corepointer(nt.id, n);
    fprintf(f, "%d bbcorepointer\n", n);
    // for each of those, what is the mech type and data size
    // and what is the data
    for (size_t i = 0; i < mla.size(); ++i) {
        int type = mla[i].first;
        if (nrn_bbcore_write_[type]) {
            int icnt, dcnt, *iArray; double* dArray;
            nrnthread_dat2_corepointer_mech(nt.id, type, icnt, dcnt, iArray, dArray);
            fprintf(f, "%d\n", type);
            fprintf(f, "%d\n%d\n", icnt, dcnt);
            if (icnt) {
                writeint(iArray, icnt);
                delete [] iArray;
            }
            if (dcnt)
            {
                writedbl(dArray, dcnt);
                delete [] dArray;
            }
        }
    }

    nrnbbcore_vecplay_write(f, nt);

    fclose(f);
}




void writeint_(int* p, size_t size, FILE* f) {
    fprintf(f, "chkpnt %d\n", chkpnt++);
    size_t n = fwrite(p, sizeof(int), size, f);
    assert(n == size);
}

void writedbl_(double* p, size_t size, FILE* f) {
    fprintf(f, "chkpnt %d\n", chkpnt++);
    size_t n = fwrite(p, sizeof(double), size, f);
    assert(n == size);
}

#define writeint(p,size) writeint_(p, size, f)
#define writedbl(p,size) writedbl_(p, size, f)

void write_contiguous_art_data(double** data, int nitem, int szitem, FILE* f) {
    fprintf(f, "chkpnt %d\n", chkpnt++);
    // the assumption is that an fwrite of nitem groups of szitem doubles can be
    // fread as a single group of nitem*szitem doubles.
    for (int i = 0; i < nitem; ++i) {
        size_t n = fwrite(data[i], sizeof(double), szitem, f);
        assert(n == szitem);
    }
}

double* contiguous_art_data(double** data, int nitem, int szitem) {
    double* d1 = new double[nitem*szitem];
    int k = 0;
    for (int i = 0; i < nitem; ++i) {
        for (int j=0; j < szitem; ++j) {
            d1[k++] = data[i][j];
        }
    }
    return d1;
}


void nrnbbcore_vecplay_write(FILE* f, NrnThread& nt) {
    // count the instances for this thread
    // error if not a VecPlayContinuous with no discon vector
    int n;
    nrnthread_dat2_vecplay(nt.id, n);
    fprintf(f, "%d VecPlay instances\n", n);
    PlayRecList* fp = net_cvode_instance->fixed_play_;
    for (int i=0; i < fp->count(); ++i) {
        int vptype, mtype, ix, sz; double *yvec, *tvec;
        if (nrnthread_dat2_vecplay_inst(nt.id, i, vptype, mtype, ix, sz, yvec, tvec)) {
            fprintf(f, "%d\n", vptype);
            fprintf(f, "%d\n", mtype);
            fprintf(f, "%d\n", ix);
            fprintf(f, "%d\n", sz);
            writedbl(yvec, sz);
            writedbl(tvec, sz);
        }
    }
}



/** Write all dataset ids to files.dat.
 *
 * Format of the files.dat file is:
 *
 *     version string
 *     -1 (if model uses gap junction)
 *     n (number of datasets)
 *     id1
 *     id2
 *     ...
 *     idN
 */
void write_nrnthread_task(const char* path, CellGroup* cgs)
{
    // ids of datasets that will be created
    std::vector<int> iSend;

    // ignore empty nrnthread (has -1 id)
    for (int iInt = 0; iInt < nrn_nthread; ++iInt)
    {
        if ( cgs[iInt].group_id >= 0) {
            iSend.push_back(cgs[iInt].group_id);
        }
    }

    // receive and displacement buffers for mpi
    std::vector<int> iRecv, iDispl;

    if (nrnmpi_myid == 0)
    {
        iRecv.resize(nrnmpi_numprocs);
        iDispl.resize(nrnmpi_numprocs);
    }

    // number of datasets on the current rank
    int num_datasets = iSend.size();

#ifdef NRNMPI
    // gather number of datasets from each task
  if (nrnmpi_numprocs > 1) {
    nrnmpi_int_gather(&num_datasets, begin_ptr(iRecv), 1, 0);
  }else{
    iRecv[0] = num_datasets;
  }
#else
    iRecv[0] = num_datasets;
#endif

    // total number of datasets across all ranks
    int iSumThread = 0;

    // calculate mpi displacements
    if (nrnmpi_myid == 0)
    {
        for (int iInt = 0; iInt < nrnmpi_numprocs; ++iInt)
        {
            iDispl[iInt] = iSumThread;
            iSumThread += iRecv[iInt];
        }
    }

    // buffer for receiving all dataset ids
    std::vector<int> iRecvVec(iSumThread);

#ifdef NRNMPI
    // gather ids into the array with correspondent offsets
  if (nrnmpi_numprocs > 1) {
    nrnmpi_int_gatherv(begin_ptr(iSend), num_datasets, begin_ptr(iRecvVec), begin_ptr(iRecv), begin_ptr(iDispl), 0);
  }else{
    for (int iInt = 0; iInt < num_datasets; ++iInt)
    {
      iRecvVec[iInt] = iSend[iInt];
    }
  }
#else
    for (int iInt = 0; iInt < num_datasets; ++iInt)
    {
        iRecvVec[iInt] = iSend[iInt];
    }
#endif

    /// Writing the file with task, correspondent number of threads and list of correspondent first gids
    if (nrnmpi_myid == 0)
    {
        std::stringstream ss;
        ss << path << "/files.dat";

        std::string filename = ss.str();

        FILE *fp = fopen(filename.c_str(), "w");
        if (!fp) {
            hoc_execerror("nrnbbcore_write write_nrnthread_task could not open for writing:", filename.c_str());
        }

        fprintf(fp, "%s\n", bbcore_write_version);

        // notify coreneuron that this model involves gap junctions
        if (nrnthread_v_transfer_) {
            fprintf(fp, "-1\n");
        }

        // total number of datasets
        fprintf(fp, "%d\n", iSumThread);

        // write all dataset ids
        for (int i = 0; i < iRecvVec.size(); ++i)
        {
            fprintf(fp, "%d\n", iRecvVec[i]);
        }

        fclose(fp);
    }

}

/** @brief dump mapping information to gid_3.dat file */
void nrn_write_mapping_info(const char *path, int gid, NrnMappingInfo &minfo) {

    /** full path of mapping file */
    std::stringstream ss;
    ss << path << "/" << gid << "_3.dat";

    std::string fname(ss.str());
    FILE *f = fopen(fname.c_str(), "w");

    if (!f) {
        hoc_execerror("nrnbbcore_write could not open for writing:", fname.c_str());
    }

    fprintf(f, "%s\n", bbcore_write_version);

    /** number of gids in NrnThread */
    fprintf(f, "%zd\n", minfo.size());

    /** all cells mapping information in NrnThread */
    for(size_t i = 0; i < minfo.size(); i++) {
        CellMapping *c = minfo.mapping[i];

        /** gid, #section, #compartments,  #sectionlists */
        fprintf(f, "%d %d %d %zd\n", c->gid, c->num_sections(), c->num_segments(), c->size());

        for(size_t j = 0; j < c->size(); j++) {
            SecMapping* s = c->secmapping[j];
            /** section list name, number of sections, number of segments */
            fprintf(f, "%s %d %zd\n", s->name.c_str(), s->nsec, s->size());

            /** section - segment mapping */
            if(s->size()) {
                writeint(&(s->sections.front()), s->size());
                writeint(&(s->segments.front()), s->size());
            }
        }
    }
    fclose(f);
}
