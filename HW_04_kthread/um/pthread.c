//=================================================================================================
//
// \file    pthread.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" UBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE
#include <fcntl.h>

#include <data.h>
#include <supportmacros.h>

#include "pthread.h"
#include "trace.h"
#include "utils.h"

//
// global data
//

PTH_CTRL PthCtrl;

//
//
//

static void *PthpWriterWorkerThread(void *Arg)
{
    const pthread_t threadId = pthread_self();
    uint32_t increment = PTR_UINT(Arg);
    uint32_t iterations = 0;

    pr_info("entering worker 0x%lX", threadId);

    while (true) {
        if (FlagOn(PthCtrl.Flags, PTH_CTLFL_WRITER_WORKER_STOP_PENDING)) {
            pr_info("writer worker 0x%lX was stopped", threadId);

            break;
        }

        XTH_WORKER_ACTION action = XthWriteData(&PthCtrl.Data, increment);

        if (action == XthWorkerActionStop) {
            pr_info("writer worker 0x%lX completed", threadId);

            break;
        }

        ++iterations;

    } // while (true)

    pr_info("leaving worker 0x%lX - iterations %u", threadId, iterations);

    return 0;
}

//
//
//

static void *PthpReaderWorkerThread(void *Arg)
{
    const pthread_t threadId = pthread_self();
    uint32_t iterations = 0;

    pr_info("entering worker 0x%lX", threadId);

    while (true) {
        if (FlagOn(PthCtrl.Flags, PTH_CTLFL_READER_WORKER_STOP_PENDING)) {
            pr_info("reader worker 0x%lX was stopped", threadId);

            break;
        }

        XTH_WORKER_ACTION action = XthReadData(&PthCtrl.Data);

        if (action == XthWorkerActionStop) {
            pr_info("reader worker 0x%lX completed", threadId);

            break;
        }

        if (action == XthWorkerActionBackOff) {
            usleep(0);

            ++PthCtrl.Data.Backoffs;
        }

        ++iterations;

    } // while (true)

    pr_info("leaving worker 0x%lX - iterations %u MaxDataWritten %u Backoffs %u", threadId,
            iterations, PthCtrl.Data.MaxDataWritten, PthCtrl.Data.Backoffs);

    return 0;
}

//
//
//

static int PthpCreateWriterWorkerThreads(void)
{
    int result = 0;
    uint32_t c = 0;

    while (c < ARRAY_SIZE(PthCtrl.WriterWorkers)) {
        void *arg = UINT_PTR((c + 1) * 10);

        // pthread_attr_t attr;

        // pthread_attr_init(&attr);

        pthread_t threadId;

        result = pthread_create(&threadId, NULL, PthpWriterWorkerThread, arg);

        if (result == 0) {
            PthCtrl.WriterWorkers[c] = threadId;

            pr_info("created writer worker 0x%lX (%u)", threadId, c);
        }

        else {
            pr_err("pthread_create() failed %d (%u)", result, c);
        }

        ++c;

    } // while (c < ARRAY_SIZE(PthCtrl.WriterWorkers))

    return result;
}

//
//
//

static int PthpCreateReaderWorkerThread(void)
{
    int result = 0;

    // pthread_attr_t attr;

    // pthread_attr_init(&attr);

    pthread_t threadId;

    result = pthread_create(&threadId, NULL, PthpReaderWorkerThread, NULL);

    if (result == 0) {
        PthCtrl.ReaderWorker = threadId;

        pr_info("created reader worker 0x%lX", threadId);
    }

    else {
        pr_err("pthread_create() failed %d", result);
    }

    return result;
}

//
//
//

static void PthpPrepareWorkers(void)
{
    uint32_t c = 0;

    while (c < ARRAY_SIZE(PthCtrl.WriterWorkers)) {
        PthCtrl.WriterWorkers[c] = PTH_INVALID_THREAD_ID;

        ++c;

    } // while (c < ARRAY_SIZE(PthCtrl.WriterWorkers))

    PthCtrl.ReaderWorker = PTH_INVALID_THREAD_ID;
}

