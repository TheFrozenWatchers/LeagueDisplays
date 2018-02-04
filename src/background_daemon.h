
#ifndef LD_BACKGROUND_DAEMON_H
#define LD_BACKGROUND_DAEMON_H

#pragma once

#include <pthread.h>
#include <vector>
#include <string>

namespace LeagueDisplays {
    class BackgroundDaemon {
        public:
            static void Start();
            static void Stop();

            static void SetAutostartEnabled(bool flag);
            static bool IsAutostartEnabled();
        private:
            static void *WorkerThreadProc(void* arg);

            static pthread_t mBackgroundChangerThread;
            static int64_t mNextBackgroundChangeTime;
    };
}

#endif // LD_BACKGROUND_DAEMON_H
