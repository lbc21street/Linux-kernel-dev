//=================================================================================================
//
// \file    iosupport.c
// \brief
// \author  lbc21street
//
//=================================================================================================

#define pr_fmt(fmt) "[" KBUILD_MODNAME "] %s(): " fmt "\n", __func__

#include <linux/ctype.h>
#include <linux/kernel.h>

#include <linux/blk-mq.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/cdrom.h>
#include <linux/fd.h>
#include <linux/hdreg.h>
#include <linux/preempt.h>

#include "data.h"
#include "supportmacros.h"

#define PWBD_COMPONENT_TRACE_MASK PWBD_TM_IO_SUPPORT
#include "tracesupport.h"

#include "devicesupport.h"
#include "iosupport.h"

//
//
//

static int PwbdpCheckDeviceOffsetAndLength(PPWBD_DEVICE Device, loff_t Offset, uint32_t DataLength,
                                           const char *Operation)
{
    if (Offset >= Device->DiskSize) {
        pr_err_tl(PWBD_TL_1, "[%s] Offset %llu exceeds DiskSize %llu device 0x%px (%u)", Operation,
                  Offset, Device->DiskSize, Device, Device->DeviceNumber);

        return -EIO;
    }

    if ((Offset + DataLength) > Device->DiskSize) {
        pr_err_tl(PWBD_TL_1,
                  "[%s] (Offset %llu + DataLength %u) (%llu) exceeds device DiskSize %llu device "
                  "0x%px (%u)",
                  Operation, Offset, DataLength, Offset + DataLength, Device->DiskSize, Device,
                  Device->DeviceNumber);

        return -EIO;
    }

    return 0;
}

//
//
//

static int PwbdpWriteToDevice(PPWBD_DEVICE Device, const void *Data, uint32_t DataLength,
                              sector_t Sector)
{
    int result = 0;

    pr_info_tl(PWBD_TL_3, "Data 0x%px DataLength %u Sector %llu device 0x%px (%u)", Data,
               DataLength, Sector, Device, Device->DeviceNumber);

    do {
        loff_t diskOffset = Sector << Device->SectorShift;

        result = PwbdpCheckDeviceOffsetAndLength(Device, diskOffset, DataLength, "WRITE");

        if (result != 0) {
            break;
        }

        void *diskData = Add2Ptr(Device->DiskData, diskOffset);

        memcpy(diskData, Data, DataLength);

    } while (FALSE);

    return result;
}

//
//
//

static int PwbdpReadFromDevice(PPWBD_DEVICE Device, void *Data, uint32_t DataLength,
                               sector_t Sector)
{
    int result = 0;

    pr_info_tl(PWBD_TL_3, "Data 0x%px DataLength %u Sector %llu device 0x%px (%u)", Data,
               DataLength, Sector, Device, Device->DeviceNumber);

    do {
        loff_t diskOffset = Sector << Device->SectorShift;

        result = PwbdpCheckDeviceOffsetAndLength(Device, diskOffset, DataLength, "READ");

        if (result != 0) {
            break;
        }

        const void *diskData = Add2Ptr(Device->DiskData, diskOffset);

        memcpy(Data, diskData, DataLength);

    } while (FALSE);

    return result;
}

#ifdef PWBD_USE_OWN_BLK_OP_NAMES

//
//
//

const char *PwbdGetBlkOpName(blk_opf_t Operation)
{
    switch (Operation) {
        PWBD_BLK_OP_NAME(REQ_OP_READ);
        PWBD_BLK_OP_NAME(REQ_OP_WRITE);
        PWBD_BLK_OP_NAME(REQ_OP_FLUSH);
        PWBD_BLK_OP_NAME(REQ_OP_DISCARD);
        PWBD_BLK_OP_NAME(REQ_OP_SECURE_ERASE);
        PWBD_BLK_OP_NAME(REQ_OP_ZONE_APPEND);
        PWBD_BLK_OP_NAME(REQ_OP_WRITE_ZEROES);
        PWBD_BLK_OP_NAME(REQ_OP_ZONE_OPEN);
        PWBD_BLK_OP_NAME(REQ_OP_ZONE_CLOSE);
        PWBD_BLK_OP_NAME(REQ_OP_ZONE_FINISH);
        PWBD_BLK_OP_NAME(REQ_OP_ZONE_RESET);
        PWBD_BLK_OP_NAME(REQ_OP_ZONE_RESET_ALL);
        PWBD_BLK_OP_NAME(REQ_OP_DRV_IN);
        PWBD_BLK_OP_NAME(REQ_OP_DRV_OUT);
        default:
            return "UNKNOWN_OP";
    }
}

