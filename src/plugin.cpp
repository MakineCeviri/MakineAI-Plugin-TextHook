/**
 * MakineAI TextHook Plugin — Entry point
 *
 * Provides game text extraction via memory hooking.
 * Methods: MinHook (inline), VEH (breakpoint), Memory Read (polling)
 */

#include <makineai/plugin/plugin_api.h>
#include <cstring>

namespace hook {
    bool init(const char* dataPath);
    void shutdown();
    bool ready();
}

static bool s_initialized = false;
static char s_error[512] = "";

extern "C" __declspec(dllexport)
MakineAiPluginInfo makineai_get_info(void)
{
    return {
        "com.makineceviri.texthook",
        "MakineAI TextHook",
        "0.1.0",
        MAKINEAI_PLUGIN_API_VERSION
    };
}

extern "C" __declspec(dllexport)
MakineAiError makineai_initialize(const char* dataPath)
{
    if (s_initialized) return MAKINEAI_OK;

    if (!dataPath) {
        std::strncpy(s_error, "dataPath is null", sizeof(s_error) - 1);
        return MAKINEAI_ERR_INVALID_PARAM;
    }

    if (!hook::init(dataPath))
        return MAKINEAI_ERR_INIT_FAILED;

    s_initialized = true;
    return MAKINEAI_OK;
}

extern "C" __declspec(dllexport)
void makineai_shutdown(void)
{
    if (s_initialized) {
        hook::shutdown();
        s_initialized = false;
    }
}

extern "C" __declspec(dllexport)
bool makineai_is_ready(void)
{
    return s_initialized && hook::ready();
}

extern "C" __declspec(dllexport)
const char* makineai_get_last_error(void)
{
    return s_error;
}
