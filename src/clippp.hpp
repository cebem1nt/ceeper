#pragma once

#include <string>
#include <cstdlib>
#include <cstdio>
#include <utility>

#if defined(_WIN32)

#elif defined(__APPLE__)

#elif defined(__linux__)

static inline bool is_wayland() {
    return std::getenv("WAYLAND_DISPLAY") != nullptr ||
           (std::getenv("XDG_SESSION_TYPE") &&
            std::string(std::getenv("XDG_SESSION_TYPE")) == "wayland");
}

inline bool copy_to_clipboard(const std::string& text) {
    // TODO 
    std::unreachable();
}

#else
inline bool copy_to_clipboard(const std::string&) { return false; }
#endif
