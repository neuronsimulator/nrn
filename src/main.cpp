#include <iostream>
#include "version/version.h"

using namespace nocmodl;

int main(int argc, char* argv[]) {

    std::cout << " NOCMODL " << version::NOCMODL_VERSION << " [" << version::GIT_REVISION << "]" << std::endl;

}
