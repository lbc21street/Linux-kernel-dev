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
#include <linux/blkdev.h>
#include <linux/cdrom.h>
#include <linux/fd.h>
#include <linux/hdreg.h>

#include "data.h"
#include "devicesupport.h"
#include "supportmacros.h"

#include "iosupport.h"

//
//
//

static int PwbdpDevOpsGetGeo(struct block_device *Bdev, struct hd_geometry *Geo);

//
//
//

static int PwbdpWriteToDevice(PPWBD_DEVICE Device, const void *Data, size_t DataLength,
                              sector_t Sector)
{
    int result = 0;

    pr_info("Data 0x%px DataLength %lu Sector %llu device 0x%px (%u)", Data, DataLength, Sector,
            Device, Device->Minor);

    uint32_t offset = (Sector & (Device->SectorsPerPage - 1)) << Device->SectorShift;

    size_t bytesToCopy = min_t(size_t, DataLength, PAGE_SIZE - offset);

    void *diskData = Add2Ptr(Device->DiskData, Sector << Device->SectorShift);

    memcpy(Add2Ptr(diskData, offset), Data, bytesToCopy);

    if (bytesToCopy < DataLength) {
        const void *tailData = Add2Ptr(Data, bytesToCopy);
        sector_t tailSector = Sector + (bytesToCopy >> Device->SectorShift);
        bytesToCopy = DataLength - bytesToCopy;

        diskData = Add2Ptr(Device->DiskData, tailSector << Device->SectorShift);

        memcpy(diskData, tailData, bytesToCopy);
    }

    return result;
}

//
//
//

static int PwbdpReadFromDevice(PPWBD_DEVICE Device, void *Data, size_t DataLength, sector_t Sector)
{
    int result = 0;

    pr_info("Data 0x%px DataLength %lu Sector %llu device 0x%px (%u)", Data, DataLength, Sector,
            Device, Device->Minor);

    uint32_t offset = (Sector & (Device->SectorsPerPage - 1)) << Device->SectorShift;

    size_t bytesToCopy = min_t(size_t, DataLength, PAGE_SIZE - offset);

    const void *diskData = Add2Ptr(Device->DiskData, Sector << Device->SectorShift);

    memcpy(Data, Add2Ptr(diskData, offset), bytesToCopy);

    if (bytesToCopy < DataLength) {
        void *tailData = Add2Ptr(Data, bytesToCopy);
        sector_t tailSector = Sector + (bytesToCopy >> Device->SectorShift);
        bytesToCopy = DataLength - bytesToCopy;

        diskData = Add2Ptr(Device->DiskData, tailSector << Device->SectorShift);

        memcpy(tailData, diskData, bytesToCopy);
    }

    return result;
}

//
//
//

[[maybe_unused]] static int PwbdpPerformIo(PPWBD_DEVICE Device, struct page *Page, uint32_t Length,
                                           uint32_t Offset, blk_opf_t OpFlags, sector_t Sector)
{
    int result = 0;
    void *address;

    address = kmap_local_page(Page);

    pr_info(
        "Page 0x%px Length %u Offset %u OpFlags 0x%08X Sector %llu address 0x%px device 0x%px (%u)",
        Page, Length, Offset, OpFlags, Sector, address, Device, Device->Minor);

    if (op_is_write(OpFlags)) {
        flush_dcache_page(Page);

        result = PwbdpWriteToDevice(Device, Add2Ptr(address, Offset), Length, Sector);
    }

    else {
        result = PwbdpReadFromDevice(Device, Add2Ptr(address, Offset), Length, Sector);

        flush_dcache_page(Page);
    }

    kunmap_local(address);

    return result;
}

//
//
//

static void PwbdpDevOpsSubmitBio(struct bio *Bio)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bio->bi_bdev->bd_disk->private_data;

    // pr_info("Bio 0x%px bi_bdev 0x%px bd_disk 0x%px sector %llu device 0x%px (%u)", Bio,
    //         Bio->bi_bdev, Bio->bi_bdev->bd_disk, sector, device, device->Minor);

    //
    // [NOTE]
    //
    // the following macro will print a stack trace if it is executed in an atomic context
    // (spinlock, irq - handler, ...). Additional sections where blocking is not allowed can be
    // annotated with non_block_start() and non_block_end() pairs
    //

    might_sleep();

    unsigned long startTime = bio_start_io_acct(Bio);

    sector_t sector = Bio->bi_iter.bi_sector;
    struct bio_vec bioVec;
    struct bvec_iter bvecIter;
    int result = 0;

    // loff_t deviceSize = device->DiskSize;
    // loff_t pos = sector << device->SectorShift;

    // bio_for_each_segment(bioVec, Bio, bvecIter)
    // {
    //     void *address = page_address(bioVec.bv_page) + bioVec.bv_offset;

    //     if ((pos + length) > deviceSize) {
    //         Bio->bi_status = BLK_STS_IOERR;
    //         break;
    //     }

    //     if (bio_data_dir(Bio)) {
    //         memcpy(Add2Ptr(device->DiskData, pos), address, length); // write
    //     }

    //     else {
    //         memcpy(address, Add2Ptr(device->DiskData, pos), length); // read
    //     }

    //     pos += length;

    // } // bio_for_each_segment

    bio_for_each_segment(bioVec, Bio, bvecIter)
    {
        uint32_t length = bioVec.bv_len;

        //
        // check for unaligned buffer
        //

        WARN_ON_ONCE((bioVec.bv_offset & (device->SectorSize - 1)) ||
                     (length & (device->SectorSize - 1)));

        result =
            PwbdpPerformIo(device, bioVec.bv_page, length, bioVec.bv_offset, Bio->bi_opf, sector);

        if (result != 0) {
            break;
        }

        sector += (length >> device->SectorShift);
    }

    bio_end_io_acct(Bio, startTime);

    if (result == 0) {
        bio_endio(Bio);
    }

    else if (FlagOn(Bio->bi_opf, REQ_NOWAIT)) {
        pr_err("calling bio_wouldblock_error() device 0x%px (%u)", device, device->Minor);

        bio_wouldblock_error(Bio);
    }

    else {
        pr_err("calling bio_io_error() device 0x%px (%u)", device, device->Minor);

        bio_io_error(Bio);
    }
}

