#pragma once

#include <vector>

#include <nrnmpiuse.h>

#include "bbsimpl.h"
class KeepArgs;
#if NRNMPI
struct bbsmpibuf;
#endif

// uses the pvm packing and unpacking functions but calls the
// server directly instead of through pvmd send, recv.
// i.e. this bbs can only be on the master

class BBSDirect: public BBSImpl {
  public:
    BBSDirect();
    ~BBSDirect() override;

    bool look(const char*) override;

    void take(const char*) override;      /* blocks til something to take */
    bool look_take(const char*) override; /* returns false if nothing to take */
    // after taking use these
    int upkint() override;
    double upkdouble() override;
    void upkvec(int, double*) override;
    char* upkstr() override;  // delete [] char* when finished
    std::vector<char> upkpickle() override;

    // before posting use these
    void pkbegin() override;
    void pkint(int) override;
    void pkdouble(double) override;
    void pkvec(int, double*) override;
    void pkstr(const char*) override;
    void pkpickle(const std::vector<char>&) override;
    void post(const char*) override;

    void post_todo(int parentid) override;
    void post_result(int id) override;
    int look_take_result(int pid) override;    // returns id, or 0 if nothing
    int master_take_result(int pid) override;  // returns id
    int look_take_todo() override;             // returns id, or 0 if nothing
    int take_todo() override;                  // returns id
    void save_args(int) override;
    void return_args(int) override;

    void context() override;

    void start() override;
    void done() override;

    void perror(const char*) override;

  private:
    KeepArgs* keepargs_;
#if NRNMPI
    bbsmpibuf *sendbuf_, *recvbuf_;
#endif
};
