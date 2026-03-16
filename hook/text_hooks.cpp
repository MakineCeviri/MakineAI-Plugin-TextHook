#include "text_hooks.h"
#include "pipe_client.h"

#include <string>
#include <cstring>
#include <mutex>

namespace hooks {

// -- Inline hook implementation --

bool InlineHook::install() {
    if (!target || !detour) return false;

    // Allocate executable memory for trampoline
    trampoline = static_cast<uint8_t*>(
        VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!trampoline) return false;

    // Save original bytes (14 bytes for x64 absolute JMP)
    memcpy(originalBytes, target, 14);

    // Build trampoline: execute original 14 bytes, then JMP back to target+14
    memcpy(trampoline, originalBytes, 14);
    // mov rax, target+14; jmp rax
    trampoline[14] = 0x48; trampoline[15] = 0xB8; // mov rax, imm64
    uint64_t returnAddr = reinterpret_cast<uint64_t>(target) + 14;
    memcpy(&trampoline[16], &returnAddr, 8);
    trampoline[24] = 0xFF; trampoline[25] = 0xE0; // jmp rax

    // Patch target with JMP to detour
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        trampoline = nullptr;
        return false;
    }

    auto* patch = static_cast<uint8_t*>(target);
    patch[0] = 0x48; patch[1] = 0xB8; // mov rax, imm64
    uint64_t detourAddr = reinterpret_cast<uint64_t>(detour);
    memcpy(&patch[2], &detourAddr, 8);
    patch[10] = 0xFF; patch[11] = 0xE0; // jmp rax
    // NOP remaining 2 bytes
    patch[12] = 0x90; patch[13] = 0x90;

    VirtualProtect(target, 14, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, 14);

    enabled = true;
    return true;
}

void InlineHook::remove() {
    if (!enabled || !target) return;

    DWORD oldProtect = 0;
    if (VirtualProtect(target, 14, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(target, originalBytes, 14);
        VirtualProtect(target, 14, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), target, 14);
    }

    if (trampoline) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        trampoline = nullptr;
    }

    enabled = false;
}

// -- Pipe client instance (shared across hooks) --

static PipeClient g_pipe;
static bool g_pipeConnected = false;

void setPipeConnected(bool connected) {
    g_pipeConnected = connected;
}

PipeClient& getPipe() {
    return g_pipe;
}

// -- Text forwarding --

void sendTextToPipe(const wchar_t* text, int len) {
    if (!g_pipeConnected || !text || len <= 0) return;

    // Convert wide string to UTF-8
    int needed = WideCharToMultiByte(CP_UTF8, 0, text, len, nullptr, 0, nullptr, nullptr);
    if (needed <= 0) return;

    std::string utf8(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, len, utf8.data(), needed, nullptr, nullptr);

    g_pipe.sendText(utf8);
}

void sendTextToPipeA(const char* text, int len) {
    if (!g_pipeConnected || !text || len <= 0) return;

    // ANSI -> Wide -> UTF-8
    int wideLen = MultiByteToWideChar(CP_ACP, 0, text, len, nullptr, 0);
    if (wideLen <= 0) return;

    std::wstring wide(wideLen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, text, len, wide.data(), wideLen);

    sendTextToPipe(wide.c_str(), wideLen);
}

// -- Glyph accumulation --

static std::mutex g_glyphMutex;
static std::wstring g_glyphBuffer;
static DWORD g_lastGlyphTime = 0;
static constexpr DWORD GLYPH_FLUSH_TIMEOUT_MS = 50;

void accumulateGlyph(wchar_t ch) {
    std::lock_guard<std::mutex> lock(g_glyphMutex);

    DWORD now = GetTickCount();
    if (!g_glyphBuffer.empty() && (now - g_lastGlyphTime > GLYPH_FLUSH_TIMEOUT_MS)) {
        // Timeout — flush previous buffer
        sendTextToPipe(g_glyphBuffer.c_str(), static_cast<int>(g_glyphBuffer.size()));
        g_glyphBuffer.clear();
    }

    if (ch == L'\n' || ch == L'\r') {
        if (!g_glyphBuffer.empty()) {
            sendTextToPipe(g_glyphBuffer.c_str(), static_cast<int>(g_glyphBuffer.size()));
            g_glyphBuffer.clear();
        }
    } else if (ch >= 0x20) {
        g_glyphBuffer += ch;
    }

    g_lastGlyphTime = now;
}

void accumulateGlyphA(char ch) {
    // Convert single ANSI char to wide
    wchar_t wc = 0;
    MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wc, 1);
    accumulateGlyph(wc);
}

