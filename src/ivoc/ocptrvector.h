#ifndef ocptrvector_h
#define ocptrvector_h

#include "oc2iv.h"
class HocCommand;

class OcPtrVector {
  public:
    OcPtrVector(std::size_t sz);
    virtual ~OcPtrVector();
    [[nodiscard]] std::size_t size() const {
        return pd_.size();
    }
    void resize(int);
    void pset(int i, neuron::container::data_handle<double> dh);
    double getval(int);
    void setval(int, double);
    void scatter(double*, int sz);
    void gather(double*, int sz);
    void ptr_update_cmd(std::unique_ptr<HocCommand> cmd);
    void ptr_update();

  public:
    std::vector<neuron::container::data_handle<double>> pd_{};
    std::unique_ptr<HocCommand> update_cmd_{};
    char* label_{};
};

#endif
