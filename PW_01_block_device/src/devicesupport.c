//=================================================================================================
//
// \file    devicesupport.c
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

#include "devicesupport.h"

//
//
//

[[nodiscard]] int PwbdpRegisterBlockDevice(void)
{
    int result = register_blkdev(PwbdCtrl.DeviceMajor, PWBD_DRIVER_NAME);

    if (result >= 0) {
        pr_info("registered block device %d", result);

        PwbdCtrl.DeviceMajor = result;
        SetFlag(PwbdCtrl.Flags, PWBD_CTLFL_DEVICE_REGISTERED);

        result = 0;
    }

    else {
        pr_err("register_blkdev() failed %d", result);
    }

    return result;
}

//
//
//

void PwbdpUnregisterBlockDevice(void)
{
    if (!FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_DEVICE_REGISTERED)) {
        return;
    }

    pr_info("unregistering block device %d", PwbdCtrl.DeviceMajor);

    unregister_blkdev(PwbdCtrl.DeviceMajor, PWBD_DRIVER_NAME);
    ClearFlag(PwbdCtrl.Flags, PWBD_CTLFL_DEVICE_REGISTERED);

    PwbdCtrl.DeviceMajor = 0;
}

//
//
//

int PwbdpCreateDisk(void)
{
    int result;

    do
    {
        size_t length = PWBD_DISK_SIZE;

        PwbdCtrl.DiskData = vmalloc(length);

        if (PwbdCtrl.DiskData == NULL) {
            result = -ENOMEM;

            pr_err("vmalloc() failed (%lu bytes)", length);

            break;
        }

        pr_info("allocated disk data 0x%px %lu bytes", PwbdCtrl.DiskData, length);

        memset(PwbdCtrl.DiskData, 0, 3 * PWBD_SECTOR_SIZE);

        // PwbdCtrl.Disk = blk_mq_alloc_disk(&PwbdCtrl.TagSet, &PwbdCtrl);

        // if (IS_ERR(PwbdCtrl.Disk)) {
        //     result = PTR_ERR(PwbdCtrl.Disk);
        //     pr_err("blk_mq_alloc_disk() failed %d", result);
        //     break;
        // }

        PwbdCtrl.Disk = blk_alloc_disk(NUMA_NO_NODE);

        if (IS_ERR(PwbdCtrl.Disk)) {
            result = PTR_ERR(PwbdCtrl.Disk);

            pr_err("blk_alloc_disk() failed %d", result);

            break;
        }

        pr_info("allocated disk 0x%px", PwbdCtrl.Disk);

        PwbdCtrl.Disk->major = PwbdCtrl.DeviceMajor;
        PwbdCtrl.Disk->first_minor = 0;
        PwbdCtrl.Disk->minors = PWBD_NUMBER_OF_PARTITIONS + 1;
        PwbdCtrl.Disk->fops = &PwbdCtrl.DevOps;
        PwbdCtrl.Disk->private_data = &PwbdCtrl;

        //
        // name of major driver, 32 symbols max
        //

        // snprintf(PwbdCtrl.Disk->disk_name, sizeof(PwbdCtrl.Disk->disk_name), PWBD_DEVICE_NAME);
        snprintf(PwbdCtrl.Disk->disk_name, sizeof(PwbdCtrl.Disk->disk_name), PWBD_DEVICE_NAME);

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
            pr_err("add_disk() failed %d", result);

            break;
        }

        SetFlag(PwbdCtrl.Flags, PWBD_CTLFL_DISK_ADDED);

        pr_info("added disk 0x%px", PwbdCtrl.Disk);

    } while (FALSE);

    return result;
}

//
//
//

void PwbdpDestroyDisk(void)
{
    if (PwbdCtrl.DiskData)
    {
        pr_info("freeing disk data 0x%px", PwbdCtrl.DiskData);

        vfree(PwbdCtrl.DiskData);
        PwbdCtrl.DiskData = NULL;
    }

    if (PwbdCtrl.Disk) {
        if (FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_DISK_ADDED))
        {
            pr_info("deleting disk 0x%px", PwbdCtrl.Disk);

            del_gendisk(PwbdCtrl.Disk);
            ClearFlag(PwbdCtrl.Flags, PWBD_CTLFL_DISK_ADDED);
        }

        pr_info("dereferencing disk 0x%px", PwbdCtrl.Disk);

        put_disk(PwbdCtrl.Disk);
        PwbdCtrl.Disk = NULL;
    }
}

//=================================================================================================
