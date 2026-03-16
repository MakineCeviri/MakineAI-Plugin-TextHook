#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>

namespace {

class PipeClient {
public:
    bool connect(DWORD targetPid) {
        std::wstring pipeName = L"\\\\.\\pipe\\MakineAI_TextHook_"
                                + std::to_wstring(targetPid);

        // Retry a few times — pipe server may not be ready yet
        for (int i = 0; i < 10; ++i) {
            m_pipe = CreateFileW(
                pipeName.c_str(),
                GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);

            if (m_pipe != INVALID_HANDLE_VALUE)
                return true;

            if (GetLastError() != ERROR_PIPE_BUSY)
                Sleep(100);
            else
                WaitNamedPipeW(pipeName.c_str(), 500);
        }
        return false;
    }

    void disconnect() {
        if (m_pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(m_pipe);
            m_pipe = INVALID_HANDLE_VALUE;
        }
    }

    bool isConnected() const {
        return m_pipe != INVALID_HANDLE_VALUE;
    }

    void sendText(const std::string& utf8Text) {
        if (m_pipe == INVALID_HANDLE_VALUE) return;
        DWORD written = 0;
        WriteFile(m_pipe, utf8Text.data(),
                  static_cast<DWORD>(utf8Text.size()), &written, nullptr);
    }

private:
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
};

} // anonymous namespace
