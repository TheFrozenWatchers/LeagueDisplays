
#ifndef LD_CROSSPROC_H
#define LD_CROSSPROC_H

#pragma once

#include "log.h"

#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <linux/limits.h>

#include <sys/mman.h>

namespace LeagueDisplays {
    template<typename T, size_t extra_alloc = 0>
    class CrossProcessObject {
        public:
            CrossProcessObject(const char* name) {
                errno = 0;

                mIsCreated = false;
                mName = name;

                mSharedMemoryObject = shm_open(name, O_RDWR, 0660);
                bool nowCreated = false;

                if (errno == ENOENT) {
                    DEBUG("Looks like the object does not exist yet\n");
                    mSharedMemoryObject = shm_open(name, O_RDWR | O_CREAT, 0660);
                    nowCreated = true;
                }

                if (errno != 0 && mSharedMemoryObject == -1) {
                    ERROR("Could not create shared memory error=%d\n", errno);
                    return;
                }

                mIsCreated = true;

                if (nowCreated)
                    INFO("Created shared memory space called %s\n", name);
                else
                    INFO("Opened shared memory space called %s\n", name);

                if (mSharedMemoryObject == -1) {
                    perror("shm_open");
                    return;
                }

                if (ftruncate(mSharedMemoryObject, sizeof(T) + extra_alloc + 1) != 0) {
                    perror("ftruncate");
                    return;
                }

                void* addr = mmap(NULL, sizeof(T) + extra_alloc + 1, PROT_READ | PROT_WRITE,
                    MAP_SHARED, mSharedMemoryObject, 0);

                if (addr == MAP_FAILED) {
                    perror("mmap");
                    return;
                }

                if (nowCreated) {
                    DEBUG("filling object with zeros [p=%p,n=%d]\n", addr, sizeof(T) + extra_alloc + 1);
                    memset(addr, 0, sizeof(T) + extra_alloc + 1);
                }

                mObject = (T*)addr;
                mIsAssigned = ((bool*)addr) + sizeof(T) + extra_alloc;
            }

            void Destroy() {
                DEBUG("Destroying shared memory space called %s\n", mName);
                mIsCreated = false;

                if (munmap((void *)mObject, sizeof(T) + extra_alloc + 1)) {
                    perror("munmap");
                    return;
                }

                mObject = NULL;
                if (close(mSharedMemoryObject)) {
                    perror("close");
                    return;
                }

                mSharedMemoryObject = 0;
                if (shm_unlink(mName)) {
                    perror("shm_unlink");
                    return;
                }
            }

            const bool IsValid() const {
                return mIsCreated;
            }

            const bool AlreadyAssigned() const {
                return *mIsAssigned;
            }

            void Assign(T* obj) {
                assert(mIsCreated);
                memcpy(mObject, obj, sizeof(T));
                *mIsAssigned = true;
            }

            T *Get() {
                assert(mIsCreated);
                return mObject;
            }
        private:
            bool        mIsCreated;
            bool*       mIsAssigned;
            const char* mName;
            int         mSharedMemoryObject;
            T*          mObject;
    };

    class CrossProcessMutex {
        public:
            CrossProcessMutex(const char* name);
            ~CrossProcessMutex();
            void Lock();
            bool TryLock();
            void Unlock();
            void Destroy();

        private:
            CrossProcessObject<pthread_mutex_t>*    mMutexPtr;
            const char*                             mName;
    };
}

#endif // LD_CROSSPROC_H
