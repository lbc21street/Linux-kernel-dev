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

//
//
//

int PwbdpPerformAsyncIo(PPWBD_DEVICE Device, struct page *Page, uint32_t Length, uint32_t Offset,
                        blk_opf_t Operation, sector_t Sector);

void PwbdpInitStaticDevOps(PPWBD_DEVICE Device);

#ifdef __cplusplus
}
#endif // __cplusplus

//=================================================================================================
