
#include "filesystem.h"
#include "scheme_handlers.h"
#include "log.h"
#include "themes_service.h"
#include "junzip.h"

#include "include/cef_request.h"
#include "include/wrapper/cef_helpers.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "Filesystem"

namespace LeagueDisplays {
    /*static*/ std::string Filesystem::mWorkingDirectory;
    /*static*/ std::string Filesystem::mExecutablePath;
    /*static*/ std::string Filesystem::mUserHome;
    /*static*/ std::string Filesystem::mRootDir;
    /*static*/ std::string Filesystem::mCefCacheDir;
    /*static*/ std::string Filesystem::mDefaultDownloadDir;
    /*static*/ std::string Filesystem::mFilesystemCacheDir;
    /*static*/ std::string Filesystem::mDebugLogPath;
    /*static*/ std::string Filesystem::mSettingsFilePath;
    /*static*/ std::string Filesystem::mPlaylistsFilePath;

    /*static*/ void Filesystem::Initialize() {
        INFO("Initializing filesystem API");

        char outp[512];
        getcwd(outp, 512);
        mWorkingDirectory = outp;

        if (outp[strlen(outp) - 1] != '/') {
            mWorkingDirectory += '/';
        }

        INFO("Working directory is %s", mWorkingDirectory.c_str());

        const char* userhome = getenv("HOME");
        if (!Exists(userhome)) {
            ERROR("HOME is not defined using working directory instead!");
            userhome = "./";
        }

        if (strlen(userhome) <= 0) {
            ERROR("Could not get root dir path - FATAL");
            abort();
        }

        mRootDir = userhome;
        if (userhome[strlen(userhome) - 1] != '/') {
            mRootDir += '/';
        }

        mUserHome = mRootDir;

        mDefaultDownloadDir = mRootDir + "Downloads/LeagueDisplays/";
        Mkdirs(mDefaultDownloadDir);
        INFO("Default download directory is %s", mDefaultDownloadDir.c_str());

        mRootDir += ".leaguedisplays/";
        Mkdirs(mRootDir);
        INFO("Root directory is %s", mRootDir.c_str());

        mDebugLogPath = mRootDir + "debug.log";
        mCefCacheDir = mRootDir + "cefCache/";
        Mkdirs(mCefCacheDir);
        INFO("CEF cache directory is %s", mCefCacheDir.c_str());

        mFilesystemCacheDir = mRootDir + "filesystem/";
        INFO("Filesystem cache directory is %s", mFilesystemCacheDir.c_str());

        mSettingsFilePath = mFilesystemCacheDir + "LeagueScreensaver/settings.json";
        mPlaylistsFilePath = mFilesystemCacheDir + "LeagueScreensaver/playlists.json";

        if (!Exists(mSettingsFilePath)) {
            WriteAllText(mSettingsFilePath, "{}");
        } else {
            INFO("Config file [settings] already exists at %s", mSettingsFilePath.c_str());
        }

        if (!Exists(mPlaylistsFilePath)) {
            WriteAllText(mPlaylistsFilePath, "{}");
        } else {
            INFO("Config file [playlists] already exists at %s", mPlaylistsFilePath.c_str());
        }
    }

    /*static*/ void Filesystem::ScreensaverInit() {
        mDebugLogPath = mRootDir + "screensaver-debug.log";
        mCefCacheDir = mRootDir + "SSCefCache/";
        Mkdirs(mCefCacheDir);

        INFO("Changed debug log path to %s", mDebugLogPath.c_str());
        INFO("Changed cache dir to %s", mCefCacheDir.c_str());
    }


    /*static*/ bool Filesystem::Exists(const char* path) {
        struct stat st = {0};
        return stat(path, &st) != -1;
    }

    /*static*/ bool Filesystem::Exists(const std::string& path) {
        return Exists(path.c_str());
    }


