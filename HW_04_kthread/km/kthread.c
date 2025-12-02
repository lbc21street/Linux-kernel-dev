//=================================================================================================
//
// \file    kthread.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <data.h>
#include <supportmacros.h>

#include "kthread.h"

//
// global data
//

KTH_CTRL KthCtrl;

//
//
//

static int KthpWriterWorkerThread(void *Data)
{
    struct task_struct *thread = current;
    uint32_t increment = PTR_UINT(Data);
    uint32_t iterations = 0;

    pr_info("entering worker 0x%px", thread);

    while (TRUE) {
        if (kthread_should_stop()) {
            pr_info("writer worker 0x%px was stopped", thread);

            break;
        }

        XTH_WORKER_ACTION action = XthWriteData(&KthCtrl.Data, increment);

        if (action == XthWorkerActionStop) {
            pr_info("writer worker 0x%px completed", thread);

            break;
        }

        ++iterations;

    } // while (TRUE)

    pr_info("leaving worker 0x%px - iterations %u", thread, iterations);

    return 0;
}

//
//
//

static int KthpReaderWorkerThread(void *Data)
{
    struct task_struct *thread = current;
    uint32_t iterations = 0;

    pr_info("entering worker 0x%px", thread);

    while (TRUE) {
        if (kthread_should_stop()) {
            pr_info("reader worker 0x%px was stopped", thread);

            break;
        }

        XTH_WORKER_ACTION action = XthReadData(&KthCtrl.Data);

        if (action == XthWorkerActionStop) {
            pr_info("reader worker 0x%px completed", thread);

            break;
        }

        if (action == XthWorkerActionBackOff) {
            schedule_timeout_idle(1);

            ++KthCtrl.Data.Backoffs;
        }

        ++iterations;

    } // while (TRUE)

    pr_info("leaving worker 0x%px - iterations %u MaxDataWritten %u Backoffs %u", thread,
            iterations, KthCtrl.Data.MaxDataWritten, KthCtrl.Data.Backoffs);

    return 0;
}

//
//
//

static int KthpCreateWriterWorkerThreads(void)
{
    int result = 0;
    uint32_t c = 0;

    while (c < ARRAY_SIZE(KthCtrl.WriterWorkers)) {
        void *data = UINT_PTR((c + 1) * 10);

        struct task_struct *thread =
            kthread_create(KthpWriterWorkerThread, data, "KthWriterThread%u", c);

        if (!IS_ERR(thread)) {
            get_task_struct(thread);
            KthCtrl.WriterWorkers[c] = thread;

            pr_info("created writer worker 0x%px (%u)", thread, c);
        }

        else {
            result = PTR_ERR(thread);

            pr_err("kthread_create() failed %d (%u)", result, c);
        }

        ++c;

    } // while (c < ARRAY_SIZE(KthCtrl.WriterWorkers))

    return result;
}

//
//
//

static int KthpCreateReaderWorkerThread(void)
{
    int result = 0;

    struct task_struct *thread = kthread_create(KthpReaderWorkerThread, NULL, "KthReaderThread");

    if (!IS_ERR(thread)) {
        get_task_struct(thread);
        KthCtrl.ReaderWorker = thread;

        pr_info("created reader worker 0x%px", thread);
    }

    else {
        result = PTR_ERR(thread);

        pr_err("kthread_create() failed %d", result);
    }

    return result;
}

//
//
//

static void KthpResumeWriterWorkerThreads(void)
{
    int result;
    uint32_t c = 0;

    while (c < ARRAY_SIZE(KthCtrl.WriterWorkers)) {
        pr_info("waking up writer worker 0x%px (%u)", KthCtrl.WriterWorkers[c], c);

        result = wake_up_process(KthCtrl.WriterWorkers[c]);

        pr_info("woke up writer worker 0x%px (%u) (result %d)", KthCtrl.WriterWorkers[c], c,
                result);

        ++c;

    } // while (c < ARRAY_SIZE(KthCtrl.WriterWorkers))
}

//
//
//

static void KthpResumeReaderWorkerThread(void)
{
    int result;

    pr_info("waking up reader worker 0x%px", KthCtrl.ReaderWorker);

    result = wake_up_process(KthCtrl.ReaderWorker);

    pr_info("woke up reader worker 0x%px (result %d)", KthCtrl.ReaderWorker, result);
}

//
//
//

static int KthpSetupWorkerThreads(void)
{
    int result = 0;

    do {
        init_rwsem(&KthCtrl.Data.Lock);

        result = KthpCreateWriterWorkerThreads();

        if (result != 0) {
            break;
        }

        result = KthpCreateReaderWorkerThread();

        if (result != 0) {
            break;
        }

        KthpResumeWriterWorkerThreads();

        KthpResumeReaderWorkerThread();

    } while (FALSE);

    return result;
}

//
//
//

static void KthpTeardownWorkerThreads(void)
{
    int result;
    uint32_t c = 0;

    while (c < ARRAY_SIZE(KthCtrl.WriterWorkers)) {
        if (KthCtrl.WriterWorkers[c] != NULL) {
            pr_info("stopping writer worker 0x%px (%u)", KthCtrl.WriterWorkers[c], c);

            //
            // sets kthread_should_stop() for the given thread to return true, wakes it, and waits
            // for it to exit; this can also be called after kthread_create() instead of calling
            // wake_up_process(): the thread will exit without calling threadfn()
            //
            // if threadfn() may call kthread_exit() itself, the caller must ensure task_struct
            // can't go away
            //
            // returns the result of threadfn(), or -EINTR if wake_up_process() was never called
            //

            //
            // we use kthread_stop_put() because our worker thread routines can exit itself
            //
            // stops a thread created by kthread_create() and put its task_struct; only use when
            // holding an extra task struct reference obtained by calling get_task_struct()
            //

            result = kthread_stop_put(KthCtrl.WriterWorkers[c]);

            pr_info("writer worker 0x%px (%u) stopped (result %d)", KthCtrl.WriterWorkers[c], c,
                    result);

            KthCtrl.WriterWorkers[c] = NULL;
        }

        ++c;

    } // while (c < ARRAY_SIZE(KthCtrl.WriterWorkers))

    if (KthCtrl.ReaderWorker != NULL) {
        pr_info("stopping reader worker 0x%px", KthCtrl.ReaderWorker);

        result = kthread_stop_put(KthCtrl.ReaderWorker);

        pr_info("reader worker 0x%px stopped (result %d)", KthCtrl.ReaderWorker, result);

        KthCtrl.ReaderWorker = NULL;
    }
}

//
//
//

static int __init KthInit(void)
{
    int result = 0;

    pr_info("entering...");

    do {
        result = KthpSetupWorkerThreads();

        if (result != 0) {
            break;
        }

    } while (FALSE);

    if (result != 0) {
        KthpTeardownWorkerThreads();

        pr_err("leaving, result %d", result);
    }

    else {
        pr_info("leaving...");
    }

    return result;
}

//
//
//

static void __exit KthExit(void)
{
    pr_info("entering...");

    KthpTeardownWorkerThreads();

    pr_info("leaving...");
}

//
//
//

module_init(KthInit);
module_exit(KthExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION(
    "kthread - a simple Linux kernel driver for testing the kthread and RW lock API");
MODULE_VERSION("1.0");

//=================================================================================================
