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

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
