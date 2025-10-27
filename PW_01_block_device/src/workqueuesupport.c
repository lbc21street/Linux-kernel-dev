//=================================================================================================
//
// \file    workqueuesupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#define PWBD_USE_MQ

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

#ifdef PWBD_USE_MQ

//
//
//

[[nodiscard]] int PwbdpAllocateWorkQueue(PPWBD_DEVICE Device)
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

void PwbdpDestroyWorkQueue(PPWBD_DEVICE Device)
{
    if (Device->WorkQueue == NULL) {
        return;
    }

    pr_info("destroying work queue 0x%px device 0x%px (%u)", Device->WorkQueue, Device,
            Device->DeviceNumber);

    destroy_workqueue(Device->WorkQueue);
    Device->WorkQueue = NULL;
}

//
//
//

static int PwbdpProcessAsyncRequest(struct request *Request)
{
    int result = 0;
    PPWBD_DEVICE device = (PPWBD_DEVICE)Request->q->queuedata;

    might_sleep();

    sector_t sector = blk_rq_pos(Request);
    struct bio_vec bioVec;
    struct req_iterator reqIter;

    rq_for_each_segment(bioVec, Request, reqIter)
    {
        uint32_t length = bioVec.bv_len;

        //
        // check for unaligned buffer
        //

        WARN_ON_ONCE((bioVec.bv_offset & (device->SectorSize - 1)) ||
                     (length & (device->SectorSize - 1)));

        result = PwbdpPerformAsyncIo(device, bioVec.bv_page, length, bioVec.bv_offset,
                                     req_op(Request), sector);

        if (result != 0) {
            break;
        }

        sector += (length >> device->SectorShift);
    }

    return result;
}

//
//
//

static void PwbdpWorkQueueRoutine(struct work_struct *WorkItem)
{
    PPWBD_REQUEST_DATA data = container_of(WorkItem, PWBD_REQUEST_DATA, WorkItem);
    struct request *request = blk_mq_rq_from_pdu(data);
    PPWBD_DEVICE device = (PPWBD_DEVICE)request->q->queuedata;

    pr_info("request 0x%px device 0x%px (%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]", request,
            device, device->DeviceNumber, preemptible(), in_atomic(), in_task(),
            in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());

    data->Result = PwbdpProcessAsyncRequest(request);

    blk_mq_complete_request(request);
}

//
//
//

bool PwbdpQueueWorkItem(struct request *Request)
{
    PPWBD_DEVICE device = Request->q->queuedata;
    PPWBD_REQUEST_DATA data = (PPWBD_REQUEST_DATA)blk_mq_rq_to_pdu(Request);

    INIT_WORK(&data->WorkItem, PwbdpWorkQueueRoutine);
    data->Result = 0;

    return queue_work(device->WorkQueue, &data->WorkItem);
}

#endif // PWBD_USE_MQ

//=================================================================================================
