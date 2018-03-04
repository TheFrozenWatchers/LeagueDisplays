
#ifndef LD_CROSS_DE_H
#define LD_CROSS_DE_H

#pragma once

#include <string>

namespace LeagueDisplays {
    enum class DesktopEnv : unsigned char {
        DEEPIN,
        KDE_PLASMA,
        GNOME,
        XFCE4,
        MATE,
        LXDE,
        LXQT,
        CINNAMON,
        UNITY,
        I3,
        UNKNOWN = 0xFF
    };

    class DesktopEnvApi {
        public:
            static void LogDesktopEnv();
            static DesktopEnv GetDesktopEnv();
            static void ChangeBackground(const std::string& path);
            static void OpenUrlInDefaultBrowser(const std::string& url);
        private:
            static bool CheckIfProcessIsRunning(const char* name);
    };
}

#endif // LD_CROSS_DE_H
