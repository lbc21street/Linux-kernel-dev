//=================================================================================================
//
// \file    defexec.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

typedef struct _DEX_WORK_DATA {
    struct timer_list Timer;
    struct tasklet_struct Tasklet;
    struct work_struct WorkItem;

    struct wait_queue_head WaitQueueList;

    atomic_t WaitInterrupted;

} DEX_WORK_DATA, *PDEX_WORK_DATA;

//
//
//

typedef enum _DEX_CTRL_FLAGS {
    DEX_CTLFL_TIMER_SET = 0x00000001,

} DEX_CTRL_FLAGS;

//
//
//

typedef struct _DEX_CTRL_FLAGS_BF {
    uint32_t FlTimerSet : 1;
    uint32_t FlReserved : 31;

} DEX_CTRL_FLAGS_BF;

static_assert(sizeof(DEX_CTRL_FLAGS_BF) == sizeof(DEX_CTRL_FLAGS));

//
//
//

typedef struct _DEX_CTRL {
    union {
        DEX_CTRL_FLAGS Flags;
        DEX_CTRL_FLAGS_BF FlagsBf;
    };

    PDEX_WORK_DATA WorkData;

} DEX_CTRL, *PDEX_CTRL;

extern DEX_CTRL DexCtrl;

//
// timer period in millisecons
//

#define DEX_TIMER_PERIOD (5 * 1000)

//
// delay in a worker routine in milliseconds
//

#define DEX_WORKER_DELAY (5 * 1000)

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
