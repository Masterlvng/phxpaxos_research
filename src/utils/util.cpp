/*
Tencent is pleased to support the open source community by making 
PhxPaxos available.
Copyright (C) 2016 THL A29 Limited, a Tencent company. 
All rights reserved.

Licensed under the BSD 3-Clause License (the "License"); you may 
not use this file except in compliance with the License. You may 
obtain a copy of the License at

https://opensource.org/licenses/BSD-3-Clause

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" basis, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
implied. See the License for the specific language governing 
permissions and limitations under the License.

See the AUTHORS file for names of contributors. 
*/

#include "util.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <math.h>

namespace phxpaxos {

uint64_t now() { 
    timeval tv;
    gettimeofday(&tv, 0);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000; 
}

const uint64_t Time :: GetTimestampMS()
{
    uint64_t llNow;
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    
    llNow = tv.tv_sec;
    llNow *= 1000;
    llNow += tv.tv_usec / 1000; 

    return llNow;
}

void Time :: MsSleep(const int iTimeMs)
{
    timespec t;
    t.tv_sec = iTimeMs / 1000; 
    t.tv_nsec = (iTimeMs % 1000) * 1000000;

    int ret = 0;
    do 
    {
        ret = ::nanosleep(&t, &t);
    } while (ret == -1 && errno == EINTR); 
}

/////////////////////////////////////////////
int FileUtils :: IsDir(const std::string & sPath, bool & bIsDir)
{
    bIsDir = false;
    struct stat tStat;
    int ret = stat(sPath.c_str(), &tStat);
    if (ret != 0)
    {
        return ret;
    }

    if (tStat.st_mode & S_IFDIR)
    {
        bIsDir = true;
    }

    return 0;
}

int FileUtils :: DeleteDir(const std::string & sDirPath)
{
    DIR * dir = nullptr;
    struct dirent  * ptr;

    dir = opendir(sDirPath.c_str());
    if (dir == nullptr)
    {
        return -1;
    }


    int ret = 0;
    while ((ptr = readdir(dir)) != nullptr)
    {
        if (strcmp(ptr->d_name, ".") == 0
                || strcmp(ptr->d_name, "..") == 0)
        {
            continue;
        }

        char sChildPath[1024] = {0};
        snprintf(sChildPath, sizeof(sChildPath), "%s/%s", sDirPath.c_str(), ptr->d_name);

        bool bIsDir = false;
        ret = FileUtils::IsDir(sChildPath, bIsDir);
        if (ret != 0)
        {
            break;
        }

        if (bIsDir)
        {
            ret = DeleteDir(sChildPath);
            if (ret != 0)
            {
                break;
            }
        }
        else
        {
            ret = remove(sChildPath);
            if (ret != 0)
            {
                break;
            }
        }
    }

    closedir(dir);

    if (ret == 0)
    {
        ret = remove(sDirPath.c_str());
    }

    return ret;
}

int FileUtils :: IterDir(const std::string & sDirPath, std::vector<std::string> & vecFilePathList)
{
    DIR * dir = nullptr;
    struct dirent  * ptr;

    dir = opendir(sDirPath.c_str());
    if (dir == nullptr)
    {
        return 0;
    }


    int ret = 0;
    while ((ptr = readdir(dir)) != nullptr)
    {
        if (strcmp(ptr->d_name, ".") == 0
                || strcmp(ptr->d_name, "..") == 0)
        {
            continue;
        }

        char sChildPath[1024] = {0};
        snprintf(sChildPath, sizeof(sChildPath), "%s/%s", sDirPath.c_str(), ptr->d_name);

        bool bIsDir = false;
        ret = FileUtils::IsDir(sChildPath, bIsDir);
        if (ret != 0)
        {
            break;
        }

        if (bIsDir)
        {
            ret = IterDir(sChildPath, vecFilePathList);
            if (ret != 0)
            {
                break;
            }
        }
        else
        {
            vecFilePathList.push_back(sChildPath);
        }
    }

    closedir(dir);

    return ret;
}

////////////////////////////////

TimeStat :: TimeStat()
{
    m_llTime = 0;
}

int TimeStat :: Point()
{
    uint64_t llNowTime = Time::GetTimestampMS();
    int llPassTime = 0;
    if (llNowTime > m_llTime)
    {
        llPassTime = llNowTime - m_llTime;
    }

    m_llTime = llNowTime;

    return llPassTime;
}

/////////////////////////////////

uint64_t OtherUtils :: GenGid(const uint64_t llNodeID)
{
    return (llNodeID ^ FastRand()) + FastRand();
}

//////////////////////////////////////////////////////////

#ifdef __i386

__inline__ uint64_t rdtsc()
{
    uint64_t x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
}

#elif __amd64

__inline__ uint64_t rdtsc()
{

    uint64_t a, d;
    __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
    return (d<<32) | a;
}

#endif

struct FastRandomSeed {
    bool init;
    unsigned int seed;
};

static __thread FastRandomSeed seed_thread_safe = { false, 0 };

static void ResetFastRandomSeed()
{
    seed_thread_safe.seed = rdtsc();
    seed_thread_safe.init = true; 
}

static void InitFastRandomSeedAtFork()
{
    pthread_atfork(ResetFastRandomSeed, ResetFastRandomSeed, ResetFastRandomSeed);
}

static void InitFastRandomSeed()
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, InitFastRandomSeedAtFork);

    ResetFastRandomSeed();
}

const uint32_t OtherUtils :: FastRand()
{
    if (!seed_thread_safe.init)
    {
        InitFastRandomSeed();
    }

    return rand_r(&seed_thread_safe.seed);
}

}

