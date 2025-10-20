//=================================================================================================
//
// \file    queuesupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/ctype.h>
#include <linux/kernel.h>

#include <linux/blk-mq.h>
#include <linux/blkdev.h>

#include "data.h"
#include "supportmacros.h"

#include "devicesupport.h"
#include "queuesupport.h"

#ifdef PWBD_USE_MQ

//
//
//

[[nodiscard]] int PwbdpAllocateTagSet(PPWBD_DEVICE Device)
{
    // int result = blk_mq_alloc_tag_set(&Device->TagSet);

    //
    // allocates and initializes a tagset for a simple single-queue device,
    // otherwise use blk_mq_alloc_tag_set()
    //

    int result = blk_mq_alloc_sq_tag_set(&Device->TagSet, &PwbdCtrl.MqOps, PWBD_DEFAULT_QUEUE_DEPTH,
                                         BLK_MQ_F_SHOULD_MERGE);

    if (result == 0) {
        SetFlag(Device->Flags, PWBD_DEVFL_TAG_SET_ALLOCATED);

        pr_info("allocated tag set @ 0x%px, queue_depth %u device 0x%px (%u)", &Device->TagSet,
                Device->TagSet.queue_depth, Device, Device->Minor);
    }

    else {
        pr_err("blk_mq_alloc_tag_set() failed %d device 0x%px (%u)", result, Device, Device->Minor);
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

    pr_info("freeing tag set @ 0x%px device 0x%px (%u)", &Device->TagSet, Device, Device->Minor);

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
    struct request *req = Data->rq;
    unsigned long start = blk_rq_pos(req) << 9;
    unsigned long length = blk_rq_cur_bytes(req);

    pr_info("Context 0x%px Data 0x%px req 0x%px start 0x%lX length %lu", Context, Data, req, start,
            length);

    blk_mq_start_request(req);

    blk_mq_end_request(req, BLK_STS_OK);

    return BLK_STS_OK;
}

//
//
//

void PwbdpInitStaticTagSet(PPWBD_DEVICE Device)
{
    Device->TagSet.ops = &PwbdCtrl.MqOps;
    Device->TagSet.nr_hw_queues = 1;
    Device->TagSet.queue_depth = PWBD_DEFAULT_QUEUE_DEPTH;
    Device->TagSet.cmd_size =
        0; // number of additional bytes to allocate per request; driver owns these additional bytes
    Device->TagSet.numa_node = NUMA_NO_NODE;
    Device->TagSet.timeout = 0;                   // request processing timeout in jiffies
    Device->TagSet.flags = BLK_MQ_F_SHOULD_MERGE; // BLK_MQ_F_NO_SCHED_BY_DEFAULT
    Device->TagSet.driver_data = Device;
}

//
//
//

void PwbdpInitStaticMqOps(PPWBD_DEVICE Device)
{
    PwbdCtrl.MqOps.queue_rq = PwbdpQueueRequest;
}

#endif // PWBD_USE_MQ

//=================================================================================================
