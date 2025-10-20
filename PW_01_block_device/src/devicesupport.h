//=================================================================================================
//
// \file    devicesupport.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/blk-mq.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define PWBD_DRIVER_NAME "pwblkdev"
#define PWBD_DEVICE_NAME "pwbd"

#define PWBD_MIN_SECTOR_SHIFT 9 // 512 bytes
#define PWBD_MIN_SECTOR_SIZE (1 << PWBD_MIN_SECTOR_SHIFT)

#define PWBD_MAX_SECTOR_SHIFT PAGE_SHIFT
#define PWBD_MAX_SECTOR_SIZE (1 << PWBD_MAX_SECTOR_SHIFT)

// #define PWBD_DEFAULT_SECTOR_SHIFT               12  // 4096 bytes
#define PWBD_DEFAULT_SECTOR_SHIFT PWBD_MIN_SECTOR_SHIFT
#define PWBD_DEFAULT_SECTOR_SIZE (1 << PWBD_DEFAULT_SECTOR_SHIFT)

#define PWBD_DEFAULT_NUMBER_OF_DEVICES 1
#define PWBD_DEFAULT_NUMBER_OF_PARTITIONS 3

//
// reasonable limit
//

#define PWBD_MAX_NUMBER_OF_DEVICES 10

//
// like the default max number in a GPT table (often limited by OS)
//

#define PWBD_MAX_NUMBER_OF_PARTITIONS 128

//
// some reserved disk size (partition tables, alignment, etc.)
//

#define PWBD_RESERVED_DISK_SIZE_MB 4
#define PWBD_RESERVED_DISK_SIZE TO_MB(PWBD_RESERVED_DISK_SIZE_MB)

//
// min disk size (in bytes)
//

#define PWBD_MIN_DISK_SIZE_MB (10 + PWBD_RESERVED_DISK_SIZE_MB)
#define PWBD_MIN_DISK_SIZE TO_MB(PWBD_MIN_DISK_SIZE_MB)

//
// max disk size (in bytes)
//

#define PWBD_MAX_DISK_SIZE_MB (1024 + PWBD_RESERVED_DISK_SIZE_MB)
#define PWBD_MAX_DISK_SIZE TO_MB(PWBD_MAX_DISK_SIZE_MB)

//
//
//

#define PWBD_DEFAULT_PARTITION_SIZE_MB 100
#define PWBD_DEFAULT_PARTITION_SIZE TO_MB(PWBD_DEFAULT_PARTITION_SIZE_MB)

#define PWBD_DEFAULT_DISK_SIZE_MB                                                                  \
    (PWBD_DEFAULT_NUMBER_OF_PARTITIONS * PWBD_DEFAULT_PARTITION_SIZE_MB +                          \
     PWBD_RESERVED_DISK_SIZE_MB)
#define PWBD_DEFAULT_DISK_SIZE TO_MB(PWBD_DEFAULT_DISK_SIZE_MB)

#define PWBD_DEFAULT_NUMBER_OF_SECTORS (PWBD_DEFAULT_DISK_SIZE >> PWBD_DEFAULT_SECTOR_SHIFT)
static_assert(PWBD_DEFAULT_SECTOR_SHIFT <= PAGE_SHIFT);

#define PWBD_DEFAULT_SECTORS_PER_PAGE (1 << (PAGE_SHIFT - PWBD_DEFAULT_SECTOR_SHIFT))

//
//
//

typedef enum _PWBD_DEVICE_FLAGS {
    PWBD_DEVFL_DISK_ADDED = 0x00000001,
    PWBD_DEVFL_TAG_SET_ALLOCATED = 0x00000002,

} PWBD_DEVICE_FLAGS;

//
//
//

typedef struct _PWBD_DEVICE_FLAGS_BF {
    uint32_t FlDiskAdded : 1;
    uint32_t FlTagSetAllocated : 1;
    uint32_t FlReserved : 30;

} PWBD_DEVICE_FLAGS_BF;

static_assert(sizeof(PWBD_DEVICE_FLAGS_BF) == sizeof(PWBD_DEVICE_FLAGS));

//
//
//

typedef struct _PWBD_DEVICE {

    union {
        PWBD_DEVICE_FLAGS Flags;
        PWBD_DEVICE_FLAGS_BF FlagsBf;
    };

    uint16_t SectorShift;
    uint16_t SectorSize;
    uint32_t SectorsPerPage;
    uint64_t DiskSize; // in bytes

    uint32_t Minor;

    sector_t Capacity; // uint64_t, in sectors

#ifdef PWBD_USE_MQ
    struct blk_mq_tag_set TagSet;
#endif // PWBD_USE_MQ

    void *DiskData;

    struct gendisk *Disk;

} PWBD_DEVICE, *PPWBD_DEVICE;

//
//
//

//
//
//

[[nodiscard]] int PwbdAddDevices(void);

void PwbdRemoveDevices(void);

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
