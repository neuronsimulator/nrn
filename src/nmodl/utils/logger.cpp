/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include "utils/logger.hpp"

/**
 * \file
 * \brief \copybrief nmodl::Logger
 */

namespace nmodl {

using logger_type = std::shared_ptr<spdlog::logger>;

/**
 * \brief Logger implementation based on spdlog
 */
struct Logger {
    logger_type logger;
    Logger(const std::string& name, std::string pattern) {
        logger = spdlog::stdout_color_mt(name);
        logger->set_pattern(std::move(pattern));
    }
};

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
Logger nmodl_logger("NMODL", "[%n] [%^%l%$] :: %v");
logger_type logger = nmodl_logger.logger;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace nmodl
