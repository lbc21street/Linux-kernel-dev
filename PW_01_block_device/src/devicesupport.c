//=================================================================================================
//
// \file    devicesupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/bitmap.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <linux/atomic.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/semaphore.h>

#include <linux/hdreg.h>

#include "data.h"
#include "supportmacros.h"

#define PWBD_COMPONENT_TRACE_MASK PWBD_TM_DEVICE_SUPPORT
#include "tracesupport.h"

#include "devicesupport.h"
#include "iosupport.h"
#include "queuesupport.h"
#include "sysfssupport.h"
#include "workqueuesupport.h"

//
//
//

static void PwbdpTeardownDevice(PPWBD_DEVICE Device);

//
//
//

static void PwbdpCalculateGeometryFromCapacity(PPWBD_DEVICE Device)
{
    Device->Geometry.start = 0;

    if (Device->Capacity > 63) {
        sector_t quotient;

        Device->Geometry.sectors = 63;
        quotient = (Device->Capacity + (63 - 1)) / 63;

        if (quotient > 255) {
            Device->Geometry.heads = 255;
            Device->Geometry.cylinders = (unsigned short)((quotient + (255 - 1)) / 255);
        }

        else {
            Device->Geometry.heads = (unsigned char)quotient;
            Device->Geometry.cylinders = 1;
        }
    }

    else {
        Device->Geometry.sectors = (unsigned char)Device->Capacity;
        Device->Geometry.cylinders = 1;
        Device->Geometry.heads = 1;
    }

    pr_info_tl(PWBD_TL_1,
               "Capacity %llu - heads %u cylinders %u sectors %u start %lu device 0x%px (%u)",
               Device->Capacity, Device->Geometry.heads, Device->Geometry.cylinders,
               Device->Geometry.sectors, Device->Geometry.start, Device, Device->DeviceNumber);
}

#ifdef PWBD_USE_GEOMETRY_CAPACITY

//
//
//

static void PwbdpRecalculateFullCapacityFromGeometry(PPWBD_DEVICE Device)
{
    sector_t origCapacity = Device->Capacity;

    Device->Capacity =
        Device->Geometry.heads * Device->Geometry.cylinders * Device->Geometry.sectors;

    Device->DiskSize = Device->Capacity << Device->SectorShift;

    pr_info_tl(PWBD_TL_1,
               "heads %u cylinders %u sectors %u - original Capacity %llu => full Capacity %llu "
               "DiskSize %llu device 0x%px (%u)",
               Device->Geometry.heads, Device->Geometry.cylinders, Device->Geometry.sectors,
               origCapacity, Device->Capacity, Device->DiskSize, Device, Device->DeviceNumber);
}

#endif // PWBD_USE_GEOMETRY_CAPACITY

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
        pr_info_tl(PWBD_TL_1, "registered block device %d", result);

        PwbdCtrl.DeviceMajor = result;
        SetFlag(PwbdCtrl.Flags, PWBD_CTLFL_DEVICE_REGISTERED);

        result = 0;
    }

    else {
        pr_err_tl(PWBD_TL_1, "register_blkdev() failed %d", result);
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

    pr_info_tl(PWBD_TL_1, "unregistering block device %d", PwbdCtrl.DeviceMajor);

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
        pr_err_tl(PWBD_TL_1, "vmalloc() failed (%llu bytes) device 0x%px (%u)", Device->DiskSize,
                  Device, Device->DeviceNumber);

        return -ENOMEM;
    }

    pr_info_tl(PWBD_TL_1, "allocated disk data 0x%px %llu bytes device 0x%px (%u)",
               Device->DiskData, Device->DiskSize, Device, Device->DeviceNumber);

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
        pr_info_tl(PWBD_TL_1, "freeing disk data 0x%px device 0x%px (%u)", Device->DiskData, Device,
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

            pr_err_tl(PWBD_TL_1, "blk_mq_alloc_disk() failed %d device 0x%px (%u)", result, Device,
                      Device->DeviceNumber);

            break;
        }
