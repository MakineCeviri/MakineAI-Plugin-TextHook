/**
 * TextHook core — initialization and hook management.
 */

#include <string>

namespace hook {

static std::string s_dataPath;

bool init(const char* dataPath)
{
    s_dataPath = dataPath;
    // TODO: Initialize MinHook library
    // TODO: Load saved hook configurations from s_dataPath/hooks.json
    return true;
}

void shutdown()
{
    // TODO: Detach all hooks
    // TODO: Uninitialize MinHook
}

bool ready()
{
    return true;
}

} // namespace hook
