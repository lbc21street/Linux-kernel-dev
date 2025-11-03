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

#ifdef PWBD_USE_OWN_BLK_OP_NAMES
#define PWBD_BLK_OP_NAME(op)                                                                       \
    case op:                                                                                       \
        return #op
#endif // PWBD_USE_OWN_BLK_OP_NAMES

//
//
//

#ifdef PWBD_USE_OWN_BLK_OP_NAMES
const char *PwbdGetBlkOpName(blk_opf_t Operation);
#endif // PWBD_USE_OWN_BLK_OP_NAMES

[[nodiscard]] int PwbdProcessAsyncRequest(struct request *Request);

//
//
//

void PwbdInitStaticDevOps(PPWBD_DEVICE Device);

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
