//=================================================================================================
//
// \file    queuesupport.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#include "devicesupport.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef PWBD_USE_MQ

//
//
//

#define PWBD_DEFAULT_QUEUE_DEPTH 128

//
//
//

[[nodiscard]] int PwbdpAllocateTagSet(PPWBD_DEVICE Device);

void PwbdpFreeTagSet(PPWBD_DEVICE Device);

void PwbdpInitStaticTagSet(PPWBD_DEVICE Device);

void PwbdpInitStaticMqOps(PPWBD_DEVICE Device);

#endif // PWBD_USE_MQ

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
