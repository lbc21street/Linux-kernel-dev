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

[[nodiscard]] int PwbdProcessAsyncRequest(struct request *Request);

//
//
//

void PwbdInitStaticDevOps(PPWBD_DEVICE Device);

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