void flushGlyphBuffer() {
    std::lock_guard<std::mutex> lock(g_glyphMutex);
    if (!g_glyphBuffer.empty()) {
        sendTextToPipe(g_glyphBuffer.c_str(), static_cast<int>(g_glyphBuffer.size()));
        g_glyphBuffer.clear();
    }
}

// -- Hook instances --

static InlineHook hookTextOutW;
static InlineHook hookTextOutA;
static InlineHook hookExtTextOutW;
static InlineHook hookExtTextOutA;
static InlineHook hookDrawTextW;
static InlineHook hookDrawTextA;
static InlineHook hookDrawTextExW;
static InlineHook hookDrawTextExA;
static InlineHook hookGetGlyphOutlineW;
static InlineHook hookGetGlyphOutlineA;

// -- Original function pointer access via trampoline --

// Trampolines cast — call trampoline to execute original function
#define TRAMPOLINE_CAST(hook, type) reinterpret_cast<type>(hook.trampoline)

// -- Detour functions --

static BOOL WINAPI HookedTextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c) {
    if (lpString && c > 0) sendTextToPipe(lpString, c);
    return TRAMPOLINE_CAST(hookTextOutW, decltype(&TextOutW))(hdc, x, y, lpString, c);
}

static BOOL WINAPI HookedTextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c) {
    if (lpString && c > 0) sendTextToPipeA(lpString, c);
    return TRAMPOLINE_CAST(hookTextOutA, decltype(&TextOutA))(hdc, x, y, lpString, c);
}

static BOOL WINAPI HookedExtTextOutW(HDC hdc, int x, int y, UINT options,
                                      const RECT* lprect, LPCWSTR lpString,
                                      UINT c, const INT* lpDx) {
    if (lpString && c > 0) sendTextToPipe(lpString, static_cast<int>(c));
    return TRAMPOLINE_CAST(hookExtTextOutW, decltype(&ExtTextOutW))(
        hdc, x, y, options, lprect, lpString, c, lpDx);
}

static BOOL WINAPI HookedExtTextOutA(HDC hdc, int x, int y, UINT options,
                                      const RECT* lprect, LPCSTR lpString,
                                      UINT c, const INT* lpDx) {
    if (lpString && c > 0) sendTextToPipeA(lpString, static_cast<int>(c));
    return TRAMPOLINE_CAST(hookExtTextOutA, decltype(&ExtTextOutA))(
        hdc, x, y, options, lprect, lpString, c, lpDx);
}

static int WINAPI HookedDrawTextW(HDC hdc, LPCWSTR lpchText, int cchText,
                                   LPRECT lprc, UINT format) {
    if (lpchText && cchText > 0)
        sendTextToPipe(lpchText, cchText);
    else if (lpchText && cchText == -1)
        sendTextToPipe(lpchText, static_cast<int>(wcslen(lpchText)));
    return TRAMPOLINE_CAST(hookDrawTextW, decltype(&DrawTextW))(
        hdc, lpchText, cchText, lprc, format);
}

static int WINAPI HookedDrawTextA(HDC hdc, LPCSTR lpchText, int cchText,
                                   LPRECT lprc, UINT format) {
    if (lpchText && cchText > 0)
        sendTextToPipeA(lpchText, cchText);
    else if (lpchText && cchText == -1)
        sendTextToPipeA(lpchText, static_cast<int>(strlen(lpchText)));
    return TRAMPOLINE_CAST(hookDrawTextA, decltype(&DrawTextA))(
        hdc, lpchText, cchText, lprc, format);
}

static int WINAPI HookedDrawTextExW(HDC hdc, LPWSTR lpchText, int cchText,
                                     LPRECT lprc, UINT format, LPDRAWTEXTPARAMS lpdtp) {
    if (lpchText && cchText > 0)
        sendTextToPipe(lpchText, cchText);
    else if (lpchText && cchText == -1)
        sendTextToPipe(lpchText, static_cast<int>(wcslen(lpchText)));
    return TRAMPOLINE_CAST(hookDrawTextExW, decltype(&DrawTextExW))(
        hdc, lpchText, cchText, lprc, format, lpdtp);
}

static int WINAPI HookedDrawTextExA(HDC hdc, LPSTR lpchText, int cchText,
                                     LPRECT lprc, UINT format, LPDRAWTEXTPARAMS lpdtp) {
    if (lpchText && cchText > 0)
        sendTextToPipeA(lpchText, cchText);
    else if (lpchText && cchText == -1)
        sendTextToPipeA(lpchText, static_cast<int>(strlen(lpchText)));
    return TRAMPOLINE_CAST(hookDrawTextExA, decltype(&DrawTextExA))(
        hdc, lpchText, cchText, lprc, format, lpdtp);
}

