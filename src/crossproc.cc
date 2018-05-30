
#include "crossproc.h"

#ifdef LOGGING_SOURCE
#undef LOGGING_SOURCE
#endif // LOGGING_SOURCE

#define LOGGING_SOURCE "CrossProcessMutex"

namespace LeagueDisplays {
    CrossProcessMutex::CrossProcessMutex(const char* name) {
        errno = 0;
        mName = name;
        mMutexPtr = new CrossProcessObject<pthread_mutex_t>(name);

        if (mMutexPtr->IsValid()) {
            pthread_mutexattr_t attr;
            if (pthread_mutexattr_init(&attr)) {
                ERROR("pthread_mutexattr_init failed [errno=%d]", errno);
                return;
            }

            if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
                ERROR("pthread_mutexattr_setpshared failed [errno=%d]", errno);
                return;
            }

            if (pthread_mutex_init(mMutexPtr->Get(), &attr)) {
                ERROR("pthread_mutex_init failed [errno=%d]", errno);
                return;
            }
        }

        INFO("Created cross-process mutex: %s", mName);
    }

    CrossProcessMutex::~CrossProcessMutex() {
        Destroy();
    }

    void CrossProcessMutex::Lock() {
        pthread_mutex_lock(mMutexPtr->Get());

        // INFO("Locked mutex %s", mName);
    }

    bool CrossProcessMutex::TryLock() {
        return !pthread_mutex_trylock(mMutexPtr->Get());
    }

    void CrossProcessMutex::Unlock() {
        pthread_mutex_unlock(mMutexPtr->Get());

        // INFO("Unlocked mutex %s", mName);
    }

    void CrossProcessMutex::Destroy() {
        if (!mMutexPtr->IsValid()) {
            return;
        }

        if ((errno = pthread_mutex_destroy(mMutexPtr->Get()))) {
            perror("pthread_mutex_destroy");
            return;
        }

        mMutexPtr->Destroy();
    }
}