#endif // PWBD_USE_OWN_BLK_OP_NAMES

//
//
//

[[nodiscard]] static int PwbdpPerformIo(PPWBD_DEVICE Device, struct page *Page, uint32_t Length,
                                        uint32_t Offset, blk_opf_t Operation, sector_t Sector)
{
    int result = 0;
    void *address;

    address = kmap_local_page(Page);

    pr_info_tl(
        PWBD_TL_2,
        "Page 0x%px Length %u Offset %u Operation <%s> (%u) Sector %llu address 0x%px device 0x%px "
        "(%u) [P %u A %u T %u SS %lu S %lu H %lu I %u]",
        Page, Length, Offset, blk_op_str(Operation), Operation, Sector, address, Device,
        Device->DeviceNumber, preemptible(), in_atomic(), in_task(), in_serving_softirq(),
        in_softirq(), in_hardirq(), irqs_disabled());

    switch (Operation) {
        case REQ_OP_READ:
            result = PwbdpReadFromDevice(Device, Add2Ptr(address, Offset), Length, Sector);

            flush_dcache_page(Page);
            break;

        case REQ_OP_WRITE:
            flush_dcache_page(Page);

            result = PwbdpWriteToDevice(Device, Add2Ptr(address, Offset), Length, Sector);
            break;

        default:
            pr_warn_tl(PWBD_TL_2, "=> UNSUPPORTED");
            result = -ENOTSUPP;
            break;

    } // switch (Operation)

    kunmap_local(address);

    return result;
}

#ifdef PWBD_USE_MQ

//
//
//

[[nodiscard]] int PwbdProcessAsyncRequest(struct request *Request)
{
    int result = 0;
    PPWBD_DEVICE device = (PPWBD_DEVICE)Request->q->queuedata;

    //
    // [NOTE]
    //
    // the following macro will print a stack trace if it is executed in an atomic context
    // (spinlock, irq-handler, ...); additional sections where blocking is not allowed can be
    // annotated with non_block_start() and non_block_end() pairs
    //

    might_sleep();

    sector_t sector = blk_rq_pos(Request);
    struct bio_vec bioVec;
    struct req_iterator reqIter;

    rq_for_each_segment(bioVec, Request, reqIter)
    {
        uint32_t length = bioVec.bv_len;

        //
        // check for unaligned buffer and buffer length
        //

        WARN_ON_ONCE((bioVec.bv_offset & (device->SectorSize - 1)) ||
                     (length & (device->SectorSize - 1)));

        result = PwbdpPerformIo(device, bioVec.bv_page, length, bioVec.bv_offset, req_op(Request),
                                sector);

        if (result != 0) {
            break;
        }

        sector += (length >> device->SectorShift);

        //
        // voluntarily yields the CPU from the currently executing task, allowing the scheduler to
        // run another task that might be waiting in the CPU's runqueue; prevents CPU hogging and
        // improves responsiveness, plus it's a mechanism of voluntary preemption in non-preemptible
        // contexts
        //

        cond_resched();
    }

    return result;
}

#else // PWBD_USE_MQ

//
//
//

