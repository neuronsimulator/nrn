/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
