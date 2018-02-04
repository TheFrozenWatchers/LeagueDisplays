
#ifndef LD_FILESYSTEM_API_H
#define LD_FILESYSTEM_API_H

#pragma once

#include <string>

namespace LeagueDisplays {
    class Filesystem {
        public:
            static void Initialize();
            static void ScreensaverInit();

            static bool Exists(const char *path);
            static bool Exists(const std::string& path);

            static void Mkdirs(const char *path);
            static void Mkdirs(const std::string& path);

            static void Delete(const char *path);
            static void Delete(const std::string& path);

            static void FileMkdirs(const std::string& path);

            static void WriteAllText(const char *path, const char *text);
            static void WriteAllText(const std::string& path, const std::string& text);

            static bool Unzip(const std::string& file, const std::string& targetDir);

            static void PIDFileSet(const char *name, unsigned short pid);
            static unsigned short PIDFileGet(const char *name);

            static std::string mWorkingDirectory;
            static std::string mExecutablePath;
            static std::string mUserHome;
            static std::string mRootDir;
            static std::string mCefCacheDir;
            static std::string mDefaultDownloadDir;
            static std::string mFilesystemCacheDir;
            static std::string mDebugLogPath;
            static std::string mSettingsFilePath;
            static std::string mPlaylistsFilePath;
    };
}

#endif // LD_FILESYSTEM_API_H
