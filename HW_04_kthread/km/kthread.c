//=================================================================================================
//
// \file    kthread.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>

#include "kthread.h"
#include "supportmacros.h"

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
    uint32_t increment = KTH_PTR_UINT(Data);

    pr_info("entering worker 0x%px", thread);

    while (TRUE) {
        if (kthread_should_stop()) {
            pr_info("writer worker 0x%px was stopped", thread);

            break;
        }

        down_write(&KthCtrl.Lock);

        if (KthCtrl.WriterCycles == KTH_MAX_CYCLES) {
            up_write(&KthCtrl.Lock);

            pr_info("writer worker 0x%px completed", thread);

            break;
        }

        ++KthCtrl.WriterCycles;

        KthCtrl.Data += increment;
        ++KthCtrl.DataWritten;

        up_write(&KthCtrl.Lock);

    } // while (TRUE)

    pr_info("leaving worker 0x%px", thread);

    return 0;
}

//
//
//

static int KthpReaderWorkerThread(void *Data)
{
    struct task_struct *thread = current;

    pr_info("entering worker 0x%px", thread);

    while (TRUE) {
        if (kthread_should_stop()) {
            pr_info("reader worker 0x%px was stopped", thread);

            break;
        }

        down_read(&KthCtrl.Lock);

        if (KthCtrl.ReaderCycles >= KTH_MAX_CYCLES) {
            up_read(&KthCtrl.Lock);

            pr_info("reader worker 0x%px completed", thread);

            break;
        }

        if (KthCtrl.DataWritten) {
            uint32_t c = 0;

            while (c < KthCtrl.DataWritten) {
                KthCtrl.Data = 0;

                ++KthCtrl.ReaderCycles;

                ++c;

            } // while (c < KthCtrl.DataWritten)

            KthCtrl.DataWritten = 0;
        }

        up_read(&KthCtrl.Lock);

    } // while (TRUE)

    pr_info("leaving worker 0x%px", thread);

    return 0;
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

static int KthpCreateWriterWorkerThreads(void)
{
    int result = 0;
    uint32_t c = 0;

    while (c < ARRAY_SIZE(KthCtrl.WriterWorkers)) {
        void *data = KTH_UINT_PTR((c + 1) * 10);

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

static int KthpSetupWorkerThreads(void)
{
    int result = 0;

    do {
        init_rwsem(&KthCtrl.Lock);

        result = KthpCreateWriterWorkerThreads();

        if (result != 0) {
            break;
        }

        result = KthpCreateReaderWorkerThread();

        if (result != 0) {
            break;
        }

        pr_info("waking up reader worker 0x%px", KthCtrl.ReaderWorker);

        wake_up_process(KthCtrl.ReaderWorker);

        pr_info("woke up reader worker 0x%px", KthCtrl.ReaderWorker);

        uint32_t c = 0;

        while (c < ARRAY_SIZE(KthCtrl.WriterWorkers)) {
            pr_info("waking up writer worker 0x%px (%u)", KthCtrl.WriterWorkers[c], c);

            wake_up_process(KthCtrl.WriterWorkers[c]);

            pr_info("woke up writer worker 0x%px (%u)", KthCtrl.WriterWorkers[c], c);

            ++c;

        } // while (c < ARRAY_SIZE(KthCtrl.WriterWorkers))

    } while (FALSE);

    return result;
}

//
//
//

static void KthpTeardownWorkerThreads(void)
{
    int result;

    if (KthCtrl.ReaderWorker != NULL) {
        pr_info("stopping reader worker 0x%px", KthCtrl.ReaderWorker);

        //
        // sets kthread_should_stop() for the given thread to return true, wakes it, and waits for
        // it to exit; this can also be called after kthread_create() instead of calling
        // wake_up_process(): the thread will exit without calling threadfn()
        //
        // if threadfn() may call kthread_exit() itself, the caller must ensure task_struct can't go
        // away
        //
        // returns the result of threadfn(), or -EINTR if wake_up_process() was never called
        //

        //
        // we use kthread_stop_put() because our worker thread routines can exit itself
        //
        // stops a thread created by kthread_create() and put its task_struct; only use when holding
        // an extra task struct reference obtained by calling get_task_struct()
        //

        result = kthread_stop_put(KthCtrl.ReaderWorker);

        pr_info("reader worker 0x%px stopped (result %d)", KthCtrl.ReaderWorker, result);

        KthCtrl.ReaderWorker = NULL;
    }

    uint32_t c = 0;

    while (c < ARRAY_SIZE(KthCtrl.WriterWorkers)) {
        if (KthCtrl.WriterWorkers[c] != NULL) {
            pr_info("stopping writer worker 0x%px (%u)", KthCtrl.WriterWorkers[c], c);

            result = kthread_stop_put(KthCtrl.WriterWorkers[c]);

            pr_info("writer worker 0x%px (%u) stopped (result %d)", KthCtrl.WriterWorkers[c], c,
                    result);

            KthCtrl.WriterWorkers[c] = NULL;
        }

        ++c;

    } // while (c < ARRAY_SIZE(KthCtrl.WriterWorkers))
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