//
//
//

static int PwbdpDevOpsOpen(struct gendisk *Disk, blk_mode_t Mode)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Disk->private_data;

    pr_info("Disk 0x%px Mode %u device 0x%px (%u)", Disk, Mode, device, device->Minor);

    return 0;
}

//
//
//

static void PwbdpDevOpsRelease(struct gendisk *Disk)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Disk->private_data;

    pr_info("Disk 0x%px device 0x%px (%u)", Disk, device, device->Minor);
}

//
//
//

static int PwbdpDevOpsIoctl(struct block_device *Bdev, blk_mode_t Mode, unsigned Cmd,
                            unsigned long Arg)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bdev->bd_disk->private_data;

    pr_info("Bdev 0x%px Mode %u Cmd 0x%08X Arg 0x%lX device 0x%px (%u)", Bdev, Mode, Cmd, Arg,
            device, device->Minor);

    int result = 0;

    switch (Cmd) {
        case FDGETFDCSTAT:
            pr_info("FDGETFDCSTAT");
            result = -EINVAL;
            break;

        case HDIO_GETGEO: {
            pr_info("HDIO_GETGEO");

            if (Arg == 0) {
                result = -EINVAL;
                break;
            }

            struct hd_geometry geometry = {0};

            PwbdpDevOpsGetGeo(Bdev, &geometry);

            if (copy_to_user((void *)Arg, &geometry, sizeof(geometry))) {
                result = -EFAULT;
            }

            break;
        }

        case CDROM_GET_CAPABILITY:
            pr_info("CDROM_GET_CAPABILITY");
            result = -EINVAL;
            break;

        case CDROM_LAST_WRITTEN:
            pr_info("CDROM_LAST_WRITTEN");
            result = -EINVAL;
            break;

        case BLKRRPART:
            pr_info("BLKRRPART");
            result = -EINVAL;
            break;

        case BLKGETSIZE:
            pr_info("BLKGETSIZE");

            result = put_user(device->Capacity,
                              (uint64_t __user *)Arg); // put_ulong(Arg, ctrl->Capacity);
            break;

        case BLKGETSIZE64:
            pr_info("BLKGETSIZE64");

            result = put_user(
                device->DiskSize,
                (uint64_t __user *)Arg); // put_u64(Arg, ctrl->Capacity << PWBD_SECTOR_SHIFT);
            break;

        case BLKSSZGET: // get block device logical block size
            pr_info("BLKSSZGET");

            result =
                put_user(device->SectorSize, (int __user *)Arg); // put_int(Arg, PWBD_SECTOR_SIZE);
            break;

        case BLKPBSZGET: // get block device physical block size
            pr_info("BLKPBSZGET");

            result = put_user(device->SectorSize,
                              (uint32_t __user *)Arg); // put_uint(Arg, PWBD_SECTOR_SIZE);
            break;

        default:
            result = -ENOTTY;
            break;

    } // switch (Cmd)

    return result;
}

//
//
//

static int PwbdpDevOpsGetGeo(struct block_device *Bdev, struct hd_geometry *Geo)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bdev->bd_disk->private_data;

    pr_info("Bdev 0x%px Geo 0x%px device 0x%px (%u)", Bdev, Geo, device, device->Minor);

    Geo->start = 0;

    if (device->Capacity > 63) {
        sector_t quotient;

        Geo->sectors = 63;
        quotient = (device->Capacity + (63 - 1)) / 63;

        if (quotient > 255) {
            Geo->heads = 255;
            Geo->cylinders = (unsigned short)((quotient + (255 - 1)) / 255);
        }

        else {
            Geo->heads = (unsigned char)quotient;
            Geo->cylinders = 1;
        }
    }

    else {
        Geo->sectors = (unsigned char)device->Capacity;
        Geo->cylinders = 1;
        Geo->heads = 1;
    }

    pr_info("heads %u cylinders %u sectors %u start %lu device 0x%px (%u)", Geo->heads,
            Geo->cylinders, Geo->sectors, Geo->start, device, device->Minor);

    return 0;
}

//
//
//

static int PwbdpDevOpsSetReadOnly(struct block_device *Bdev, bool Ro)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Bdev->bd_disk->private_data;

    pr_info("Bdev 0x%px Ro %u device 0x%px (%u)", Bdev, Ro, device, device->Minor);

    return -EINVAL;
}

//
//
//

static void PwbdpDevOpsFreeDisk(struct gendisk *Disk)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Disk->private_data;

    pr_info("Disk 0x%px device 0x%px (%u)", Disk, device, device->Minor);
}

//
//
//

static int PwbdpDevOpsGetUniqueId(struct gendisk *Disk, u8 Id[16], enum blk_unique_id IdType)
{
    PPWBD_DEVICE device = (PPWBD_DEVICE)Disk->private_data;

    pr_info("Disk 0x%px id_type %u device 0x%px (%u)", Disk, IdType, device, device->Minor);

    return -EINVAL;
}

//
//
//

void PwbdpInitStaticDevOps(PPWBD_DEVICE Device)
{
    PwbdCtrl.DevOps.submit_bio = PwbdpDevOpsSubmitBio;
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
