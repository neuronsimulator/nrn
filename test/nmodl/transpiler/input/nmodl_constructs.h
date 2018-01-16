#ifndef NMODL_TEST_CONSTRUCTS
#define NMODL_TEST_CONSTRUCTS

#include <string>
#include <map>

struct NmodlTestCase {
    /// name of the test
    std::string name;

    /// input nmodl construct
    std::string input;

    /// expected nmodl output
    std::string output;

    /// \todo : add associated json (to use in visitor test)

    NmodlTestCase() = delete;

    NmodlTestCase(std::string name, std::string input)
        : name(name), input(input), output(input) {
    }

    NmodlTestCase(std::string name, std::string input, std::string output)
            : name(name), input(input), output(output) {
    }
};

extern std::map<std::string, NmodlTestCase> nmdol_invalid_constructs;
extern std::map<std::string, NmodlTestCase> nmodl_valid_constructs;

#endif