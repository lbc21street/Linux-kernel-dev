//=================================================================================================
//
// \file    data.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/device/class.h>
#include <linux/types.h>

#include "devicesupport.h"
#include "supportmacros.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// how pr_xxx() works:
//
// #define pr_info(fmt, ...)       (KERN_INFO pr_fmt(fmt), ##__VA_ARGS__)
//

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

typedef enum _PWBD_CTRL_FLAGS {
    PWBD_CTLFL_DEVICE_REGISTERED = 0x00000001,
    PWBD_CTLFL_PARAMETERS_CAPTURED = 0x00000002,

} PWBD_CTRL_FLAGS;

//
//
//

typedef struct _PWBD_CTRL_FLAGS_BF {
    uint32_t FlDeviceRegistered : 1;
    uint32_t FlParametersCaptured : 1;

    uint32_t FlReserved : 30;

} PWBD_CTRL_FLAGS_BF;

static_assert(sizeof(PWBD_CTRL_FLAGS_BF) == sizeof(PWBD_CTRL_FLAGS));

//
//
//

typedef struct _PWBD_CTRL {
    unsigned DeviceMajor;

    union {
        PWBD_CTRL_FLAGS Flags;
        PWBD_CTRL_FLAGS_BF FlagsBf;
    };

    struct class *Class;

    //
    // copy of sysfs parameters
    //

    uint8_t NumberOfDevices;
    uint8_t NumberOfPartitions;
    uint16_t SectorSize;
    uint64_t DiskSize; // in bytes

    PPWBD_DEVICE *Devices;

    struct blk_mq_ops MqOps;

    struct block_device_operations DevOps;

} PWBD_CTRL, *PPWBD_CTRL;

extern PWBD_CTRL PwbdCtrl;

//
//
//

static inline bool PwbdpIsParametersCaptured(void)
{
    return BooleanFlagOn(PwbdCtrl.Flags, PWBD_CTLFL_PARAMETERS_CAPTURED);
}

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
