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

#include "committer.h"
#include "commitctx.h"
#include "ioloop.h"
#include "commdef.h"

namespace phxpaxos
{

Committer :: Committer(Config * poConfig, CommitCtx * poCommitCtx, IOLoop * poIOLoop, SMFac * poSMFac)
    : m_poConfig(poConfig), m_poCommitCtx(poCommitCtx), m_poIOLoop(poIOLoop), m_poSMFac(poSMFac), m_iTimeoutMs(-1)
{
}

Committer :: ~Committer()
{
}

int Committer :: NewValue(const std::string & sValue)
{
    uint64_t llInstanceID = 0;
    return NewValueGetID(sValue, llInstanceID, nullptr, nullptr);
}

int Committer :: NewValueGetID(const std::string & sValue, uint64_t & llInstanceID)
{
    return NewValueGetID(sValue, llInstanceID, nullptr, nullptr);
}

int Committer :: NewValueGetID(const std::string & sValue, uint64_t & llInstanceID, StateMachine * poSM, SMCtx * poSMCtx)
{
    BP->GetCommiterBP()->NewValue();

    int iRetryCount = 3;
    int ret = PaxosTryCommitRet_OK;
    while(iRetryCount--)
    {
        TimeStat oTimeStat;
        oTimeStat.Point();

        ret = NewValueGetIDNoRetry(sValue, llInstanceID, poSM, poSMCtx);
        if (ret != PaxosTryCommitRet_Conflict)
        {
            if (ret == 0)
            {
                BP->GetCommiterBP()->NewValueCommitOK(oTimeStat.Point());
            }
            else
            {
                BP->GetCommiterBP()->NewValueCommitFail();
            }
            break;
        }

        BP->GetCommiterBP()->NewValueConflict();

        if (poSMCtx != nullptr && poSMCtx->m_iSMID == MASTER_V_SMID)
        {
            //master sm not retry
            break;
        }
    }

    return ret;
}

int Committer :: NewValueGetIDNoRetry(const std::string & sValue, uint64_t & llInstanceID, 
        StateMachine * poSM, SMCtx * poSMCtx)
{
    int iLockUseTimeMs = 0;
    bool bHasLock = m_oWaitLock.Lock(m_iTimeoutMs, iLockUseTimeMs);
    if (!bHasLock)
    {
        BP->GetCommiterBP()->NewValueGetLockTimeout();
        PLGErr("Try get lock, but timeout, lockusetime %dms", iLockUseTimeMs);
        return PaxosTryCommitRet_Timeout; 
    }

    int iLeftTimeoutMs = -1;
    if (m_iTimeoutMs > 0)
    {
        iLeftTimeoutMs = m_iTimeoutMs > iLockUseTimeMs ? m_iTimeoutMs - iLockUseTimeMs : 0;
        if (iLeftTimeoutMs < 100)
        {
            PLGErr("Get lock ok, but lockusetime %dms too long, lefttimeout %dms", iLockUseTimeMs, iLeftTimeoutMs);

            BP->GetCommiterBP()->NewValueGetLockTimeout();

            m_oWaitLock.UnLock();
            return PaxosTryCommitRet_Timeout;
        }
    }

    PLGImp("GetLock ok, use time %dms", iLockUseTimeMs);
    
    BP->GetCommiterBP()->NewValueGetLockOK(iLockUseTimeMs);

    //pack smid to value
    int iSMID = poSM != nullptr ? poSM->SMID() : 0;
    if (iSMID == 0)
    {
        iSMID = poSMCtx != nullptr ? poSMCtx->m_iSMID : 0;
    }
    
    string sPackSMIDValue = sValue;
    m_poSMFac->PackPaxosValue(sPackSMIDValue, iSMID);

    m_poCommitCtx->NewCommit(&sPackSMIDValue, poSM, poSMCtx, iLeftTimeoutMs);
    m_poIOLoop->AddNotify();

    int ret = m_poCommitCtx->GetResult(llInstanceID);

    m_oWaitLock.UnLock();
    return ret;
}

////////////////////////////////////////////////////

void Committer :: SetTimeoutMs(const int iTimeoutMs)
{
    m_iTimeoutMs = iTimeoutMs;
}
    
}

