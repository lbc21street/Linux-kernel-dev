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

#include <linux/blkdev.h>
#include <linux/blk-mq.h>

#include "data.h"
#include "supportmacros.h"

#include "iosupport.h"

//
//
//

static int PwbdpWriteToDevice(PPWBD_CTRL Ctrl, const void *Data, size_t DataLength, sector_t Sector)
{
    int result = 0;

    pr_info("Data 0x%px DataLength %lu Sector %llu", Data, DataLength, Sector);

    __u32 offset = (Sector & (PWBD_SECTORS_PER_PAGE - 1)) << PWBD_SECTOR_SHIFT;

    size_t bytesToCopy = min_t(size_t, DataLength, PAGE_SIZE - offset);

    void *diskData = Add2Ptr(Ctrl->DiskData, Sector << PWBD_SECTOR_SHIFT);

    memcpy(Add2Ptr(diskData, offset), Data, bytesToCopy);

    if (bytesToCopy < DataLength) {
        const void *tailData = Add2Ptr(Data, bytesToCopy);
        sector_t tailSector = Sector + (bytesToCopy >> PWBD_SECTOR_SHIFT);
        bytesToCopy = DataLength - bytesToCopy;

        diskData = Add2Ptr(Ctrl->DiskData, tailSector << PWBD_SECTOR_SHIFT);

        memcpy(diskData, tailData, bytesToCopy);
    }

    return result;
}

//
//
//

static int PwbdpReadFromDevice(PPWBD_CTRL Ctrl, void *Data, size_t DataLength, sector_t Sector)
{
    int result = 0;

    pr_info("Data 0x%px DataLength %lu Sector %llu", Data, DataLength, Sector);

    __u32 offset = (Sector & (PWBD_SECTORS_PER_PAGE - 1)) << PWBD_SECTOR_SHIFT;

    size_t bytesToCopy = min_t(size_t, DataLength, PAGE_SIZE - offset);

    const void *diskData = Add2Ptr(Ctrl->DiskData, Sector << PWBD_SECTOR_SHIFT);;

    if (diskData) {
        memcpy(Data, Add2Ptr(diskData, offset), bytesToCopy);
    }

    else {
        memset(Data, 0, bytesToCopy);
    }

    if (bytesToCopy < DataLength) {
        void *tailData = Add2Ptr(Data, bytesToCopy);
        sector_t tailSector = Sector + (bytesToCopy >> PWBD_SECTOR_SHIFT);
        bytesToCopy = DataLength - bytesToCopy;

        diskData = Add2Ptr(Ctrl->DiskData, tailSector << PWBD_SECTOR_SHIFT);

        if (diskData) {
            memcpy(tailData, diskData, bytesToCopy);
        }

        else {
            memset(tailData, 0, bytesToCopy);
        }
    }

    return result;
}

//
//
//

static int PwbdpPerformIo(PPWBD_CTRL Ctrl, struct page *Page, __u32 Length, __u32 Offset, blk_opf_t OpFlags, sector_t Sector)
{
    int result = 0;
    void *address;

    address = kmap_atomic(Page);

    pr_info("Page 0x%px Length %u Offset %u OpFlags 0x%08X Sector %llu address 0x%px",
        Page,
        Length,
        Offset,
        OpFlags,
        Sector,
        address);

    if (op_is_write(OpFlags)) {
        flush_dcache_page(Page);

        result = PwbdpWriteToDevice(Ctrl, Add2Ptr(address, Offset), Length, Sector);
    }

    else {
        result = PwbdpReadFromDevice(Ctrl, Add2Ptr(address, Offset), Length, Sector);

        flush_dcache_page(Page);
    }

    kunmap_atomic(address);

    return result;
}

//
//
//

static void PwbdpSubmitBio(struct bio *Bio)
{
    [[maybe_unused]] PPWBD_CTRL ctrl = (PPWBD_CTRL)Bio->bi_bdev->bd_disk->private_data;
    sector_t sector = Bio->bi_iter.bi_sector;
    struct bio_vec bioVec;
    struct bvec_iter bvecIter;

    // pr_info("Bio 0x%px bi_bdev 0x%px bd_disk 0x%px sector %llu", Bio, Bio->bi_bdev, Bio->bi_bdev->bd_disk, sector);

    bio_for_each_segment(bioVec, Bio, bvecIter) {
        __u32 length = bioVec.bv_len;

        //
        // check for unaligned buffer
        //

        WARN_ON_ONCE((bioVec.bv_offset & (PWBD_SECTOR_SIZE - 1)) || (length & (PWBD_SECTOR_SIZE - 1)));

        int result = PwbdpPerformIo(ctrl, bioVec.bv_page, length, bioVec.bv_offset, Bio->bi_opf, sector);

        if (result != 0) {
            if ((result == -ENOMEM) && FlagOn(Bio->bi_opf, REQ_NOWAIT)) {
                bio_wouldblock_error(Bio);
                return;
            }

            bio_io_error(Bio);
            return;
        }

        sector += (length >> PWBD_SECTOR_SHIFT);
    }

    bio_endio(Bio);
}

//
//
//

static int PwbdpOpen(struct gendisk *Disk, blk_mode_t Mode)
{
    pr_info("Disk 0x%px Mode %u", Disk, Mode);

    return 0;
}

//
//
//

static void PwbdpRelease(struct gendisk *Disk)
{
    pr_info("Disk 0x%px", Disk);
}

//
//
//

[[maybe_unused]] static int PwbdpIoctl(struct block_device *Bdev, blk_mode_t Mode, unsigned Cmd, unsigned long Arg)
{
    return 0;
}

//
//
//

void PwbdpInitStaticDevOps(void)
{
    PwbdCtrl.DevOps.submit_bio = PwbdpSubmitBio;
    PwbdCtrl.DevOps.open = PwbdpOpen;
    PwbdCtrl.DevOps.release = PwbdpRelease;
    // PwbdCtrl.DevOps.ioctl = PwbdpIoctl;
    PwbdCtrl.DevOps.owner = THIS_MODULE;
}

//=================================================================================================
