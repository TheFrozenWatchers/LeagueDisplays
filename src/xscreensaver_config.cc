
#include "xscreensaver_config.h"

#include <fstream>
#include <sstream>
#include <cstdio>
#include <assert.h>

namespace LeagueDisplays {
    XScreensaverConfig::XScreensaverConfig(const std::string& file) {
        this->mFilePath = file;
        this->Reload();
    }

    void XScreensaverConfig::Reload() {
        std::ifstream infile(this->mFilePath);
        std::string line, key, value;
        size_t pos;
        bool split_line = false;
        int lineno = 0;
        
        this->mConfig.clear();
        while (std::getline(infile, line)) {
            lineno++;
            pos = line.find('#');

            if (pos != std::string::npos) {
                line = line.substr(0, pos);
            }

            while (line[0] == ' ' || line[0] == '\t') {
                line = line.substr(1);
            }

            if (line.empty()) {
                split_line = false;
                continue;
            }

            if (!split_line) {
                // Store key/value pair
                if (!key.empty() && !value.empty()) {
                    this->mConfig.insert(std::pair<std::string, std::string>(key, value));
                }

                // Get config key & value
                pos = line.find(':');

                if (pos != std::string::npos) {
                    key = line.substr(0, pos);
                    value = line.substr(pos + 1);

                    while (value[0] == ' ' || value[0] == '\t') {
                        value = value.substr(1);
                    }
                } else {
                    printf("Syntax error: missing ':' in line %d\n", lineno);
                    continue;
                }

                while (value.length() && (value[value.length() - 1] == ' ' || value[value.length() - 1] == '\t')) {
                    value = value.substr(0, value.length() - 1);
                }

                if (value[value.length() - 1] == '\\') {
                    split_line = true;
                    value = value.substr(0, value.length() - 1);
                }

                while ((pos = value.find("\\n")) != std::string::npos) {
                    value = value.substr(0, pos) + "\n" + value.substr(pos + 2);
                }

                continue;
            } else {
                split_line = false;

                while (line.length() && (line[line.length() - 1] == ' ' || line[line.length() - 1] == '\t')) {
                    line = line.substr(0, line.length() - 1);
                }

                if (line[line.length() - 1] == '\\') {
                    split_line = true;
                    line = line.substr(0, line.length() - 1);
                }

                while ((pos = line.find("\\n")) != std::string::npos) {
                    line = line.substr(0, pos) + "\n" + line.substr(pos + 2);
                }

                value += line;
                continue;
            }
        }

        // Store key/value pair
        if (!key.empty() && !value.empty()) {
            this->mConfig.insert(std::pair<std::string, std::string>(key, value));
        }

        this->mPrograms.clear();
        if (this->HasKey("programs")) {
            std::stringstream ss(this->GetKeyValue("programs"));

            ld_screensaver_program_t prg;

            while (std::getline(ss, line)) {
                prg.enabled = line[0] != '-';
                
                while (line[0] == '-' || line[0] == ' ' || line[0] == '\t') {
                    line = line.substr(1);
                }

                while (line[line.length() - 1] == ' ' || line[line.length() - 1] == '\t') {
                    line = line.substr(0, line.length() - 1);
                }

                if ((pos = line.find(':')) != std::string::npos && pos < 40 /*?*/) {
                    prg.mode = line.substr(0, pos);
                    line = line.substr(pos + 1);

                    while (line[0] == ' ' || line[0] == '\t') {
                        line = line.substr(1);
                    }
                } else {
                    prg.mode = "";
                }

                prg.commandline = line;
                
                // Don't look at the following line, I just wanted to make it in a single substr
                prg.name = prg.commandline.substr((pos = prg.commandline.substr(0, (pos = prg.commandline.find(' ')) == std::string::npos ? (pos = prg.commandline.find('\t')) == std::string::npos ? prg.commandline.length() : pos : pos).rfind('/')) == std::string::npos ? 0 : pos + 1, (-pos - 1) + ((pos = prg.commandline.find(' ')) == std::string::npos ? (pos = prg.commandline.find('\t')) == std::string::npos ? prg.commandline.length() : pos : pos));

                this->mPrograms.push_back(prg);
            }
        }
    }

    void XScreensaverConfig::Save() {
        this->SaveAs(this->mFilePath);
    }

    void XScreensaverConfig::SaveAs(const std::string& file) {
        FILE* f = fopen(file.c_str(), "w");
        assert(f);

        fputs("# XScreenSaver Preferences File\n", f);
        fputs("# Written by leaguedisplays\n", f);
        fputs("\n", f);

        if (this->mPrograms.size() > 0) {
            std::string val = "";

            for (ld_screensaver_program_t prg : this->mPrograms) {
                if (!prg.enabled) {
                    val += "- ";
                }

                if (prg.mode != "") {
                    val += prg.mode + ": ";
                }

                val += "                              ";
                val += prg.commandline + "           \n";
            }

            this->SetKeyValue("programs", val);
        }

        size_t pos, pos2;
        std::string value;

        for (std::pair<std::string, std::string> p : this->mConfig) {
            value = p.second;
            
            pos2 = 0;

            // Don't ask, this works
            while ((pos = value.substr(pos2).find("\n")) != std::string::npos) {
                value = value.substr(0, pos2 + pos) + "\\n\\" + value.substr(pos2 + pos);
                pos2 = pos2 + pos + 4;
            }

            fprintf(f, "%s: %s\n", p.first.c_str(), value.c_str());
        }

        fclose(f);
    }

    bool XScreensaverConfig::HasKey(const std::string& name) {
        return this->mConfig.find(name) != this->mConfig.end();
    }

    std::string XScreensaverConfig::GetKeyValue(const std::string& name) {
        if (!this->HasKey(name)) {
            throw std::invalid_argument("Key does not exist");
        }

        return this->mConfig.at(name);
    }

    void XScreensaverConfig::SetKeyValue(const std::string& name, const std::string& value) {
        if (this->mConfig.find(name) != this->mConfig.end()) {
            this->mConfig.erase(this->mConfig.find(name));
        }

        this->mConfig.insert(std::pair<std::string, std::string>(name, value));
    }

    bool XScreensaverConfig::ScreensaverExists(const std::string& name) {
        for (ld_screensaver_program_t prg : this->mPrograms) {
            if (prg.name == name) {
                return true;
            }
        }

        return false;
    }

    std::string XScreensaverConfig::GetScreensaverCommandLine(const std::string& name) {
        for (ld_screensaver_program_t prg : this->mPrograms) {
            if (prg.name == name) {
                return prg.commandline;
            }
        }

        return "";
    }

    void XScreensaverConfig::AddOrModifyScreensaver(const std::string& name, const std::string& commandline, const std::string& mode, bool enabled) {
        for (ld_screensaver_program_t& prg : this->mPrograms) {
            if (prg.name == name) {
                prg.enabled = enabled;
                prg.mode = mode;
                prg.commandline = commandline;
                return;
            }
        }

        ld_screensaver_program_t newprg;
        newprg.name = name;
        newprg.commandline = commandline;
        newprg.enabled = enabled;
        newprg.mode = mode;
        this->mPrograms.push_back(newprg);
    }
}
