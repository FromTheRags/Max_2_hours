#include "common.h"
#include <cstdio>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#else
    #include <unistd.h>
#endif

static std::string get_executable_dir()
{
#if defined(_WIN32)
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH)
    {
        std::string path(buffer, len);
        size_t pos = path.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            return path.substr(0, pos + 1);
        }
    }
#elif defined(__APPLE__)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0)
    {
        std::string path(buffer);
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos)
        {
            return path.substr(0, pos + 1);
        }
    }
#else
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1)
    {
        buffer[len] = '\0';
        std::string path(buffer);
        size_t pos = path.find_last_of('/');
        if (pos != std::string::npos)
        {
            return path.substr(0, pos + 1);
        }
    }
#endif
    return "";
}

std::string get_version_string()
{
    const SDL_version *v = SDL_Linked_version();
    char buffer[96];
    snprintf(buffer, sizeof(buffer), "v%s (SDL %d.%d.%d)", APP_VERSION,
             (int)v->major, (int)v->minor, (int)v->patch);
    return std::string(buffer);
}

std::string get_asset_path(const char *name)
{
    // 1. Relatif au répertoire de travail : assets/<name>
    std::string candidate = std::string("assets/") + name;
    FILE *f = fopen(candidate.c_str(), "rb");
    if (f)
    {
        fclose(f);
        return candidate;
    }

    // 2. À côté de l'exécutable : <exe_dir>/assets/<name>
    std::string exe_dir = get_executable_dir();
    if (!exe_dir.empty())
    {
        candidate = exe_dir + "assets/" + name;
        f = fopen(candidate.c_str(), "rb");
        if (f)
        {
            fclose(f);
            return candidate;
        }
    }

    // 3. Répertoire parent : ../assets/<name> (utile en build/debug)
    candidate = std::string("../assets/") + name;
    f = fopen(candidate.c_str(), "rb");
    if (f)
    {
        fclose(f);
        return candidate;
    }

    // 4. Racine courante
    candidate = std::string(name);
    f = fopen(candidate.c_str(), "rb");
    if (f)
    {
        fclose(f);
        return candidate;
    }

    return std::string("assets/") + name;
}
