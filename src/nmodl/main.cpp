#include <iostream>
#include "version/version.h"

using namespace nmodl;

int main(int /*argc*/, char const** /*argv*/) {

    std::cout << " NMODL " << version::NMODL_VERSION << " [" << version::GIT_REVISION << "]" << std::endl;

}
