//=================================================================================================
//
// \file    defexec.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/preempt.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include "defexec.h"
#include "supportmacros.h"

//
// global data
//

DEX_CTRL DexCtrl;

//
//
//

static inline uintptr_t DexpGetCurrentTime(void)
{
    return jiffies;
}

//
//
//

static inline uintptr_t DexpCalculateExpirationTimeEx(uintptr_t StartTime, uint32_t Milliseconds)
{
    return StartTime + msecs_to_jiffies(Milliseconds);
}

//
//
//

static inline uintptr_t DexpCalculateExpirationTime(uint32_t Milliseconds)
{
    return DexpCalculateExpirationTimeEx(DexpGetCurrentTime(), Milliseconds);
}

//
//
//

static inline uint32_t DexpGetElapsedTime(uintptr_t StartTime, uintptr_t EndTime)
{
    return jiffies_to_msecs(EndTime - StartTime);
}

//
//
//

static void DexpWorkerRoutine(struct work_struct *WorkItem)
{
    PDEX_WORK_DATA data = container_of(WorkItem, DEX_WORK_DATA, WorkItem);

    // clang-format off
    pr_info("cpu %u data 0x%px [P %u A %u I %lu S %lu SS %lu] busy %u",
        smp_processor_id(),
        data,
        preemptible(),
        in_atomic(),
        in_interrupt(),
        in_softirq(),
        in_serving_softirq(),
        work_busy(WorkItem));
    // clang-format on

    uint32_t delay = DEX_WORKER_DELAY;

    pr_info("starting to wait for %u ms", delay);

    //
    // returns:
    //
    //   0 - if the condition evaluated to false after the timeout elapsed
    //   1 - if the condition evaluated to true after the timeout elapsed, or the remaining
    //       jiffies (at least 1) if the condition evaluated to true before the timeout elapsed
    //

    // clang-format off
    int result = wait_event_timeout(data->WaitQueueList,
        (atomic_read(&data->WaitInterrupted) == 1),
        msecs_to_jiffies(delay));
    // clang-format on

    pr_info("wait for %u ms completed (wait result %u ms)", delay, jiffies_to_msecs(result));
}

//
//
//

static void DexpTaskletCallback(struct tasklet_struct *Tasklet)
{
    PDEX_WORK_DATA data = from_tasklet(data, Tasklet, Tasklet);

    // clang-format off
    pr_info("cpu %u data 0x%px [P %u A %u I %lu S %lu SS %lu]",
        smp_processor_id(),
        data,
        preemptible(),
        in_atomic(),
        in_interrupt(),
        in_softirq(),
        in_serving_softirq());
    // clang-format on

    pr_info("scheduling WorkItem 0x%px busy %u", &data->WorkItem, work_busy(&data->WorkItem));

    schedule_work(&data->WorkItem);

    pr_info("scheduled WorkItem 0x%px busy %u", &data->WorkItem, work_busy(&data->WorkItem));
}

//
//
//

static void DexpTimerCallback(struct timer_list *Timer)
{
    PDEX_WORK_DATA data = from_timer(data, Timer, Timer);

    // clang-format off
    pr_info("cpu %u data 0x%px [P %u A %u I %lu S %lu SS %lu]",
        smp_processor_id(),
        data,
        preemptible(),
        in_atomic(),
        in_interrupt(),
        in_softirq(),
        in_serving_softirq());
    // clang-format on

    // clang-format off
    pr_info("scheduling Tasklet 0x%px (count %u state 0x%lX)",
        &data->Tasklet,
        atomic_read(&data->Tasklet.count),
        data->Tasklet.state);
    // clang-format on

    tasklet_schedule(&data->Tasklet);

    // clang-format off
    pr_info("scheduled Tasklet 0x%px (count %u state 0x%lX)",
        &data->Tasklet,
        atomic_read(&data->Tasklet.count),
        data->Tasklet.state);
    // clang-format on
}

//
//
//

static inline void DexpInitWorkData(PDEX_WORK_DATA Data)
{
    tasklet_setup(&Data->Tasklet, DexpTaskletCallback);

    INIT_WORK(&Data->WorkItem, DexpWorkerRoutine);

    init_waitqueue_head(&Data->WaitQueueList);

    atomic_set(&Data->WaitInterrupted, 0);
}

//
//
//

