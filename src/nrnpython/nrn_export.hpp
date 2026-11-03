#pragma once

#if defined(_WIN32)
#define NRN_EXPORT __declspec(dllexport)
#else
#define NRN_EXPORT __attribute__((visibility("default")))
#endif