static DWORD WINAPI HookedGetGlyphOutlineW(HDC hdc, UINT uChar, UINT fuFormat,
                                             LPGLYPHMETRICS lpgm, DWORD cjBuffer,
                                             LPVOID pvBuffer, const MAT2* lpmat2) {
    if (fuFormat != GGO_METRICS)
        accumulateGlyph(static_cast<wchar_t>(uChar));
    return TRAMPOLINE_CAST(hookGetGlyphOutlineW, decltype(&GetGlyphOutlineW))(
        hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);
}

static DWORD WINAPI HookedGetGlyphOutlineA(HDC hdc, UINT uChar, UINT fuFormat,
                                             LPGLYPHMETRICS lpgm, DWORD cjBuffer,
                                             LPVOID pvBuffer, const MAT2* lpmat2) {
    if (fuFormat != GGO_METRICS)
        accumulateGlyphA(static_cast<char>(uChar));
    return TRAMPOLINE_CAST(hookGetGlyphOutlineA, decltype(&GetGlyphOutlineA))(
        hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);
}

// -- Install / Remove --

static void* resolveGdiFunc(const char* name) {
    HMODULE hGdi = GetModuleHandleW(L"gdi32.dll");
    if (!hGdi) hGdi = LoadLibraryW(L"gdi32.dll");
    return hGdi ? reinterpret_cast<void*>(GetProcAddress(hGdi, name)) : nullptr;
}

static void* resolveUser32Func(const char* name) {
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (!hUser) hUser = LoadLibraryW(L"user32.dll");
    return hUser ? reinterpret_cast<void*>(GetProcAddress(hUser, name)) : nullptr;
}

bool installTextHooks() {
    bool anyInstalled = false;

    auto tryInstall = [&](InlineHook& hook, void* target, void* detour, const char* name) {
        if (!target) return;
        hook.target = target;
        hook.detour = detour;
        if (hook.install()) {
            anyInstalled = true;
        }
    };

    // GDI text functions
    tryInstall(hookTextOutW, resolveGdiFunc("TextOutW"),
               reinterpret_cast<void*>(&HookedTextOutW), "TextOutW");
    tryInstall(hookTextOutA, resolveGdiFunc("TextOutA"),
               reinterpret_cast<void*>(&HookedTextOutA), "TextOutA");
    tryInstall(hookExtTextOutW, resolveGdiFunc("ExtTextOutW"),
               reinterpret_cast<void*>(&HookedExtTextOutW), "ExtTextOutW");
    tryInstall(hookExtTextOutA, resolveGdiFunc("ExtTextOutA"),
               reinterpret_cast<void*>(&HookedExtTextOutA), "ExtTextOutA");

    // User32 draw text functions
    tryInstall(hookDrawTextW, resolveUser32Func("DrawTextW"),
               reinterpret_cast<void*>(&HookedDrawTextW), "DrawTextW");
    tryInstall(hookDrawTextA, resolveUser32Func("DrawTextA"),
               reinterpret_cast<void*>(&HookedDrawTextA), "DrawTextA");
    tryInstall(hookDrawTextExW, resolveUser32Func("DrawTextExW"),
               reinterpret_cast<void*>(&HookedDrawTextExW), "DrawTextExW");
    tryInstall(hookDrawTextExA, resolveUser32Func("DrawTextExA"),
               reinterpret_cast<void*>(&HookedDrawTextExA), "DrawTextExA");

    // GDI glyph functions
    tryInstall(hookGetGlyphOutlineW, resolveGdiFunc("GetGlyphOutlineW"),
               reinterpret_cast<void*>(&HookedGetGlyphOutlineW), "GetGlyphOutlineW");
    tryInstall(hookGetGlyphOutlineA, resolveGdiFunc("GetGlyphOutlineA"),
               reinterpret_cast<void*>(&HookedGetGlyphOutlineA), "GetGlyphOutlineA");

    return anyInstalled;
}

void removeTextHooks() {
    flushGlyphBuffer();

    hookTextOutW.remove();
    hookTextOutA.remove();
    hookExtTextOutW.remove();
    hookExtTextOutA.remove();
    hookDrawTextW.remove();
    hookDrawTextA.remove();
    hookDrawTextExW.remove();
    hookDrawTextExA.remove();
    hookGetGlyphOutlineW.remove();
    hookGetGlyphOutlineA.remove();
}

} // namespace hooks