static void PwbdpDevOpsSubmitBio(struct bio *Bio)
{
    int result = 0;
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bio->bi_bdev->bd_disk->private_data;

    //
    // [NOTE]
    //
    // the following macro will print a stack trace if it is executed in an atomic context
    // (spinlock, irq-handler, ...); additional sections where blocking is not allowed can be
    // annotated with non_block_start() and non_block_end() pairs
    //

    might_sleep();

    unsigned long startTime = bio_start_io_acct(Bio);

    sector_t sector = Bio->bi_iter.bi_sector;
    struct bio_vec bioVec;
    struct bvec_iter bvecIter;

    bio_for_each_segment(bioVec, Bio, bvecIter)
    {
        uint32_t length = bioVec.bv_len;

        //
        // check for unaligned buffer and buffer length
        //

        WARN_ON_ONCE((bioVec.bv_offset & (device->SectorSize - 1)) ||
                     (length & (device->SectorSize - 1)));

        result =
            PwbdpPerformIo(device, bioVec.bv_page, length, bioVec.bv_offset, bio_op(Bio), sector);

        if (result != 0) {
            break;
        }

        sector += (length >> device->SectorShift);

        //
        // voluntarily yields the CPU from the currently executing task, allowing the scheduler to
        // run another task that might be waiting in the CPU's runqueue; prevents CPU hogging and
        // improves responsiveness, plus it's a mechanism of voluntary preemption in non-preemptible
        // contexts
        //

        cond_resched();

    } // bio_for_each_segment

    bio_end_io_acct(Bio, startTime);

    if (result != 0) {
        if (FlagOn(Bio->bi_opf, REQ_NOWAIT)) {
            pr_err_tl(PWBD_TL_1, "calling bio_wouldblock_error() device 0x%px (%u)", device,
                      device->DeviceNumber);

            bio_wouldblock_error(Bio);

            return;
        }
    }

    Bio->bi_status = errno_to_blk_status(result);

    pr_info_tl(PWBD_TL_3, "completing with %u (%s) device 0x%px (%u)", Bio->bi_status,
               PwbdGetBlkOpName(Bio->bi_status), device, device->DeviceNumber);

    bio_endio(Bio);
}

#endif // PWBD_USE_MQ

//
//
//

static int PwbdpDevOpsOpen(struct block_device *Bdev, fmode_t Mode)
{
    [[maybe_unused]] PPWBD_DEVICE device = (PPWBD_DEVICE)Bdev->bd_disk->private_data;

    pr_info_tl(PWBD_TL_3, "Bdev 0x%px Mode 0x%08X device 0x%px (%u)", Bdev, Mode, device,
               device->DeviceNumber);

    return 0;
}

//
//
//

static void PwbdpDevOpsRelease(struct gendisk *Disk, fmode_t Mode)
{
    [[maybe_unused]] PPWBD_DEVICE device = (PPWBD_DEVICE)Disk->private_data;

    pr_info_tl(PWBD_TL_3, "Disk 0x%px Mode 0x%08X openers %u device 0x%px (%u)", Disk, Mode,
               disk_openers(Disk), device, device->DeviceNumber);
}

//
//
//

static int PwbdpDevOpsGetGeo(struct block_device *Bdev, struct hd_geometry *Geometry)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bdev->bd_disk->private_data;

    pr_info_tl(PWBD_TL_2, "Bdev 0x%px Geometry 0x%px device 0x%px (%u)", Bdev, Geometry, device,
               device->DeviceNumber);

    *Geometry = device->Geometry;

    pr_info_tl(PWBD_TL_2, "heads %u cylinders %u sectors %u start %lu device 0x%px (%u)",
               Geometry->heads, Geometry->cylinders, Geometry->sectors, Geometry->start, device,
               device->DeviceNumber);

    return 0;
}

//
//
//

static const char *PwbdpGetIoctlName(uint32_t Cmd)
{
    switch (Cmd) {
        PWBD_IOCTL_NAME(FDGETFDCSTAT);
        PWBD_IOCTL_NAME(HDIO_GETGEO);
        PWBD_IOCTL_NAME(CDROM_GET_CAPABILITY);
        PWBD_IOCTL_NAME(CDROM_LAST_WRITTEN);
        PWBD_IOCTL_NAME(BLKRRPART);
        PWBD_IOCTL_NAME(BLKGETSIZE);
        PWBD_IOCTL_NAME(BLKGETSIZE64);
        PWBD_IOCTL_NAME(BLKSSZGET);
        PWBD_IOCTL_NAME(BLKPBSZGET);
        default:
            return "UNKNOWN_CMD";
    }
}

//
//
//

