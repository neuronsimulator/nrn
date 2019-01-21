#ifndef NMODL_LOGGER_HPP
#define NMODL_LOGGER_HPP

// clang-format off
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
// clang-format on

using logger_type = std::shared_ptr<spdlog::logger>;
extern logger_type logger;

#endif
