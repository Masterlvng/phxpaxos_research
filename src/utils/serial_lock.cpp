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

#include "serial_lock.h"
#include "util.h"

namespace phxpaxos
{

SerialLock :: SerialLock() : m_oCond(m_oMutex)
{
}

SerialLock :: ~SerialLock()
{
}

void SerialLock :: Lock()
{
    m_oMutex.lock();
}

void SerialLock :: UnLock()
{
    m_oMutex.unlock();
}

void SerialLock :: Wait()
{
    m_oCond.wait();
}

void SerialLock :: Interupt()
{
    m_oCond.signal();
}

bool SerialLock :: WaitTime(const int iTimeMs)
{
    uint64_t llTimeout = Time::GetTimestampMS() + iTimeMs;

    timespec ts;
    ts.tv_sec = (time_t)(llTimeout / 1000);
    ts.tv_nsec = (llTimeout % 1000) * 1000000;
    
    return m_oCond.tryWait(&ts);
}

}

