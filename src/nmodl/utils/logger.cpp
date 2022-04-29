/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

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

Logger nmodl_logger("NMODL", "[%n] [%^%l%$] :: %v");
logger_type logger = nmodl_logger.logger;

}  // namespace nmodl
