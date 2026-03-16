#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <cstdint>

namespace hooks {

// x64 inline hook — patches target with 14-byte absolute JMP
struct InlineHook {
    void* target = nullptr;
    void* detour = nullptr;
    uint8_t originalBytes[14]{};
    uint8_t* trampoline = nullptr;
    bool enabled = false;

    bool install();
    void remove();
};

// Install all GDI text hooks
bool installTextHooks();

// Remove all hooks and cleanup
void removeTextHooks();

// Called by hooks to forward captured text to the pipe
void sendTextToPipe(const wchar_t* text, int len);
void sendTextToPipeA(const char* text, int len);

// Glyph accumulation
void accumulateGlyph(wchar_t ch);
void accumulateGlyphA(char ch);
void flushGlyphBuffer();

// Set pipe connection for sending text
void setPipeConnected(bool connected);

} // namespace hooks
