#import <AppKit/AppKit.h>
#include <string>

// MacOS clipcy implementation
// https://github.com/suvetha24/clip_n/blob/main/clip_osx.mm
// TODO can't test it :(

inline bool clipcpy(const std::string& text) 
{
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        [pb clearContents];
        return [pb setString:[NSString stringWithUTF8String:text.c_str()]
                     forType:NSPasteboardTypeString];
    }
}