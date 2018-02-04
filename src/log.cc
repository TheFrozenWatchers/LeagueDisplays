
#include "log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex>
#include <thread>
#include <stdarg.h>

#define _fprintf(fp,format,...) if (fp) fprintf(fp, format, __VA_ARGS__)
#define _fflush(fp) if (fp) fflush(fp)

namespace LeagueDisplays {
    std::map<std::string, Logger*> Logger::mInstances;
    unsigned char Logger::mVerboseLevel = 0;
    bool Logger::mEnableFileLogging = false;
    pthread_mutex_t Logger::mLoggerMutex = PTHREAD_MUTEX_INITIALIZER;
    FILE *Logger::mLogfile = NULL;
    FILE *Logger::mConsoleOutput = stdout;

    Logger::Logger(const char* name) {
        struct stat st = {0};

        if (stat("logs/", &st) == -1)
            mkdir("logs/", 0700);
        
        if (!mLogfile && mEnableFileLogging) {
            time_t rawtime;
            struct tm *timeinfo;

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            char logfname[256];
            snprintf(logfname, 256, "logs/LeagueDisplays_P%04x_%04d-%02d-%02dT%02d-%02d-%02d.log", getpid(), timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            mLogfile = fopen(logfname, "w");
        }

        this->mName = name;
    }

    /*static*/ FILE* Logger::GetLogFilePointer() {
        return mLogfile;
    }

    /*static*/ FILE* Logger::GetConsoleFilePointer() {
        return mConsoleOutput;
    }

    /*static*/ void Logger::Flush() {
        Lock();
        _fflush(mLogfile);
        _fflush(mConsoleOutput);
        Unlock();
    }

    inline void _log_raw_sync(Logger *self, const char *function, _LogLevel level, char *buff) {
        const char *type = "???";
        switch (level) {
            case LL_DEBUG0:
            case LL_DEBUG1:
            case LL_DEBUG2:
                type = "debug";
                break;
            case LL_INFO:
                type = "info";
                break;
            case LL_FIXME:
                type = "fixme";
                break;
            case LL_WARN:
                type = "warn";
                break;
            case LL_ERROR:
                type = "error";
                break;
        }

        std::string _format = buff;

        switch (level) {
            case LL_DEBUG0:
            case LL_DEBUG1:
            case LL_DEBUG2:
                _format = "$$8" + _format;
                break;
            case LL_INFO:
                _format = "$$a" + _format;
                break;
            case LL_FIXME:
                _format = "$$d" + _format;
                break;
            case LL_WARN:
                _format = "$$e" + _format;
                break;
            case LL_ERROR:
                _format = "$$c" + _format;
                break;
        }

        static const char *format = "[%04x] (%-20s) %6s:%-20s: %s\n";

        Logger::Lock();
        _fprintf(Logger::GetConsoleFilePointer(), format, getpid(), self->mName.c_str(),
            type, function, Logger::ApplyFormattingEscapes(_format).c_str() );
        
        _fprintf(Logger::GetLogFilePointer(), format, getpid(), self->mName.c_str(),
            type, function, Logger::RemoveFormattingEscapes(_format).c_str());

        Logger::Unlock();

        free(buff);

        Logger::Flush();
    }

    void Logger::LogRaw(const char *function, _LogLevel level, const char *format, ...) {
        switch (level) {
            case LL_DEBUG0:
                if (mVerboseLevel < 3)
                    return;
                break;
            case LL_DEBUG1:
                if (mVerboseLevel < 2)
                    return;
                break;
            case LL_DEBUG2:
                if (mVerboseLevel < 1)
                    return;
                break;
            default:
                break;
        }

        char *buff = (char*)malloc(4096);
        
        va_list args;
        va_start(args, format);
        vsprintf(buff, format, args);
        va_end(args);

        std::thread(_log_raw_sync, this, function, level, buff).detach();
    }

    inline void _log_raw_nofix_sync(_LogLevel level, char *buff) {
        Logger::Lock();
        fprintf(stderr, "%s", Logger::ApplyFormattingEscapes(buff).c_str() );
        fprintf(Logger::GetLogFilePointer(), "%s", Logger::RemoveFormattingEscapes(buff).c_str());
        Logger::Unlock();

        free(buff);

        Logger::Flush();
    }

    void Logger::LogRawNofix(_LogLevel level, const char *format, ...) {
        switch (level) {
            case LL_DEBUG0:
                if (mVerboseLevel < 3)
                    return;
                break;
            case LL_DEBUG1:
                if (mVerboseLevel < 2)
                    return;
                break;
            case LL_DEBUG2:
                if (mVerboseLevel < 1)
                    return;
                break;
            default:
                break;
        }
        
        char* buff = (char*)malloc(4096);
        
        va_list args;
        va_start(args, format);
        vsprintf(buff, format, args);
        va_end(args);

        std::thread(_log_raw_nofix_sync, level, buff).detach();
    }

    /*static*/ std::string Logger::ApplyFormattingEscapes(std::string input) {
        input = "$$r" + input + "$$r";
        std::string res = "";

        while (input.length() != 0) {
            if (input.length() >= 3 && input.compare(0, 2, "$$") == 0) {
                input = input.substr(2);

                switch (input[0]) {
                    // COLORS:
                    case '0':
                        res += "\e[30m";
                        break;
                    case '1':
                        res += "\e[34m";
                        break;
                    case '2':
                        res += "\e[32m";
                        break;
                    case '3':
                        res += "\e[36m";
                        break;
                    case '4':
                        res += "\e[31m";
                        break;
                    case '5':
                        res += "\e[35m";
                        break;
                    case '6':
                        res += "\e[33m";
                        break;
                    case '7':
                        res += "\e[37m";
                        break;
                    case '8':
                        res += "\e[90m";
                        break;
                    case '9':
                        res += "\e[94m";
                        break;
                    case 'a':
                        res += "\e[92m";
                        break;
                    case 'b':
                        res += "\e[96m";
                        break;
                    case 'c':
                        res += "\e[91m";
                        break;
                    case 'd':
                        res += "\e[95m";
                        break;
                    case 'e':
                        res += "\e[93m";
                        break;
                    case 'f':
                        res += "\e[97m";
                        break;

                    // FORMATS:
                    case 'k':
                        res += "\e[2m";
                        break;
                    case 'l':
                        res += "\e[1m";
                        break;
                    case 'm':
                        res += "\e[5m";
                        break;
                    case 'n':
                        res += "\e[4m";
                        break;
                    case 'o':
                        res += "\e[7m";
                        break;
                    case 'r':
                        res += "\e[39m\e[0m";
                        break;
                    default:
                        res += input[0];
                        break;
                }
            } else {
                res += input[0];
            }

            input = input.substr(1);
        }
        
        return res;
    }

    /*static*/ std::string Logger::RemoveFormattingEscapes(std::string input) {
        input = "$$r" + input + "$$r";
        std::string res = "";
        
        while (input.length() != 0) {
            if (input.length() >= 3 && input.compare(0, 2, "$$") == 0) {
                input = input.substr(2);

                switch (input[0]) {
                    // COLORS:
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case 'a':
                    case 'b':
                    case 'c':
                    case 'd':
                    case 'e':
                    case 'f':

                    // FORMATS:
                    case 'k':
                    case 'l':
                    case 'm':
                    case 'n':
                    case 'o':
                    case 'r':
                        break;
                    default:
                        res += input[0];
                        break;
                }
            } else {
                res += input[0];
            }

            input = input.substr(1);
        }
        
        return res;
    }

    /*static*/ void Logger::SetVerboseLevel(const unsigned char level) {
        mVerboseLevel = level;
    }

    /*static*/ Logger* Logger::Get(const char *name) {
        if (mInstances.find(name) != mInstances.end())
            return mInstances[name];
        
        Logger* res = new Logger(name);
        
#ifndef LD_NO_LOGGER_INSTANCE_CACHING
            mInstances.insert(std::pair<std::string, Logger*>(name, res));
#endif // LD_NO_LOGGER_INSTANCE_CACHING

        return res;
    }

    /*static*/ void Logger::Lock() {
        pthread_mutex_lock(&mLoggerMutex);
    }

    /*static*/ void Logger::Unlock() {
        pthread_mutex_unlock(&mLoggerMutex);
    }

    /*static*/ void Logger::DisableConsoleLogging() {
        Lock();
        mConsoleOutput = NULL;
        Unlock();
    }
}
