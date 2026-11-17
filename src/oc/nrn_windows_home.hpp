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

// GetModuleFileNameW returns the character count excluding NUL. A return
// equal to the buffer size means the path was truncated and is not usable.
// Grow until the length is below the buffer size. 32768 characters is the
// documented Windows path limit, including the NUL. cap is the first buffer
// size so a caller can prove the retry (production starts at MAX_PATH).
inline std::filesystem::path nrn_win_module_path(HMODULE module, DWORD cap = MAX_PATH) {
    constexpr DWORD limit = 32768;
    if (cap < 2) {
        cap = 2;
    }
    for (int attempt = 0; attempt < 16; ++attempt) {
        std::wstring buf(static_cast<size_t>(cap), L'\0');
        DWORD const n = GetModuleFileNameW(module, &buf[0], cap);
        if (n == 0) {
            return {};
        }
        if (n < cap) {
            buf.resize(n);
            return std::filesystem::path(std::move(buf));
        }
        if (cap >= limit) {
            return {};
        }
        cap = cap > limit / 2 ? limit : cap * 2;
    }
    return {};
}

// neuron_home is a char*. CP_ACP is the encoding GetModuleFileNameA used and
// the encoding fopen still uses. Empty means the wide path does not fit.
inline std::string nrn_win_path_acp(const std::filesystem::path& path) {
    std::wstring const wide = path.wstring();
    if (wide.empty()) {
        return {};
    }
    int const n = WideCharToMultiByte(
        CP_ACP, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) {
        return {};
    }
    std::string narrow(static_cast<size_t>(n), '\0');
    if (WideCharToMultiByte(CP_ACP,
                            0,
                            wide.data(),
                            static_cast<int>(wide.size()),
                            &narrow[0],
                            n,
                            nullptr,
                            nullptr) != n) {
        return {};
    }
    return narrow;
}

// addr is a symbol in this binary (setneuronhome lives in nrniv.dll / the exe).
inline std::filesystem::path nrn_win_neuronhome_from_symbol(const void* addr, const char* argv0) {
    HMODULE self = nullptr;
    std::filesystem::path found;
    // FROM_ADDRESS: the second argument is an address in the module, not a
    // module name. The wide entry point is that same documented ABI.
    if (addr && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   reinterpret_cast<LPCWSTR>(addr),
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
