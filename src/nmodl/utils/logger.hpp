#ifndef NMODL_LOGGER_HPP
#define NMODL_LOGGER_HPP

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using logger_type = std::shared_ptr<spdlog::logger>;
extern logger_type logger;

#endif
