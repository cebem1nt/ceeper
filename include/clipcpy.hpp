#pragma once

#include <string>
#include <cstdlib>

#if defined(_WIN32)

#elif defined(__APPLE__)

#elif defined(__linux__) || defined(__ANDROID__)

inline bool clipcpy(const std::string& text) 
{
    FILE* f = nullptr;

    if (std::getenv("TERMUX_VERSION") != nullptr) {
        // Termux (requires termux api)
        if ((f = popen("termux-clipboard-set", "w"))) {  
            fwrite(text.data(), 1, text.size(), f);
            pclose(f);
            return true;
        }
    }

    auto is_wayland = []() {
        return std::getenv("WAYLAND_DISPLAY") != nullptr ||
            (std::getenv("XDG_SESSION_TYPE") &&
                std::string(std::getenv("XDG_SESSION_TYPE")) == "wayland");
    };

    if (is_wayland()) {
        if (!(f = popen("wl-copy", "w")))
            return false;
    } else {
        if (!(f = popen("xclip -selection c", "w")))
            return false;
    }

    fwrite(text.data(), 1, text.size(), f);
    pclose(f);

    return true;
}

#else
inline bool clipcpy(const std::string&) { return false; }
#endif
