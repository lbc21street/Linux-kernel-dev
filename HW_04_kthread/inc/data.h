//=================================================================================================
//
// \file    data.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#ifdef USERMODE
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#else // USERMODE
#include <linux/kernel.h>
#include <linux/rwsem.h>
#include <linux/types.h>
#endif // USERMODE

#include <supportmacros.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define XTH_NUMBER_OF_WRITER_THREADS 2

#define XTH_DATA_ITERATIONS 100000000

//
//
//

typedef struct _XTH_DATA {

    uint32_t Data;
    uint32_t DataWritten;
    uint32_t MaxDataWritten;

    uint32_t WriterIterations;
    uint32_t ReaderIterations;

    uint32_t Backoffs;

#ifdef USERMODE
    pthread_rwlock_t Lock;
#else  // USERMODE
    struct rw_semaphore Lock;
#endif // USERMODE

} XTH_DATA, *PXTH_DATA;

//
//
//

typedef enum _XTH_WORKER_ACTION {

    XthWorkerActionStop = 0,
    XthWorkerActionContinue,
    XthWorkerActionBackOff,

} XTH_WORKER_ACTION;

#ifdef USERMODE

#ifndef TRUE
#define TRUE true
#endif // TRUE

#ifndef FALSE
#define FALSE false
#endif // FALSE

//
//
//

static inline void XthAcquireLockExclusive(PXTH_DATA Data)
{
    pthread_rwlock_wrlock(&Data->Lock);
}

//
//
//

static inline void XthReleaseLockExclusive(PXTH_DATA Data)
{
    pthread_rwlock_unlock(&Data->Lock);
}

//
//
//

static inline void XthAcquireLockShared(PXTH_DATA Data)
{
    pthread_rwlock_rdlock(&Data->Lock);
}

//
//
//

static inline void XthReleaseLockShared(PXTH_DATA Data)
{
    pthread_rwlock_unlock(&Data->Lock);
}

#else // USERMODE

//
//
//

static inline void XthAcquireLockExclusive(PXTH_DATA Data)
{
    down_write(&Data->Lock);
}

//
//
//

static inline void XthReleaseLockExclusive(PXTH_DATA Data)
{
    up_write(&Data->Lock);
}

//
//
//

static inline void XthAcquireLockShared(PXTH_DATA Data)
{
    down_read(&Data->Lock);
}

//
//
//

static inline void XthReleaseLockShared(PXTH_DATA Data)
{
    up_read(&Data->Lock);
}

#endif // USERMODE

//
//
//

static XTH_WORKER_ACTION XthWriteData(PXTH_DATA Data, uint32_t Increment)
{
    XthAcquireLockExclusive(Data);

    if (Data->WriterIterations == XTH_DATA_ITERATIONS) {
        XthReleaseLockExclusive(Data);

        return XthWorkerActionStop;
    }

    ++Data->WriterIterations;

    Data->Data += Increment;
    ++Data->DataWritten;

    XthReleaseLockExclusive(Data);

    return XthWorkerActionContinue;
}

//
//
//

static XTH_WORKER_ACTION XthReadData(PXTH_DATA Data)
{
    XthAcquireLockShared(Data);

    if (Data->ReaderIterations == XTH_DATA_ITERATIONS) {
        XthReleaseLockShared(Data);

        return XthWorkerActionStop;
    }

    XTH_WORKER_ACTION action = XthWorkerActionContinue;

    if (Data->DataWritten) {
        uint32_t c = 0;

        while (c < Data->DataWritten) {
            Data->Data = 0;

            ++Data->ReaderIterations;

            ++c;

        } // while (c < Data->DataWritten)

        if (Data->MaxDataWritten < Data->DataWritten) {
            Data->MaxDataWritten = Data->DataWritten;
        }

        Data->DataWritten = 0;
    }

    else {
        action = XthWorkerActionBackOff;
    }

    XthReleaseLockShared(Data);

    return action;
}

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
