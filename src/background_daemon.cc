
#include "background_daemon.h"
#include "log.h"
#include "filesystem.h"
#include "crossde.h"
#include "themes_service.h"

#include <unistd.h>

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif

#define LOGGING_SOURCE "BackgroundDaemon"

namespace LeagueDisplays {
    /*static*/ pthread_t BackgroundDaemon::mBackgroundChangerThread;
    /*static*/ int64_t BackgroundDaemon::mNextBackgroundChangeTime = 0;

    /*static*/ void BackgroundDaemon::Start() {
        INFO("Starting daemon service...");

        AppConfig* cfg = AppConfig::Acquire();
        cfg->LoadSettings();
        cfg->LoadPlaylist();
        AppConfig::Release();

        pthread_create(&mBackgroundChangerThread, NULL, WorkerThreadProc, NULL);
    }

    /*static*/ void BackgroundDaemon::Stop() {
        INFO("Stopping daemon service...");

        pthread_cancel(mBackgroundChangerThread);
    }

    /*static*/ void BackgroundDaemon::SetAutostartEnabled(bool flag) {
        std::string autostart_dir = Filesystem::mUserHome + ".config/autostart/";
        if (!Filesystem::Exists(autostart_dir)) {
            Filesystem::Mkdirs(autostart_dir);
        }

        if (!flag) {
            Filesystem::Delete(autostart_dir + "leaguedisplaysagent.desktop");
            INFO("Removed agent from autostart");
        } else {
            Filesystem::WriteAllText(autostart_dir + "leaguedisplaysagent.desktop",
                "[Desktop Entry]\n"
                "Version=1.0\n"
                "Name=LeagueDisplays Agent\n"
                "Comment=Start LeagueDisplay's agent\n"
                "Exec=" + Filesystem::mExecutablePath + " --tray --ld-fork\n"
                "Path=" + Filesystem::mWorkingDirectory + "\n"
                "Terminal=false\n"
                "Type=Application\n"
            );
            
            INFO("Added agent to autostart");
        }
    }

    /*static*/ bool BackgroundDaemon::IsAutostartEnabled() {
        return Filesystem::Exists(Filesystem::mUserHome + ".config/autostart/leaguedisplaysagent.desktop");
    }

    /*static*/ void* BackgroundDaemon::WorkerThreadProc(void* arg) {
        INFO("Created background changer thread[%08x]!", pthread_self());

        int wallpaper_index = 0;
        unsigned int current_wp_hash = 0;
        AppConfig* cfg;

        std::string base_path = Filesystem::mFilesystemCacheDir + "LeagueScreensaver/content-cache/asset/";

        while (true) {
            cfg = AppConfig::Acquire();

            cfg->ReloadIfRequested();

            if (cfg->BackgroundPlaylistChanged()) {
                DEBUG("Playlist changed!");
                wallpaper_index = -1;
                
                if (current_wp_hash) {
                    int i = 0;
                    for (std::string s : cfg->BackgroundPlaylist()) {
                        if (current_wp_hash == fnvHash(s.c_str())) {
                            DEBUG("Current background is still in the playlist, keeping it!");
                            wallpaper_index = i;
                            break;
                        }

                        i++;
                    }
                }

                if (wallpaper_index == -1) {
                    wallpaper_index = 0;
                    mNextBackgroundChangeTime = 0;
                }
            }

            if (cfg->BackgroundPlaylist().size() > 0 && mNextBackgroundChangeTime <= time(NULL)) {
                std::string wp = cfg->BackgroundPlaylist().at(wallpaper_index++);

                if (wallpaper_index >= cfg->BackgroundPlaylist().size()) {
                    wallpaper_index = 0;
                }

                unsigned int hash = fnvHash(wp.c_str());

                if (hash != current_wp_hash) {
                    std::string path = base_path + "c-o-" + wp;

                    if (Filesystem::Exists(path + ".jpg"))
                        path += ".jpg";
                    else if (Filesystem::Exists(path + ".png"))
                        path += ".png";
                    else {
                        ERROR("Could not find file for wallpaper: %s", wp.c_str());
                        goto _end;
                    }

                    DesktopEnvApi::ChangeBackground(path);
                }

                current_wp_hash = hash;

                _end:
                mNextBackgroundChangeTime = time(NULL) + cfg->Settings()->wp_imageDuration;
            }

            AppConfig::Release();
            usleep(250 * 1000);
        }

        return NULL;
    }
}
