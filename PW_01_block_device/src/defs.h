//=================================================================================================
//
//
//
//
//=================================================================================================

#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

///////////////////////////////////////////////////////////////////////////////

#define ASCII_NULL ((char)'\0')
#define ASCII_LF ((char)'\n')

#ifndef TRUE
#define TRUE 1
#endif // TRUE

#ifndef FALSE
#define FALSE 0
#endif // FALSE

///////////////////////////////////////////////////////////////////////////////

#define Pwbd_CMD_ADD "add "
#define Pwbd_CMD_ADD_LENGTH (sizeof(Pwbd_CMD_ADD) - sizeof(ASCII_NULL))
#define Pwbd_CMD_ADD_TAIL "addtail "
#define Pwbd_CMD_ADD_TAIL_LENGTH (sizeof(Pwbd_CMD_ADD_TAIL) - sizeof(ASCII_NULL))
#define Pwbd_CMD_REMOVE "remove"
#define Pwbd_CMD_REMOVE_TAIL "removetail"
#define Pwbd_CMD_REMOVE_BY "removeby "
#define Pwbd_CMD_REMOVE_BY_LENGTH (sizeof(Pwbd_CMD_REMOVE_BY) - sizeof(ASCII_NULL))
#define Pwbd_CMD_PEEK "peek"
#define Pwbd_CMD_PEEK_TAIL "peektail"
#define Pwbd_CMD_FIND "find "
#define Pwbd_CMD_FIND_LENGTH (sizeof(Pwbd_CMD_FIND) - sizeof(ASCII_NULL))
#define Pwbd_CMD_ENUM "enum"
#define Pwbd_CMD_ENUM_TAIL "enumtail"
#define Pwbd_CMD_ROTATE "rotate"
#define Pwbd_CMD_SIZE "size"
#define Pwbd_CMD_CLEAR "clear"
#define Pwbd_CMD_EXIT "exit"

///////////////////////////////////////////////////////////////////////////////

struct Pwbd_DATA_ENTRY {
    struct list_head Links;
    __u32 Value;
};

#define Pwbd_MAX_DATA_ENTRIES           100000


//
//
//

#define PWBD_DEVICE_NAME                "pwblkdev"

//
//
//

#define PWBD_DEFAULT_QUEUE_DEPTH        128

#define PWBD_NUMBER_OF_PARTITIONS       3
#define PWBD_SECTOR_SIZE                (4 * 1024)
#define PWBD_PARTITION_SIZE             (100 * 1024 * 1024)
#define PWBD_DISK_SIZE                  (PWBD_NUMBER_OF_PARTITIONS * PWBD_PARTITION_SIZE)
#define PWBD_NUMBER_OF_DISK_SECTORS     (PWBD_DISK_SIZE / PWBD_SECTOR_SIZE)


//
// routines of interest
//

// static void blk_report_disk_dead(struct gendisk *disk, bool surprise);  // not exported
// void blk_mark_disk_dead(struct gendisk *disk);
// void invalidate_disk(struct gendisk *disk);
// void put_disk(struct gendisk *disk);
// void set_disk_ro(struct gendisk *disk, bool read_only);


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

} PWBD_CTRL, *PPWBD_CTRL;

///////////////////////////////////////////////////////////////////////////////


static int PwbdSetParam(const char *val, const struct kernel_param *kp);




#ifdef __cplusplus
}
#endif  // __cplusplus

//=================================================================================================