#else  // PWBD_USE_MQ
        Device->Disk = blk_alloc_disk(NUMA_NO_NODE);

        if (IS_ERR(Device->Disk)) {
            result = PTR_ERR(Device->Disk);

            pr_err_tl(PWBD_TL_1, "blk_alloc_disk() failed %d device 0x%px (%u)", result, Device,
                      Device->DeviceNumber);

            break;
        }
#endif // PWBD_USE_MQ

        pr_info_tl(PWBD_TL_1, "allocated disk 0x%px device 0x%px (%u)", Device->Disk, Device,
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

        SetFlag(Device->Disk->events, DISK_EVENT_MEDIA_CHANGE);

        //
        // forward events to udev
        //

        SetFlag(Device->Disk->event_flags, DISK_EVENT_FLAG_UEVENT);

        //
        // name of major driver, 32 symbols max
        //

        snprintf(Device->Disk->disk_name, sizeof(Device->Disk->disk_name), "%s%u", PWBD_DEVICE_NAME,
                 Device->DeviceNumber);

        //
        // sets disk capacity and notifies if the size is not currently zero and will not be set to
        // zero; returns true if a uevent was sent, otherwise false
        //
        // disk size in sectors
        //

        set_capacity_and_notify(Device->Disk, Device->Capacity);

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

        // blk_queue_flag_set(QUEUE_FLAG_SYNCHRONOUS, Device->Disk->queue);

        //
        // device supports NOWAIT
        //

        // blk_queue_flag_set(QUEUE_FLAG_NOWAIT, Device->Disk->queue);

        //
        // disable merge attempts
        //

        // blk_queue_flag_set(QUEUE_FLAG_NOMERGES, Device->Disk->queue);

        blk_queue_max_hw_sectors(Device->Disk->queue, BLK_DEF_MAX_SECTORS);

        pr_info_tl(PWBD_TL_2, "adding disk <%s> device 0x%px (%u)", Device->Disk->disk_name, Device,
                   Device->DeviceNumber);

        result = add_disk(Device->Disk);

        if (result != 0) {
            pr_err_tl(PWBD_TL_1, "add_disk() failed %d device 0x%px (%u)", result, Device,
                      Device->DeviceNumber);

            break;
        }

        SetFlag(Device->Flags, PWBD_DEVFL_DISK_ADDED);

        pr_info_tl(PWBD_TL_1, "added disk 0x%px device 0x%px (%u)", Device->Disk, Device,
                   Device->DeviceNumber);

        disk_force_media_change(Device->Disk, DISK_EVENT_MEDIA_CHANGE);

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
            disk_force_media_change(Device->Disk, DISK_EVENT_MEDIA_CHANGE);

            pr_info_tl(PWBD_TL_1, "deleting disk 0x%px device 0x%px (%u)", Device->Disk, Device,
                       Device->DeviceNumber);

            del_gendisk(Device->Disk);
            ClearFlag(Device->Flags, PWBD_DEVFL_DISK_ADDED);
        }

        pr_info_tl(PWBD_TL_1, "dereferencing disk 0x%px device 0x%px (%u)", Device->Disk, Device,
                   Device->DeviceNumber);

        put_disk(Device->Disk);
        Device->Disk = NULL;
    }

    PwbdpFreeDiskData(Device);
}

//
//
//

