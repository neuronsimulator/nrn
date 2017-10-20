#ifndef _NMODL_VERBATIM_CONTEXT_
#define _NMODL_VERBATIM_CONTEXT_

#include <iostream>

class VerbatimContext {
  public:
    void* scanner;
    std::istream* is;

    std::string* result;

    VerbatimContext(std::istream* is = &std::cin) {
        scanner = NULL;
        result = NULL;
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

#endif  // _NMODL_VERBATIM_CONTEXT_
