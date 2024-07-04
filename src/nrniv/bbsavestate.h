#pragma once
#include "neuron/container/data_handle.hpp"

struct Object;
struct Section;
struct Node;
struct Prop;
struct Point_process;

// base class to allow either counting, reading, writing files, buffers, etc.
// All the BBSaveState methods take a BBSS_IO subclass and are merely iterators
class BBSS_IO {
  public:
    enum Type { IN, OUT, CNT };
    BBSS_IO();
    virtual ~BBSS_IO() {}
    virtual void i(int& j, int chk = 0) = 0;
    virtual void d(int n, double& p) = 0;
    virtual void d(int n, double* p) = 0;
    virtual void d(int n, double** p) = 0;
    virtual void d(int n, neuron::container::data_handle<double> h) = 0;
    virtual void s(char* cp, int chk = 0) = 0;
    virtual Type type() = 0;
    virtual void skip(int) {}  // only when reading
};

class BBSaveState {
  public:
    BBSaveState();
    virtual ~BBSaveState();
    virtual void apply(BBSS_IO* io);
    BBSS_IO* f;

    int counts(int** gids, int** counts);
    void core();
    void init();
    void finish();
    void gids();
    void gid2buffer(int gid, char* buffer, int size);
    void buffer2gid(int gid, char* buffer, int size);
    void gidobj(int basegid);
    void gidobj(int spgid, Object*);
    void cell(Object*);
    int cellsize(Object*);
    void section_exist_info(Section*);
    void section(Section*);
    int sectionsize(Section*);
    void seccontents(Section*);
    void node(Node*);
    void node01(Section*, Node*);
    void v_vext(Node*);
    void mech(Prop*);
    void netrecv_pp(Point_process*);
    void possible_presyn(int gid);

    void mk_base2spgid();
    void mk_pp2de();
    void mk_presyn_info();
    void del_pp2de();
};

/** BBSaveState API
 *  See save_test_bin and restore_test_bin for an example of the use of this
 *  following interface. Note in particular the use in restore_test_bin of a
 *  prior clear_event_queue() in order to allow bbss_buffer_counts to pass an
 *  assert during the restore process.
 */

/** First call to return the information needed to make the other calls. Returns
 *  a pointer used by the other methods. Caller is reponsible for freeing (using
 *  free() and not delete []) the returned gids and sizes arrays when finished.
 *  The sizes array and global_size are needed for the caller to construct
 *  proper buffer sizes to pass to bbss_save_global and bbss_save for filling
 *  in. The size of these arrays is returned in *len. They are not needed for
 *  restoring (since the caller is passing already filled in buffers that are
 *  read by bbss_restore_global and bbss_restore. The gids returned are base
 *  gids. It is the callers responsibility to somehow concatenate buffers with
 *  the same gid (from different hosts) either after save or before restore and
 *  preserve the piece count of the number of concatenated buffers for each base
 *  gid. Global_size will only be non_zero for host 0.
 */
void* bbss_buffer_counts(int* len, int** gids, int** sizes, int* global_size);

/** Call only on host 0 with a buffer of size equal to the global_size returned
 *  from the bbss_buffer_counts call on host 0 sz is the size of the buffer
 *  (for error checking only, buffer+sz is out of bounds)
 */
void bbss_save_global(void* bbss, char* buffer, int sz);

/** Call on all hosts with the buffer contents returned from the call to
 *  bbss_save_global. This must be called prior to any calls to bbss_restore sz
 *  is the size of the buffer (error checking only). This also does some other
 *  important restore initializations.
 */
void bbss_restore_global(void* bbss, char* buffer, int sz);

/** Call this for each item of the gids from bbss_buffer_counts along with a
 *  buffer of size from the corresponding sizes array. The buffer will be filled
 *  in with savestate information. The gid may be the same on different hosts,
 *  in which case it is the callers responsibility to concatentate buffers at
 *  some point to allow calling of bbss_restore. sz is the size of the buffer
 *  (error checking only).
 */
void bbss_save(void* bbss, int gid, char* buffer, int sz);

/** Call this for each item of the gids from bbss_buffer_counts, the number of
 * buffers that were concatenated for the gid, and the concatenated buffer (the
 * concatenated buffer does NOT contain npiece as the first value in the char*
 * buffer pointer). sz is the size of the buffer (error checking only).
 */
void bbss_restore(void* bbss, int gid, int npiece, char* buffer, int sz);

/** At the end of the save process, call this to cleanup. When this call
 *  returns, bbss will be invalid.
 */
void bbss_save_done(void* bbss);

/** At the end of the restore process, call this to do some extra setting up and
 *  cleanup. When this call returns, bbss will be invalid.
 */
void bbss_restore_done(void* bbss);
