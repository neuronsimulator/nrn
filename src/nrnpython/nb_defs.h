#pragma once

#if defined(_WIN32)
#define NB_EXPORT   __declspec(dllexport)
#define NB_IMPORT   __declspec(dllimport)
#define NB_INLINE   __forceinline
#define NB_NOINLINE __declspec(noinline)
#define NB_INLINE_LAMBDA
#else
#define NB_EXPORT   __attribute__((visibility("default")))
#define NB_IMPORT   NB_EXPORT
#define NB_INLINE   inline __attribute__((always_inline))
#define NB_NOINLINE __attribute__((noinline))
#endif
