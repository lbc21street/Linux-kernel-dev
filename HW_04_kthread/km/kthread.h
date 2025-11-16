//=================================================================================================
//
// \file    kthread.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
//
//

#define KTH_NUMBER_OF_WRITER_THREADS 2

#define KTH_MAX_CYCLES 100000000

//
//
//

typedef struct _KTH_CTRL {

    uint32_t Data;
    uint32_t DataWritten;

    uint32_t WriterCycles;
    uint32_t ReaderCycles;

    uint32_t Counter;

    struct task_struct *WriterWorkers[KTH_NUMBER_OF_WRITER_THREADS];
    struct task_struct *ReaderWorker;

    struct rw_semaphore Lock;

} KTH_CTRL, *PKTH_CTRL;

extern KTH_CTRL KthCtrl;

//
//
//

#define KTH_UINT_PTR(val) ((void *)(uintptr_t)(val))
#define KTH_PTR_UINT(ptr) ((uint32_t)(uintptr_t)(ptr))

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
