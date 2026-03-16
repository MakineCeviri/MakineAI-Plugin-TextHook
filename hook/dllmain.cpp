#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "text_hooks.h"
#include "pipe_client.h"

static PipeClient g_pipeClient;

static void onAttach(HMODULE hModule) {
    // Connect to the pipe server in the launcher process.
    // The pipe name uses this process's PID — the launcher creates the pipe
    // with the target PID before injecting.
    DWORD myPid = GetCurrentProcessId();

    if (!g_pipeClient.connect(myPid)) {
        // Could not connect to pipe — hooks will still install but text
        // won't be forwarded. The launcher may not be running the server yet.
    }

    hooks::setPipeConnected(g_pipeClient.isConnected());

    // Install all GDI text hooks
    hooks::installTextHooks();
}

static void onDetach() {
    // Remove all hooks first so no new text arrives
    hooks::removeTextHooks();

    // Disconnect pipe
    g_pipeClient.disconnect();
    hooks::setPipeConnected(false);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID /*reserved*/) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        onAttach(hModule);
        break;
    case DLL_PROCESS_DETACH:
        onDetach();
        break;
    default:
        break;
    }
    return TRUE;
}
