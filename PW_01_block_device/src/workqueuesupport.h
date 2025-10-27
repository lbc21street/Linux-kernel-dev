//=================================================================================================
//
// \file    workqueuesupport.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include "devicesupport.h"
#include <linux/blk-mq.h>
#include <linux/workqueue.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef PWBD_USE_MQ

//
//
//

typedef struct _PWBD_REQUEST_DATA {

    struct work_struct WorkItem;

    int Result;

} PWBD_REQUEST_DATA, *PPWBD_REQUEST_DATA;

//
//
//

[[nodiscard]] int PwbdpAllocateWorkQueue(PPWBD_DEVICE Device);

void PwbdpDestroyWorkQueue(PPWBD_DEVICE Device);

bool PwbdpQueueWorkItem(struct request *Request);

#endif // PWBD_USE_MQ

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
