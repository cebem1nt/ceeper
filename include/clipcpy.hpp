#pragma once

#include <string>
#include <cstdio>

#if defined(_WIN32)

#include <Windows.h>
#include <winuser.h>

// TODO not tested yet
// https://cplusplus.com/forum/general/48837/
inline bool clipcpy(const std::string& text) 
{
    if (!OpenClipboard(nullptr))
        return false;
    
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size()+1);

    if (!hg) {
        CloseClipboard();
        return false;
    }

    memcpy(GlobalLock(hg), text.c_str(), text.size() + 1);
    GlobalUnlock(hg);
    SetClipboardData(CF_TEXT, hg);
    CloseClipboard();
    GlobalFree(hg);

    return true;
}

#elif defined(__APPLE__)

// defined in clipcpy.mm
inline bool clipcpy(const std::string& text);

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