//
//
//

static int PthpSetupWorkerThreads(void)
{
    int result = 0;

    do {
        PthpPrepareWorkers();

        result = pthread_rwlock_init(&PthCtrl.Data.Lock, NULL);

        if (result != 0) {
            pr_err("pthread_rwlock_init() failed %d", result);

            break;
        }

        pr_info("inited Lock %p", &PthCtrl.Data.Lock);

        SetFlag(PthCtrl.Flags, PTH_CTLFL_LOCK_INITED);

        result = PthpCreateWriterWorkerThreads();

        if (result != 0) {
            break;
        }

        result = PthpCreateReaderWorkerThread();

        if (result != 0) {
            break;
        }

    } while (false);

    return result;
}

//
//
//

static void PthpTeardownWorkerThreads(void)
{
    if (!FlagOn(PthCtrl.Flags, PTH_CTLFL_LOCK_INITED)) {
        return;
    }

    int result;
    uint32_t c = 0;

    while (c < ARRAY_SIZE(PthCtrl.WriterWorkers)) {
        atomic_fetch_or((atomic_uint *)&PthCtrl.Flags, PTH_CTLFL_WRITER_WORKER_STOP_PENDING);

        if (PthCtrl.WriterWorkers[c] != PTH_INVALID_THREAD_ID) {
            pr_info("stopping writer worker 0x%lX (%u)", PthCtrl.WriterWorkers[c], c);

            result = pthread_join(PthCtrl.WriterWorkers[c], NULL);

            if (result == 0) {
                pr_info("writer worker 0x%lX (%u) terminated", PthCtrl.WriterWorkers[c], c);

                PthCtrl.WriterWorkers[c] = PTH_INVALID_THREAD_ID;
            }

            else {
                pr_err("pthread_join() failed %d writer worker 0x%lX (%u)", result,
                       PthCtrl.WriterWorkers[c], c);
            }
        }

        ++c;

    } // while (c < ARRAY_SIZE(PthCtrl.WriterWorkers))

    if (PthCtrl.ReaderWorker != PTH_INVALID_THREAD_ID) {
        atomic_fetch_or((atomic_uint *)&PthCtrl.Flags, PTH_CTLFL_READER_WORKER_STOP_PENDING);

        pr_info("stopping reader worker 0x%lX", PthCtrl.ReaderWorker);

        result = pthread_join(PthCtrl.ReaderWorker, NULL);

        if (result == 0) {
            pr_info("reader worker 0x%lX terminated", PthCtrl.ReaderWorker);

            PthCtrl.ReaderWorker = PTH_INVALID_THREAD_ID;
        }

        else {
            pr_err("pthread_join() failed %d reader worker 0x%lX", result, PthCtrl.ReaderWorker);
        }
    }

    pr_info("destroying Lock %p", &PthCtrl.Data.Lock);

    result = pthread_rwlock_destroy(&PthCtrl.Data.Lock);

    if (result != 0) {
        pr_err("pthread_rwlock_destroy() failed %d", result);
    }

    ClearFlag(PthCtrl.Flags, PTH_CTLFL_LOCK_INITED);
}

//
//
//

int main(int argc, const char *argv[])
{
    printf("[" UBUILD_MODNAME "] entering\n");

    int result = 0;

    do {
        result = PthOpenKmsg();

        if (result != 0) {
            break;
        }

        result = PthpSetupWorkerThreads();

        if (result != 0) {
            break;
        }

        printf("[" UBUILD_MODNAME "] press any key to terminate...\n");

        getch();

    } while (false);

    PthpTeardownWorkerThreads();

    PthCloseKmsg();

    printf("[" UBUILD_MODNAME "] leaving\n");

    return 0;
}

//=================================================================================================
