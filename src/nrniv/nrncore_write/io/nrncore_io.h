#ifndef NRN_NRNCORE_IO_H
#define NRN_NRNCORE_IO_H

#include <string>
#include <vector>

class CellGroup;
struct NrnThread;
union Datum;
struct NrnMappingInfo;

void create_dir_path(const std::string& path);
std::string get_write_path();
std::string get_filename(const std::string& path, std::string file_name);

// returns start pointer of the container's data
template <typename T>
inline T* begin_ptr(std::vector<T>& v) {
    return v.empty() ? NULL : &v[0];
}


// to avoid incompatible dataset between neuron and coreneuron
// add version string to the dataset files
extern const char* bbcore_write_version;
extern int chkpnt;

void write_memb_mech_types(const char* fname);
void write_globals(const char* fname);
void write_nrnthread(const char* fname, NrnThread& nt, CellGroup& cg);
void writeint_(int* p, size_t size, FILE* f);
void writedbl_(double* p, size_t size, FILE* f);

#define writeint(p, size) writeint_(p, size, f)
#define writedbl(p, size) writedbl_(p, size, f)
typedef void (*bbcore_write_t)(double*, int*, int*, int*, double*, Datum*, Datum*, NrnThread*);

void write_contiguous_art_data(double** data, int nitem, int szitem, FILE* f);
double* contiguous_art_data(double** data, int nitem, int szitem);
void write_nrnthread_task(const char*, CellGroup* cgs, bool append);
void nrnbbcore_vecplay_write(FILE* f, NrnThread& nt);

void nrn_write_mapping_info(const char* path, int gid, NrnMappingInfo& minfo);


#endif  // NRN_NRNCORE_IO_H
