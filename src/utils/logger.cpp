#include <memory>
#include <utility>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using logger_type = std::shared_ptr<spdlog::logger>;

struct Logger {
    logger_type logger;
    Logger(const std::string& name, std::string pattern) {
        logger = spdlog::stdout_color_mt(name);
        logger->set_pattern(std::move(pattern));
    }
};

Logger nmodl_logger("NMODL", "[%n] [%^%l%$] :: %v");
logger_type logger = nmodl_logger.logger;
