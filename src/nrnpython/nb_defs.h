#pragma once

#if defined(_WIN32)
#define NB_EXPORT   __declspec(dllexport)
#else
#define NB_EXPORT   __attribute__((visibility("default")))
#endif
