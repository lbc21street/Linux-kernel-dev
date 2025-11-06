//=================================================================================================
//
// \file    timertest.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/gfp.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

typedef struct _TT_TIMER_DATA {
    struct timer_list Timer;

    uintptr_t StartTime;
    uintptr_t StopTime;
    uintptr_t TimerSetupTime;

    atomic_t Counter;

    atomic_t StopFlag;

} TT_TIMER_DATA, *PTT_TIMER_DATA;

//
//
//

typedef enum _TT_CTRL_FLAGS {
    TT_CTLFL_TIMER_SET = 0x00000001,

} TT_CTRL_FLAGS;

//
//
//

typedef struct _TT_CTRL_FLAGS_BF {
    uint32_t FlTimerSet : 1;
    uint32_t FlReserved : 31;

} TT_CTRL_FLAGS_BF;

static_assert(sizeof(TT_CTRL_FLAGS_BF) == sizeof(TT_CTRL_FLAGS));

//
//
//

typedef struct _TT_CTRL {
    union {
        TT_CTRL_FLAGS Flags;
        TT_CTRL_FLAGS_BF FlagsBf;
    };

    PTT_TIMER_DATA TimerData;

} TT_CTRL, *PTT_CTRL;

extern TT_CTRL TtCtrl;

//
// timer period in millisecons
//

#define TT_TIMER_PERIOD (30 * 1000)

//
// total working time in milliseconds (no more than)
//

#define TT_WORKING_TIME (5 * 60 * 1000)

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
