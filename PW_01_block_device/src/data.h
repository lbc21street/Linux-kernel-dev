//=================================================================================================
//
// \file    data.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//
// how pr_xxx() works:
//
// #define pr_info(fmt, ...)       (KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
//

struct Pwbd_DATA_ENTRY {
    struct list_head Links;
    __u32 Value;
};

#define Pwbd_MAX_DATA_ENTRIES           100000


//
//
//

#define PWBD_DRIVER_NAME                "pwblkdev"
#define PWBD_DEVICE_NAME                "pwbd0"

//
//
//

#define PWBD_DEFAULT_QUEUE_DEPTH        128

#define PWBD_SECTOR_SHIFT               12  // 4096 bytes
#define PWBD_SECTOR_SIZE                (1 << PWBD_SECTOR_SHIFT)

#define PWBD_NUMBER_OF_PARTITIONS       3
#define PWBD_PARTITION_SIZE             (100 * 1024 * 1024)
#define PWBD_DISK_SIZE                  (PWBD_NUMBER_OF_PARTITIONS * PWBD_PARTITION_SIZE)
#define PWBD_NUMBER_OF_DISK_SECTORS     (PWBD_DISK_SIZE >> PWBD_SECTOR_SHIFT)

static_assert(PWBD_SECTOR_SHIFT <= PAGE_SHIFT);

#define PWBD_SECTORS_PER_PAGE           (1 << (PAGE_SHIFT - PWBD_SECTOR_SHIFT))

//
// [NOTE]
//
// routines and other stuff of interest
//
// static void blk_report_disk_dead(struct gendisk *disk, bool surprise);  // not exported
// void blk_mark_disk_dead(struct gendisk *disk);
// void invalidate_disk(struct gendisk *disk);
// void put_disk(struct gendisk *disk);
// void set_disk_ro(struct gendisk *disk, bool read_only);
//
// set by the device driver based upon the capabilities of the I/O controller
//
// void blk_queue_max_hw_sectors(struct request_queue *q, unsigned int max_hw_sectors);
//
// print_dev_t(buffer, dev)
// format_dev_t(buffer, dev)
//
// #define MINORBITS	20
// #define MINORMASK	((1U << MINORBITS) - 1)
// #define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
// #define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))
// #define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))
//



//
//
//

typedef enum PWBD_CTRL_FLAGS
{
    PWBD_CTLFL_DEVICE_REGISTERED = 0x00000001,
    PWBD_CTLFL_TAG_SET_ALLOCATED = 0x00000002,
    PWBD_CTLFL_DISK_ADDED = 0x00000004,

} PWBD_CTRL_FLAGS;

typedef struct PWBD_CTRL_FLAGS_BF
{
    __u32 FlDeviceRegistered : 1;
    __u32 FlTagSetAllocated : 1;
    __u32 FlDiskAdded : 1;

    __u32 FlReserved : 30;

} PWBD_CTRL_FLAGS_BF;

//
//
//

typedef struct PWBD_CTRL
{
    unsigned DeviceMajor;

    union
    {
        PWBD_CTRL_FLAGS Flags;
        PWBD_CTRL_FLAGS_BF FlagsBf;
    };

    struct blk_mq_ops MqOps;

    struct block_device_operations DevOps;

    //
    // required by blk_mq_free_tag_set()
    //

    struct blk_mq_tag_set TagSet;

    struct gendisk *Disk;

    void *DiskData;

} PWBD_CTRL, *PPWBD_CTRL;

extern PWBD_CTRL PwbdCtrl;

//
//
//




#ifdef __cplusplus
}
#endif  // __cplusplus

//=================================================================================================
