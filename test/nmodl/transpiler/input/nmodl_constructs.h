#ifndef NMODL_TEST_CONSTRUCTS
#define NMODL_TEST_CONSTRUCTS

#include <string>
#include <map>

struct NmodlTestCase {
    std::string name;
    std::string nmodl_text;
    /// \todo : add associated json (to use in visitor test)
};

extern std::map<std::string, NmodlTestCase> nmdol_invalid_constructs;
extern std::map<std::string, NmodlTestCase> nmodl_valid_constructs;

#endif