static void PwbdpInitDeviceParameters(PPWBD_DEVICE Device, uint32_t DeviceNumber)
{
    Device->DeviceNumber = DeviceNumber;

    //
    // [TODO]
    //
    // they can be parameterized for different disk devices
    // for now using global data for all devices
    //

    Device->SectorSize = PwbdCtrl.SectorSize;
    Device->DiskSize = PwbdCtrl.DiskSize;
#ifndef PWBD_USE_GEOMETRY_CAPACITY
    Device->DiskSize += PWBD_RESERVED_DISK_SIZE;
#endif // PWBD_USE_GEOMETRY_CAPACITY

    Device->SectorShift = ilog2(Device->SectorSize);
    Device->SectorsPerPage = (1 << (PAGE_SHIFT - Device->SectorShift));
    Device->Capacity = Device->DiskSize >> Device->SectorShift;

    PwbdpCalculateGeometryFromCapacity(Device);

#ifdef PWBD_USE_GEOMETRY_CAPACITY
    PwbdpRecalculateFullCapacityFromGeometry(Device);
#endif // PWBD_USE_GEOMETRY_CAPACITY

    pr_info_tl(
        PWBD_TL_1,
        "set params => SectorSize %u SectorShift %u SectorsPerPage %u DiskSize %llu Capacity "
        "%llu device 0x%px (%u)",
        Device->SectorSize, Device->SectorShift, Device->SectorsPerPage, Device->DiskSize,
        Device->Capacity, Device, Device->DeviceNumber);
}

//
//
//

static inline void PwbdpSetDeviceBit(uint32_t DeviceNumber)
{
    bitmap_set(PwbdCtrl.DeviceBitmap, DeviceNumber, 1);
}

//
//
//

static inline void PwbdpClearDeviceBit(uint32_t DeviceNumber)
{
    bitmap_clear(PwbdCtrl.DeviceBitmap, DeviceNumber, 1);
}

//
//
//

static inline void PwbdpSetDeviceSlot(PPWBD_DEVICE Device, uint32_t DeviceNumber)
{
    PwbdpSetDeviceBit(DeviceNumber);

    PwbdCtrl.Devices[DeviceNumber] = Device;
}

//
//
//

static inline void PwbdpClearDeviceSlot(uint32_t DeviceNumber)
{
    PwbdpClearDeviceBit(DeviceNumber);

    PwbdCtrl.Devices[DeviceNumber] = NULL;
}

//
//
//

[[nodiscard]] int PwbdFindFreeDeviceSlot(void)
{
    int index = find_next_zero_bit(PwbdCtrl.DeviceBitmap, PwbdCtrl.NumberOfDevices, 0);

    if (index == PwbdCtrl.NumberOfDevices) {
        pr_warn_tl(PWBD_TL_1, "no free device slot found (%u)", PwbdCtrl.NumberOfDevices);

        index = -EMFILE;
    }

    return index;
}

//
//
//

static void PwbdpSignalDeviceRemovalEvent(void)
{
    pr_info_tl(PWBD_TL_2, "signalling DeviceRemovalEvent");

    up(&PwbdCtrl.DeviceRemovalEvent);
}

static void PwbdpWaitForDeviceRemovalEvent(void)
{
    pr_info_tl(PWBD_TL_2, "waiting for DeviceRemovalEvent");

    down(&PwbdCtrl.DeviceRemovalEvent);

    pr_info_tl(PWBD_TL_2, "waiting for DeviceRemovalEvent complete");
}

//
//
//

static inline void PwbdpAcquireDeviceLock(void)
{
    mutex_lock(&PwbdCtrl.DeviceLock);
}

//
//
//

static inline void PwbdpReleaseDeviceLock(void)
{
    mutex_unlock(&PwbdCtrl.DeviceLock);
}

//
//
//

[[nodiscard]] static int PwbdpAddDevice(uint32_t DeviceNumber)
{
    PPWBD_DEVICE device = NULL;
    int result = 0;

    do {
        device = (PPWBD_DEVICE)kzalloc(sizeof(PWBD_DEVICE), GFP_KERNEL);

        if (device == NULL) {
            result = -ENOMEM;

            pr_err_tl(PWBD_TL_1, "memory alloc failed for device (%u bytes) DeviceNumber %u",
                      (uint32_t)sizeof(PWBD_DEVICE), DeviceNumber);

            break;
        }

        pr_info_tl(PWBD_TL_1, "allocated device 0x%px (%u)", device, DeviceNumber);

        PwbdpInitDeviceParameters(device, DeviceNumber);

#ifdef PWBD_USE_MQ
        PwbdpInitStaticMqOps(device);
        PwbdpInitStaticTagSet(device);
#endif // PWBD_USE_MQ

        PwbdInitStaticDevOps(device);

#ifdef PWBD_USE_MQ
        result = PwbdAllocateDeviceWorkQueue(device);

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

        result = PwbdCreateClassDevice(device);

        if (result != 0) {
            break;
        }

        result = PwbdCreateDeviceAttributeRemove(device);

        if (result != 0) {
            break;
        }

        PwbdpSetDeviceSlot(device, DeviceNumber);

        atomic_inc(&PwbdCtrl.DeviceCount);

    } while (FALSE);

    if (result != 0) {
        if (device) {
            PwbdpTeardownDevice(device);
        }
    }

    return result;
}

