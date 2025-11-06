//=================================================================================================
//
// \file    timertest.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/types.h>

#include "supportmacros.h"
#include "timertest.h"

//
// global data
//

TT_CTRL TtCtrl;

//
//
//

static inline uintptr_t TtpGetCurrentTime(void)
{
    return jiffies;
}

//
//
//

static inline uintptr_t TtpCalculateExpirationTimeEx(uintptr_t StartTime, uint32_t Milliseconds)
{
    return StartTime + msecs_to_jiffies(Milliseconds);
}

//
//
//

static inline uintptr_t TtpCalculateExpirationTime(uint32_t Milliseconds)
{
    return TtpCalculateExpirationTimeEx(TtpGetCurrentTime(), Milliseconds);
}

//
//
//

static inline uint32_t TtpGetElapsedTime(uintptr_t StartTime, uintptr_t EndTime)
{
    return jiffies_to_msecs(EndTime - StartTime);
}

//
//
//

static inline void TtpInitTimerData(PTT_TIMER_DATA Data)
{
    raw_atomic_set(&Data->Counter, 0);
    raw_atomic_set(&Data->StopFlag, 0);
    Data->StartTime = TtpGetCurrentTime();
    Data->StopTime = TtpCalculateExpirationTime(TT_WORKING_TIME);
}

//
//
//

static inline void TtpStoreTimerSetupTime(PTT_TIMER_DATA Data)
{
    Data->TimerSetupTime = TtpGetCurrentTime();
}

//
//
//

static void TtpTimerCallback(struct timer_list *Timer)
{
    PTT_TIMER_DATA data = from_timer(data, Timer, Timer);

    uint32_t counter = raw_atomic_fetch_add(1, &data->Counter);

    do {

        uintptr_t currentTime = TtpGetCurrentTime();
        uint32_t elapsedTime = TtpGetElapsedTime(data->StartTime, currentTime);
        uint32_t elapsedSinceLastCall = TtpGetElapsedTime(data->TimerSetupTime, currentTime);

        if (raw_atomic_read(&data->StopFlag)) {
            // clang-format off
            pr_warn("[%u] stop flag set - quitting timer, current time %lu stop time %lu elapsed %u ms (%u ms)",
                counter,
                currentTime,
                data->StopTime,
                elapsedTime,
                elapsedSinceLastCall);
            // clang-format on

            break;
        }

        if (time_before_eq(currentTime, data->StopTime)) {

            // clang-format off
            pr_info("[%u] Hello, timer! currentTime %lu elapsed %u ms (%u ms)",
                counter,
                currentTime,
                elapsedTime,
                elapsedSinceLastCall);
            // clang-format on

            uint32_t period = TT_TIMER_PERIOD;
            uintptr_t expirationTime = TtpCalculateExpirationTimeEx(currentTime, period);

            // clang-format off
            pr_info("[%u] setting timer with period %u ms (expires %lu)",
                counter,
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

            int result = mod_timer(&data->Timer, expirationTime);

            if (result != 0) {
                // clang-format off
                pr_err("[%u] mod_timer() worked on already active timer! (%d)",
                    counter,
                    result);
                // clang-format on
            }

            TtpStoreTimerSetupTime(data);

            break;
        }

        // clang-format off
        pr_info("[%u] currentTime %lu elapsed %u ms (%u ms)",
            counter,
            currentTime,
            elapsedTime,
            elapsedSinceLastCall);

        pr_info("[%u] stopped timer - originalPeriod %u ms (startTime %lu stopTime %lu)",
            counter,
            TtpGetElapsedTime(data->StartTime, data->StopTime),
            data->StartTime,
            data->StopTime);
        // clang-format on

    } while (FALSE);
}

//
//
//

static void TtpTeardownTimer(void)
{
    if (TtCtrl.TimerData == NULL) {
        return;
    }

    if (FlagOn(TtCtrl.Flags, TT_CTLFL_TIMER_SET)) {
        pr_info("setting stop flag...");

        raw_atomic_or(1, &TtCtrl.TimerData->StopFlag);

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

        int result = timer_delete_sync(&TtCtrl.TimerData->Timer);

        if (result == 0) {
            pr_info("timer wasn't pending");
        }

        else {
            pr_info("timer was pending and deactivated");
        }
    }

    pr_info("freeing TimerData 0x%px", TtCtrl.TimerData);

    kfree(TtCtrl.TimerData);
    TtCtrl.TimerData = NULL;
}

//
//
//

static int TtpSetupTimer(void)
{
    int result = 0;

    do {
        uint32_t dataLength = sizeof(TT_TIMER_DATA);

        TtCtrl.TimerData = (PTT_TIMER_DATA)kzalloc(dataLength, GFP_KERNEL);

        if (TtCtrl.TimerData == NULL) {
            result = -ENOMEM;

            pr_err("memory alloc failed for timer data (%u bytes)", dataLength);

            break;
        }

        pr_info("allocated timer data 0x%px", TtCtrl.TimerData);

        TtpInitTimerData(TtCtrl.TimerData);

        uint32_t timerFlags = 0;

        timer_setup(&TtCtrl.TimerData->Timer, TtpTimerCallback, timerFlags);

        uint32_t period = TT_TIMER_PERIOD;
        uintptr_t expirationTime = TtpCalculateExpirationTime(period);

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

        result = mod_timer(&TtCtrl.TimerData->Timer, expirationTime);

        if (result != 0) {
            pr_err("mod_timer() worked on already active timer! (%d)", result);

            result = -EBUSY;

            break;
        }

        TtpStoreTimerSetupTime(TtCtrl.TimerData);

        // clang-format off
        pr_info("set up timer with period %u ms, stop after %u ms, stop time %lu",
            period,
            TT_WORKING_TIME,
            TtCtrl.TimerData->StopTime);
        // clang-format on

        SetFlag(TtCtrl.Flags, TT_CTLFL_TIMER_SET);

    } while (FALSE);

    return result;
}

//
//
//

static int __init TtInit(void)
{
    int result = 0;

    pr_info("entering...");

    do {
        result = TtpSetupTimer();

        if (result != 0) {
            break;
        }

    } while (FALSE);

    if (result != 0) {
        TtpTeardownTimer();

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

static void __exit TtExit(void)
{
    pr_info("entering...");

    TtpTeardownTimer();

    pr_info("leaving...");
}

//
//
//

module_init(TtInit);
module_exit(TtExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("timertest - a simple Linux kernel driver for testing the timer API");
MODULE_VERSION("1.0");

//=================================================================================================
