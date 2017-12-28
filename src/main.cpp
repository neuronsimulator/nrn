#include <iostream>
#include "version/version.h"

using namespace nocmodl;

int main(int /*argc*/, char const** /*argv*/) {

    std::cout << " NOCMODL " << version::NOCMODL_VERSION << " [" << version::GIT_REVISION << "]" << std::endl;

}
