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
#if defined(__clang__)
#define NB_INLINE_LAMBDA __attribute__((always_inline))
#else
#define NB_INLINE_LAMBDA
#endif
#endif

#if defined(__GNUC__) && !defined(_WIN32)
#define NB_NAMESPACE nanobind __attribute__((visibility("hidden")))
#else
#define NB_NAMESPACE nanobind
#endif

#if defined(__GNUC__)
#define NB_UNLIKELY(x) __builtin_expect(bool(x), 0)
#define NB_LIKELY(x)   __builtin_expect(bool(x), 1)
#else
#define NB_LIKELY(x)   x
#define NB_UNLIKELY(x) x
#endif

#if defined(NB_SHARED)
#if defined(NB_BUILD)
#define NB_CORE NB_EXPORT
#else
#define NB_CORE NB_IMPORT
#endif
#else
#define NB_CORE
#endif

#if !defined(NB_SHARED) && defined(__GNUC__) && !defined(_WIN32)
#define NB_EXPORT_SHARED __attribute__((visibility("hidden")))
#else
#define NB_EXPORT_SHARED
#endif
