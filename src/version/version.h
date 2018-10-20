#pragma once

#include <string>

namespace nmodl {
    struct version {
        static const std::string GIT_REVISION;
        static const std::string NMODL_VERSION;
        std::string to_string() {
                return NMODL_VERSION + " [" + GIT_REVISION + "]";
        }
    };
}  // namespace nmodl
