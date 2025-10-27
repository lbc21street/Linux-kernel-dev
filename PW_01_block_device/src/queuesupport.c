//=================================================================================================
//
// \file    queuesupport.c
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
#include "queuesupport.h"
#include "workqueuesupport.h"

#ifdef PWBD_USE_MQ

//
//
//

[[nodiscard]] int PwbdpAllocateTagSet(PPWBD_DEVICE Device)
{
    //
    // allocates and initializes a tagset for a simple single-queue device,
    //
    // int result = blk_mq_alloc_sq_tag_set(&Device->TagSet, &PwbdCtrl.MqOps,
    // PWBD_DEFAULT_QUEUE_DEPTH,
    //                                      BLK_MQ_F_SHOULD_MERGE);

    int result = blk_mq_alloc_tag_set(&Device->TagSet);

    if (result == 0) {
        SetFlag(Device->Flags, PWBD_DEVFL_TAG_SET_ALLOCATED);

        pr_info("allocated tag set @ 0x%px, queue_depth %u device 0x%px (%u)", &Device->TagSet,
                Device->TagSet.queue_depth, Device, Device->DeviceNumber);
    }

    else {
        pr_err("blk_mq_alloc_tag_set() failed %d device 0x%px (%u)", result, Device,
               Device->DeviceNumber);
    }

    return result;
}

//
//
//

void PwbdpFreeTagSet(PPWBD_DEVICE Device)
{
    if (!FlagOn(Device->Flags, PWBD_DEVFL_TAG_SET_ALLOCATED)) {
        return;
    }

    pr_info("freeing tag set @ 0x%px device 0x%px (%u)", &Device->TagSet, Device,
            Device->DeviceNumber);

    blk_mq_free_tag_set(&Device->TagSet);
    ClearFlag(Device->Flags, PWBD_DEVFL_TAG_SET_ALLOCATED);
    // memset(&Device->TagSet, 0, sizeof(Device->TagSet));
}

//
//
//

static blk_status_t PwbdpQueueRequest(struct blk_mq_hw_ctx *Context,
                                      const struct blk_mq_queue_data *Data)
{
    struct request *request = Data->rq;
    PPWBD_DEVICE device = request->q->queuedata;

    //
    // current sector
    //

    unsigned long start = blk_rq_pos(request) << device->SectorShift;

    //
    // bytes left in the current segment
    //

    unsigned long length = blk_rq_cur_bytes(request);

    pr_info_detailed("Context 0x%px Data 0x%px request 0x%px sync %u start 0x%lX length %lu device "
                     "0x%px (%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]",
                     Context, Data, request, rq_is_sync(request), start, length, device,
                     device->DeviceNumber, preemptible(), in_atomic(), in_task(),
                     in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());

    blk_mq_start_request(request);

    PwbdpQueueWorkItem(request);

    // blk_mq_end_request(request, BLK_STS_OK);

    return BLK_STS_OK;
}

//
//
//

static void PwbdpCompleteRequest(struct request *Request)
{
    PPWBD_DEVICE device = Request->q->queuedata;
    PPWBD_REQUEST_DATA data = blk_mq_rq_to_pdu(Request);

    pr_info_detailed("Request 0x%px data 0x%px Result %d device 0x%px (%u) [P %u A %u T %u SS %lu "
                     "S %lu H %lu I %u]",
                     Request, data, data->Result, device, device->DeviceNumber, preemptible(),
                     in_atomic(), in_task(), in_serving_softirq(), in_softirq(), in_hardirq(),
                     irqs_disabled());

    blk_status_t status = errno_to_blk_status(data->Result);

    blk_mq_end_request(Request, status);
}

#ifdef PWBD_MQ_DIAG

//
//
//

static enum blk_eh_timer_return PwbdpTimeout(struct request *Request)
{
    PPWBD_DEVICE device = Request->q->queuedata;

