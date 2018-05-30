
#include "themes_service.h"
#include "log.h"
#include "filesystem.h"
#include "scheme_handlers.h"
#include "background_daemon.h"
#include "xscreensaver_config.h"

#include "include/rapidjson/istreamwrapper.h"

#include <fstream>
#include <utility>
#include <array>

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "ThemesService"

#define _LD_AC_CASE_GENERAL_INT64(name) \
    case fnvHashConst(#name): \
        assert(value.IsBool() || value.IsInt() || value.IsUint() || value.IsInt64()); \
        if (value.IsInt64()) \
            mConfig->Get()->name = value.GetInt64(); \
        else if (value.IsUint()) \
            mConfig->Get()->name = value.GetUint(); \
        else if (value.IsInt()) \
            mConfig->Get()->name = value.GetInt(); \
        else if (value.IsBool()) { \
            WARN("Converting value from bool to int64"); \
            mConfig->Get()->name = value.GetBool() ? 1 : 0; \
        } \
        return true

#define _LD_AC_CASE_USCD_INT64(name) \
    case fnvHashConst(const_replace(#name, '_', '-')): \
        assert(value.IsBool() || value.IsInt() || value.IsUint() || value.IsInt64()); \
        if (value.IsInt64()) \
            mConfig->Get()->name = value.GetInt64(); \
        else if (value.IsUint()) \
            mConfig->Get()->name = value.GetUint(); \
        else if (value.IsInt()) \
            mConfig->Get()->name = value.GetInt(); \
        else if (value.IsBool()) { \
            WARN("Converting value from bool to int64"); \
            mConfig->Get()->name = value.GetBool() ? 1 : 0; \
        } \
        return true

#define _LD_AC_CASE_GENERAL_BOOL(name) \
    case fnvHashConst(#name): \
        assert(value.IsBool() || value.IsNumber()); \
        mConfig->Get()->name = (value.IsBool() && value.GetBool()) || \
            (value.IsNumber() && value.GetDouble() != 0); \
        return true

#define _LD_AC_CASE_GENERAL_STRING(name) \
    case fnvHashConst(#name): \
        assert(value.IsString()); \
        mConfig->Get()->name = fnvHash(value.GetString()); \
        return true

#define _LD_AC_CHECK_MUTEX(func) \
        if (mConfigMutex->TryLock()) { \
            ERROR("AppConfig mutex was not locked before calling " func " - FATAL"); \
            abort(); \
        }

namespace LeagueDisplays {
    /*static*/ CrossProcessMutex* AppConfig::mConfigMutex = new CrossProcessMutex("/LeagueDisplays_AppConfigMutex");
    /*static*/ AppConfig* AppConfig::mInstance = NULL;

    unsigned int fnvHash(const char* str) {
        const size_t length = strlen(str) + 1;
        unsigned int hash = OFFSET_BASIS;

        for (size_t i = 0; i < length; ++i) {
            hash ^= *str++;
            hash *= FNV_PRIME;
        }

        return hash;
    }

    template<typename Out>
    void split(const std::string &s, char delim, Out result) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, std::back_inserter(elems));
        return elems;
    }

    constexpr char cosnt_replace_char_helper(const char c, const char search, const char replacement) {
        return c == search ? replacement : c;
    }

    template <unsigned N, typename T, T... Nums>
    constexpr const std::array<char, N> const_replace_helper(const char (&str)[N], std::integer_sequence<T, Nums...>, const char search, const char replacement) {
        return {cosnt_replace_char_helper(str[Nums], search, replacement)...};
    }

    template <unsigned N>
    constexpr const std::array<char, N> const_replace(const char (&str)[N], const char search, const char replacement) {
        return const_replace_helper(str, std::make_integer_sequence<unsigned, N>(), search, replacement);
    }

    template <unsigned long N>
    static constexpr unsigned int fnvHashConst(const std::array<char, N> str, unsigned long I = N) {
        return I == 1 ? (OFFSET_BASIS ^ str[0]) * FNV_PRIME : (fnvHashConst(str, I - 1) ^ str[I - 1]) * FNV_PRIME;
    }

#pragma region AppConfig
    AppConfig::AppConfig() {
        mBackgroundPlaylistChanged = false;
        mScreensaverPlaylistChanged = false;
        mConfig = new CrossProcessObject<struct _ld_settings_t>("/LeagueDisplays_Config");
    }

    /*static*/ AppConfig* AppConfig::Acquire() {
        mConfigMutex->Lock();
        
        if (!mInstance) {
            mInstance = new AppConfig();
        }

        return mInstance;
    }

    /*static*/ void AppConfig::Release() {
        _LD_AC_CHECK_MUTEX("AppConfig::Release")
        mConfigMutex->Unlock();
    }

    void AppConfig::LoadSettings() {
        _LD_AC_CHECK_MUTEX("AppConfig::LoadSettings")

        mConfig->Get()->mReloadConfig = true;

        DEBUG("Loading settings from %s", Filesystem::mSettingsFilePath.c_str());

        std::ifstream ifs(Filesystem::mSettingsFilePath);
        assert(ifs.is_open());
        rapidjson::IStreamWrapper wrapper(ifs);

        rapidjson::Document d;
        d.ParseStream(wrapper);
        ifs.close();

        for (rapidjson::Value::ConstMemberIterator iter = d.MemberBegin(); iter != d.MemberEnd(); ++iter){
            const rapidjson::Value& v = iter->value;
            ModifyOption(std::string(iter->name.GetString()), v, ConfigType::GENERAL);
        }
    }

    void AppConfig::LoadPlaylist() {
        _LD_AC_CHECK_MUTEX("AppConfig::LoadPlaylist")

        mConfig->Get()->mReloadPlaylist = true;

        DEBUG("Loading playlist from %s", Filesystem::mPlaylistsFilePath.c_str());

        std::ifstream ifs(Filesystem::mPlaylistsFilePath);
        assert(ifs.is_open());
        rapidjson::IStreamWrapper wrapper(ifs);

        rapidjson::Document d;
        d.ParseStream(wrapper);
        ifs.close();

        if (d.HasMember("wallpaper")) {
            const rapidjson::Value& wallpaper_obj = d["wallpaper"];

            if (wallpaper_obj.HasMember("assets")) {
                const rapidjson::Value& wallpaper_assets_list = wallpaper_obj["assets"];

                if(wallpaper_assets_list.IsArray()) {
                    mBackgroundPlaylist.clear();
                    mBackgroundPlaylistChanged = true;

                    for (size_t i = 0; i < wallpaper_assets_list.Size(); i++) {
                        if (!wallpaper_assets_list[i].IsString())
                            continue;

                        mBackgroundPlaylist.push_back(wallpaper_assets_list[i].GetString());
                    }
                }
            }
        }

        if (d.HasMember("screensaver")) {
            const rapidjson::Value& screensaver_obj = d["screensaver"];

            if (screensaver_obj.HasMember("assets")) {
                const rapidjson::Value& screensaver_assets_list = screensaver_obj["assets"];

                if(screensaver_assets_list.IsArray()) {
                    mScreensaverPlaylist.clear();
                    mScreensaverPlaylistChanged = true;

                    for (size_t i = 0; i < screensaver_assets_list.Size(); i++) {
                        if (!screensaver_assets_list[i].IsString()) {
                            continue;
                        }

                        mScreensaverPlaylist.push_back(screensaver_assets_list[i].GetString());
                    }
                }
            }
        }

        DEBUG("There are %d wallpapers and %d screensavers after loading playlist",
            mBackgroundPlaylist.size(), mScreensaverPlaylist.size());
    }

    bool AppConfig::ModifyOption(const std::string& fnv_name, const rapidjson::Value& value, const ConfigType type) {
        DEBUG("ModifyOption(%s)", fnv_name.c_str());
        return AppConfig::ModifyOption(fnvHash(fnv_name.c_str()), value, type);
    }

    bool AppConfig::ModifyOption(const unsigned int fnv_name, const rapidjson::Value& value, const ConfigType type) {
        _LD_AC_CHECK_MUTEX("AppConfig::ModifyOption")

        if (type == ConfigType::GENERAL) {
            switch (fnv_name) {
                _LD_AC_CASE_GENERAL_INT64  ( firstTimeConfig                );
                _LD_AC_CASE_GENERAL_INT64  ( lifetimeSessions               );
                _LD_AC_CASE_GENERAL_INT64  ( lifetimeSaves                  );
                _LD_AC_CASE_GENERAL_INT64  ( wp_imageDuration               );
                _LD_AC_CASE_GENERAL_INT64  ( ss_videoDuration               );
                _LD_AC_CASE_GENERAL_BOOL   ( ss_mirrorDisplay               );
                _LD_AC_CASE_GENERAL_INT64  ( agreeLicenseTimestamp          );
                _LD_AC_CASE_GENERAL_INT64  ( openConfigTimestamp            );
                _LD_AC_CASE_GENERAL_INT64  ( newestContentTimestamp         );
                _LD_AC_CASE_GENERAL_STRING ( locale                         );
                _LD_AC_CASE_GENERAL_BOOL   ( crop_images                    );
                _LD_AC_CASE_GENERAL_INT64  ( savePlaylistTimestamp          );

                default:
                    ERROR("Not handled option in group 'GENERAL': hash=0x%08x", fnv_name);
                    return false;
            }
        } else if (type == ConfigType::WALLPAPER) {
            switch (fnv_name) {
                _LD_AC_CASE_USCD_INT64     ( wallpaper_rotate_frequency     );

                default:
                    ERROR("Not handled option in group 'WALLPAPER': hash=0x%08x", fnv_name);
                    return false;
            }
        } else if (type == ConfigType::SCREENSAVER) {
            switch (fnv_name) {
                _LD_AC_CASE_USCD_INT64     ( screensaver_timeout            );

                default:
                    ERROR("Not handled option in group 'SCREENSAVER': hash=0x%08x", fnv_name);
                    return false;
            }
        } else {
            ERROR("Invalid option group - FATAL");
            abort();
            return false;
        }
    }

    void AppConfig::ReloadIfRequested() {
        if (mConfig->Get()->mReloadConfig) {
            LoadSettings();
            mConfig->Get()->mReloadConfig = false;
        }

        if (mConfig->Get()->mReloadPlaylist) {
            LoadPlaylist();
            mConfig->Get()->mReloadPlaylist = false;
        }
    }

    std::vector<std::string>& AppConfig::BackgroundPlaylist() {
        return this->mBackgroundPlaylist;
    }

    std::vector<std::string>& AppConfig::ScreensaverPlaylist() {
        return this->mScreensaverPlaylist;
    }

    struct _ld_settings_t *AppConfig::Settings() {
        if (mConfig->Get()->mReloadConfig) {
            mConfig->Get()->mReloadConfig = false;
            LoadSettings();
        }

        return this->mConfig->Get();
    }

    bool AppConfig::BackgroundPlaylistChanged() {
        if (mConfig->Get()->mReloadPlaylist) {
            mConfig->Get()->mReloadPlaylist = false;
            LoadPlaylist();
        }

        return this->mBackgroundPlaylistChanged ? !(this->mBackgroundPlaylistChanged = false) : false;
    }

    bool AppConfig::ScreensaverPlaylistChanged() {
        if (mConfig->Get()->mReloadPlaylist) {
            mConfig->Get()->mReloadPlaylist = false;
            LoadPlaylist();
        }

        return this->mScreensaverPlaylistChanged ? !(this->mScreensaverPlaylistChanged = false) : false;
    }
#pragma endregion

    static inline void UpdateScreensaverStuff() {
        std::string base_path = Filesystem::mFilesystemCacheDir + "LeagueScreensaver/content-cache/asset/";
        std::string dst_path = Filesystem::mFilesystemCacheDir + "screensaver/";
        
        Filesystem::Mkdirs(dst_path + "playlist/");
        system(("rm -f " + dst_path + "playlist/* &> /dev/null").c_str());

        FILE *f = fopen((dst_path + "playlist.js").c_str(), "w");
        assert(f);
        fputs("// Automatically generated file, DO NOT EDIT!\n", f);

        AppConfig *cfg = AppConfig::Acquire();
        std::vector<std::string> playlist = cfg->ScreensaverPlaylist();

        if (playlist.size() > 0) {
            fputs("PLAYLIST.custom = {\n", f);
            fputs("  assets: [\n", f);

            for (std::string s : playlist) {
                std::string ext, s2 = "c-o-" + s;
                std::string path = base_path + s2;

                if (Filesystem::Exists(path + ".jpg"))
                    ext = ".jpg";
                else if (Filesystem::Exists(path + ".png"))
                    ext = ".png";
                else if (Filesystem::Exists(path + ".webm"))
                    ext = ".webm";
                else if (Filesystem::Exists(path + ".mp4"))
                    ext = ".mp4";
                else {
                    ERROR("Could not find file for screensaver asset: %s", path.c_str());
                    continue;
                }

                fprintf(f, "    'playlist/%s',\n", (s2 + ext).c_str());

                symlink((base_path + s2 + ext).c_str(), (dst_path + "playlist/" + s2 + ext).c_str());
            }

            fputs("  ]\n", f);
            fputs("};\n", f);
        }

        AppConfig::Release();

        fclose(f);
    }

    namespace CEF {
        bool ThemesResourceHandler::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
            std::string url = request->GetURL().ToString();
            std::string method = request->GetMethod().ToString();
            DEBUG("Processing request (%s %s)", method.c_str(), url.c_str());

            std::vector<std::string> splitted = split(url, '/');
            for (int i = 0; i < 3; i++) {
                splitted.erase(splitted.begin());
            }

            unsigned int service = fnvHash(splitted.size() > 0 ? splitted[0].c_str() : "");
            unsigned int call    = fnvHash(splitted.size() > 1 ? (method + ":" + splitted[1]).c_str() : "");

            DEBUG("API call: %s/%s [%08lx%08lx]",
                splitted.size() > 0 ? splitted[0].c_str() : "",
                splitted.size() > 1 ? (method + ":" + splitted[1]).c_str() : "",
                service, call);

            std::string post_data_s;

            if (method == "PUT" || method == "POST") {
                CefRefPtr<CefPostData> post_data = request->GetPostData();
                if (post_data) {
                    CefPostData::ElementVector chunks;
                    post_data->GetElements(chunks);

                    for (CefRefPtr<CefPostDataElement> chunk : chunks) {
                        size_t chunksize = chunk->GetBytesCount();
                        void* buffer = malloc(chunksize);

                        assert(buffer);
                        assert(chunk->GetBytes(chunksize, buffer) == chunksize);
                        
                        post_data_s += std::string((char*)buffer, chunksize);
                        free(buffer);
                    }

                    chunks.clear();
                }
            }

            // DEBUG("Post data: %s", post_data_s.c_str());

            rapidjson::Document d;

            switch (service) {
                case fnvHashConst("screensaver"): {
                    switch (call) {
                        case fnvHashConst("GET:settings"): {
                            int timeout = 0;

                            XScreensaverConfig* cfg = new XScreensaverConfig(Filesystem::mUserHome + ".xscreensaver");
                            std::string value = cfg->GetKeyValue("timeout");
                            delete cfg;

                            std::string::size_type pos = value.find(':');
                            timeout += atoi(value.substr(0, pos).c_str()) * 60 * 60;

                            value = value.substr(pos + 1);
                            pos = value.find(':');
                            timeout += atoi(value.substr(0, pos).c_str()) * 60;

                            value = value.substr(pos + 1);
                            timeout += atoi(value.c_str());

                            mStream << "{\"screensaver_timeout\":" << timeout << "}\n";
                            mErrorCode = 200;
                            callback->Continue();
                            return true;
                        }
                        case fnvHashConst("PUT:settings"):
                            if (splitted.size() <= 2) {
                                d.Parse(post_data_s.c_str());
                                
                                AppConfig* cfg = AppConfig::Acquire();

                                for (rapidjson::Value::ConstMemberIterator iter = d.MemberBegin(); iter != d.MemberEnd(); ++iter){
                                    const rapidjson::Value& v = iter->value;
                                    cfg->ModifyOption(std::string(iter->name.GetString()), v, ConfigType::SCREENSAVER);
                                }

                                AppConfig::Release();
                            } else {
                                if (splitted[2] == "screensaver-timeout") {
                                    int timeout = atoi(post_data_s.c_str());
                                    std::stringstream ss;
                                    ss << timeout / 60 / 60;
                                    ss << ":";
                                    ss << timeout / 60 - ((timeout / 60 / 60) * 60);
                                    ss << ":";
                                    ss << timeout - ((timeout / 60 - ((timeout / 60 / 60) * 60)) * 60) - ((timeout / 60 / 60) * 60 * 60);

                                    XScreensaverConfig* cfg = new XScreensaverConfig(Filesystem::mUserHome + ".xscreensaver");
                                    cfg->SetKeyValue("timeout", ss.str());
                                    cfg->Save();
                                    delete cfg;

                                    break;
                                }
                                WARN("partial screensaver/PutSettings is not implemented yet!");
                            }
                            break;
                        case fnvHashConst("POST:playlist"): {
                                AppConfig* cfg = AppConfig::Acquire();
                                cfg->LoadPlaylist();
                                AppConfig::Release();
                                mErrorCode = 1200;
                                callback->Continue();
                                return true;
                            }
                        case fnvHashConst("PUT:activation"):
                            UpdateScreensaverStuff();
                            mErrorCode = 1200;
                            callback->Continue();
                            return true;
                        default:
                            WARN("Unhandled call %08lx", call);
                            break;
                    }

                    break;
                }
                case fnvHashConst("wallpaper"): {
                    switch (call) {
                        case fnvHashConst("GET:settings"):
                            WARN("wallpaper/GetSettings is not implemented yet!");
                            break;
                        case fnvHashConst("PUT:settings"):
                            if (splitted.size() <= 2) {
                                d.Parse(post_data_s.c_str());
                                
                                AppConfig* cfg = AppConfig::Acquire();

                                for (rapidjson::Value::ConstMemberIterator iter = d.MemberBegin(); iter != d.MemberEnd(); ++iter){
                                    const rapidjson::Value& v = iter->value;
                                    std::string key = iter->name.GetString();

                                    if (key == "rotate-frequency") {
                                        cfg->ModifyOptionS("wp_imageDuration", v, ConfigType::GENERAL);
                                    }
                                    else {
                                        cfg->ModifyOption(std::string(iter->name.GetString()), v, ConfigType::WALLPAPER);
                                    }
                                }

                                AppConfig::Release();

                                mErrorCode = 1200;
                                callback->Continue();
                                return true;
                            } else {
                                WARN("partial wallpaper/PutSettings is not implemented yet!");
                            }

                            break;
                        case fnvHashConst("POST:playlist"): {
                            INFO("Playlist reload requested");
                            AppConfig* cfg = AppConfig::Acquire();
                            cfg->LoadPlaylist();
                            AppConfig::Release();

                            mErrorCode = 1200;
                            callback->Continue();
                            return true;
                        }
                        default:
                            WARN("Unhandled call %08lx", call);
                            break;
                    }

                    break;
                }
                case fnvHashConst("assistant"): {
                    switch (call) {
                        case fnvHashConst("GET:settings"):
                            mStream << "{\"enable-assistant-startup\":" << BOOL2STR(BackgroundDaemon::IsAutostartEnabled()) << "}\n";
                            mErrorCode = 200;
                            callback->Continue();
                            return true;
                        case fnvHashConst("PUT:settings"):
                            d.Parse(post_data_s.c_str());

                            if (d.HasMember("enable-assistant-startup")) {
                                BackgroundDaemon::SetAutostartEnabled(d["enable-assistant-startup"].GetBool());
                            }
                            break;
                        default:
                            WARN("Unhandled call %08lx", call);
                            break;
                    }

                    break;
                }
                case fnvHashConst("powersettings"): {
                    WARN("Power-management settings not implemented yet!");
                    break;
                }
                default:
                    WARN("Unhandled service %08lx", service);
                    break;
            }

            return false;
        }

        void ThemesResourceHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) {

            if (mErrorCode > 1000) {
                response->SetStatus(mErrorCode - 1000);
                response_length = 0;
            } else {
                response->SetStatus(mErrorCode);
                mStream.seekp(0, std::ios::end);
                response_length = mStream.tellp();
                response_length--;
                mStream.seekp(0);
            }

            DEBUG("Code: %d, Response length: %d", mErrorCode, response_length);
        }

        bool ThemesResourceHandler::ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) {
            char ch;
            
            for (bytes_read = 0; bytes_read < bytes_to_read; bytes_read++) {
                mStream >> ch;
                ((char*)data_out)[bytes_read] = ch;
            }

            DEBUG("BytesToRead: %d, BytesRead: %d", bytes_to_read, bytes_read);

            return true;
        }

        void ThemesResourceHandler::Cancel() {
            mStream.clear();
        }
    }
}
