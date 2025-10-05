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

#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include "data.h"
#include "supportmacros.h"

#include "queuesupport.h"

//
//
//

int PwbdpAllocateTagSet(void)
{
    // int result = blk_mq_alloc_tag_set(&PwbdCtrl.TagSet);

    //
    // allocates and initializes a tagset for a simple single-queue device,
    // otherwise use blk_mq_alloc_tag_set()
    //

    int result = blk_mq_alloc_sq_tag_set(&PwbdCtrl.TagSet,
        &PwbdCtrl.MqOps,
        PWBD_DEFAULT_QUEUE_DEPTH,
        BLK_MQ_F_SHOULD_MERGE);

    if (result == 0) {
        pr_info("allocated tag set @ 0x%px, queue_depth %u",
            &PwbdCtrl.TagSet,
            PwbdCtrl.TagSet.queue_depth);
    }

    else {
        pr_err("blk_mq_alloc_tag_set() failed %d", result);
    }

    return result;
}

//
//
//

void PwbdpFreeTagSet(void)
{
    if (!FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_TAG_SET_ALLOCATED)) {
        return;
    }

    pr_info("freeing tag set @ 0x%px", &PwbdCtrl.TagSet);

    blk_mq_free_tag_set(&PwbdCtrl.TagSet);
    ClearFlag(PwbdCtrl.Flags, PWBD_CTLFL_TAG_SET_ALLOCATED);
    // memset(&PwbdCtrl.TagSet, 0, sizeof(PwbdCtrl.TagSet));
}

//
//
//

static blk_status_t PwbdpQueueRequest(struct blk_mq_hw_ctx *Context, const struct blk_mq_queue_data *Data)
{
    struct request* req = Data->rq;
    unsigned long start = blk_rq_pos(req) << 9;
    unsigned long length = blk_rq_cur_bytes(req);

    pr_info("Context 0x%px Data 0x%px req 0x%px start 0x%lX length %lu",
        Context,
        Data,
        req,
        start,
        length);

    blk_mq_start_request(req);

    blk_mq_end_request(req, BLK_STS_OK);

    return BLK_STS_OK;
}

//
//
//

void PwbdpInitStaticTagSet(void)
{
    PwbdCtrl.TagSet.ops = &PwbdCtrl.MqOps;
    PwbdCtrl.TagSet.nr_hw_queues = 1;
    PwbdCtrl.TagSet.queue_depth = PWBD_DEFAULT_QUEUE_DEPTH;
    PwbdCtrl.TagSet.cmd_size = 0;  // number of additional bytes to allocate per request; driver owns these additional bytes
    PwbdCtrl.TagSet.numa_node = NUMA_NO_NODE;
    PwbdCtrl.TagSet.timeout = 0;  // request processing timeout in jiffies
    PwbdCtrl.TagSet.flags = BLK_MQ_F_SHOULD_MERGE;
    PwbdCtrl.TagSet.driver_data = &PwbdCtrl;
}

//
//
//

void PwbdpInitStaticMqOps(void)
{
    PwbdCtrl.MqOps.queue_rq = PwbdpQueueRequest;
}

//=================================================================================================