static int PwbdpDevOpsIoctl(struct block_device *Bdev, fmode_t Mode, unsigned Cmd,
                            unsigned long Arg)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bdev->bd_disk->private_data;

    pr_info_tl(PWBD_TL_2, "Bdev 0x%px Mode 0x%08X Cmd <%s> (0x%08X) Arg 0x%lX device 0x%px (%u)",
               Bdev, Mode, PwbdpGetIoctlName(Cmd), Cmd, Arg, device, device->DeviceNumber);

    //
    // [NOTE]
    //
    // in the Linux kernel, if an IOCTL handler receives an unknown or unsupported command number,
    // it should return an error code, the recommended error code for this situation is -ENOTTY
    // (Inappropriate ioctl for device)
    //
    // historically, -ENOIOCTLCMD was also used, especially for compat_ioctl handlers in older
    // kernels, but the current recommendation for a general unsupported operation is -ENOTTY; some
    // subsystems might also return -EINVAL (Invalid argument) or -ENOSYS (Function not implemented)
    // for historical reasons, but -ENOTTY is the most appropriate and standard return value for an
    // unknown or unsupported IOCTL command
    //

    int result = -ENOTTY;

    switch (Cmd) {
        case HDIO_GETGEO: {
            if (Arg == 0) {
                result = -EINVAL;
                break;
            }

            result = 0;

            struct hd_geometry geometry = {0};

            PwbdpDevOpsGetGeo(Bdev, &geometry);

            if (copy_to_user((void *)Arg, &geometry, sizeof(geometry))) {
                result = -EFAULT;
            }

            break;
        }

        case BLKGETSIZE:
            result = put_user(device->Capacity,
                              (uint64_t __user *)Arg); // put_ulong(Arg, ctrl->Capacity);
            break;

        case BLKGETSIZE64:
            result = put_user(
                device->DiskSize,
                (uint64_t __user *)Arg); // put_u64(Arg, ctrl->Capacity << PWBD_SECTOR_SHIFT);
            break;

        case BLKSSZGET: // get block device logical block size
            result =
                put_user(device->SectorSize, (int __user *)Arg); // put_int(Arg, PWBD_SECTOR_SIZE);
            break;

        case BLKPBSZGET: // get block device physical block size
            result = put_user(device->SectorSize,
                              (uint32_t __user *)Arg); // put_uint(Arg, PWBD_SECTOR_SIZE);
            break;

        default:
            break;

    } // switch (Cmd)

    return result;
}

//
//
//

static int PwbdpDevOpsSetReadOnly(struct block_device *Bdev, bool Ro)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bdev->bd_disk->private_data;

    pr_info_tl(PWBD_TL_2, "Bdev 0x%px Ro %u device 0x%px (%u)", Bdev, Ro, device,
               device->DeviceNumber);

    return -EINVAL;
}

//
//
//

static void PwbdpDevOpsFreeDisk(struct gendisk *Disk)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Disk->private_data;

    pr_info_tl(PWBD_TL_3, "Disk 0x%px device 0x%px (%u)", Disk, device, device->DeviceNumber);
}

//
//
//

static int PwbdpDevOpsGetUniqueId(struct gendisk *Disk, u8 Id[16], enum blk_unique_id IdType)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Disk->private_data;

    pr_info_tl(PWBD_TL_2, "Disk 0x%px id_type %u device 0x%px (%u)", Disk, IdType, device,
               device->DeviceNumber);

    return -EINVAL;
}

//
//
//

void PwbdInitStaticDevOps(PPWBD_DEVICE Device)
{
#ifndef PWBD_USE_MQ
    PwbdCtrl.DevOps.submit_bio = PwbdpDevOpsSubmitBio;
#endif // PWBD_USE_MQ
    PwbdCtrl.DevOps.open = PwbdpDevOpsOpen;
    PwbdCtrl.DevOps.release = PwbdpDevOpsRelease;
    PwbdCtrl.DevOps.ioctl = PwbdpDevOpsIoctl;
    PwbdCtrl.DevOps.getgeo = PwbdpDevOpsGetGeo;
    PwbdCtrl.DevOps.set_read_only = PwbdpDevOpsSetReadOnly;
    PwbdCtrl.DevOps.free_disk = PwbdpDevOpsFreeDisk;
    PwbdCtrl.DevOps.get_unique_id = PwbdpDevOpsGetUniqueId;
    PwbdCtrl.DevOps.owner = THIS_MODULE;
}

//=================================================================================================