static void DexpTeardownWorkData(void)
{
    if (DexCtrl.WorkData == NULL) {
        return;
    }

    if (FlagOn(DexCtrl.Flags, DEX_CTLFL_TIMER_SET)) {
        pr_info("cancelling timer...");

        //
        // [NOTE]
        //
        // we should use timer_shutdown_sync() in teardown cases just to prevent rearming the timer
        // concurrently
        //
        // if the timer should be reused after shutdown, it has to be initialized again
        //
        // timer_delete_sync() returns:
        //
        //   0 - the timer was not pending
        //   1 - the timer was pending and deactivated
        //

        int result = timer_delete_sync(&DexCtrl.WorkData->Timer);

        if (result == 0) {
            pr_info("timer wasn't pending");
        }

        else {
            pr_info("timer was pending and deactivated");
        }
    }

    //
    //
    //

    // clang-format off
    pr_info("killing Tasklet 0x%px (count %u state 0x%lX)",
        &DexCtrl.WorkData->Tasklet,
        atomic_read(&DexCtrl.WorkData->Tasklet.count),
        DexCtrl.WorkData->Tasklet.state);
    // clang-format on

    tasklet_kill(&DexCtrl.WorkData->Tasklet);

    // clang-format off
    pr_info("killed Tasklet 0x%px (count %u state 0x%lX)",
        &DexCtrl.WorkData->Tasklet,
        atomic_read(&DexCtrl.WorkData->Tasklet.count),
        DexCtrl.WorkData->Tasklet.state);
    // clang-format on

    pr_info("waking up worker");

    atomic_or(1, &DexCtrl.WorkData->WaitInterrupted);

    //
    // if this function wakes up a task, it executes a full memory barrier before accessing the task
    // state
    //
    // returns the number of exclusive tasks that were awaken
    //

    int result = wake_up(&DexCtrl.WorkData->WaitQueueList);

    pr_info("woke up worker (result %d)", result);

    pr_info("cancelling WorkItem 0x%px (busy %u)", &DexCtrl.WorkData->WorkItem,
            work_busy(&DexCtrl.WorkData->WorkItem));

    //
    // cancel work and wait for its execution to finish; this function can be used even if the work
    // re-queues itself or migrates to another workqueue; on return from this function, work is
    // guaranteed to be not pending or executing on any CPU
    //
    // the caller must ensure that the workqueue on which work was last queued can't be destroyed
    // before this function returns
    //
    // returns:
    //
    //   true - if work was pending
    //   false - otherwise
    //

    bool pending = cancel_work_sync(&DexCtrl.WorkData->WorkItem);

    pr_info("cancelled WorkItem 0x%px (busy %u pending %u)", &DexCtrl.WorkData->WorkItem,
            work_busy(&DexCtrl.WorkData->WorkItem), pending);

    pr_info("freeing WorkData 0x%px", DexCtrl.WorkData);

    kfree(DexCtrl.WorkData);
    DexCtrl.WorkData = NULL;
}

//
//
//

static int DexpSetupWorkData(void)
{
    int result = 0;

    do {
        uint32_t dataLength = sizeof(DEX_WORK_DATA);

        DexCtrl.WorkData = (PDEX_WORK_DATA)kzalloc(dataLength, GFP_KERNEL);

        if (DexCtrl.WorkData == NULL) {
            result = -ENOMEM;

            pr_err("memory alloc failed for work data (%u bytes)", dataLength);

            break;
        }

        pr_info("allocated work data 0x%px", DexCtrl.WorkData);

        DexpInitWorkData(DexCtrl.WorkData);

        uint32_t timerFlags = 0;

        timer_setup(&DexCtrl.WorkData->Timer, DexpTimerCallback, timerFlags);

        uint32_t period = DEX_TIMER_PERIOD;
        uintptr_t expirationTime = DexpCalculateExpirationTime(period);

        // clang-format off
        pr_info("setting timer with period %u ms (expires %lu)",
            period,
            expirationTime);
        // clang-format on

        //
        // [NOTE]
        //
        // mod_timer() returns:
        //
        //   0 - if it modified an inactive timer
        //   1 - if it modified a pending (active) timer
        //

        result = mod_timer(&DexCtrl.WorkData->Timer, expirationTime);

        if (result != 0) {
            pr_err("mod_timer() worked on already active timer! (%d)", result);

            result = -EBUSY;

            break;
        }

        // clang-format off
        pr_info("set up timer with period %u ms (expires %lu)",
            period,
            expirationTime);
        // clang-format on

        SetFlag(DexCtrl.Flags, DEX_CTLFL_TIMER_SET);

    } while (FALSE);

    return result;
}

//
//
//

static int __init DexInit(void)
{
    int result = 0;

    pr_info("entering...");

    do {
        result = DexpSetupWorkData();

        if (result != 0) {
            break;
        }

    } while (FALSE);

    if (result != 0) {
        DexpTeardownWorkData();

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

static void __exit DexExit(void)
{
    pr_info("entering...");

    DexpTeardownWorkData();

    pr_info("leaving...");
}

//
//
//

module_init(DexInit);
module_exit(DexExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("defexec - a simple Linux kernel driver for testing the deferred execution API");
MODULE_VERSION("1.0");

//=================================================================================================
