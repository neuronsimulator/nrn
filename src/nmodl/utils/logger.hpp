/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief Implement logger based on spdlog library
 */

// clang-format off
// disable clang-format to keep order of inclusion
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// clang-format on

namespace nmodl {

using logger_type = std::shared_ptr<spdlog::logger>;
extern logger_type logger;

}  // namespace nmodl