    /*static*/ void Filesystem::Mkdirs(const char* path) {
        char tmp[256];
        char* p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp), "%s", path);

        len = strlen(tmp);

        if (tmp[len - 1] == '/') {
            tmp[len - 1] = 0;
        }

        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;

                if (!Exists(tmp)) {
                    DEBUG("Creating directory: %s", tmp);
                    mkdir(tmp, 0700);
                }

                *p = '/';
            }
        }

        if (!Exists(tmp)) {
            DEBUG("Creating directory: %s", tmp);
            mkdir(tmp, 0700);
        }
    }

    /*static*/ void Filesystem::Mkdirs(const std::string& path) {
        Mkdirs(path.c_str());
    }

    /*static*/ void Filesystem::Delete(const char* path) {
        if (Exists(path)) {
            unlink(path);
        }
    }

    /*static*/ void Filesystem::Delete(const std::string& path) {
        Delete(path.c_str());
    }

    /*static*/ void Filesystem::FileMkdirs(const std::string& path) {
        std::string dirpath = path;

        while (dirpath.size() > 1 && dirpath.at(dirpath.size() - 1) != '/') {
            dirpath = dirpath.substr(0, dirpath.size() - 1);
        }

        Mkdirs(dirpath);
    }


    /*static*/ void Filesystem::WriteAllText(const char* path, const char* text) {
        FileMkdirs(path);

        FILE* f = fopen(path, "w");
        assert(f);

        fwrite(text, sizeof(char), strlen(text), f);
        fclose(f);
    }

    /*static*/ void Filesystem::WriteAllText(const std::string& path, const std::string& text) {
        WriteAllText(path.c_str(), text.c_str());
    }

    int _JZProcessFile(JZFile* zip, const std::string& targetDir) {
        JZFileHeader header;
        char filename[1024];
        unsigned char* data;

        if (jzReadLocalFileHeader(zip, &header, filename, sizeof(filename))) {
            ERROR("Couldn't read local file header!");
            return -1;
        }

        std::string path = targetDir + filename;

        if ((data = (unsigned char *)malloc(header.uncompressedSize)) == NULL) {
            ERROR("Couldn't allocate memory!");
            return -1;
        }

        if (jzReadData(zip, &header, data)) {
            ERROR("Couldn't read file data!");
            free(data);
            return -1;
        }
        
        if (path[path.length() - 1] != '/' && !Filesystem::Exists(path)) {
            DEBUG("Unpacking %s [size: %d bytes]", filename, header.uncompressedSize);

            Filesystem::FileMkdirs(path);

            FILE* f = fopen(path.c_str(), "wb");
            if (!f) {
                ERROR("Could not write file: %s", path.c_str());
            }

            assert(f);

            fwrite(data, 1, header.uncompressedSize, f);
            fclose(f);
        }

        free(data);

        return 0;
    }

    int _JZRecordCallback(JZFile* zip, int idx, JZFileHeader* header, char* filename, void* user_data) {
        const std::string* targetDir = (const std::string*)user_data;
        long offset;

        offset = zip->tell(zip); // store current position

        if (zip->seek(zip, header->offset, SEEK_SET)) {
            ERROR("Cannot seek in zip file!");
            return 0; // abort
        }

        _JZProcessFile(zip, *targetDir); // alters file offset

        zip->seek(zip, offset, SEEK_SET); // return to position

        return 1; // continue
    }

    /*static*/ bool Filesystem::Unzip(const std::string& file, const std::string& targetDir) {
        INFO("Unzip: %s -> %s", file.c_str(), targetDir.c_str());

        Mkdirs(targetDir);
        bool ret = false;

        if (!Exists(file)) {
            return false;
        }

        FILE *f = fopen(file.c_str(), "rb");
        assert(f);

        JZFile *zip = jzfile_from_stdio_file(f);
        assert(zip);

        JZEndRecord endRecord;

        if (jzReadEndRecord(zip, &endRecord)) {
            ERROR("Couldn't read ZIP file end record.");
            goto failed;
        }

        if (jzReadCentralDirectory(zip, &endRecord, _JZRecordCallback, (void*)&targetDir)) {
            ERROR("Couldn't read ZIP file central record.");
            goto failed;
        }

        ret = true;

        failed:
        zip->close(zip);
        fclose(f);
        return ret;
    }

    /*static*/ void Filesystem::PIDFileSet(const char* name, unsigned short pid) {
        std::string path = mRootDir + name;
        WriteAllText(path, std::to_string(pid).c_str());
    }

    /*static*/ unsigned short Filesystem::PIDFileGet(const char* name) {
        std::string path = mRootDir + name;

        FILE* f = fopen(path.c_str(), "r");

        if (!f) {
            return 0;
        }

        int p;
        fscanf(f, "%d", &p);
        fclose(f);

        return p & 0xFFFF;
    }

    namespace CEF {
        bool FilesystemResourceHandler::ProcessRequest(CefRefPtr<CefRequest> request, CefRefPtr<CefCallback> callback) {
            CEF_REQUIRE_IO_THREAD();

            std::string url = request->GetURL().ToString();
            std::string method = request->GetMethod().ToString();
            DEBUG("Processing request (%s %s)", method.c_str(), url.c_str());

            std::string path = Filesystem::mFilesystemCacheDir + url.substr(strlen("http://filesystem/"));

            Filesystem::FileMkdirs(path);

            mFileStream = NULL;

            if (method == "POST") {
                mErrorCode = 200;
                mResponseLength = 0;

                CefRefPtr<CefPostData> post_data = request->GetPostData();
                if (post_data) {
                    CefPostData::ElementVector file_chunks;
                    post_data->GetElements(file_chunks);

                    FILE* f = fopen(path.c_str(), "wb");

                    for (CefRefPtr<CefPostDataElement> chunk : file_chunks) {
                        size_t chunksize = chunk->GetBytesCount();
                        void* buffer = malloc(chunksize);
                        assert(buffer);

                        assert(chunk->GetBytes(chunksize, buffer) == chunksize);
                        assert(fwrite(buffer, 1, chunksize, f) == chunksize);

                        free(buffer);
                    }

                    size_t total_size = ftell(f);

                    fflush(f);
                    fclose(f);

                    INFO("Saved file %s [%ld bytes]", path.c_str(), total_size);
                } else {
                    FILE* f = fopen(path.c_str(), "wb");
                    fclose(f);

                    INFO("Saved empty file %s", path.c_str());
                }

                if (path == Filesystem::mSettingsFilePath) {
                    AppConfig* cfg = AppConfig::Acquire();
                    cfg->LoadSettings();
                    AppConfig::Release();
                }

                callback->Continue();
            } else if (method == "GET") {
                if (!Filesystem::Exists(path)) {
                    mErrorCode = 404;
                    mResponseLength = 0;

                    WARN("File not found: %s", path.c_str());
                } else {
                    mErrorCode = 200;

                    mFileStream = fopen(path.c_str(), "rb");
                    fseek(mFileStream, 0, SEEK_END);
                    mResponseLength = ftell(mFileStream);
                    fseek(mFileStream, 0, SEEK_SET);
                }

                callback->Continue();
            } else if (method == "OPTIONS") {
                mResponseLength = 0;
                mErrorCode = Filesystem::Exists(path) ? 200 : 404;
                callback->Continue();
            } else {
                ERROR("Unhandled method: %s - FATAL", method.c_str());

                CefRequest::HeaderMap headers;
                request->GetHeaderMap(headers);

                for (std::pair<CefString, CefString> q : headers) {
                    INFO("%s: %s", q.first.ToString().c_str(), q.second.ToString().c_str());
                }

                abort();
            }

            return true;
        }

        void FilesystemResourceHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl) {
            response->SetStatus(mErrorCode);
            response_length = mResponseLength;
        }

        bool FilesystemResourceHandler::ReadResponse(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefCallback> callback) {
            CEF_REQUIRE_IO_THREAD();
            
            if (mFileStream) {
                bytes_read = fread(data_out, 1, bytes_to_read, mFileStream);
                
                if (ftell(mFileStream) >= mResponseLength) {
                    fclose(mFileStream);
                    mFileStream = NULL;
                }

                callback->Continue();
                return true;
            }

            bytes_read = 0;
            return false;
        }

        void FilesystemResourceHandler::Cancel() {
            if (mFileStream) {
                fclose(mFileStream);
                mFileStream = NULL;
            }
        }
    }
}
