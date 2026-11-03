#pragma once

#include <cstdint>

#include "RNG.h"
#include "mcran4.h"

// The decision that has to be made is whether each generator instance
// should have its own seed or only one seed for all. We choose separate
// seed for each but if arg not present or 0 then seed chosen by system.

// the addition of ilow > 0 means that value is used for the lowindex
// instead of the mcell_ran4_init global 32 bit lowindex.

class MCellRan4: public RNG {
  public:
    MCellRan4(std::uint32_t ihigh = 0, std::uint32_t ilow = 0);
    virtual ~MCellRan4();
    virtual std::uint32_t asLong() {
        return (std::uint32_t) (ilow_ == 0 ? mcell_iran4(&ihigh_) : nrnRan4int(&ihigh_, ilow_));
    }
    virtual void reset() {
        ihigh_ = orig_;
    }
    virtual double asDouble() {
        return (ilow_ == 0 ? mcell_ran4a(&ihigh_) : nrnRan4dbl(&ihigh_, ilow_));
    }
    std::uint32_t ihigh_;
    std::uint32_t orig_;
    std::uint32_t ilow_;

  private:
    static std::uint32_t cnt_;
};