    pr_info("Request 0x%px device 0x%px (%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]", Request,
            device, device->DeviceNumber, preemptible(), in_atomic(), in_task(),
            in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());

    return BLK_EH_DONE;
}

//
//
//

static int PwbdpInitHctx(struct blk_mq_hw_ctx *Context, void *Data, unsigned int HctxIndex)
{
    pr_info("Data 0x%px [P %u A %u T %u SS %lu S %lu H %lu I %u]", Data, preemptible(), in_atomic(),
            in_task(), in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());

    return 0;
}

//
//
//

static void PwbdpExitHctx(struct blk_mq_hw_ctx *Context, unsigned int HctxIndex)
{
    pr_info("[P %u A %u T %u SS %lu S %lu H %lu I %u]", preemptible(), in_atomic(), in_task(),
            in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());
}

//
//
//

// static int PwbdpInitRequest(struct blk_mq_tag_set *TagSet, struct request *Request,
//                             unsigned int HctxIndex, unsigned int Node)
// {
//     PPWBD_DEVICE device = Request->q->queuedata;

//     pr_info("Request 0x%px device 0x%px (%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]", Request,
//             device, device->DeviceNumber, preemptible(), in_atomic(), in_task(),
//             in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());

//     return 0;
// }

//
//
//

// static void PwbdpExitRequest(struct blk_mq_tag_set *TagSet, struct request *Request,
//                              unsigned int HctxIndex)
// {
//     PPWBD_DEVICE device = Request->q->queuedata;

//     pr_info("Request 0x%px device 0x%px (%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]", Request,
//             device, device->DeviceNumber, preemptible(), in_atomic(), in_task(),
//             in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());
// }

//
//
//

static void PwbdpCleanupRequest(struct request *Request)
{
    PPWBD_DEVICE device = Request->q->queuedata;

    pr_info("Request 0x%px device 0x%px (%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]", Request,
            device, device->DeviceNumber, preemptible(), in_atomic(), in_task(),
            in_serving_softirq(), in_softirq(), in_hardirq(), irqs_disabled());
}

#endif // PWBD_MQ_DIAG

//
//
//

void PwbdpInitStaticTagSet(PPWBD_DEVICE Device)
{
    Device->TagSet.ops = &PwbdCtrl.MqOps;
    Device->TagSet.nr_hw_queues = 1;
    Device->TagSet.queue_depth = PWBD_DEFAULT_QUEUE_DEPTH;

    //
    // number of additional bytes to allocate per request; driver owns these additional bytes
    //

    Device->TagSet.cmd_size = sizeof(PWBD_REQUEST_DATA);
    Device->TagSet.numa_node = NUMA_NO_NODE;

    //
    // request processing timeout in jiffies
    //

    Device->TagSet.timeout = 0;
    Device->TagSet.flags = BLK_MQ_F_SHOULD_MERGE; // BLK_MQ_F_NO_SCHED_BY_DEFAULT
    Device->TagSet.driver_data = Device;
}

//
//
//

void PwbdpInitStaticMqOps(PPWBD_DEVICE Device)
{
    PwbdCtrl.MqOps.queue_rq = PwbdpQueueRequest;
    PwbdCtrl.MqOps.complete = PwbdpCompleteRequest;

#ifdef PWBD_MQ_DIAG
    //
    // for diag purposes
    //

    PwbdCtrl.MqOps.timeout = PwbdpTimeout;
    PwbdCtrl.MqOps.init_hctx = PwbdpInitHctx;
    PwbdCtrl.MqOps.exit_hctx = PwbdpExitHctx;
    // PwbdCtrl.MqOps.init_request = PwbdpInitRequest;
    // PwbdCtrl.MqOps.exit_request = PwbdpExitRequest;
    PwbdCtrl.MqOps.cleanup_rq = PwbdpCleanupRequest;
#endif // PWBD_MQ_DIAG
}

#endif // PWBD_USE_MQ

//=================================================================================================
