//=================================================================================================
//
//
//
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt
// #define pr_fmt(fmt) "[" KBUILD_MODNAME "] " fmt

#include <linux/ctype.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include "defs.h"
#include "supportmacros.h"

///////////////////////////////////////////////////////////////////////////////

static const struct kernel_param_ops PwbdParamOps = {
    .set = PwbdSetParam,
    .get = NULL,
};

char *cmd = "";

module_param_cb(cmd, &PwbdParamOps, NULL, S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(cmd, "Just a parameter placeholder for incoming commands");

///////////////////////////////////////////////////////////////////////////////


static PWBD_CTRL PwbdCtrl;

///////////////////////////////////////////////////////////////////////////////

static inline struct Pwbd_DATA_ENTRY *PwbdpAllocateDataEntry(void)
{
    __u32 dataLength = sizeof(struct Pwbd_DATA_ENTRY);

    struct Pwbd_DATA_ENTRY *entry = (struct Pwbd_DATA_ENTRY *)kmalloc(dataLength, GFP_KERNEL);

    if (entry == NULL) {
        pr_err("PwbdpAllocateDataEntry(): memory alloc failed for data entry (%u bytes)\n", dataLength);
    }

    return entry;
}

///////////////////////////////////////////////////////////////////////////////

static inline void PwbdpFreeDataEntry(struct Pwbd_DATA_ENTRY *Entry)
{
    kfree(Entry);
}

///////////////////////////////////////////////////////////////////////////////

static bool PwbdCheckCommand(const char *val, __u32 valLength, const char *cmd)
{
    __u32 length = max(valLength, strlen(cmd));

    if (!strncmp(val, cmd, length)) {
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

static int PwbdSetParam(const char *val, const struct kernel_param *kp)
{
    // if (!PwbdActive) {
    //     pr_warn("PwbdSetParam(): command processing is stopped\n");

    //     return -ENODEV;
    // }

    __u32 valLength = strlen(val);

    if (valLength && (val[valLength - 1] == ASCII_LF)) {
        --valLength;
    }

    if (valLength == 0) {
        pr_err("PwbdSetParam(): invalid input string\n");

        return -EINVAL;
    }

    if (PwbdCheckCommand(val, valLength, "test"))
    {
        return 0;
    }


    pr_err("PwbdSetParam(): invalid command <%.*s>\n", valLength, val);

    return -EINVAL;
}

///////////////////////////////////////////////////////////////////////////////

static blk_status_t PwbdpQueueRequest(struct blk_mq_hw_ctx *Context, const struct blk_mq_queue_data *Data)
{
    struct request* req = Data->rq;
    unsigned long start = blk_rq_pos(req) << 9;
    unsigned long length = blk_rq_cur_bytes(req);

    pr_info("PwbdpQueueRequest(): ctx 0x%px data 0x%px req 0x%px start 0x%lX length %lu\n",
        Context,
        Data,
        req,
        start,
        length);

    blk_mq_start_request(req);

    blk_mq_end_request(req, BLK_STS_OK);

    return BLK_STS_OK;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpSubmitBio(struct bio *Bio)
{
    pr_info("PwbdpSubmitBio(): bio 0x%px\n", Bio);

    bio_endio(Bio);
}

///////////////////////////////////////////////////////////////////////////////

[[nodiscard]] static int PwbdpRegisterBlockDevice(void)
{
    int result = register_blkdev(PwbdCtrl.DeviceMajor, PWBD_DEVICE_NAME);

    if (result >= 0) {
        pr_info("PwbdCreateBlockDevice(): registered block device %d\n", result);

        PwbdCtrl.DeviceMajor = result;
        SetFlag(PwbdCtrl.Flags, PWBD_CTLFL_DEVICE_REGISTERED);

        result = 0;
    }

    else {
        pr_err("PwbdCreateBlockDevice(): register_blkdev() failed %d\n", result);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpUnregisterBlockDevice(void)
{
    if (!FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_DEVICE_REGISTERED)) {
        return;
    }

    pr_info("PwbdpUnregisterBlockDevice(): unregistering block device %d\n", PwbdCtrl.DeviceMajor);

    unregister_blkdev(PwbdCtrl.DeviceMajor, PWBD_DEVICE_NAME);
    ClearFlag(PwbdCtrl.Flags, PWBD_CTLFL_DEVICE_REGISTERED);

    PwbdCtrl.DeviceMajor = 0;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpInitStaticMqOps(void)
{
    PwbdCtrl.MqOps.queue_rq = PwbdpQueueRequest;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpInitStaticDevOps(void)
{
    PwbdCtrl.DevOps.owner = THIS_MODULE;
    PwbdCtrl.DevOps.submit_bio = PwbdpSubmitBio;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpInitStaticTagSet(void)
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

///////////////////////////////////////////////////////////////////////////////

static int PwbdpAllocateTagSet(void)
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
        pr_info("PwbdpAllocateTagSet(): allocated tag set @ 0x%px, queue_depth %u\n",
            &PwbdCtrl.TagSet,
            PwbdCtrl.TagSet.queue_depth);
    }

    else {
        pr_err("PwbdpAllocateTagSet(): blk_mq_alloc_tag_set() failed %d\n", result);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpFreeTagSet(void)
{
    if (!FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_TAG_SET_ALLOCATED)) {
        return;
    }

    pr_info("PwbdpFreeTagSet(): freeing tag set @ 0x%px\n", &PwbdCtrl.TagSet);

    blk_mq_free_tag_set(&PwbdCtrl.TagSet);
    ClearFlag(PwbdCtrl.Flags, PWBD_CTLFL_TAG_SET_ALLOCATED);
    // memset(&PwbdCtrl.TagSet, 0, sizeof(PwbdCtrl.TagSet));
}

///////////////////////////////////////////////////////////////////////////////

static int PwbdpCreateDisk(void)
{
    int result;

    do
    {
        PwbdCtrl.Disk = blk_mq_alloc_disk(&PwbdCtrl.TagSet, &PwbdCtrl);

        if (IS_ERR(PwbdCtrl.Disk)) {
            result = PTR_ERR(PwbdCtrl.Disk);
            pr_err("PwbdpAllocateDisk(): blk_mq_alloc_disk() failed %d\n", result);
            break;
        }

        pr_info("PwbdpAllocateDisk(): allocated disk 0x%px\n", PwbdCtrl.Disk);

        PwbdCtrl.Disk->major = PwbdCtrl.DeviceMajor;
        PwbdCtrl.Disk->first_minor = 0;
        PwbdCtrl.Disk->minors = PWBD_NUMBER_OF_PARTITIONS + 1;
        PwbdCtrl.Disk->fops = &PwbdCtrl.DevOps;
        PwbdCtrl.Disk->private_data = &PwbdCtrl;

        //
        // name of major driver, 32 symbols max
        //

        // snprintf(PwbdCtrl.Disk->disk_name, sizeof(PwbdCtrl.Disk->disk_name), PWBD_DEVICE_NAME);
        snprintf(PwbdCtrl.Disk->disk_name, sizeof(PwbdCtrl.Disk->disk_name), "pwbd0");

        //
        // size in sectors
        //

        set_capacity(PwbdCtrl.Disk, PWBD_NUMBER_OF_DISK_SECTORS);

        //
        // should be researched
        //

        // blk_queue_physical_block_size(PwbdCtrl.Disk->queue, PWBD_SECTOR_SIZE);

        blk_queue_logical_block_size(PwbdCtrl.Disk->queue, PWBD_SECTOR_SIZE);

        blk_queue_flag_set(QUEUE_FLAG_NONROT, PwbdCtrl.Disk->queue);  // non-rotational device (SSD)
        blk_queue_flag_set(QUEUE_FLAG_SYNCHRONOUS, PwbdCtrl.Disk->queue);  // always completes in submit context
        // blk_queue_flag_set(QUEUE_FLAG_NOWAIT, PwbdCtrl.Disk->queue);  // device supports NOWAIT

        result = add_disk(PwbdCtrl.Disk);

        if (result != 0) {
            pr_err("PwbdpAllocateDisk(): add_disk() failed %d\n", result);
            break;
        }

        pr_info("PwbdpAllocateDisk(): added disk 0x%px\n", PwbdCtrl.Disk);

    } while (FALSE);

    return result;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpDestroyDisk(void)
{
    if (PwbdCtrl.Disk == NULL) {
        return;
    }

    if (FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_DISK_ADDED))
    {
        pr_info("PwbdpDestroyDisk(): deleting disk 0x%px\n", PwbdCtrl.Disk);

        del_gendisk(PwbdCtrl.Disk);
        ClearFlag(PwbdCtrl.Flags, PWBD_CTLFL_DISK_ADDED);
    }

    pr_info("PwbdpDestroyDisk(): dereferencing disk 0x%px\n", PwbdCtrl.Disk);

    put_disk(PwbdCtrl.Disk);
    PwbdCtrl.Disk = NULL;
}

///////////////////////////////////////////////////////////////////////////////

static void PwbdpTeardown(void)
{
    PwbdpUnregisterBlockDevice();

    PwbdpDestroyDisk();

    PwbdpFreeTagSet();
}

///////////////////////////////////////////////////////////////////////////////

static int __init PwbdInit(void)
{
    int result = 0;

    pr_info("PwbdInit(): entering\n");

    do
    {
        result = PwbdpRegisterBlockDevice();

        if (result != 0) {
            break;
        }

        PwbdpInitStaticMqOps();
        PwbdpInitStaticTagSet();
        PwbdpInitStaticDevOps();

        result = PwbdpAllocateTagSet();

        if (result != 0) {
            break;
        }

        result = PwbdpCreateDisk();

        if (result != 0) {
            break;
        }

    } while (FALSE);

    if (result != 0) {
        PwbdpTeardown();
    }

    pr_info("PwbdInit(): leaving, result %d\n", result);

    return result;
}

///////////////////////////////////////////////////////////////////////////////

static void __exit PwbdExit(void)
{
    pr_info("PwbdExit(): entering\n");

    PwbdpTeardown();

    pr_info("PwbdExit(): leaving\n");
}

///////////////////////////////////////////////////////////////////////////////

module_init(PwbdInit);
module_exit(PwbdExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lbc21street");
MODULE_DESCRIPTION("pwblkdev test block device driver for the Linux kernel");

//=================================================================================================
