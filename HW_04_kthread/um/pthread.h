//=================================================================================================
//
// \file    pthread.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <assert.h>
#include <pthread.h>
#include <stdint.h>

#include <data.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define PTH_INVALID_THREAD_ID ((pthread_t) - 1)

//
//
//

typedef enum _PTH_CTRL_FLAGS {
    PTH_CTLFL_LOCK_INITED = 0x00000001,
    PTH_CTLFL_WRITER_WORKER_STOP_PENDING = 0x00000002,
    PTH_CTLFL_READER_WORKER_STOP_PENDING = 0x00000004,

} PTH_CTRL_FLAGS;

//
//
//

typedef struct _PTH_CTRL_FLAGS_BF {
    uint32_t FlLockInited : 1;
    uint32_t FlWriterWorkerStopPending : 1;
    uint32_t FlReaderWorkerStopPending : 1;
    uint32_t FlReserved : 29;

} PTH_CTRL_FLAGS_BF;

static_assert(sizeof(PTH_CTRL_FLAGS_BF) == sizeof(PTH_CTRL_FLAGS));

//
//
//

typedef struct _PTH_CTRL {

    union {
        PTH_CTRL_FLAGS Flags;
        PTH_CTRL_FLAGS_BF FlagsBf;
    };

    int KmsgFd;

    XTH_DATA Data;

    pthread_t WriterWorkers[XTH_NUMBER_OF_WRITER_THREADS];
    pthread_t ReaderWorker;

} PTH_CTRL, *PPTH_CTRL;

extern PTH_CTRL PthCtrl;

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
