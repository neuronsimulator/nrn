#pragma once
/**
 * @brief Header wrapping <filesystem> to help us use it with older compilers.
 *
 * It should be safe-ish to use this inside functions, but using it in interfaces is almost
 * certainly a bad idea.
 */
#if __has_include(<filesystem>)
#include <filesystem>
namespace neuron::std {
namespace filesystem = ::std::filesystem;
}
#else
#include <experimental/filesystem>
namespace neuron::std {
namespace filesystem = ::std::experimental::filesystem;
}
#endif
