
#ifndef LD_THEMES_SERVICE_H
#define LD_THEMES_SERVICE_H

#pragma once

#include <pthread.h>
#include <vector>
#include <string>

// X11 defines 'Bool' so we need to undef it
// before including rapidjson stuff
#ifdef Bool
#undef Bool
#include "include/rapidjson/document.h"
#define Bool int
#else
#include "include/rapidjson/document.h"
#endif // Bool

#include "crossproc.h"

#ifndef ModifyOptionS
#define ModifyOptionS(name, value, type) ModifyOption(fnvHashConst(name), value, type);
#endif // ModifyOptionS

namespace LeagueDisplays {
    static const unsigned int FNV_PRIME = 16777619u;
    static const unsigned int OFFSET_BASIS = 2166136261u;
    
    template <unsigned int N>
    static constexpr unsigned int fnvHashConst(const char (&str)[N], unsigned int I = N) {
        return I == 1 ? (OFFSET_BASIS ^ str[0]) * FNV_PRIME : (fnvHashConst(str, I - 1) ^ str[I - 1]) * FNV_PRIME;
    }

    unsigned int fnvHash(const char* str);

    enum class ConfigType {
        GENERAL,
        WALLPAPER,
        SCREENSAVER
    };

    typedef struct _ld_settings_t {
        // General settings:
        int64_t     firstTimeConfig;
        int64_t     lifetimeSessions;
        int64_t     lifetimeSaves;
        int64_t     wp_imageDuration;
        int64_t     ss_videoDuration;
        bool        ss_mirrorDisplay;
        int64_t     agreeLicenseTimestamp;
        int64_t     openConfigTimestamp;
        int64_t     newestContentTimestamp;
        uint32_t    locale;
        bool        crop_images;
        int64_t     savePlaylistTimestamp;

        // Wallpaper settings:
        int64_t     wallpaper_rotate_frequency;

        // Screensaver settings:
        int64_t     screensaver_timeout;

        // Cross-process reload requests
        bool        mReloadConfig;
        bool        mReloadPlaylist;
    } ld_settings_t;

    class AppConfig {
        public:
            static AppConfig *Acquire();
            static void Release();

            void LoadSettings();
            void LoadPlaylist();
            bool ModifyOption(const std::string& fnv_name, const rapidjson::Value& value, const ConfigType type);
            bool ModifyOption(const unsigned int fnv_name, const rapidjson::Value& value, const ConfigType type);

            void ReloadIfRequested();

            std::vector<std::string>& BackgroundPlaylist();
            std::vector<std::string>& ScreensaverPlaylist();
            struct _ld_settings_t *Settings();
            bool BackgroundPlaylistChanged();
            bool ScreensaverPlaylistChanged();
        private:
            AppConfig();

            static AppConfig*                           mInstance;

            std::vector<std::string>                    mBackgroundPlaylist;
            std::vector<std::string>                    mScreensaverPlaylist;
            
            static CrossProcessMutex*                   mConfigMutex;
            CrossProcessObject<struct _ld_settings_t>*  mConfig;

            bool                                        mBackgroundPlaylistChanged;
            bool                                        mScreensaverPlaylistChanged;
    };
}

#endif // LD_THEMES_SERVICE_H
