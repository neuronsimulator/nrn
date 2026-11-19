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

#include <cstdio>
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

// neuron_home is a char*. The CRT fopen and getenv use the active ANSI code
// page. WideCharToMultiByte succeeds when it substitutes a default or best-fit
// character, and that would make NEURONHOME a different path. Empty means the
// wide path does not fit. CP_UTF8 can represent every Unicode scalar, and
// WC_NO_BEST_FIT_CHARS is rejected for that code page.
inline std::string nrn_win_path_utf8(std::wstring const& wide) {
    if (wide.empty()) {
        return {};
    }
    int const n = WideCharToMultiByte(
        CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) {
        return {};
    }
    std::string u8(static_cast<size_t>(n), '\0');
    if (WideCharToMultiByte(
            CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), &u8[0], n, nullptr, nullptr) !=
        n) {
        return {};
    }
    return u8;
}

inline void nrn_win_report_unrepresentable_path(std::wstring const& wide) {
    std::fprintf(stderr,
                 "NEURONHOME: the install path does not fit the active ANSI code page (CP %u).\n"
                 "NEURON will not substitute characters into a different path.\n",
                 GetACP());
    HANDLE const err = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode = 0;
    if (err != nullptr && err != INVALID_HANDLE_VALUE && GetConsoleMode(err, &mode)) {
        DWORD wrote = 0;
        WriteConsoleW(err, wide.data(), static_cast<DWORD>(wide.size()), &wrote, nullptr);
        WriteConsoleW(err, L"\n", 1, &wrote, nullptr);
        return;
    }
    std::string const u8 = nrn_win_path_utf8(wide);
    if (!u8.empty()) {
        std::fprintf(stderr, "%s\n", u8.c_str());
    }
}

inline bool nrn_win_acp_roundtrip(UINT cp, std::wstring const& wide, std::string const& narrow) {
    DWORD flags = MB_ERR_INVALID_CHARS;
    int n =
        MultiByteToWideChar(cp, flags, narrow.data(), static_cast<int>(narrow.size()), nullptr, 0);
    if (n <= 0 && GetLastError() == ERROR_INVALID_FLAGS) {
        flags = 0;
        n = MultiByteToWideChar(
            cp, flags, narrow.data(), static_cast<int>(narrow.size()), nullptr, 0);
    }
    if (n <= 0) {
        return false;
    }
    std::wstring back(static_cast<size_t>(n), L'\0');
    if (MultiByteToWideChar(
            cp, flags, narrow.data(), static_cast<int>(narrow.size()), &back[0], n) != n) {
        return false;
    }
    return back == wide;
}

inline std::string nrn_win_path_acp(const std::filesystem::path& path) {
    std::wstring const wide = path.wstring();
    if (wide.empty()) {
        return {};
    }
    UINT const cp = GetACP();
    DWORD flags = 0;
    if (cp != CP_UTF8) {
        flags = WC_NO_BEST_FIT_CHARS;
    }
    BOOL used = FALSE;
    BOOL* used_ptr = flags == 0 ? nullptr : &used;
    int n = WideCharToMultiByte(
        cp, flags, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, used_ptr);
    if (n <= 0 && flags != 0 && GetLastError() == ERROR_INVALID_FLAGS) {
        flags = 0;
        used = FALSE;
        used_ptr = nullptr;
        n = WideCharToMultiByte(
            cp, flags, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    }
    // The size query can set lpUsedDefaultChar without writing a character.
    // Read the flag only from the conversion that fills narrow.
    if (n <= 0) {
        nrn_win_report_unrepresentable_path(wide);
        return {};
    }
    std::string narrow(static_cast<size_t>(n), '\0');
    used = FALSE;
    if (WideCharToMultiByte(cp,
                            flags,
                            wide.data(),
                            static_cast<int>(wide.size()),
                            &narrow[0],
                            n,
                            nullptr,
                            used_ptr) != n) {
        nrn_win_report_unrepresentable_path(wide);
        return {};
    }
    // lpUsedDefaultChar sticks on some code pages after a lossless conversion,
    // so a TRUE flag is a different directory only when the bytes do not
    // convert back. A clear flag is not enough either: best-fit can change a
    // character and leave the flag FALSE.
    bool const same = nrn_win_acp_roundtrip(cp, wide, narrow);
    if (used) {
        if (!same) {
            nrn_win_report_unrepresentable_path(wide);
            return {};
        }
    } else if (!same) {
        nrn_win_report_unrepresentable_path(wide);
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