//
//
//

[[nodiscard]] int PwbdAddDevice(void)
{
    PwbdpAcquireDeviceLock();

    if (FlagOn(PwbdCtrl.Flags, PWBD_CTLFL_TEARING_DOWN)) {
        PwbdpReleaseDeviceLock();

        pr_warn_tl(PWBD_TL_1, "driver is already being torn down");

        return -ENODEV;
    }

    int result = PwbdFindFreeDeviceSlot();

    if (result < 0) {
        PwbdpReleaseDeviceLock();

        return result;
    }

    uint32_t index = result;

    pr_info_tl(PWBD_TL_1, "found free device slot %u", index);

    result = PwbdpAddDevice(index);

    PwbdpReleaseDeviceLock();

    return result;
}

//
//
//

static bool PwbdpStartingToRemoveDevice(PPWBD_DEVICE Device)
{
    PWBD_DEVICE_FLAGS flags =
        atomic_fetch_or(PWBD_DEVFL_STARTING_TO_REMOVE, (atomic_t *)&Device->Flags);

    if (!FlagOn(flags, PWBD_DEVFL_STARTING_TO_REMOVE)) {
        pr_info_tl(PWBD_TL_1, "device 0x%px (%u) has been marked for removal", Device,
                   Device->DeviceNumber);

        return TRUE;
    }

    pr_info_tl(PWBD_TL_1, "device 0x%px (%u) is already being removed", Device,
               Device->DeviceNumber);

    return FALSE;
}

//
//
//

static void PwbdpTeardownDevice(PPWBD_DEVICE Device)
{
    PwbdRemoveDeviceAttributeRemove(Device);

    PwbdDestroyClassDevice(Device);

    PwbdpDestroyDisk(Device);

#ifdef PWBD_USE_MQ
    PwbdpFreeTagSet(Device);

    PwbdDestroyDeviceWorkQueue(Device);
#endif // PWBD_USE_MQ

    pr_info_tl(PWBD_TL_1, "freeing device 0x%px", Device);

    kfree(Device);
}

//
//
//

static void PwbdpRemoveDevice(PPWBD_DEVICE Device)
{
    uint32_t index = Device->DeviceNumber;

    //
    // [NOTE]
    // [IMPORTANT]
    //
    // after this call the Device pointer will get invalid
    //

    PwbdpTeardownDevice(Device);

    PwbdpClearDeviceSlot(index);

    if (!atomic_dec_return(&PwbdCtrl.DeviceCount)) {
        PwbdpSignalDeviceRemovalEvent();
    }
}

//
//
//

static bool PwbdRemoveDevice(PPWBD_DEVICE Device)
{
    //
    // [NOTE]
    //
    // protect ourselves from concurrent device removals (this one and in the teardown code)
    //

    if (!PwbdpStartingToRemoveDevice(Device)) {
        return FALSE;
    }

    PwbdpRemoveDevice(Device);

    return TRUE;
}

//
//
//

static void PwbdpDeviceRemovalWorkerRoutine(struct work_struct *WorkItem)
{
    PPWBD_DEVICE device = container_of(WorkItem, PWBD_DEVICE, DeferredRemovalWorkItem);

    PwbdpAcquireDeviceLock();

    PwbdpRemoveDevice(device);

    PwbdpReleaseDeviceLock();
}

//
//
//

