#pragma once

// Shared-library visibility.
// NRN_DLLEXPORT: definition exported from the providing library
// NRN_DLLIMPORT: declaration imported from another DLL (MSVC)
// NRN_EXPORT: synonym of NRN_DLLEXPORT (name already used on master)
// NRN_DLLSYM: NRN_DLLEXPORT when building a NEURON DLL (NRN_DLL && NRN_DLL_EXPORTS),
//             NRN_DLLIMPORT when consuming one, empty if NRN_DLL is unset.
// Use NRN_DLLSYM on public ABI in headers once CMake defines NRN_DLL.
// Use NRN_EXPORT / NRN_DLLEXPORT on standalone exported definitions.
// Do not add one-off per-symbol macros.
// MSVC: WINDOWS_EXPORT_ALL_SYMBOLS covers functions only. Data (including
// function pointers) must be NRN_DLLSYM or the consumer gets a local copy.
// MinGW auto-exports like ELF. __declspec(dllimport) there looks up __imp_*
// and fails against an auto-export import library (Windows installer).

// PE: __declspec(dllexport) fills the export table. ELF visibility is a
// no-op on PE, so MinGW hoc.pyd needs dllexport on NRN_EXPORT
// (nrnpy_hoc / Py2NRNString in libnrnpython).
//
// Do not dllexport NRN_DLLSYM on MinGW. One explicit dllexport in a DLL
// turns off --export-all-symbols, so hoc_execerror and friends vanish
// from libnrniv.dll.a and libnrnpython fails to link (Windows installer).
// MinGW auto-exports libnrniv like ELF. dllimport stays MSVC-only.
#if defined(_WIN32)
#define NRN_DLLEXPORT __declspec(dllexport)
#else
#define NRN_DLLEXPORT __attribute__((visibility("default")))
#endif
#if defined(_MSC_VER)
#define NRN_DLLIMPORT __declspec(dllimport)
#else
#define NRN_DLLIMPORT
#endif

#define NRN_EXPORT NRN_DLLEXPORT

#if defined(NRN_DLL)
#if defined(NRN_DLL_EXPORTS)
#if defined(_MSC_VER)
#define NRN_DLLSYM NRN_DLLEXPORT
#else
#define NRN_DLLSYM
#endif
#else
#define NRN_DLLSYM NRN_DLLIMPORT
#endif
#else
#define NRN_DLLSYM
#endif
