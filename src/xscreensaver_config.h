
#ifndef LD_XSCREENSAVER_CONFIG_H
#define LD_XSCREENSAVER_CONFIG_H

#pragma once

#include <string>
#include <map>
#include <vector>

namespace LeagueDisplays {
    typedef struct _ld_xscreensaver_program_t {
        bool enabled;
        std::string mode;
        std::string name;
        std::string commandline;
    } ld_screensaver_program_t;

    class XScreensaverConfig {
        public:
            XScreensaverConfig(const std::string& file);
            void Reload();
            void Save();
            void SaveAs(const std::string& file);

            bool HasKey(const std::string& name);
            std::string GetKeyValue(const std::string& name);
            void SetKeyValue(const std::string& name, const std::string& value);

            bool ScreensaverExists(const std::string& name);
            std::string GetScreensaverCommandLine(const std::string& name);
            void AddOrModifyScreensaver(const std::string& name, const std::string& commandline, const std::string& mode, bool enabled);
        private:
            std::string mFilePath;
            std::map<std::string, std::string> mConfig;
            std::vector<struct _ld_xscreensaver_program_t> mPrograms;
    };
}

#endif // LD_XSCREENSAVER_CONFIG_H
