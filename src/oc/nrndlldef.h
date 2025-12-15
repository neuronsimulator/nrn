#pragma once

#if defined(_WIN32)
#define NRN_DLLEXPORT __declspec(dllexport)
#define NRN_DLLIMPORT __declspec(dllimport)
#else
#define NRN_DLLEXPORT __attribute__((visibility("default")))
#define NRN_DLLIMPORT
#endif

#if defined(NRN_DLL)
#if defined(NRN_DLL_EXPORTS)
#define NRN_API NRN_DLLEXPORT
#else
#define NRN_API NRN_DLLIMPORT
#endif
#else
#define NRN_API
#endif
