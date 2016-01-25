/** @file
 * Implementation of ThreadTask
 */

/*
 * Copyright (C) 2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/thread.h>

#include "VirtualBoxBase.h"
#include "ThreadTask.h"

/**
 *  The function takes ownership of "this" instance (object
 *  instance which calls this function).
 *  And the function is responsible for deletion of "this"
 *  pointer in all cases.
 *  Possible way of usage:
 * 
 *  int vrc = VINF_SUCCESS;
 *  HRESULT hr = S_OK;
 * 
 *  SomeTaskInheritedFromThreadTask* pTask = NULL;
 *  try
 *  {
 *      pTask = new SomeTaskInheritedFromThreadTask(this);
 *      if (!pTask->Init())//some init procedure
 *      {
 *          delete pTask;
 *          throw E_FAIL;
 *      }
 *      //this function delete pTask in case of exceptions, so
 *      there is no need the call of delete operator
 * 
 *      hr = pTask->createThread();
 *  }
 *  catch(...)
 *  {
 *      vrc = E_FAIL;
 *  }
 */
HRESULT ThreadTask::createThread(PRTTHREAD pThread, RTTHREADTYPE enmType)
{
    HRESULT rc = S_OK;

    m_pThread = pThread;
    int vrc = RTThreadCreate(m_pThread,
                             taskHandler,
                             (void *)this,
                             0,
                             enmType,
                             0,
                             this->getTaskName().c_str());

    if (RT_FAILURE(vrc))
    {
        delete this; 
        return E_FAIL;
    }

    return rc;
}

/**
 * Static method that can get passed to RTThreadCreate to have a
 * thread started for a Task.
 */
/* static */ DECLCALLBACK(int) ThreadTask::taskHandler(RTTHREAD /* thread */, void *pvUser)
{
    HRESULT rc = S_OK;
    if (pvUser == NULL)
        return VERR_INVALID_POINTER;

    ThreadTask *pTask = static_cast<ThreadTask *>(pvUser);

    /*
    *  handler shall catch and process all possible cases as errors and exceptions.
    */
    pTask->handler();

    delete pTask;

    return 0;
}

/*static*/ HRESULT ThreadTask::setErrorStatic(HRESULT aResultCode,
                                    const Utf8Str &aText)
{
    NOREF(aText);
    return aResultCode;
}

