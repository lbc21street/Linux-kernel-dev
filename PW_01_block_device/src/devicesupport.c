//=================================================================================================
//
// \file    devicesupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#define PWBD_USE_MQ

#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <linux/blk-mq.h>
#include <linux/blkdev.h>

#include "data.h"
#include "supportmacros.h"

#include "devicesupport.h"
#include "iosupport.h"
#include "queuesupport.h"
#include "workqueuesupport.h"

//
//
//

[[nodiscard]] static int PwbdpRegisterBlockDevice(void)
{
    //
    // - if a major device number was requested in range [1..BLKDEV_MAJOR_MAX-1] then the
    //   function returns zero on success, or a negative error code
    // - if any unused major number was requested with @major = 0 parameter then the return
    //   value is the allocated major number in range [1..BLKDEV_MAJOR_MAX-1] or a negative
    //   error code otherwise
    //

    int result = register_blkdev(PwbdCtrl.DeviceMajor, PWBD_DRIVER_NAME);

    if (result > 0) {
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

static void PwbdpUnregisterBlockDevice(void)
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

[[nodiscard]] static int PwbdpAllocateDiskData(PPWBD_DEVICE Device)
{
    Device->DiskData = vmalloc(Device->DiskSize);

    if (Device->DiskData == NULL) {
        pr_err("__vmalloc() failed (%llu bytes) device 0x%px (%u)", Device->DiskSize, Device,
               Device->DeviceNumber);

        return -ENOMEM;
    }

    pr_info("allocated disk data 0x%px %llu bytes device 0x%px (%u)", Device->DiskData,
            Device->DiskSize, Device, Device->DeviceNumber);

    //
    // zero first 10 sectors
    //

    memset(Device->DiskData, 0, 10 * Device->SectorSize);

    return 0;
}

//
//
//

static void PwbdpFreeDiskData(PPWBD_DEVICE Device)
{
    if (Device->DiskData) {
        pr_info("freeing disk data 0x%px device 0x%px (%u)", Device->DiskData, Device,
                Device->DeviceNumber);

        vfree(Device->DiskData);
        Device->DiskData = NULL;
    }
}

//
//
//

[[nodiscard]] static int PwbdpCreateDisk(PPWBD_DEVICE Device)
{
    int result;

    do {
        result = PwbdpAllocateDiskData(Device);

        if (result != 0) {
            break;
        }

#ifdef PWBD_USE_MQ
        Device->Disk = blk_mq_alloc_disk(&Device->TagSet, Device);

        if (IS_ERR(Device->Disk)) {
            result = PTR_ERR(Device->Disk);

            pr_err("blk_mq_alloc_disk() failed %d device 0x%px (%u)", result, Device,
                   Device->DeviceNumber);

            break;
        }
#else  // PWBD_USE_MQ
        Device->Disk = blk_alloc_disk(NUMA_NO_NODE);

        if (IS_ERR(Device->Disk)) {
            result = PTR_ERR(Device->Disk);

            pr_err("blk_alloc_disk() failed %d device 0x%px (%u)", result, Device,
                   Device->DeviceNumber);

            break;
        }
#endif // PWBD_USE_MQ

        pr_info("allocated disk 0x%px device 0x%px (%u)", Device->Disk, Device,
                Device->DeviceNumber);

        Device->Capacity = Device->DiskSize >> Device->SectorShift;

        //
        // [NOTE]
        //
        // it seems strange, but there's a note in blkdev.h:
        //
        // major/first_minor/minors should not be set by any new driver, the block core will take
        // care of allocating them automatically
        //

        //
        // plus one minor (partition) for the whole disk, for example:
        //
        // NAME      MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
        // pwbd0     251:0    0  304M  0 disk
        // ├─pwbd0p1 251:1    0  100M  0 part
        // ├─pwbd0p2 251:2    0  100M  0 part
        // ├─pwbd0p3 251:3    0  100M  0 part
        // └─pwbd0p4 251:4    0    2M  0 part
        // pwbd1     251:5    0  304M  0 disk
        // ├─pwbd1p1 251:6    0  200M  0 part
        // └─pwbd1p2 251:7    0  102M  0 part
        //

        Device->Disk->major = PwbdCtrl.DeviceMajor;
        Device->Disk->first_minor = Device->DeviceNumber * (1 + PwbdCtrl.NumberOfPartitions);
        Device->Disk->minors = 1 + PwbdCtrl.NumberOfPartitions;

        Device->Disk->fops = &PwbdCtrl.DevOps;
        Device->Disk->private_data = Device;

        //
        // to check
        //

        //
        // indicates that the block device gives access to removable media;
        // when set, the device remains present even when media is not inserted;
        // shall not be set for devices which are removed entirely when the media is removed
        //

        // SetFlag(Device->Disk->flags, GENHD_FL_REMOVABLE);

        //
        // should also check for DISK_EVENT_EJECT_REQUEST
        //

        // Device->Disk->events = DISK_EVENT_MEDIA_CHANGE;

        //
        // forward events to udev
        //

        // Device->Disk->event_flags = DISK_EVENT_FLAG_UEVENT;

        //
        // name of major driver, 32 symbols max
        //

        snprintf(Device->Disk->disk_name, sizeof(Device->Disk->disk_name), "%s%u", PWBD_DEVICE_NAME,
                 Device->DeviceNumber);

        //
        // disk size in sectors
        //

        set_capacity(Device->Disk, Device->Capacity);

        //
        // should be researched to use other sector sizes
        //

        blk_queue_physical_block_size(Device->Disk->queue, Device->SectorSize);
        blk_queue_logical_block_size(Device->Disk->queue, Device->SectorSize);
        blk_queue_io_min(Device->Disk->queue, Device->SectorSize);
        // blk_queue_io_opt(Device->Disk->queue, Device->SectorSize);

        //
        // non-rotational device (SSD)
        //

        blk_queue_flag_set(QUEUE_FLAG_NONROT, Device->Disk->queue);

        //
        // always completes in submit context
        //

        blk_queue_flag_set(QUEUE_FLAG_SYNCHRONOUS, Device->Disk->queue);

        //
        // device supports NOWAIT
        //

        // blk_queue_flag_set(QUEUE_FLAG_NOWAIT, Device->Disk->queue);

        //
        // disable merge attempts
        //

        // blk_queue_flag_set(QUEUE_FLAG_NOMERGES, Device->Disk->queue);

        blk_queue_max_hw_sectors(Device->Disk->queue, BLK_DEF_MAX_SECTORS_CAP);

        pr_info("adding disk <%s> device 0x%px (%u)", Device->Disk->disk_name, Device,
                Device->DeviceNumber);

        result = add_disk(Device->Disk);

        if (result != 0) {
            pr_err("add_disk() failed %d device 0x%px (%u)", result, Device, Device->DeviceNumber);

            break;
        }

        SetFlag(Device->Flags, PWBD_DEVFL_DISK_ADDED);

        pr_info("added disk 0x%px device 0x%px (%u)", Device->Disk, Device, Device->DeviceNumber);

    } while (FALSE);

    return result;
}

//
//
//

static void PwbdpDestroyDisk(PPWBD_DEVICE Device)
{
    if (Device->Disk) {
        if (FlagOn(Device->Flags, PWBD_DEVFL_DISK_ADDED)) {
            pr_info("deleting disk 0x%px device 0x%px (%u)", Device->Disk, Device,
                    Device->DeviceNumber);

            del_gendisk(Device->Disk);
            ClearFlag(Device->Flags, PWBD_DEVFL_DISK_ADDED);
        }

        pr_info("dereferencing disk 0x%px device 0x%px (%u)", Device->Disk, Device,
                Device->DeviceNumber);

        put_disk(Device->Disk);
        Device->Disk = NULL;
    }

    PwbdpFreeDiskData(Device);
}

//
//
//

static void PwbdpInitDeviceParameters(PPWBD_DEVICE Device, uint32_t Minor)
{
    Device->DeviceNumber = Minor;

    //
    // [TODO]
    //
    // they can be parameterized for different disk devices
    // for now using global data for all devices
    //

    Device->SectorSize = PwbdCtrl.SectorSize;
    Device->DiskSize = PwbdCtrl.DiskSize;

    Device->SectorShift = ilog2(Device->SectorSize);
    Device->SectorsPerPage = (1 << (PAGE_SHIFT - Device->SectorShift));
    Device->Capacity = Device->DiskSize >> Device->SectorShift;

    pr_info("set params => device 0x%px (%u) SectorSize %u SectorShift %u SectorsPerPage %u "
            "DiskSize %llu Capacity %llu",
            Device, Minor, Device->SectorSize, Device->SectorShift, Device->SectorsPerPage,
            Device->DiskSize, Device->Capacity);
}

//
//
//

[[nodiscard]] static PPWBD_DEVICE PwbdpAddDevice(uint32_t Minor)
{
    PPWBD_DEVICE device = NULL;
    int result = 0;

    do {
        device = (PPWBD_DEVICE)kzalloc(sizeof(PWBD_DEVICE), GFP_KERNEL);

        if (device == NULL) {
            result = -ENOMEM;

            pr_err("memory alloc failed for device (%u bytes) minor %u",
                   (uint32_t)sizeof(PWBD_DEVICE), Minor);

            break;
        }

        pr_info("allocated device 0x%px (%u)", device, Minor);

        PwbdpInitDeviceParameters(device, Minor);

#ifdef PWBD_USE_MQ
        PwbdpInitStaticMqOps(device);
        PwbdpInitStaticTagSet(device);
#endif // PWBD_USE_MQ

        PwbdpInitStaticDevOps(device);

#ifdef PWBD_USE_MQ
        result = PwbdpAllocateWorkQueue(device);

        if (result != 0) {
            break;
        }

        result = PwbdpAllocateTagSet(device);

        if (result != 0) {
            break;
        }
#endif // PWBD_USE_MQ

        result = PwbdpCreateDisk(device);

        if (result != 0) {
            break;
        }

        PwbdCtrl.Devices[Minor] = device;

    } while (FALSE);

    if (result != 0) {
        device = ERR_PTR(result);
    }

    return device;
}

//
//
//

static void PwbdpRemoveDevice(PPWBD_DEVICE Device)
{
    PwbdpDestroyDisk(Device);

#ifdef PWBD_USE_MQ
    PwbdpFreeTagSet(Device);

    PwbdpDestroyWorkQueue(Device);
#endif // PWBD_USE_MQ
}

//
//
//

[[nodiscard]] int PwbdAddDevices(void)
{
    int result = 0;

    do {
        result = PwbdpRegisterBlockDevice();

        if (result != 0) {
            break;
        }

        uint32_t blockSize = PwbdCtrl.NumberOfDevices * sizeof(PPWBD_DEVICE);

        PwbdCtrl.Devices = (PPWBD_DEVICE *)kzalloc(blockSize, GFP_KERNEL);

        if (PwbdCtrl.Devices == NULL) {
            result = -ENOMEM;

            pr_err("memory alloc failed for device array (%u bytes)", blockSize);

            break;
        }

        int index = 0;

        while (index < PwbdCtrl.NumberOfDevices) {
            PPWBD_DEVICE device = PwbdpAddDevice(index);

            if (IS_ERR(device)) {
                result = PTR_ERR(device);

                break;
            }

            ++index;

        } // while (i < PwbdCtrl.NumberOfDevices)

    } while (FALSE);

    return result;
}

//
//
//

void PwbdRemoveDevices(void)
{
    PwbdpUnregisterBlockDevice();

    if (PwbdCtrl.Devices == NULL) {
        return;
    }

    int index = 0;

    while (index < PwbdCtrl.NumberOfDevices) {
        if (PwbdCtrl.Devices[index]) {
            PwbdpRemoveDevice(PwbdCtrl.Devices[index]);
            PwbdCtrl.Devices[index] = NULL;
        }

        ++index;

    } // while (index < PwbdCtrl.NumberOfDevices)
}

//=================================================================================================
