
#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdio.h>
#include <string>
#include <map>
#include <pthread.h>

#ifndef LOGGING_SOURCE
#define LOGGING_SOURCE "LeagueDisplays"
#endif // LOGGING_SOURCE

#define BOOL2STR(b) ((b) ? "true" : "false")

namespace LeagueDisplays {
    enum _LogLevel : unsigned char {
        LL_DEBUG0,
        LL_DEBUG1,
        LL_DEBUG2,
        LL_INFO,
        LL_FIXME,
        LL_WARN,
        LL_ERROR
    };

    class Logger {
        public:
            static FILE *GetLogFilePointer();
            static FILE *GetConsoleFilePointer();
            static void Flush();
            
            static Logger* Get(const char *name);
            void LogRaw(const char *function, _LogLevel level, const char *format, ...);
            void LogRawNofix(_LogLevel level, const char *format, ...);
            
            static std::string ApplyFormattingEscapes(std::string input);
            static std::string RemoveFormattingEscapes(std::string input);
            
            static void SetVerboseLevel(const unsigned char level);
            
            static void Lock();
            static void Unlock();
            static void DisableConsoleLogging();
            
            std::string                            mName;
            static bool                            mEnableFileLogging;
        private:
            Logger(const char* name);

            static FILE                           *mConsoleOutput;
            static FILE                           *mLogfile;
            static std::map<std::string, Logger*>  mInstances;
            static unsigned char                   mVerboseLevel;
            static pthread_mutex_t                 mLoggerMutex;
    };
}

#define DEBUG(...)      LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(__FUNCTION__, LeagueDisplays::LL_DEBUG2, __VA_ARGS__)
#define FIXME(...)      LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(__FUNCTION__, LeagueDisplays::LL_FIXME,  __VA_ARGS__)
#define INFO(...)       LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(__FUNCTION__, LeagueDisplays::LL_INFO,   __VA_ARGS__)
#define WARN(...)       LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(__FUNCTION__, LeagueDisplays::LL_WARN,   __VA_ARGS__)
#define ERROR(...)      LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(__FUNCTION__, LeagueDisplays::LL_ERROR,  __VA_ARGS__)

#define DEBUG_S(S,...)  LeagueDisplays::Logger::Get(S)->LogRaw(__FUNCTION__, LeagueDisplays::LL_DEBUG2, __VA_ARGS__)
#define FIXME_S(S,...)  LeagueDisplays::Logger::Get(S)->LogRaw(__FUNCTION__, LeagueDisplays::LL_FIXME,  __VA_ARGS__)
#define INFO_S(S,...)   LeagueDisplays::Logger::Get(S)->LogRaw(__FUNCTION__, LeagueDisplays::LL_INFO,   __VA_ARGS__)
#define WARN_S(S,...)   LeagueDisplays::Logger::Get(S)->LogRaw(__FUNCTION__, LeagueDisplays::LL_WARN,   __VA_ARGS__)
#define ERROR_S(S,...)  LeagueDisplays::Logger::Get(S)->LogRaw(__FUNCTION__, LeagueDisplays::LL_ERROR,  __VA_ARGS__)


#define DEBUG_F(...)     LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_DEBUG2, __VA_ARGS__)
#define FIXME_F(...)     LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_FIXME,  __VA_ARGS__)
#define INFO_F(...)      LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_INFO,   __VA_ARGS__)
#define WARN_F(...)      LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_WARN,   __VA_ARGS__)
#define ERROR_F(...)     LeagueDisplays::Logger::Get(LOGGING_SOURCE)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_ERROR,  __VA_ARGS__)

#define DEBUG_FS(S,...)  LeagueDisplays::Logger::Get(S)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_DEBUG2, __VA_ARGS__)
#define FIXME_FS(S,...)  LeagueDisplays::Logger::Get(S)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_FIXME,  __VA_ARGS__)
#define INFO_FS(S,...)   LeagueDisplays::Logger::Get(S)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_INFO,   __VA_ARGS__)
#define WARN_FS(S,...)   LeagueDisplays::Logger::Get(S)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_WARN,   __VA_ARGS__)
#define ERROR_FS(S,...)  LeagueDisplays::Logger::Get(S)->LogRaw(LOGGING_FUNCTION_NAME_OVERRIDE, LeagueDisplays::LL_ERROR,  __VA_ARGS__)

#endif // LOG_H
