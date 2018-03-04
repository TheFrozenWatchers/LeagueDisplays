
#include "crossde.h"
#include "log.h"

#include <stdio.h>
#include <sstream>
#include <cstring>

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "DesktopEnvApi"

namespace LeagueDisplays {
    /*static*/ void DesktopEnvApi::LogDesktopEnv() {
        switch (GetDesktopEnv()) {
            case DesktopEnv::DEEPIN:
                INFO("Detected desktop: $$lDeepin");
                break;
            case DesktopEnv::KDE_PLASMA:
                INFO("Detected desktop: $$lKDE plasma");
                break;
            case DesktopEnv::GNOME:
                INFO("Detected desktop: $$lGNOME");
                break;
            case DesktopEnv::LXDE:
                INFO("Detected desktop: $$lLXDE");
                break;
            case DesktopEnv::LXQT:
                INFO("Detected desktop: $$lLXQT");
                break;
            case DesktopEnv::XFCE4:
                INFO("Detected desktop: $$lXFCE 4");
                break;
            case DesktopEnv::CINNAMON:
                INFO("Detected desktop: $$lCINNAMON");
                break;
            case DesktopEnv::UNITY:
                INFO("Detected desktop: $$lUNITY");
                break;
            case DesktopEnv::MATE:
                INFO("Detected desktop: $$lMATE");
                break;
            case DesktopEnv::I3:
                INFO("Detected desktop: $$li3");
                break;
            default:
                WARN("Unknown desktop environment");
                break;
        }
    }

    /*static*/ DesktopEnv DesktopEnvApi::GetDesktopEnv() {
        if (CheckIfProcessIsRunning("startdde"))
            return DesktopEnv::DEEPIN;

        if (CheckIfProcessIsRunning("plasmashell") || CheckIfProcessIsRunning("plasma-desktop"))
            return DesktopEnv::KDE_PLASMA;

        if (CheckIfProcessIsRunning("i3"))
            return DesktopEnv::I3;

        if (CheckIfProcessIsRunning("gnome-panel") && !strcmp(getenv("DESKTOP_SESSION"), "gnome"))
            return DesktopEnv::GNOME;

        if (CheckIfProcessIsRunning("xfce4-panel"))
            return DesktopEnv::XFCE4;

        if (CheckIfProcessIsRunning("mate-panel") || CheckIfProcessIsRunning("mate-session"))
            return DesktopEnv::MATE;

        if (CheckIfProcessIsRunning("lxqt-session"))
            return DesktopEnv::LXQT;

        if (CheckIfProcessIsRunning("lxpanel"))
            return DesktopEnv::LXDE;

        if (CheckIfProcessIsRunning("cinnamon"))
            return DesktopEnv::CINNAMON;

        if (CheckIfProcessIsRunning("unity-settings"))
            return DesktopEnv::UNITY;

        return DesktopEnv::UNKNOWN;
    }

    /*static*/ void DesktopEnvApi::ChangeBackground(const std::string& path) {
        std::ostringstream cmdstream;
    
        switch (GetDesktopEnv()) {
            case DesktopEnv::DEEPIN:
                cmdstream << "gsettings set com.deepin.wrap.gnome.desktop.background picture-uri \"file://" << path << "\"";
                break;
            case DesktopEnv::KDE_PLASMA:
                cmdstream << "dbus-send --session "
                               "--dest=org.new_wallpaper.Plasmoid --type=method_call "
                               "/org/new_wallpaper/Plasmoid/0 org.new_wallpaper.Plasmoid.SetWallpaper "
                               "string:\"" << path << "\"";
                break;
            case DesktopEnv::GNOME:
                cmdstream << "gconftool-2 --type=string --set /desktop/gnome/background/picture_filename \"" << path << "\"";
                break;
            case DesktopEnv::LXDE:
                cmdstream << "pcmanfm --set-wallpaper \"" << path << "\"";
                break;
            case DesktopEnv::LXQT:
                cmdstream << "pcmanfm-qt --set-wallpaper \"" << path << "\"";
                break;
            case DesktopEnv::XFCE4:
                cmdstream << "xfce4-set-wallpaper \"" << path << "\"";
                break;
            case DesktopEnv::CINNAMON:
                cmdstream << "gsettings set org.cinnamon.desktop.background picture-uri \"file://" << path << "\"";
                break;
            case DesktopEnv::UNITY:
                cmdstream << "gsettings set org.gnome.desktop.background picture-uri \"file://" << path << "\"";
                break;
            case DesktopEnv::MATE:
                cmdstream << "gsettings set org.mate.caja.preferences background-uri \"file://" << path << "\";";
                cmdstream << "gsettings set org.mate.background picture-filename \"" << path << "\"";
                break;
            case DesktopEnv::I3:
                cmdstream << "feh --bg-scale \"" << path << "\";";
                break;
            default:
                WARN("Unknown desktop environment, using GNOMEShell\n");
                cmdstream << "gsettings set org.gnome.desktop.background picture-uri \"file://" << path << "\"";
                break;
        }
        
        INFO("Setting wallpaper: %s\n    command: %s", path.c_str(), cmdstream.str().c_str());
        system(cmdstream.str().c_str());
    }

    /*static*/ void DesktopEnvApi::OpenUrlInDefaultBrowser(const std::string& url) {
        std::string cmd = "xdg-open \"" + url + "\"";
        INFO("CMD: %s", cmd.c_str());
        system(cmd.c_str());
    }

    /*static*/ bool DesktopEnvApi::CheckIfProcessIsRunning(const char* name) {
        char cmd[1024];

        // Gets the number of lines from the process list that match the given process name
        snprintf(cmd, 1024, "ps -A | grep '%s' | wc -l", name);

        FILE* fp = popen(cmd, "r");
        assert(fp);

        int ch;

        if (!fp) {
            fprintf(stderr,"Error popen with %s\n", cmd);
            abort();
            return false;
        }   

        ch = fgetc(fp);
        pclose(fp);

        return (ch != EOF && ch != '0');
    }
}
