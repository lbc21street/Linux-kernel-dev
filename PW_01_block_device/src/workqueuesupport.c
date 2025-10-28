//=================================================================================================
//
// \file    workqueuesupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/ctype.h>
#include <linux/kernel.h>

#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/workqueue.h>

#include "data.h"
#include "supportmacros.h"

#include "devicesupport.h"
#include "iosupport.h"
#include "workqueuesupport.h"

//
//
//

[[nodiscard]] int PwbdAllocateDeviceRemovalWorkQueue(void)
{
    //
    // WQ_FREEZABLE - drain and freeze queue during system suspend
    // WQ_UNBOUND - not bound to any CPU
    // WQ_POWER_EFFICIENT - workqueues marked with WQ_POWER_EFFICIENT are per-CPU by default but
    //     become unbound if workqueue.power_efficient kernel param is specified. Per-CPU workqueues
    //     which are identified to contribute significantly to power-consumption are identified
    //     and marked with this flag and enabling the power_efficient mode leads to noticeable power
    //     saving at the cost of small performance disadvantage
    //

    uint32_t flags = WQ_FREEZABLE | WQ_UNBOUND; // WQ_POWER_EFFICIENT

    //
    // max in-flight work items per CPU, 0 for default
    //

    int maxActive = 0;

    PwbdCtrl.DeviceRemovalWorkQueue = alloc_workqueue("DeviceRemoval", flags, maxActive);

    if (PwbdCtrl.DeviceRemovalWorkQueue != NULL) {
        pr_info("allocated device removal work queue 0x%px (flags 0x%08X maxActive %u)",
                PwbdCtrl.DeviceRemovalWorkQueue, flags, maxActive);

        return 0;
    }

    pr_err("alloc_workqueue() failed");

    return -ENOMEM;
}

//
//
//

void PwbdDestroyDeviceRemovalWorkQueue(void)
{
    if (PwbdCtrl.DeviceRemovalWorkQueue == NULL) {
        return;
    }

    pr_info("draining device removal work queue 0x%px", PwbdCtrl.DeviceRemovalWorkQueue);

    drain_workqueue(PwbdCtrl.DeviceRemovalWorkQueue);

    pr_info("destroying device removal work queue 0x%px", PwbdCtrl.DeviceRemovalWorkQueue);

    destroy_workqueue(PwbdCtrl.DeviceRemovalWorkQueue);
    PwbdCtrl.DeviceRemovalWorkQueue = NULL;
}

//
//
//

#ifdef PWBD_USE_MQ

//
//
//

[[nodiscard]] int PwbdAllocateDeviceWorkQueue(PPWBD_DEVICE Device)
{
    //
    // WQ_FREEZABLE - drain and freeze queue during system suspend
    // WQ_UNBOUND - not bound to any CPU
    // WQ_POWER_EFFICIENT - workqueues marked with WQ_POWER_EFFICIENT are per-CPU by default but
    //     become unbound if workqueue.power_efficient kernel param is specified. Per-CPU workqueues
    //     which are identified to contribute significantly to power-consumption are identified
    //     and marked with this flag and enabling the power_efficient mode leads to noticeable power
    //     saving at the cost of small performance disadvantage
    //

    uint32_t flags = WQ_FREEZABLE | WQ_UNBOUND; // WQ_POWER_EFFICIENT

    //
    // max in-flight work items per CPU, 0 for default
    //

    int maxActive = 0;

    Device->WorkQueue =
        alloc_workqueue("%s%u", flags, maxActive, PWBD_DEVICE_NAME, Device->DeviceNumber);

    if (Device->WorkQueue != NULL) {
        pr_info("allocated work queue 0x%px (flags 0x%08X maxActive %u) device 0x%px (%u)",
                Device->WorkQueue, flags, maxActive, Device, Device->DeviceNumber);

        return 0;
    }

    pr_err("alloc_workqueue() failed device 0x%px (%u)", Device, Device->DeviceNumber);

    return -ENOMEM;
}

//
//
//

void PwbdDestroyDeviceWorkQueue(PPWBD_DEVICE Device)
{
    if (Device->WorkQueue == NULL) {
        return;
    }

    //
    // [NOTE]
    // [QUESTIONABLE]
    //
    // waits for all work items in a workqueue to be processed, ensuring the workqueue is empty
    // before the function returns; it prevents new work from starting while draining is in
    // progress, and only allowing "chain queuing" (where a work item can queue another work item)
    // to ensure forward progress
    //
    // this acts as a barrier to ensure a workqueue is empty, useful in scenarios like device
    // shutdown or module unloading
    //

    pr_info("draining work queue 0x%px device 0x%px (%u)", Device->WorkQueue, Device,
            Device->DeviceNumber);

    drain_workqueue(Device->WorkQueue);
    // flush_workqueue(Device->WorkQueue);

    pr_info("destroying work queue 0x%px device 0x%px (%u)", Device->WorkQueue, Device,
            Device->DeviceNumber);

    destroy_workqueue(Device->WorkQueue);
    Device->WorkQueue = NULL;
}

//
//
//

static void PwbdpAsyncRequestWorkerRoutine(struct work_struct *WorkItem)
{
    PPWBD_REQUEST_DATA data = container_of(WorkItem, PWBD_REQUEST_DATA, WorkItem);
    struct request *request = blk_mq_rq_from_pdu(data);
    [[maybe_unused]] PPWBD_DEVICE device = (PPWBD_DEVICE)request->q->queuedata;

    pr_info_detailed("request 0x%px device 0x%px (%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]",
                     request, device, device->DeviceNumber, preemptible(), in_atomic(), in_task(),
                     in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());

    data->Result = PwbdProcessAsyncRequest(request);

    blk_mq_complete_request(request);
}

//
//
//

bool PwbdQueueAsyncRequestWorkItem(struct request *Request)
{
    PPWBD_DEVICE device = Request->q->queuedata;
    PPWBD_REQUEST_DATA data = (PPWBD_REQUEST_DATA)blk_mq_rq_to_pdu(Request);

    INIT_WORK(&data->WorkItem, PwbdpAsyncRequestWorkerRoutine);
    data->Result = 0;

    return queue_work(device->WorkQueue, &data->WorkItem);
}

#endif // PWBD_USE_MQ

//=================================================================================================