bool PwbdRemoveDeviceDeferred(PPWBD_DEVICE Device)
{
    //
    // [NOTE]
    //
    // protect ourselves from concurrent device removals (this one and in the teardown code)
    //

    if (!PwbdpStartingToRemoveDevice(Device)) {
        return FALSE;
    }

    pr_info_tl(PWBD_TL_1, "queueing device removal - device 0x%px (%u) count %u", Device,
               Device->DeviceNumber, atomic_read(&PwbdCtrl.DeviceCount));

    INIT_WORK(&Device->DeferredRemovalWorkItem, PwbdpDeviceRemovalWorkerRoutine);

    //
    // [NOTE]
    // [QUESTIONABLE]
    //
    // perhaps we should use our own workqueue, just to be able to flush or drain it
    //

    queue_work(PwbdCtrl.DeviceRemovalWorkQueue, &Device->DeferredRemovalWorkItem);

    return TRUE;
}

//
//
//

[[nodiscard]] int PwbdInitializeDevices(void)
{
    int result = 0;

    do {
        mutex_init(&PwbdCtrl.DeviceLock);

        sema_init(&PwbdCtrl.DeviceRemovalEvent, 0);

        result = PwbdpRegisterBlockDevice();

        if (result != 0) {
            break;
        }

        PwbdCtrl.DeviceBitmap = bitmap_zalloc(PwbdCtrl.NumberOfDevices, GFP_KERNEL);

        if (PwbdCtrl.DeviceBitmap == NULL) {
            result = -ENOMEM;

            pr_err_tl(PWBD_TL_1, "bitmap_zalloc() failed for device bitmap (%u bytes)",
                      (uint32_t)BITS_TO_BYTES(PwbdCtrl.NumberOfDevices));

            break;
        }

        uint32_t blockSize = PwbdCtrl.NumberOfDevices * sizeof(PPWBD_DEVICE);

        PwbdCtrl.Devices = (PPWBD_DEVICE *)kzalloc(blockSize, GFP_KERNEL);

        if (PwbdCtrl.Devices == NULL) {
            result = -ENOMEM;

            pr_err_tl(PWBD_TL_1, "memory alloc failed for device array (%u bytes)", blockSize);

            break;
        }

        int index = 0;

        PwbdpAcquireDeviceLock();

        while (index < PwbdCtrl.NumberOfDevices) {
            result = PwbdpAddDevice(index);

            if (result != 0) {
                break;
            }

            ++index;

        } // while (i < PwbdCtrl.NumberOfDevices)

        PwbdpReleaseDeviceLock();

        if (result != 0) {
            //
            // if we've failed while creating and adding devices, signal the device removal event
            // just not to get stuck on that event in teardown code
            //

            PwbdpSignalDeviceRemovalEvent();
        }

    } while (FALSE);

    return result;
}

//
//
//

void PwbdUninitializeDevices(void)
{
    PwbdpUnregisterBlockDevice();

    if (PwbdCtrl.Devices) {
        int index = 0;

        PwbdpAcquireDeviceLock();

        SetFlag(PwbdCtrl.Flags, PWBD_CTLFL_TEARING_DOWN);

        while (index < PwbdCtrl.NumberOfDevices) {
            if (PwbdCtrl.Devices[index]) {
                PwbdRemoveDevice(PwbdCtrl.Devices[index]);
            }

            ++index;

        } // while (index < PwbdCtrl.NumberOfDevices)

        PwbdpReleaseDeviceLock();

        //
        // wait for all devices to go away
        //

        PwbdpWaitForDeviceRemovalEvent();

        pr_info_tl(PWBD_TL_1, "freeing Devices 0x%px", PwbdCtrl.Devices);

        kfree(PwbdCtrl.Devices);
        PwbdCtrl.Devices = NULL;
    }

    if (PwbdCtrl.DeviceBitmap) {
        pr_info_tl(PWBD_TL_1, "freeing DeviceBitmap 0x%px", PwbdCtrl.DeviceBitmap);

        bitmap_free(PwbdCtrl.DeviceBitmap);
        PwbdCtrl.DeviceBitmap = NULL;
    }
}

//=================================================================================================
