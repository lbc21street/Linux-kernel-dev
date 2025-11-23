//=================================================================================================
//
// \file    ex_queue.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/types.h>
#include <linux/kfifo.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define EXQ_CMD_ADD "add "
#define EXQ_CMD_ADD_LENGTH (sizeof(EXQ_CMD_ADD) - sizeof(ASCII_NULL))
#define EXQ_CMD_REMOVE "remove"
#define EXQ_CMD_PEEK "peek"
#define EXQ_CMD_LEN "len"
#define EXQ_CMD_AVAIL "avail"
#define EXQ_CMD_HASH "hash"
#define EXQ_CMD_RESET "reset"
#define EXQ_CMD_EXIT "exit"

//
//
//

typedef enum _EXQ_CTRL_FLAGS {

    EXQ_CTLFL_QUEUE_CREATED = 0x00000001,

} EXQ_CTRL_FLAGS;

//
//
//

typedef struct _EXQ_CTRL_FLAGS_BF {

    uint32_t FlQueueCreated : 1;
    uint32_t FlReserved : 31;

} EXQ_CTRL_FLAGS_BF;

static_assert(sizeof(EXQ_CTRL_FLAGS_BF) == sizeof(EXQ_CTRL_FLAGS));

//
//
//

typedef struct _EXQ_CTRL {

    DECLARE_KFIFO_PTR(Queue, uint32_t);

    union {
        EXQ_CTRL_FLAGS Flags;
        EXQ_CTRL_FLAGS_BF FlagsBf;
    };

    bool Active;

} EXQ_CTRL, *PEXQ_CTRL;

#define EXQ_MAX_DATA_ENTRIES 32

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
