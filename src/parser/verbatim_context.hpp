#ifndef NMODL_VERBATIM_CONTEXT
#define NMODL_VERBATIM_CONTEXT

#include <iostream>

class VerbatimContext {
  public:
    void* scanner = nullptr;
    std::istream* is = nullptr;
    std::string* result = nullptr;

    VerbatimContext(std::istream* is = &std::cin) {
        init_scanner();
        this->is = is;
    }

    virtual ~VerbatimContext() {
        destroy_scanner();
        if (result) {
            delete result;
        }
    }

  protected:
    /* defined in nmodlext.l */
    void init_scanner();
    void destroy_scanner();
};

int Verbatim_parse(VerbatimContext*);

#endif  // NMODL_VERBATIM_CONTEXT
