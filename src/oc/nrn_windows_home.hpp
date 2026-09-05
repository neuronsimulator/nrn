#pragma once

// Unix NEURONHOME is getenv or NEURON_DATA_DIR (prefix/share/nrn: lib/hoc,
// lib/nrn.defaults). Windows cannot use the compile-time data dir (relocatable
// wheel / cmake prefix). Walk from a loaded module the way Unix dladdr walks
// from libnrniv. GetModuleFileName(NULL) is the process EXE: python.exe is
// C:\Python312\python.exe and two parents is C:.
//
// Layouts, from the module file's directory upward:
//   bin/nrniv.dll -> prefix/share/nrn          (cmake install, wheel .data)
//   bin/<config>/nrniv.dll -> prefix/share/nrn (VS multi-config build tree)
//   bin/nrniv.exe -> prefix                    (setup.exe: prefix/lib/hoc)

#include <windows.h>

#include <filesystem>
#include <string>

inline bool nrn_win_is_neuronhome(const std::filesystem::path& dir) {
    std::error_code ec;
    if (std::filesystem::is_regular_file(dir / "lib" / "nrn.defaults", ec)) {
        return true;
    }
    return std::filesystem::is_directory(dir / "lib" / "hoc", ec);
}

inline std::filesystem::path nrn_win_neuronhome_from_file(std::filesystem::path file) {
    if (file.empty()) {
        return {};
    }
    std::error_code ec;
    auto dir = std::filesystem::is_directory(file, ec) ? file : file.parent_path();
    for (int i = 0; i < 6 && !dir.empty(); ++i) {
        auto const share = dir / "share" / "nrn";
        if (nrn_win_is_neuronhome(share)) {
            return share;
        }
        if (nrn_win_is_neuronhome(dir)) {
            return dir;
        }
        auto parent = dir.parent_path();
        if (parent == dir) {
            break;
        }
        dir = std::move(parent);
    }
    return {};
}

inline std::filesystem::path nrn_win_module_path(HMODULE module) {
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(module, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return {};
    }
    return std::filesystem::path(std::string(buf, n));
}

// addr is a symbol in this binary (setneuronhome lives in nrniv.dll / the exe).
inline std::filesystem::path nrn_win_neuronhome_from_symbol(const void* addr, const char* argv0) {
    HMODULE self = nullptr;
    std::filesystem::path found;
    if (addr && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCSTR>(addr),
                                   &self)) {
        found = nrn_win_neuronhome_from_file(nrn_win_module_path(self));
    }
    if (found.empty()) {
        found = nrn_win_neuronhome_from_file(nrn_win_module_path(nullptr));
    }
    if (found.empty() && argv0 && argv0[0]) {
        found = nrn_win_neuronhome_from_file(std::filesystem::path(argv0));
    }
    return found;
}
