//=================================================================================================
//
// \file    iosupport.h
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

#define PWBD_IOCTL_NAME(cmd)                                                                       \
    case cmd:                                                                                      \
        return #cmd

/*
#define PWBD_BLK_OP_NAME(op)                                                                       \
case op:                                                                                       \
return #op
*/

//
//
//

[[nodiscard]] int PwbdProcessAsyncRequest(struct request *Request);

//
//
//

void PwbdInitStaticDevOps(PPWBD_DEVICE Device);

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
