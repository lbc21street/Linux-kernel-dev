//=================================================================================================
//
// \file    kthread.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include <linux/gfp.h>
#include <linux/kthread.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/types.h>

#include <data.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// [NOTE]
//
// it's absent in kernel 6.1.130, but defined as a macro for usb drivers
//

#define kthread_stop_put(k)                                                                        \
    ({                                                                                             \
        int ret = kthread_stop(k);                                                                 \
        put_task_struct(k);                                                                        \
        ret;                                                                                       \
    })

//
//
//

typedef struct _KTH_CTRL {

    XTH_DATA Data;

    struct task_struct *WriterWorkers[XTH_NUMBER_OF_WRITER_THREADS];
    struct task_struct *ReaderWorker;

} KTH_CTRL, *PKTH_CTRL;

extern KTH_CTRL KthCtrl;

//
//
//

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
